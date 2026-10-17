#include "flight.hpp"
#include "telemetry.hpp"
#include <iostream>
#include <stdexcept>
#define CHECK(x) do{if(!(x))throw std::runtime_error(#x);}while(0)

int main() {
    using namespace inova;
    Quaternion q=Quaternion::alignUp({-1,0,9.75});CHECK(std::abs(q.rotate({-1,0,9.75}).x)<1e-9);
    q={};q.integrate({0,pi/2,0},1);CHECK(std::abs(q.rotate({0,0,1}).x-1)<1e-9);
    Telemetry t;t.flight.altitude_m=-10.25;t.flight.baro_velocity_m_s=-12.34;t.latitude_e7=384000000;
    auto p=encode(t);CHECK(validPacket(p));CHECK(static_cast<int32_t>(get(p,12,4))==-1025);
    CHECK(static_cast<int16_t>(get(p,16,2))==-1234);
    for(size_t i=0;i<p.size();i++)for(int bit=0;bit<8;bit++) {auto damaged=p;damaged[i]^=1<<bit;CHECK(!validPacket(damaged));}
    CHECK(std::abs(timeOnAir(25,9)-0.205824)<1e-8);CHECK(timeOnAir(packet_size)<0.2);
    FlightComputer f;Sample s;s.pressure_pa=90210;s.accel_m_s2={0,0,9.797};s.imu_new=true;
    for(int i=0;i<500;i++){s.time_s=i*.01;s.baro_new=i%2==0;f.step(s);}
    CHECK(f.output().state==State::Ready);CHECK(!f.output().release_requested);
    s.arm=true;s.baro_new=true;
    for(int i=500;i<535;i++){s.time_s=i*.01;f.step(s);}
    CHECK(f.output().state==State::Armed);
    s.time_s=4;f.step(s);CHECK(f.output().state==State::Fault);CHECK(!f.output().release_requested);
    FlightComputer invalid; s={};s.imu_new=s.baro_new=true;s.pressure_pa=NAN;s.accel_m_s2={0,0,9.797};
    for(int i=0;i<500;i++){s.time_s=i*.01;invalid.step(s);}CHECK(invalid.output().state==State::Boot);
    std::cout<<"Core invariants + all 272 single-bit packet corruptions: PASS\n";
}
