#include "flight.hpp"
#include "telemetry.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

int main(int argc,char** argv) {
    if(argc!=4) {std::cerr<<"Usage: replay input.csv output.csv packets.hex\n";return 2;}
    try {
        std::ifstream input(argv[1]);std::ofstream output(argv[2]),packets(argv[3]);
        if(!input||!output||!packets) throw std::runtime_error("Cannot open replay files");
        std::string line;std::getline(input,line);
        if(!line.empty()&&line.back()=='\r')line.pop_back();
        const std::string header="time_s,pressure_pa,ax,ay,az,gx,gy,gz,baro_new,imu_new,arm,truth_alt_m,truth_vz_m_s,radio_busy,gps_valid,latitude,longitude,battery_mv,reset";
        if(line!=header) throw std::runtime_error("Unexpected CSV schema");
        inova::FlightComputer computer;unsigned sent=0,dropped=0;double next_tx=0,tx_done=0;uint16_t seq=0;
        output<<"time_s,truth_alt_m,truth_vz_m_s,state,altitude_m,baro_v_m_s,inertial_v_m_s,tilt_deg,release,health,tx_sent,tx_dropped\n";
        while(std::getline(input,line)) {
            std::vector<double> v;std::stringstream ss(line);std::string cell;
            while(std::getline(ss,cell,',')) v.push_back(std::stod(cell));
            if(v.size()!=19) throw std::runtime_error("Unexpected CSV row length");
            if(v[18]!=0) computer=inova::FlightComputer{};
            inova::Sample s{v[0],v[1],{v[2],v[3],v[4]},{v[5],v[6],v[7]},v[8]!=0,v[9]!=0,v[10]!=0};
            auto out=computer.step(s);
            if(s.time_s+1e-7>=next_tx) {
                next_tx=s.time_s+0.2;
                if(v[13]!=0||s.time_s<tx_done) dropped++;
                else {
                    inova::Telemetry t; t.sequence=seq++;t.uptime_ms=static_cast<uint32_t>(s.time_s*1000);
                    t.latitude_e7=static_cast<int32_t>(v[15]*1e7);t.longitude_e7=static_cast<int32_t>(v[16]*1e7);
                    t.gps_valid=v[14]!=0;t.battery_mv=static_cast<uint16_t>(v[17]);t.flight=out;
                    auto p=inova::encode(t);if(!inova::validPacket(p)) throw std::runtime_error("Invalid generated packet");
                    packets<<std::fixed<<std::setprecision(3)<<s.time_s<<',';
                    for(auto b:p) packets<<std::hex<<std::setfill('0')<<std::setw(2)<<unsigned(b);
                    packets<<std::dec<<'\n';sent++;tx_done=s.time_s+inova::timeOnAir(p.size());
                }
            }
            output<<std::fixed<<std::setprecision(6)<<s.time_s<<','<<v[11]<<','<<v[12]<<','<<inova::stateName(out.state)<<','
                  <<out.altitude_m<<','<<out.baro_velocity_m_s<<','<<out.inertial_velocity_m_s<<','<<out.tilt_deg<<','
                  <<out.release_requested<<','<<out.health<<','<<sent<<','<<dropped<<'\n';
        }
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
