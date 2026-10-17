#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace inova {
constexpr double pi = 3.14159265358979323846;
struct Vec3 {
    double x=0, y=0, z=0;
    Vec3 operator+(Vec3 v) const { return {x+v.x,y+v.y,z+v.z}; }
    Vec3 operator-(Vec3 v) const { return {x-v.x,y-v.y,z-v.z}; }
    Vec3 operator*(double s) const { return {x*s,y*s,z*s}; }
    double norm() const { return std::sqrt(x*x+y*y+z*z); }
    bool finite() const { return std::isfinite(x)&&std::isfinite(y)&&std::isfinite(z); }
};
struct Quaternion {
    double w=1,x=0,y=0,z=0;
    Quaternion operator*(Quaternion q) const {
        return {w*q.w-x*q.x-y*q.y-z*q.z, w*q.x+x*q.w+y*q.z-z*q.y,
                w*q.y-x*q.z+y*q.w+z*q.x, w*q.z+x*q.y-y*q.x+z*q.w};
    }
    void normalize() { double n=std::sqrt(w*w+x*x+y*y+z*z); w/=n;x/=n;y/=n;z/=n; }
    Vec3 rotate(Vec3 v) const {
        auto r=(*this)*Quaternion{0,v.x,v.y,v.z}*Quaternion{w,-x,-y,-z};
        return {r.x,r.y,r.z};
    }
    static Quaternion alignUp(Vec3 a) {
        a=a*(1/a.norm());
        if(a.z < -0.999999) return {0,1,0,0};
        Quaternion q{1+a.z,a.y,-a.x,0};q.normalize();return q;
    }
    void integrate(Vec3 omega, double dt) {
        double angle=omega.norm()*dt;
        if(angle<1e-12) return;
        double s=std::sin(angle/2)/omega.norm();
        *this=(*this)*Quaternion{std::cos(angle/2),omega.x*s,omega.y*s,omega.z*s};normalize();
    }
};
enum class State : uint8_t { Boot, Ready, Armed, Boost, Coast, Descent, Landed, Fault };
inline const char* stateName(State s) {
    constexpr const char* names[]={"BOOT","READY","ARMED","BOOST","COAST","DESCENT","LANDED","FAULT"};
    return names[static_cast<unsigned>(s)];
}
enum Health : uint16_t { BaroBad=1, ImuBad=2, InertialLost=4, Saturated=8, ClockBad=16 };
struct Config {
    double gravity=9.797; // From this .ork site; stationary magnitude also measured at boot.
    double calibration_s=2.0;
    double arm_confirm_s=0.25;
    double baro_timeout_s=0.12, imu_timeout_s=0.04;
    double launch_g=1.8, launch_confirm_s=0.08;
    double coast_g=0.55, coast_confirm_s=0.15;
    double minimum_recovery_height_m=50;
    double drop_from_peak_m=1.5, descent_speed_m_s=-0.8;
    double inertial_descent_speed_m_s=-0.5, descent_confirm_s=0.10;
    double altitude_filter_tau_s=0.05;
    double launch_temperature_k=281.845; // .ork ISA launch site at 970 m MSL.
    double max_baro_rate_m_s=350;
};
struct Sample {
    double time_s=0;
    double pressure_pa=0;
    Vec3 accel_m_s2{}, gyro_rad_s{}; // Body +Z is noseward; accelerometer is SPECIFIC force.
    bool baro_new=false, imu_new=false, arm=false;
};
struct Output {
    State state=State::Boot;
    double altitude_m=0, baro_velocity_m_s=0, inertial_velocity_m_s=0;
    double peak_m=0, tilt_deg=0;
    bool release_requested=false;
    uint16_t health=0;
};

