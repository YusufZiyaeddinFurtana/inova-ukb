#pragma once
#include "sensors.hpp"
#include "mpu9255.hpp"
#include "bmp388.hpp"
namespace sim {
class Rig {
public:
    struct ResetClock{ResetClock(){clock_us=0;}} reset_clock;
    TwoWire imu_bus;std::shared_ptr<MPUModel> imu_model=std::make_shared<MPUModel>();
    MPU9255 imu{imu_bus};
    TwoWire baro_bus;std::shared_ptr<BMPModel> baro_model=std::make_shared<BMPModel>();
    BMP388 baro{baro_bus};
    Rig(){imu_bus.devices[0x68]=imu_model;baro_bus.devices[0x77]=baro_model;}
    bool begin(){return imu.begin()&&baro.begin();}
    void stimulate(double time_s,inova::Vec3 accel,inova::Vec3 gyro,double pressure,double temperature){
        clock_us=uint64_t(std::llround(time_s*1e6));imu_model->applied_accel=accel;imu_model->applied_gyro=gyro;
        baro_model->applied_pressure=pressure;baro_model->applied_temperature=temperature;
    }
    double temperature()const{return last_temperature;}
    inova::Sample read(bool baro_due=true){inova::Sample s;s.time_s=clock_us/1e6;s.imu_new=imu.read(s.accel_m_s2,s.gyro_rad_s);
        s.baro_new=baro_due&&baro.read(s.pressure_pa,last_temperature);return s;}
private:
    double last_temperature=0;
};
}
