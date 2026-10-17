#include "sensor_rig.hpp"
#include "ek7.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

int main(int argc,char** argv){try {
    if(argc!=3){std::cerr<<"Usage: sensor_replay stimulus.csv trace.csv\n";return 2;}
    std::ifstream input(argv[1]);std::ofstream output(argv[2]);if(!input||!output)return 2;
    sim::Rig rig;if(!rig.begin()){std::cerr<<"Sensor model initialization failed\n";return 1;}
    inova::ek7::Session session;std::string line;std::getline(input,line);
    output<<"time_s,mode,state,baro_new,imu_new,pressure_pa,temperature_c,ax,ay,az,gx,gy,gz,altitude_m,baro_v,imu_v,tilt_deg,health,release,imu_conversions,baro_conversions,tx\n";
    uint64_t last_baro=0;
    while(std::getline(input,line)) {
        std::vector<std::string> v;std::istringstream stream(line);std::string value;
        while(std::getline(stream,value,','))v.push_back(value);
        if(v.size()!=13)throw std::runtime_error("Invalid stimulus schema");
        double t=std::stod(v[0]);inova::Vec3 a{std::stod(v[3]),std::stod(v[4]),std::stod(v[5])};
        inova::Vec3 g{std::stod(v[6]),std::stod(v[7]),std::stod(v[8])};
        rig.imu_model->connected=v[11]!="1";rig.imu_model->short_read=v[11]=="2";rig.imu_model->stop_updates=v[11]=="3";
        rig.baro_model->connected=v[12]!="1";rig.baro_model->short_read=v[12]=="2";rig.baro_model->stop_conversions=v[12]=="3";
        rig.stimulate(t,a,g,std::stod(v[1]),std::stod(v[2]));uint64_t now=sim::clock_us;
        session.setArm(v[9]=="1");
        if(v[10].size()%2)throw std::runtime_error("Invalid command bytes");
        for(size_t i=0;i<v[10].size();i+=2)session.receive(std::stoul(v[10].substr(i,2),nullptr,16),now);
        bool due=now-last_baro>=20000;if(due)last_baro=now;
        auto sample=rig.read(due);sample.arm=v[9]=="1";session.realSample(sample);
        auto frame=session.poll(sim::clock_us);auto out=session.output();
        output<<std::setprecision(10)<<sim::clock_us/1e6<<','<<int(session.mode())<<','<<inova::stateName(out.state)<<','
              <<sample.baro_new<<','<<sample.imu_new<<','<<(sample.baro_new?sample.pressure_pa:NAN)<<','<<rig.temperature()<<','
              <<sample.accel_m_s2.x<<','<<sample.accel_m_s2.y<<','<<sample.accel_m_s2.z<<','
              <<sample.gyro_rad_s.x<<','<<sample.gyro_rad_s.y<<','<<sample.gyro_rad_s.z<<','
              <<out.altitude_m<<','<<out.baro_velocity_m_s<<','<<out.inertial_velocity_m_s<<','<<out.tilt_deg<<','
              <<out.health<<','<<out.release_requested<<','<<rig.imu_model->conversions<<','<<rig.baro_model->conversions<<',';
        for(size_t i=0;i<frame.size;i++)output<<std::hex<<std::setw(2)<<std::setfill('0')<<int(frame.bytes[i]);
        output<<std::dec<<'\n';
    }
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
