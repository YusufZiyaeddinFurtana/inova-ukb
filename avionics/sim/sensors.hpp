#pragma once
#include "Wire.h"
#include "flight.hpp"
namespace sim {
class MPUModel : public I2CDevice {
public:
    inova::Vec3 applied_accel{0,0,9.80665},applied_gyro{},accel_bias{},gyro_bias{};
    bool stop_updates=false;
    double tau_s=0.004; // Explicit first-order DLPF surrogate, not a measured silicon response.
    unsigned conversions=0;
    MPUModel(){reset();}
    void write(uint8_t r,const uint8_t* p,size_t n)override {
        if(r==0x6b && n && (p[0]&0x80)){reset();return;}
        I2CDevice::write(r,p,n);
    }
    void read(uint8_t r,uint8_t* dst,size_t n)override {
        tick();I2CDevice::read(r,dst,n);if(r==0x3a&&n)reg[0x3a]=0;
    }
private:
    uint64_t previous=0;inova::Vec3 filtered_a{},filtered_g{};
    void reset(){auto who=reg[0x75]?reg[0x75]:0x73;reg.fill(0);reg[0x75]=who;reg[0x6b]=0x40;previous=clock_us;}
    void word(unsigned offset,int value){value=std::clamp(value,-32768,32767);auto bits=uint16_t(int16_t(value));reg[offset]=bits>>8;reg[offset+1]=bits&255;}
    void tick(){
        if(stop_updates||(reg[0x6b]&0x40))return;
        uint64_t period=(reg[0x19]+1)*1000ULL;if(clock_us<previous+period)return;
        double dt=(clock_us-previous)/1e6;previous=clock_us;
        double alpha=tau_s>0?1-std::exp(-dt/tau_s):1;
        filtered_a=filtered_a+(applied_accel+accel_bias-filtered_a)*alpha;
        filtered_g=filtered_g+(applied_gyro+gyro_bias-filtered_g)*alpha;
        double arange=double(2u<<((reg[0x1c]>>3)&3));
        constexpr double scales[]={131,65.5,32.8,16.4};double gs=scales[(reg[0x1b]>>3)&3];
        double av[]={filtered_a.x,filtered_a.y,filtered_a.z},gv[]={filtered_g.x,filtered_g.y,filtered_g.z};
        for(unsigned i=0;i<3;i++){word(0x3b+2*i,std::lround(av[i]/9.80665*32768/arange));word(0x43+2*i,std::lround(gv[i]*180/inova::pi*gs));}
        reg[0x3a]|=1;++conversions;
    }
};
class BMPModel : public I2CDevice {
public:
    double applied_pressure=90209.744,applied_temperature=20;
    bool stop_conversions=false,corrupt_nvm_crc=false;
    unsigned conversions=0;
    BMPModel(){reset();}
    void write(uint8_t r,const uint8_t* p,size_t n)override {
        // BMP388 multi-register write alternates value,next-register,value,... .
        for(size_t i=0;i<n;) {
            uint8_t value=p[i++];
            if(r==0x7e&&value==0xb6){reset();}
            else {
                reg[r]=value;
                if(r==0x1b&&((value>>4)&3)==1) {
                    uint64_t duration=234+392+(2000ULL<<(reg[0x1c]&7))+313+(2000ULL<<((reg[0x1c]>>3)&7));
                    pending=true;ready_at=clock_us+duration;
                    pending_pressure=applied_pressure;pending_temperature=applied_temperature;
                }
            }
            if(i<n)r=p[i++];
        }
    }
    void read(uint8_t r,uint8_t* dst,size_t n)override {
        tick();I2CDevice::read(r,dst,n);
        if(r<=0x06&&unsigned(r)+n>0x04)reg[3]&=~0x20;
        if(r<=0x09&&unsigned(r)+n>0x07)reg[3]&=~0x40;
    }
private:
    bool pending=false;uint64_t ready_at=0;double pending_pressure=0,pending_temperature=0;
    void reset(){
        uint8_t who=reg[0]?reg[0]:0x50;reg.fill(0);reg[0]=who;reg[3]=0x10;pending=false;
        // Deliberate affine calibration fixture: T=(rawT-5120000)/65536;
        // P=131072-rawP/128. Not factory trims from a real module.
        constexpr uint8_t trim[]={0x20,0x4e,0,0x40,0,0,0x20,0,0x40,0,0,0,0x40,0,0,0,0,0,0,0,0};
        std::copy(std::begin(trim),std::end(trim),reg.begin()+0x31);
        reg[0x30]=corrupt_nvm_crc?0xbd:0xbc; // Fixed CRC-8 fixture, independently calculated.
    }
    void raw24(unsigned offset,uint32_t value){for(unsigned i=0;i<3;i++)reg[offset+i]=(value>>(i*8))&255;}
    void tick(){
        if(!pending||stop_conversions||clock_us<ready_at)return;
        raw24(4,uint32_t(std::clamp(std::llround((131072-pending_pressure)*128),0LL,16777215LL)));
        raw24(7,uint32_t(std::clamp(std::llround(5120000+pending_temperature*65536),0LL,16777215LL)));
        reg[3]|=0x60;reg[0x1b]&=0xcf;pending=false;++conversions;
    }
};
}
