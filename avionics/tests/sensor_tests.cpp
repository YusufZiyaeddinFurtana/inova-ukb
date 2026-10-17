#include "sensor_rig.hpp"
#include <iostream>
#include <stdexcept>
#define CHECK(x) do{if(!(x))throw std::runtime_error(#x);}while(0)
int main(){try {
    sim::Rig rig;
    CHECK(rig.begin());
    rig.stimulate(9.80665,{0,0,9.80665},{0,0,90*inova::pi/180},90209.744,20);
    auto sample=rig.read();
    CHECK(sample.imu_new);
    CHECK(std::abs(sample.accel_m_s2.z-9.80665)<.005);
    CHECK(std::abs(sample.gyro_rad_s.z-inova::pi/2)<.002);
    std::cout<<"PASS MPU9255 physical stimulus through real driver\n";
    rig.stimulate(10.0,{0,0,9.80665},{},90000,25);
    rig.read();
    rig.stimulate(10.02,{0,0,9.80665},{},90000,25);
    sample=rig.read();
    CHECK(sample.baro_new);
    CHECK(std::abs(sample.pressure_pa-90000)<.1);
    CHECK(std::abs(rig.temperature()-25)<.01);
    std::cout<<"PASS BMP388 pressure and temperature through vendor compensation\n";
    rig.baro_model->stop_conversions=true;
    rig.stimulate(10.04,{0,0,9.80665},{},90000,25);
    sample=rig.read();
    CHECK(!sample.baro_new);
    std::cout<<"PASS stopped BMP388 conversion is not a fresh sample\n";
    rig.baro_model->stop_conversions=false;
    rig.stimulate(10.5,{0,0,9.80665},{},90000,25);
    CHECK(!rig.read().baro_new); // The completed pre-gap result is too old.
    rig.stimulate(10.52,{0,0,9.80665},{},90000,25);
    CHECK(rig.read().baro_new);
    std::cout<<"PASS old conversion after a long gap is discarded\n";
    rig.stimulate(10.54,{-19.6133,39.2266,300},{-50,0,0},90000,25);
    rig.read();
    rig.stimulate(10.60,{-19.6133,39.2266,300},{-50,0,0},90000,25);sample=rig.read();
    CHECK(sample.accel_m_s2.x< -19.60&&sample.accel_m_s2.y>39.20);
    CHECK(sample.accel_m_s2.z>156.8&&sample.accel_m_s2.z<157);
    CHECK(sample.gyro_rad_s.x< -34.8&&sample.gyro_rad_s.x> -35);
    std::cout<<"PASS MPU9255 signs and configured ADC saturation\n";
    CHECK(!rig.read(false).imu_new);
    std::cout<<"PASS duplicate IMU read is not a new sample\n";
    rig.imu_model->connected=false;
    rig.stimulate(10.62,{},{},90000,25);sample=rig.read();
    CHECK(!sample.imu_new&&sample.baro_new);
    std::cout<<"PASS independent barometer survives IMU bus NACK\n";
    {sim::Rig bad;bad.imu_model->reg[0x75]=0x71;CHECK(!bad.begin());}
    std::cout<<"PASS MPU9250 is not silently accepted as MPU9255\n";
    {sim::Rig bad;bad.baro_model->reg[0]=0x60;CHECK(!bad.begin());}
    std::cout<<"PASS BMP390 is not silently accepted as BMP388\n";
    {sim::Rig bad;bad.baro_model->corrupt_nvm_crc=true;CHECK(!bad.begin());}
    std::cout<<"PASS corrupt BMP388 calibration is rejected\n";
    {sim::Rig cold;CHECK(cold.begin());CHECK(!cold.read().baro_new);}
    std::cout<<"PASS unready startup conversion is never published\n";
    {sim::Rig alternate;alternate.baro_bus.devices.erase(0x77);alternate.baro_bus.devices[0x76]=alternate.baro_model;CHECK(alternate.begin());}
    std::cout<<"PASS BMP388 alternate I2C address works\n";
    return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
