#include "ek7.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

int main(int argc,char** argv) {
    if(argc<3||argc>4){std::cerr<<"Usage: ek7_replay events.csv output.csv [input_hz]\n";return 2;}
    std::ifstream input(argv[1]);std::ofstream output(argv[2]);
    if(!input||!output)return 2;
    inova::ek7::Profile profile;
    if(argc==4){double hz=std::stod(argv[3]);if(hz<5||hz>100)return 2;profile.synthetic_period_s=1/hz;}
    inova::ek7::Session session(profile);std::string line;
    output<<"time_us,action,mode,state,milestones,health,release,altitude_m,inertial_v,tx\n";
    while(std::getline(input,line)) {
        if(line.empty()||line[0]=='#')continue;
        std::istringstream stream(line);std::string time,action,payload;
        std::getline(stream,time,',');std::getline(stream,action,',');std::getline(stream,payload);
        uint64_t now=std::stoull(time);
        if(action=="RX") {
            if(payload.size()%2)return 2;
            for(size_t i=0;i<payload.size();i+=2)session.receive(std::stoul(payload.substr(i,2),nullptr,16),now);
        } else if(action=="ARM")session.setArm(payload=="1");
        else if(action=="REAL") {
            std::replace(payload.begin(),payload.end(),',',' ');std::istringstream values(payload);
            inova::Sample sample;sample.time_s=now/1e6;int enabled=0;
            if(!(values>>sample.pressure_pa>>sample.accel_m_s2.x>>sample.accel_m_s2.y>>sample.accel_m_s2.z
                >>sample.gyro_rad_s.x>>sample.gyro_rad_s.y>>sample.gyro_rad_s.z>>enabled))return 2;
            sample.arm=enabled;sample.baro_new=sample.imu_new=true;session.realSample(sample);
        } else if(action!="POLL")return 2;
        auto frame=session.poll(now);auto out=session.output();
        output<<now<<','<<action<<','<<int(session.mode())<<','<<inova::stateName(out.state)<<','<<int(out.milestones)
              <<','<<out.health<<','<<out.release_requested<<','<<out.altitude_m<<','<<out.inertial_velocity_m_s<<',';
        for(size_t i=0;i<frame.size;i++)output<<std::hex<<std::setw(2)<<std::setfill('0')<<int(frame.bytes[i]);
        output<<std::dec<<'\n';
    }
}
