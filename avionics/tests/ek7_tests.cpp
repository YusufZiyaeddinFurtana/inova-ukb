#include "ek7.hpp"
#include <functional>
#include <iostream>
#include <random>
#include <stdexcept>
#define CHECK(x) do{if(!(x))throw std::runtime_error(#x);}while(0)
using namespace inova;
using namespace inova::ek7;
void command(Session& s,uint8_t c,uint64_t now){auto f=commandFrame(c);for(size_t i=0;i<f.size;i++)s.receive(f.bytes[i],now);}
void data(Session& s,Data d,uint64_t now){auto f=dataFrame(d);for(size_t i=0;i<f.size;i++)s.receive(f.bytes[i],now);}
Data pad{970,902.09744f,9.797f,0,0,0,0,0};
void check(const char* name,const std::function<void()>& test){test();std::cout<<"PASS "<<name<<'\n';}
int main() {try {
    check("canonical_commands",[]{
        for(uint8_t c:{0x20,0x22,0x24}){auto f=commandFrame(c);CHECK(f.size==5);CHECK(f.bytes[2]==c+0x6c);Parser p;Event e;for(size_t i=0;i<f.size;i++)e=p.feed(f.bytes[i],i*100);CHECK(e.kind==Kind::Command&&e.command==c);}
    });
    check("explicit_checksum_profiles",[]{
        auto f=statusFrame(0x14);CHECK(f.bytes[3]==0xbe);Profile alt;alt.checksum=ChecksumScope::PrefixAndFooter;
        CHECK(statusFrame(0x14,alt).bytes[3]==0xd5);CHECK(f.bytes[3]!=0x1a);
        alt.commands=CommandPolicy::SumPrefix;CHECK(commandFrame(0x20,alt).bytes[2]==0xca);
        alt.commands=CommandPolicy::SumWithFooter;CHECK(commandFrame(0x20,alt).bytes[2]==0xe1);
    });
    check("float_endian_golden_vectors",[]{
        uint8_t bytes[4];putFloat(bytes,1.0,FloatOrder::Little);CHECK(bytes[0]==0&&bytes[1]==0&&bytes[2]==0x80&&bytes[3]==0x3f);
        putFloat(bytes,-2.5,FloatOrder::Big);CHECK(bytes[0]==0xc0&&bytes[1]==0x20&&bytes[2]==0&&bytes[3]==0);
        CHECK(getFloat(bytes,FloatOrder::Big)==-2.5);
    });
    check("36_byte_measurement_roundtrip",[]{
        for(auto order:{FloatOrder::Little,FloatOrder::Big}){Profile profile;profile.floats=order;Data d{970,902.1,9.8,-2,3,90,-45,170};auto f=dataFrame(d,profile);CHECK(f.size==36&&f.bytes[34]==13&&f.bytes[35]==10);Parser p(profile);Event e;for(auto b:f.bytes)e=p.feed(b,10);CHECK(e.kind==Kind::Data&&e.data==d);}
    });
    check("all_288_single_bit_measurement_corruptions",[]{
        auto f=dataFrame(pad);for(size_t i=0;i<36;i++)for(int bit=0;bit<8;bit++){Parser parser;auto broken=f;broken.bytes[i]^=1<<bit;bool accepted=false;for(auto b:broken.bytes)accepted=accepted||parser.feed(b,100).kind==Kind::Data;CHECK(!accepted);}
    });
    check("all_40_single_bit_command_corruptions",[]{
        auto f=commandFrame(0x22);for(size_t i=0;i<5;i++)for(int bit=0;bit<8;bit++){Parser parser;auto broken=f;broken.bytes[i]^=1<<bit;bool accepted=false;for(size_t n=0;n<5;n++)accepted=accepted||parser.feed(broken.bytes[n],100).kind==Kind::Command;CHECK(!accepted);}
    });
    check("split_frames_noise_and_back_to_back",[]{
        Parser parser;for(uint8_t b:{0,1,2,13,10})CHECK(parser.feed(b,0).kind==Kind::None);
        auto f=commandFrame(0x20);Event e;for(size_t i=0;i<5;i++)e=parser.feed(f.bytes[i],i*19000);CHECK(e.kind==Kind::Command);
        for(size_t i=0;i<5;i++)e=parser.feed(f.bytes[i],80000+i);CHECK(e.kind==Kind::Command);
    });
    check("100ms_total_frame_timeout",[]{
        Parser parser;auto f=commandFrame(0x22);for(size_t i=0;i<4;i++)parser.feed(f.bytes[i],i*25000);
        CHECK(parser.feed(f.bytes[4],100000).kind==Kind::None);CHECK(parser.timeouts==1);
    });
    check("one_second_start_and_stop_cancellation",[]{
        Session s;command(s,0x20,0);CHECK(s.mode()==Mode::SitPending);CHECK(s.poll(999999).size==0);
        CHECK(s.poll(1000000).size==36);CHECK(s.mode()==Mode::Sit);command(s,0x24,1000001);
        CHECK(s.mode()==Mode::Normal&&s.poll(1200000).size==0&&s.output().milestones==0);
        command(s,0x22,2000000);command(s,0x24,2999999);CHECK(s.poll(3000000).size==0&&s.mode()==Mode::Normal);
    });
    check("ten_hz_without_bursts",[]{
        Session s;command(s,0x20,0);unsigned n=0;for(uint64_t t=0;t<3000000;t+=1000){auto f=s.poll(t);if(f.size){CHECK(t>=1000000&&(t-1000000)%100000==0);++n;}}CHECK(n==20);
        CHECK(s.poll(4000000).size==36);CHECK(s.poll(4000001).size==0);
    });
    check("SUT_1000ms_timeout_invalid_frames_do_not_refresh",[]{
        Session s;command(s,0x22,0);CHECK(s.poll(1000000).size==6);data(s,pad,1100000);
        auto bad=pad;bad[1]=NAN;data(s,bad,1800000);CHECK(s.invalid_data==1);
        s.poll(2099999);CHECK(s.mode()==Mode::Sut);s.poll(2100000);CHECK(s.mode()==Mode::Normal&&s.session_timeouts==1&&s.output().milestones==0);
    });
    check("SUT_real_sensor_isolation",[]{
        Session s;command(s,0x22,0);s.poll(1000000);Sample live;live.time_s=1.01;live.baro_new=live.imu_new=true;
        live.pressure_pa=90209;live.accel_m_s2={1000,0,0};s.realSample(live);CHECK(s.output().health==0&&s.output().state==State::Boot);
    });
    check("SUT_repeated_run_calibration_10hz",[]{
        Session s;s.setArm(true);
        for(int repeat=0;repeat<3;repeat++){uint64_t base=repeat*10000000ULL;command(s,0x22,base);s.poll(base+1000000);
            for(int i=0;i<40;i++)data(s,pad,base+1010000+i*100000);CHECK(s.output().state==State::Armed);CHECK(!s.output().release_requested);
            command(s,0x24,base+5100000);CHECK(s.output().state==State::Boot&&s.output().milestones==0);}
    });
    check("SIT_real_data_MSL_mBar_axes_rounding",[]{
        Session s;command(s,0x20,0);s.poll(1000000);Sample live;live.baro_new=live.imu_new=true;
        live.pressure_pa=90209.744;live.accel_m_s2={1.1234,2.3456,9.797};live.arm=true;
        for(int i=0;i<250;i++){live.time_s=1.01+i*.01;s.realSample(live);}
        auto f=s.poll(3500000);CHECK(f.size==36);Parser parser;Event e;for(auto b:f.bytes)e=parser.feed(b,3500000);
        CHECK(e.kind==Kind::Data);CHECK(std::abs(e.data[0]-970)<.1);CHECK(std::abs(e.data[1]-902.1)<.001);
        CHECK(std::abs(e.data[2]-9.8)<.001&&std::abs(e.data[3]-1.12)<.001&&std::abs(e.data[4]-2.35)<.001);
        CHECK(!s.output().release_requested&&s.output().state==State::Ready);
    });
    check("SIT_unavailable_data_is_NaN",[]{
        Session s;command(s,0x20,0);auto f=s.poll(1000000);CHECK(std::isnan(getFloat(f.bytes.data()+1,FloatOrder::Little)));
    });
    check("proper_right_handed_axis_and_attitude_conversion",[]{
        Vec3 v{1,2,3};auto a=toInternal(v);CHECK(a.x==2&&a.y==3&&a.z==1);CHECK(toOfficial(a).x==1);
        for(Vec3 angles: {Vec3{90,0,0},Vec3{0,45,0},Vec3{0,0,-90},Vec3{30,40,50}}){auto q=fromEuler(angles);auto e=toEuler(q);CHECK((e-angles).norm()<1e-8);}
        auto q=fromEuler({90,0,0});CHECK((q.rotate({0,0,1})-Vec3{0,0,1}).norm()<1e-9);
        q=fromEuler({0,90,0});CHECK(std::abs(q.rotate({0,0,1}).y+1)<1e-9);
    });
    check("status_low_byte_first_reserved_zero",[]{
        for(int i=0;i<256;i++){auto f=statusFrame(i);CHECK(f.size==6&&f.bytes[1]==i&&f.bytes[2]==0);CHECK(f.bytes[3]==uint8_t(0xaa+i));}
    });
    check("sparse_baro_values_do_not_break_real_calibration",[]{
        FlightComputer c;Sample s;s.imu_new=true;s.accel_m_s2={0,0,9.797};
        for(int i=0;i<300;i++){s.time_s=i*.01;s.baro_new=i%2==0;s.pressure_pa=s.baro_new?90209:0;c.step(s);}CHECK(c.output().state==State::Ready);
    });
    check("bounded_parser_random_stream",[]{
        Parser p;std::mt19937 rng(20261017);for(uint64_t i=0;i<200000;i++)p.feed(rng()&255,i*10);
        p.expire(3000000);auto f=commandFrame(0x24);Event e;for(size_t i=0;i<5;i++)e=p.feed(f.bytes[i],3000001+i);CHECK(e.kind==Kind::Command);
    });
    check("data_payload_can_contain_command_markers",[]{
        Data d=pad;uint32_t bits=0x0a0dabaa;std::memcpy(&d[4],&bits,4);auto f=dataFrame(d);Parser p;Event e;
        for(auto b:f.bytes){auto next=p.feed(b,1);if(next.kind!=Kind::None){CHECK(next.kind==Kind::Data);e=next;}}CHECK(e.kind==Kind::Data&&e.data==d);
    });
    check("synthetic_full_flight_all_applicable_milestones",[]{
        Session s;s.setArm(true);command(s,0x22,0);s.poll(1000000);uint8_t seen=0;
        for(int i=0;i<180;i++) {
            double ft=i*.1-4,h=0,force=9.797;
            if(ft>=0&&ft<=1){h=.5*29.391*ft*ft;force=4*9.797;}
            if(ft>1){double u=ft-1;h=14.6955+29.391*u-.5*9.797*u*u;force=0;}
            if(h<0){h=0;force=9.797;}
            float pressure=float(90209.744*std::pow(1-h*.0065/281.845,1/.190294957)/100);
            Data d{float(970+h),pressure,float(force),0,0,0,ft>5?60.0f:0.0f,0};
            data(s,d,1010000+i*100000);seen|=s.output().milestones;
        }
        CHECK(s.output().release_requested);CHECK(seen==0xdf); // Every defined A1 milestone except unused drogue.
        command(s,0x24,19100000);CHECK(s.output().milestones==0&&!s.output().release_requested);
    });
    check("SIT_rotation_90deg_and_three_metre_response",[]{
        Session s;command(s,0x20,0);s.poll(1000000);Sample live;live.imu_new=live.baro_new=true;
        for(int i=0;i<500;i++) {
            live.time_s=1.01+i*.01;double angle=std::clamp((i-250)*.01,0.0,1.0)*pi/2;
            Quaternion q;q.integrate({pi/2,0,0},angle/(pi/2));
            live.accel_m_s2=Quaternion{q.w,-q.x,-q.y,-q.z}.rotate({0,0,9.797});
            live.gyro_rad_s=(i>=250&&i<350)?Vec3{pi/2,0,0}:Vec3{};
            double h=i>=350?3:0;live.pressure_pa=90209.744*std::pow(1-h*.0065/281.845,1/.190294957);s.realSample(live);
        }
        auto f=s.poll(6000000);CHECK(std::abs(getFloat(f.bytes.data()+1,FloatOrder::Little)-973)<.1);
        CHECK(std::abs(getFloat(f.bytes.data()+25,FloatOrder::Little)-90)<.1);CHECK(!s.output().release_requested);
    });
    check("test_commands_cannot_replace_real_flight",[]{
        Session s;Sample live;live.imu_new=live.baro_new=live.arm=true;live.pressure_pa=90209;
        for(int i=0;i<500;i++){live.time_s=.01+i*.01;live.accel_m_s2={0,0,i<400?9.797:40};s.realSample(live);}
        CHECK(s.output().state==State::Boost);command(s,0x22,5100000);CHECK(s.mode()==Mode::Normal&&s.blocked_starts==1);
        command(s,0x24,5200000);CHECK(s.output().state==State::Boost);
    });
    std::cout<<"EK7 invariants: PASS\n";return 0;
} catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
