#pragma once
#include <Wire.h>
#include "flight.hpp"

// Only accelerometer + gyro are used. Magnetometer is intentionally excluded near the magnetic lock.
class MPU9255 {
    TwoWire& wire;
    static constexpr uint8_t addr=0x68;
    bool writeReg(uint8_t reg,uint8_t value) {
        wire.beginTransmission(addr);wire.write(reg);wire.write(value);return wire.endTransmission()==0;
    }
    bool readReg(uint8_t reg,uint8_t* dst,size_t n) {
        wire.beginTransmission(addr);wire.write(reg);if(wire.endTransmission(false)!=0)return false;
        if(wire.requestFrom(addr,static_cast<uint8_t>(n))!=n)return false;
        for(size_t i=0;i<n;i++)dst[i]=wire.read();return true;
    }
public:
    explicit MPU9255(TwoWire& w):wire(w){}
    bool begin() {
        uint8_t who=0;if(!readReg(0x75,&who,1)||who!=0x73)return false;
        if(!writeReg(0x6b,0x80))return false;delay(100); // Startup only; not in the flight loop.
        if(!writeReg(0x6b,0x01))return false;delay(10);
        return writeReg(0x6c,0)&&writeReg(0x1a,3)&&writeReg(0x19,9)&&writeReg(0x1b,0x18)&&
               writeReg(0x1c,0x18)&&writeReg(0x1d,3)&&writeReg(0x38,1);
    }
    bool read(inova::Vec3& a,inova::Vec3& g) {
        uint8_t status=0,b[14];if(!readReg(0x3a,&status,1)||!(status&1)||!readReg(0x3b,b,14))return false;
        auto word=[&](int i){return static_cast<int16_t>((uint16_t(b[i])<<8)|b[i+1]);};
        constexpr double accel_scale=9.80665/2048.0,gyro_scale=inova::pi/(180.0*16.4);
        a={word(0)*accel_scale,word(2)*accel_scale,word(4)*accel_scale};
        g={word(8)*gyro_scale,word(10)*gyro_scale,word(12)*gyro_scale};return true;
    }
};
