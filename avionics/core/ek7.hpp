#pragma once
#include "flight.hpp"
#include <cstring>
#include <limits>

namespace inova::ek7 {
// Local interoperability profile. EK-7 does NOT resolve these choices consistently.
enum class FloatOrder { Little, Big };
enum class ChecksumScope { Prefix, PrefixAndFooter };
enum class CommandPolicy { TableLiterals, SumPrefix, SumWithFooter };
struct Profile {
    FloatOrder floats=FloatOrder::Little;
    ChecksumScope checksum=ChecksumScope::Prefix;
    CommandPolicy commands=CommandPolicy::TableLiterals;
    double synthetic_period_s=0.1; // Configured input cadence, NOT a claimed official requirement.
};
constexpr size_t data_size=36,status_size=6,command_size=5;
constexpr uint64_t frame_timeout_us=100000,test_timeout_us=1000000,start_delay_us=1000000,tx_period_us=100000;
using Data=std::array<float,8>; // MSL m, mBar, specific acceleration XYZ m/s2, Euler XYZ degrees.
struct Frame {std::array<uint8_t,data_size> bytes{};size_t size=0;};
inline uint8_t sum(const uint8_t* p,size_t n,ChecksumScope scope) {
    unsigned v=scope==ChecksumScope::PrefixAndFooter?0x0d+0x0a:0;
    for(size_t i=0;i<n;i++)v+=p[i];
    return v&255;
}
inline void putFloat(uint8_t* p,float f,FloatOrder order) {
    static_assert(sizeof(float)==4 && std::numeric_limits<float>::is_iec559,"IEEE754 binary32 required");
    uint32_t bits;std::memcpy(&bits,&f,4);
    for(int i=0;i<4;i++)p[i]=(bits>>(8*(order==FloatOrder::Little?i:3-i)))&255;
}
inline float getFloat(const uint8_t* p,FloatOrder order) {
    uint32_t bits=0;for(int i=0;i<4;i++)bits|=uint32_t(p[i])<<(8*(order==FloatOrder::Little?i:3-i));
    float f;std::memcpy(&f,&bits,4);return f;
}
inline Frame dataFrame(const Data& d,const Profile& profile={},bool round_two=false) {
    Frame f;f.size=data_size;f.bytes[0]=0xab;
    for(size_t i=0;i<d.size();i++) {
        float value=round_two?static_cast<float>(std::round(double(d[i])*100.0)/100.0):d[i];
        putFloat(f.bytes.data()+1+i*4,value,profile.floats);
    }
    f.bytes[33]=sum(f.bytes.data(),33,profile.checksum);f.bytes[34]=13;f.bytes[35]=10;return f;
}
inline Frame statusFrame(uint8_t milestones,const Profile& profile={}) {
    Frame f;f.size=status_size;f.bytes[0]=0xaa;f.bytes[1]=milestones;f.bytes[2]=0; // Reserved bits 8..15.
    f.bytes[3]=sum(f.bytes.data(),3,profile.checksum);f.bytes[4]=13;f.bytes[5]=10;return f;
}
inline Frame commandFrame(uint8_t command,const Profile& profile={}) {
    Frame f;f.size=command_size;f.bytes[0]=0xaa;f.bytes[1]=command;
    f.bytes[2]=profile.commands==CommandPolicy::TableLiterals?uint8_t(command+0x6c):
        sum(f.bytes.data(),2,profile.commands==CommandPolicy::SumWithFooter?ChecksumScope::PrefixAndFooter:ChecksumScope::Prefix);
    f.bytes[3]=13;f.bytes[4]=10;return f;
}
enum class Kind { None, Command, Data };
struct Event {Kind kind=Kind::None;uint8_t command=0;Data data{};};
class Parser {
public:
    explicit Parser(Profile p={}) : profile(p) {}
    void clear(){used=0;}
    void expire(uint64_t now) {if(used && now-started>=frame_timeout_us){clear();++timeouts;}}
    Event feed(uint8_t b,uint64_t now) {
        expire(now);
        if(!used)started=now;
        buffer[used++]=b;
        while(used) {
            if(buffer[0]!=0xaa && buffer[0]!=0xab){discard();continue;}
            size_t want=buffer[0]==0xaa?command_size:data_size;
            if(used<want)return {};
            if(buffer[0]==0xaa) {
                uint8_t c=buffer[1];auto expected=commandFrame(c,profile);
                if((c==0x20||c==0x22||c==0x24) && std::equal(buffer.begin(),buffer.begin()+command_size,expected.bytes.begin())) {
                    clear();return {Kind::Command,c,{}};
                }
            } else if(buffer[34]==13 && buffer[35]==10 && buffer[33]==sum(buffer.data(),33,profile.checksum)) {
                Event e;e.kind=Kind::Data;
                for(size_t i=0;i<8;i++)e.data[i]=getFloat(buffer.data()+1+i*4,profile.floats);
                clear();return e;
            }
            ++bad_frames;discard();
        }
        return {};
    }
    uint32_t bad_frames=0,timeouts=0;
private:
    Profile profile;std::array<uint8_t,data_size> buffer{};size_t used=0;uint64_t started=0;
    void discard(){std::move(buffer.begin()+1,buffer.begin()+used,buffer.begin());--used;}
};

// A right-handed cyclic basis change: internal +Z nose <=> EK-7 +X nose.
inline Vec3 toInternal(Vec3 v){return {v.y,v.z,v.x};}
inline Vec3 toOfficial(Vec3 v){return {v.z,v.x,v.y};}
inline Quaternion fromEuler(Vec3 deg) {
    double r=deg.x*pi/360,p=deg.y*pi/360,y=deg.z*pi/360;
    Quaternion off=Quaternion{std::cos(y),0,0,std::sin(y)}*Quaternion{std::cos(p),0,std::sin(p),0}*Quaternion{std::cos(r),std::sin(r),0,0};
    return {off.w,off.y,off.z,off.x};
}
inline Vec3 toEuler(Quaternion internal) {
    Quaternion q{internal.w,internal.z,internal.x,internal.y};q.normalize();
    return {std::atan2(2*(q.w*q.x+q.y*q.z),1-2*(q.x*q.x+q.y*q.y))*180/pi,
            std::asin(std::clamp(2*(q.w*q.y-q.z*q.x),-1.0,1.0))*180/pi,
            std::atan2(2*(q.w*q.z+q.x*q.y),1-2*(q.y*q.y+q.z*q.z))*180/pi};
}
inline Config syntheticConfig(Config c,Profile p) {
    c.imu_period_s=c.baro_period_s=p.synthetic_period_s;
    c.imu_timeout_s=4*p.synthetic_period_s;c.baro_timeout_s=6*p.synthetic_period_s;
    return c; // Decision thresholds, duration gates and filters are shared with real flight.
}
enum class Mode { Normal, SitPending, SutPending, Sit, Sut };
class Session {
public:
    explicit Session(Profile p={},Config c={}) : profile(p),cfg(c),parser(p),computer(c) {}
    Mode mode() const{return current;}
    const Output& output()const{return computer.output();}
    const Parser& diagnostics()const{return parser;}
    uint32_t invalid_data=0,session_timeouts=0,blocked_starts=0;
    void receive(uint8_t byte,uint64_t now) {
        service(now);auto e=parser.feed(byte,now);
        if(e.kind==Kind::Command) {
            if(e.command==0x24){
                if(current!=Mode::Normal || !(output().milestones&1))stop();
                return;
            }
            // Ground-only entry: never remotely replace a running real flight.
            if(current==Mode::Normal && (output().milestones&1)){++blocked_starts;return;}
            stop();current=e.command==0x20?Mode::SitPending:Mode::SutPending;activate_at=now+start_delay_us;
        } else if(e.kind==Kind::Data && current==Mode::Sut) {
            bool valid=true;for(float v:e.data)valid=valid&&std::isfinite(v);
            valid=valid&&e.data[0]>=-500&&e.data[0]<=50000&&e.data[1]>=300&&e.data[1]<=1250;
            for(size_t i=5;i<8;i++)valid=valid&&std::abs(e.data[i])<=360;
            if(!valid){++invalid_data;return;}
            if(has_synthetic && now<=last_synthetic)return;
            last_synthetic=now;has_synthetic=true;
            // EK-7 gives no gyro field. Supply measured attitude to the SAME estimator.
            Sample s;s.time_s=now/1e6;s.arm=arm;
            s.pressure_pa=double(e.data[1])*100;s.baro_new=s.imu_new=s.attitude_new=true;
            s.altitude_msl_m=e.data[0];s.baro_altitude_new=true;
            s.accel_m_s2=toInternal({e.data[2],e.data[3],e.data[4]});
            s.body_to_world=fromEuler({e.data[5],e.data[6],e.data[7]});
            // Both supplied altitude and pressure enter the shared barometric consistency/filter path.
            synthetic_msl=e.data[0];computer.step(s);
        }
    }
    void setArm(bool enabled){arm=enabled;}
    void realSample(const Sample& s) {
        if(current==Mode::Sut || current==Mode::SutPending)return;
        Sample live=s;
        if(current!=Mode::Normal)live.arm=false; // SIT cannot produce a recovery command.
        computer.step(live);
        if(s.baro_new && std::isfinite(s.pressure_pa)&&s.pressure_pa>0){pressure=s.pressure_pa;baro_at=s.time_s;}
        if(s.imu_new && s.accel_m_s2.finite()){accel=s.accel_m_s2;imu_at=s.time_s;}
    }
    Frame poll(uint64_t now) {
        service(now);
        if(current!=Mode::Sit && current!=Mode::Sut)return {};
        if(now<next_tx)return {};
        next_tx=now+tx_period_us; // Never enqueue catch-up bursts.
        if(current==Mode::Sut)return statusFrame(output().milestones,profile);
        double t=now/1e6;
        // NaN explicitly represents unavailable/stale sensors, never a fabricated zero.
        float bad=std::numeric_limits<float>::quiet_NaN();
        bool bp=t-baro_at<=cfg.baro_timeout_s,ip=t-imu_at<=cfg.imu_timeout_s;
        Vec3 a=toOfficial(accel),e=toEuler(output().body_to_world);
        double msl=cfg.launch_msl_m+output().altitude_m; // Surveyed site + filtered barometric AGL.
        Data d{bp&&output().reference_pressure_pa>0?float(msl):bad,bp?float(pressure/100):bad,
               ip?float(a.x):bad,ip?float(a.y):bad,ip?float(a.z):bad,
               ip&&output().state!=State::Boot?float(e.x):bad,
               ip&&output().state!=State::Boot?float(e.y):bad,
               ip&&output().state!=State::Boot?float(e.z):bad};
        return dataFrame(d,profile,true);
    }
    void stop() {
        current=Mode::Normal;parser.clear();computer=FlightComputer(cfg);
        activate_at=next_tx=last_synthetic=0;has_synthetic=false;synthetic_msl=0;
        baro_at=imu_at=-1e6;pressure=0;accel={};
    }
    double synthetic_msl=0;
private:
    Profile profile;Config cfg;Parser parser;FlightComputer computer;Mode current=Mode::Normal;
    bool arm=false,has_synthetic=false;uint64_t activate_at=0,next_tx=0,last_synthetic=0;
    double pressure=0,baro_at=-1e6,imu_at=-1e6;Vec3 accel{};
    void service(uint64_t now) {
        parser.expire(now);
        if((current==Mode::SitPending||current==Mode::SutPending) && now>=activate_at) {
            bool sut=current==Mode::SutPending;current=sut?Mode::Sut:Mode::Sit;
            computer=FlightComputer(sut?syntheticConfig(cfg,profile):cfg);last_synthetic=activate_at;next_tx=activate_at;
        }
        if(current==Mode::Sut && now-last_synthetic>=test_timeout_us){++session_timeouts;stop();}
    }
};
} // namespace inova::ek7
