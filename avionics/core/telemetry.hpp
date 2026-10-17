#pragma once
#include "flight.hpp"
#include <cstddef>
#include <limits>

namespace inova {
constexpr size_t packet_size=34;
using Packet=std::array<uint8_t,packet_size>;
struct Telemetry {
    uint16_t sequence=0, battery_mv=0,health=0;
    uint32_t uptime_ms=0;
    int32_t latitude_e7=0,longitude_e7=0;
    bool gps_valid=false;
    Output flight{};
};
inline uint16_t crc16(const uint8_t* p,size_t n) {
    uint16_t c=0xffff;
    for(size_t i=0;i<n;i++){c^=static_cast<uint16_t>(p[i])<<8;for(int k=0;k<8;k++)c=(c&0x8000)?(c<<1)^0x1021:c<<1;}
    return c;
}
inline void put(Packet& p,size_t offset,uint32_t value,size_t n) {for(size_t i=0;i<n;i++)p[offset+i]=(value>>(8*i))&255;}
inline uint32_t get(const Packet& p,size_t offset,size_t n) {uint32_t v=0;for(size_t i=0;i<n;i++)v|=uint32_t(p[offset+i])<<(8*i);return v;}
inline int32_t scaled(double v,double factor,double lo,double hi) {return std::isfinite(v)?static_cast<int32_t>(std::round(std::clamp(v*factor,lo,hi))):0;}
inline Packet encode(const Telemetry& t) {
    Packet p{};p[0]=0x49;p[1]=0x4e;p[2]=1;p[3]=static_cast<uint8_t>(t.flight.state);
    put(p,4,t.sequence,2);put(p,6,t.flight.health|t.health|(t.gps_valid?0x8000:0)|(t.flight.release_requested?0x4000:0),2);
    put(p,8,t.uptime_ms,4);put(p,12,scaled(t.flight.altitude_m,100,-2147483648.0,2147483647.0),4);
    put(p,16,scaled(t.flight.baro_velocity_m_s,100,-32768,32767),2);
    put(p,18,scaled(t.flight.inertial_velocity_m_s,100,-32768,32767),2);
    put(p,20,t.latitude_e7,4);put(p,24,t.longitude_e7,4);put(p,28,t.battery_mv,2);
    put(p,30,scaled(t.flight.tilt_deg,100,0,18000),2);put(p,32,crc16(p.data(),32),2);return p;
}
inline bool validPacket(const Packet& p) {return p[0]==0x49&&p[1]==0x4e&&p[2]==1&&p[3]<=7&&get(p,32,2)==crc16(p.data(),32);}
inline double timeOnAir(size_t payload,int sf=7,double bw=125000,int cr=1) {
    int de=(std::pow(2,sf)/bw>0.016)?1:0;
    double symbols=8+std::max(0.0,std::ceil((8.0*payload-4*sf+28+16)/(4*(sf-2*de)))*(cr+4));
    return (8+4.25+symbols)*std::pow(2,sf)/bw;
}
} // namespace inova
