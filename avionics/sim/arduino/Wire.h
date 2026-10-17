#pragma once
#include "Arduino.h"
#include <array>
#include <map>
#include <memory>
#include <vector>
namespace sim {
struct I2CDevice {
    std::array<uint8_t,256> reg{};
    bool connected=true,short_read=false;
    virtual ~I2CDevice()=default;
    virtual void read(uint8_t addr,uint8_t* dst,size_t n){std::copy_n(reg.begin()+addr,n,dst);}
    virtual void write(uint8_t addr,const uint8_t* src,size_t n){std::copy_n(src,n,reg.begin()+addr);}
};
}
// Arduino Wire compatibility only. All sensor interpretation stays in the real drivers.
class TwoWire {
public:
    std::map<uint8_t,std::shared_ptr<sim::I2CDevice>> devices;
    void begin(){} void end(){} void setClock(uint32_t){}
    void beginTransmission(uint8_t addr){address=addr;tx.clear();}
    size_t write(uint8_t b){tx.push_back(b);return 1;}
    size_t write(const uint8_t* p,size_t n){tx.insert(tx.end(),p,p+n);return n;}
    uint8_t endTransmission(bool=true){
        auto d=find(address);if(!d)return 2;
        if(!tx.empty()){cursor=tx[0];if(tx.size()>1)d->write(cursor,tx.data()+1,tx.size()-1);}return 0;
    }
    size_t requestFrom(uint8_t addr,uint8_t n,uint8_t=1){
        rx.clear();at=0;auto d=find(addr);if(!d)return 0;
        size_t amount=d->short_read&&n?n-1:n;rx.resize(amount);d->read(cursor,rx.data(),amount);
        cursor=uint8_t(cursor+amount);return amount;
    }
    int read(){return at<rx.size()?rx[at++]:-1;}
private:
    uint8_t address=0,cursor=0;size_t at=0;std::vector<uint8_t> tx,rx;
    std::shared_ptr<sim::I2CDevice> find(uint8_t addr){auto i=devices.find(addr);return i!=devices.end()&&i->second->connected?i->second:nullptr;}
};
inline TwoWire Wire;