// Same class is compiled for the desktop runner and ESP32. No Arduino dependencies.
class FlightComputer {
public:
    explicit FlightComputer(Config c={}) : cfg(c) {}
    const Output& output() const { return out; }
    Output step(const Sample& s) {
        if(!std::isfinite(s.time_s)||s.time_s<0 || (started && s.time_s<=last_time)) {
            out.health|=ClockBad;out.state=State::Fault;return out;
        }
        started=true;last_time=s.time_s;
        bool imu_ok=s.imu_new && s.accel_m_s2.finite() && s.gyro_rad_s.finite();
        if(imu_ok && (std::max({std::abs(s.accel_m_s2.x),std::abs(s.accel_m_s2.y),std::abs(s.accel_m_s2.z)})>=15.8*cfg.gravity ||
                       std::max({std::abs(s.gyro_rad_s.x),std::abs(s.gyro_rad_s.y),std::abs(s.gyro_rad_s.z)})>=1990*pi/180)) {
            out.health|=Saturated;imu_ok=false;
            if(inFlight()) { inertial_valid=false;out.health|=InertialLost; }
        }
        bool baro_ok=s.baro_new && std::isfinite(s.pressure_pa) && s.pressure_pa>=30000 && s.pressure_pa<=125000;
        if(out.state==State::Boot) { calibrate(s,imu_ok,baro_ok);return out; }
        if(imu_ok) {
            double dt=s.time_s-last_imu;
            if(dt>cfg.imu_timeout_s && (inFlight()||out.state==State::Armed)) {
                inertial_valid=false;out.health|=InertialLost;
                if(out.state==State::Armed)out.state=State::Fault;
            }
            attitude.integrate(s.gyro_rad_s-gyro_bias,std::min(dt,cfg.imu_timeout_s));
            double az=attitude.rotate(s.accel_m_s2).z-measured_g;
            if(inFlight() && inertial_valid) out.inertial_velocity_m_s+=0.5*(previous_az+az)*dt;
            previous_az=az;last_imu_dt=dt;last_imu=s.time_s;last_accel=s.accel_m_s2.norm();
            out.tilt_deg=std::acos(std::clamp(attitude.rotate({0,0,1}).z,-1.0,1.0))*180/pi;
        }
        if(baro_ok) baro_ok=updateBaro(s);
        out.health &= static_cast<uint16_t>(~(BaroBad|ImuBad));
        bool baro_fresh=s.time_s-last_baro<=cfg.baro_timeout_s;
        bool imu_fresh=s.time_s-last_imu<=cfg.imu_timeout_s;
        if(!baro_fresh) { out.health|=BaroBad;count=0;descent_since=-1; }
        if(!imu_fresh) {
            out.health|=ImuBad;
            if(inFlight()) { inertial_valid=false;out.health|=InertialLost; }
            if(out.state==State::Armed)out.state=State::Fault;
        }
        if(out.state==State::Fault) return out;
        if(!s.arm) {
            launch_since=-1;launch_impulse=0;arm_since=-1;
            if(out.state==State::Armed) out.state=State::Ready;
            else if(inFlight() && !out.release_requested) out.state=State::Fault;
            return out;
        }
        if(out.state==State::Ready) {
            bool stationary=imu_ok && last_imu_dt<=cfg.imu_timeout_s && last_accel>0.85*measured_g &&
                last_accel<1.15*measured_g && (s.gyro_rad_s-gyro_bias).norm()<0.10;
            if(baro_fresh && imu_fresh && stationary && !(out.health&InertialLost)) {
                if(arm_since<0)arm_since=s.time_s;
                if(s.time_s-arm_since>=cfg.arm_confirm_s)out.state=State::Armed;
            } else arm_since=-1;
        }
        if(out.state==State::Armed && imu_ok && baro_fresh) {
            if(last_accel>cfg.launch_g*measured_g) {
                if(launch_since<0) {launch_since=s.time_s;launch_impulse=0;}
                launch_impulse+=previous_az*last_imu_dt;
                if(s.time_s-launch_since>=cfg.launch_confirm_s) {
                    out.state=State::Boost;out.inertial_velocity_m_s=launch_impulse;coast_since=-1;
                }
            } else {launch_since=-1;launch_impulse=0;}
        }
        if(out.state==State::Boost && imu_ok) {
            if(last_accel<cfg.coast_g*measured_g) {
                if(coast_since<0) coast_since=s.time_s;
                if(s.time_s-coast_since>=cfg.coast_confirm_s) out.state=State::Coast;
            } else coast_since=-1;
        }
        if(out.state==State::Coast && baro_ok) {
            bool descending=baro_fresh && imu_fresh && inertial_valid && count>=16 &&
                out.altitude_m>=cfg.minimum_recovery_height_m &&
                out.peak_m-out.altitude_m>=cfg.drop_from_peak_m &&
                out.baro_velocity_m_s<cfg.descent_speed_m_s &&
                out.inertial_velocity_m_s<cfg.inertial_descent_speed_m_s;
            if(descending) {
                if(descent_since<0) descent_since=s.time_s;
                if(s.time_s-descent_since>=cfg.descent_confirm_s) {
                    out.release_requested=true;out.state=State::Descent;
                }
            } else descent_since=-1;
        }
        if(out.state==State::Descent && baro_fresh) {
            if(out.altitude_m<25 && std::abs(out.baro_velocity_m_s)<1) {
                if(landed_since<0) landed_since=s.time_s;
                if(s.time_s-landed_since>=3) out.state=State::Landed;
            } else landed_since=-1;
        }
        return out;
    }
private:
    Config cfg;
    Output out{};
    Quaternion attitude{};
    Vec3 gyro_bias{},accel_sum{},gyro_sum{};
    double measured_g=9.797, reference_pressure=0, pressure_sum=0;
    unsigned calibration_count=0,calibration_baro_count=0;
    double calibration_start=-1,calibration_p_min=1e9,calibration_p_max=0;
    bool started=false,inertial_valid=true;
    double last_time=0,last_baro=-1e6,last_imu=-1e6,previous_az=0,last_accel=0,last_imu_dt=0.01;
    double arm_since=-1,launch_since=-1,launch_impulse=0,coast_since=-1,descent_since=-1,landed_since=-1;
    std::array<double,21> times{},heights{};
    std::array<double,3> median_buffer{};
    unsigned count=0,head=0,median_count=0,median_head=0;
    double previous_raw_alt=0,filtered_alt=0;
    bool inFlight() const {return out.state==State::Boost||out.state==State::Coast||out.state==State::Descent;}
    void resetCalibration() {
        calibration_start=-1;calibration_count=calibration_baro_count=0;accel_sum={};gyro_sum={};pressure_sum=0;
        calibration_p_min=1e9;calibration_p_max=0;
    }
    void calibrate(const Sample& s,bool imu_ok,bool baro_ok) {
        if(baro_ok) {
            last_baro=s.time_s;pressure_sum+=s.pressure_pa;calibration_baro_count++;
            calibration_p_min=std::min(calibration_p_min,s.pressure_pa);calibration_p_max=std::max(calibration_p_max,s.pressure_pa);
        }
        out.health=(s.time_s-last_imu>cfg.imu_timeout_s?ImuBad:0)|(s.time_s-last_baro>cfg.baro_timeout_s?BaroBad:0);
        if(s.time_s-last_baro>cfg.baro_timeout_s || (s.imu_new&&!imu_ok) || (s.baro_new&&!baro_ok)) {resetCalibration();return;}
        if(!imu_ok) {if(s.time_s-last_imu>cfg.imu_timeout_s)resetCalibration();return;}
        last_imu=s.time_s;
        if(s.accel_m_s2.norm()<0.85*cfg.gravity ||
           s.accel_m_s2.norm()>1.15*cfg.gravity || s.gyro_rad_s.norm()>0.10) {resetCalibration();return;}
        if(calibration_start<0) calibration_start=s.time_s;
        accel_sum=accel_sum+s.accel_m_s2;gyro_sum=gyro_sum+s.gyro_rad_s;calibration_count++;
        calibration_p_min=std::min(calibration_p_min,s.pressure_pa);calibration_p_max=std::max(calibration_p_max,s.pressure_pa);
        if(calibration_p_max-calibration_p_min>40) {resetCalibration();return;}
        if(s.time_s-calibration_start>=cfg.calibration_s && calibration_count>=90 && calibration_baro_count>=50) {
            Vec3 a=accel_sum*(1.0/calibration_count);
            if(a.norm()<0.9*cfg.gravity) {resetCalibration();return;}
            measured_g=a.norm();attitude=Quaternion::alignUp(a);gyro_bias=gyro_sum*(1.0/calibration_count);
            reference_pressure=pressure_sum/calibration_baro_count;
            last_baro=last_imu=s.time_s;out.state=State::Ready;out.health=0;
        }
    }
    bool updateBaro(const Sample& s) {
        double raw=(cfg.launch_temperature_k/0.0065)*(1-std::pow(s.pressure_pa/reference_pressure,0.190294957));
        double dt=s.time_s-last_baro;
        if(dt<=0) return false;
        if(dt<=cfg.baro_timeout_s && count && std::abs(raw-previous_raw_alt)>2+cfg.max_baro_rate_m_s*dt) return false;
        if(dt>cfg.baro_timeout_s) {count=0;median_count=0;descent_since=-1;filtered_alt=raw;}
        previous_raw_alt=raw;last_baro=s.time_s;
        median_buffer[median_head]=raw;median_head=(median_head+1)%3;median_count=std::min(median_count+1,3u);
        auto sorted=median_buffer;std::sort(sorted.begin(),sorted.begin()+median_count);
        double med=sorted[median_count/2];
        filtered_alt+=dt/(cfg.altitude_filter_tau_s+dt)*(med-filtered_alt);
        out.altitude_m=filtered_alt;
        times[head]=s.time_s;heights[head]=filtered_alt;head=(head+1)%times.size();count=std::min(count+1,static_cast<unsigned>(times.size()));
        if(count>=3) {
            double tx=0,hy=0,xy=0,xx=0;
            for(unsigned i=0;i<count;i++) {double t=times[i]-s.time_s;tx+=t;hy+=heights[i];xy+=t*heights[i];xx+=t*t;}
            double denom=count*xx-tx*tx;
            if(denom>1e-9) out.baro_velocity_m_s=(count*xy-tx*hy)/denom;
        }
        if(inFlight()) out.peak_m=std::max(out.peak_m,out.altitude_m);
        return true;
    }
};
} // namespace inova
