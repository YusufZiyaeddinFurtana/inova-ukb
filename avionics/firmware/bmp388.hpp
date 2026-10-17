#pragma once
#include <Adafruit_BMP3XX.h>
#include <cmath>

// Pipeline forced conversions: Adafruit starts the NEXT conversion and reads
// the preceding result immediately. Gate that read on the preceding DRDY flags.
// The priming result is deliberately discarded. One owner task per device.
class BMP388 {
public:
    explicit BMP388(TwoWire& bus):wire(bus){}
    bool begin() {
        address=0x77;
        if(!sensor.begin_I2C(address,&wire)) {
            address=0x76;if(!sensor.begin_I2C(address,&wire))return false;
        }
        if(sensor.chipID()!=0x50 || !sensor.setPressureOversampling(BMP3_OVERSAMPLING_2X) ||
           !sensor.setTemperatureOversampling(BMP3_NO_OVERSAMPLING) ||
           !sensor.setIIRFilterCoeff(BMP3_IIR_FILTER_DISABLE) || !sensor.setOutputDataRate(BMP3_ODR_50_HZ))return false;
        bool primed=sensor.performReading();triggered_us=micros();return primed;
    }
    bool read(double& pressure_pa,double& temperature_c) {
        uint8_t status=0;
        wire.beginTransmission(address);wire.write(0x03);
        if(wire.endTransmission(false)!=0 || wire.requestFrom(address,uint8_t(1))!=1)return false;
        status=wire.read();if((status&0x60)!=0x60)return false;
        uint32_t age_us=uint32_t(micros())-triggered_us;
        if(!sensor.performReading())return false;
        triggered_us=micros(); // Next conversion was started; the result just read is the preceding one.
        if(age_us>100000)return false;
        if(!std::isfinite(sensor.pressure)||!std::isfinite(sensor.temperature) ||
           sensor.pressure<30000||sensor.pressure>125000||sensor.temperature<-40||sensor.temperature>85)return false;
        pressure_pa=sensor.pressure;temperature_c=sensor.temperature;return true;
    }
private:
    TwoWire& wire;uint8_t address=0x77;Adafruit_BMP3XX sensor;uint32_t triggered_us=0;
};
