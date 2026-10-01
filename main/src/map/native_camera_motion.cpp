#include "fates/map/native_camera_motion.hpp"
#include <bit>
#include <cmath>
#include <limits>

namespace fates::map::native {
namespace {
// VFP scalar instructions round each result separately. In particular VMLA
// here is multiply then add, not a fused host FMA.
float Add(float a,float b) noexcept {volatile float r=a+b;return r;}
float Sub(float a,float b) noexcept {volatile float r=a-b;return r;}
float Mul(float a,float b) noexcept {volatile float r=a*b;return r;}
float Div(float a,float b) noexcept {volatile float r=a/b;return r;}
bool Finite(float x) noexcept {return std::isfinite(x);}
bool Finite(HeightVector v) noexcept {return Finite(v.x)&&Finite(v.y)&&Finite(v.z);}
float Read(const FieldConfiguration& c,std::size_t offset) noexcept {
    const auto* p=c.numeric.data()+offset-0x18;
    return std::bit_cast<float>(std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|
        (std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24));
}
float Clamp(float v,float lo,float hi) noexcept {
    if(lo>hi)return Mul(Add(lo,hi),0.5f);
    if(v>=hi)return hi;
    return v>lo?v:lo;
}
// ARM VCVT.S32.F32 saturates and truncates toward zero, with NaN -> zero.
std::int32_t Convert(float v) noexcept {
    if(std::isnan(v))return 0;
    if(v>=2147483648.0f)return std::numeric_limits<std::int32_t>::max();
    if(v<=-2147483648.0f)return std::numeric_limits<std::int32_t>::min();
    return static_cast<std::int32_t>(v);
}
float Delta(std::uint32_t d,std::uint8_t mode,bool roundtrip) noexcept {
    if(mode==1)d<<=1;
    const auto f=static_cast<float>(std::bit_cast<std::int32_t>(d));
    return roundtrip?static_cast<float>(Convert(f)):f;
}
bool Advance(float& elapsed,float delta,float& ratio) noexcept {
    if(!Finite(elapsed))return false;
    const auto sum=Add(elapsed,delta);if(!Finite(sum))return false;
    elapsed=60.0f>=sum?sum:60.0f;
    return CameraMoveRatioExact(Convert(elapsed),ratio);
}
bool Interpolate(float origin,float target,float ratio,float& out) noexcept {
    if(!Finite(origin)||!Finite(target))return false;
    const auto difference=Sub(target,origin),product=Mul(difference,ratio);
    const auto result=Add(Div(product,1.0f),origin);
    if(!Finite(difference)||!Finite(product)||!Finite(result))return false;
    out=result;return true;
}
bool Rotate(HeightVector degrees,HeightVector& radians) noexcept {
    const auto k=std::bit_cast<float>(std::uint32_t{0x3c8efa35});
    const HeightVector r{Mul(degrees.x,k),Mul(degrees.y,k),Mul(degrees.z,k)};
    if(!Finite(degrees)||!Finite(r))return false;radians=r;return true;
}
}
CameraFieldParameters ReadCameraFieldParameters(const FieldConfiguration& c) noexcept {
    CameraFieldParameters p;
    for(unsigned i=0;i<3;++i) {
        p.distances[i]=Read(c,0x80+4*i);p.near_limits[i]=Read(c,0x8c+4*i);
        p.far_limits[i]=Read(c,0x98+4*i);p.angles[i]=Read(c,0xa4+4*i);
    }
    p.stereo_depth=Read(c,0xb0);p.stereo_intensity=Read(c,0xb4);return p;
}
bool CameraMoveRatioExact(std::int32_t frame,float& output) noexcept {
    const auto t=Sub(60.0f,static_cast<float>(frame));
    auto power=Mul(t,t);power=Mul(t,power);power=Mul(t,power);
    auto scale=Mul(3600.0f,60.0f);scale=Mul(scale,60.0f);
    const auto ratio=Sub(1.0f,Div(Mul(Sub(1.0f,0.0f),power),scale));
    if(!Finite(power)||!Finite(ratio))return false;
    // Original signed-word upper test precedes its floating lower test.
    output=std::bit_cast<std::int32_t>(ratio)>=0x3f800000?1.0f:ratio<=0.0f?0.0f:ratio;
    return true;
}
bool LimitCameraTargetExact(HeightVector v,float distance,CameraPlayArea a,
    const CameraFieldParameters& p,HeightVector& out) noexcept {
    if(!Finite(v)||!Finite(distance)||!Finite(p.distances[0])||!Finite(p.distances[2]))return false;
    for(auto f:p.near_limits)if(!Finite(f))return false;
    for(auto f:p.far_limits)if(!Finite(f))return false;
    const auto span=Sub(p.distances[2],p.distances[0]);
    const auto offset=Sub(distance,p.distances[0]);
    const auto ratio=Div(offset,span);
    if(!Finite(span)||!Finite(offset)||!Finite(ratio))return false;
    const auto limits=[&](const std::array<float,3>& l,HeightVector& result) {
        const auto x0=Add(static_cast<float>(a.x),l[0]);
        const auto x1=Sub(static_cast<float>(a.x+a.width),l[0]);
        const auto y0=Add(static_cast<float>(a.y),l[1]);
        const auto y1=Sub(static_cast<float>(a.y+a.height),l[2]);
        if(!Finite(x0)||!Finite(x1)||!Finite(y0)||!Finite(y1))return false;
        result={Clamp(v.x,x0,x1),v.y,Clamp(v.z,y0,y1)};return Finite(result);
    };
    HeightVector near,far,result;
    if(!limits(p.near_limits,near)||!limits(p.far_limits,far))return false;
    const auto blend=[&](float x,float y,float& z) {
        const auto difference=Sub(y,x),product=Mul(difference,ratio);
        z=Add(x,Mul(product,1.0f));return Finite(difference)&&Finite(product)&&Finite(z);
    };
    if(!blend(near.x,far.x,result.x)||!blend(near.y,far.y,result.y)||!blend(near.z,far.z,result.z))return false;
    out=result;return true;
}
bool CameraScrollProlixityExact(const CameraMotionState& s,bool& out) noexcept {
    if(!Finite(s.current)||!Finite(s.limited))return false;
    const auto dx=Sub(s.limited.x,s.current.x),dy=Sub(s.limited.y,s.current.y),dz=Sub(s.limited.z,s.current.z);
    const auto x=Mul(dx,dx),y=Mul(dy,dy),z=Mul(dz,dz),xy=Add(x,y),sum=Add(xy,z);
    if(!Finite(dx)||!Finite(dy)||!Finite(dz)||!Finite(x)||!Finite(y)||!Finite(z)||!Finite(xy)||!Finite(sum))return false;
    out=std::bit_cast<std::int32_t>(sum)>=0x40800000 && std::bit_cast<std::int32_t>(s.elapsed)<0x41f00000;
    return true;
}
bool UpdateCameraTargetExact(CameraMotionState& s,CameraPlayArea a,const CameraFieldParameters& p) noexcept {
    HeightVector limited;
    if(!Finite(s.current)||!Finite(s.target)||!LimitCameraTargetExact(s.destination,s.distance.destination,a,p,limited))return false;
    s.limited=limited;
    if(limited!=s.target) {s.origin=s.current;s.elapsed=0.0f;s.target=limited;}
    return true;
}
bool TickCameraTargetExact(CameraMotionState& s,std::uint32_t delta,CameraPlayArea a,const CameraFieldParameters& p) noexcept {
    auto n=s;float ratio;
    if(!UpdateCameraTargetExact(n,a,p)||!Advance(n.elapsed,Delta(delta,n.mode,true),ratio)||!Finite(n.offset))return false;
    if(!Interpolate(n.origin.x,n.target.x,ratio,n.current.x)||!Interpolate(n.origin.y,n.target.y,ratio,n.current.y)||
        !Interpolate(n.origin.z,n.target.z,ratio,n.current.z))return false;
    n.position={Add(n.current.x,n.offset.x),Add(n.current.y,n.offset.y),Add(n.current.z,n.offset.z)};
    if(!Finite(n.position))return false;s=n;return true;
}
bool UpdateCameraDistanceExact(CameraMotionState& s) noexcept {
    auto n=s;auto& d=n.distance;
    if(!Finite(d.destination)||!Finite(d.target)||!Finite(n.view_distance))return false;
    if(d.destination!=d.target){d.elapsed=0.0f;d.origin=n.view_distance;d.target=d.destination;}
    s=n;return true;
}
bool TickCameraDistanceExact(CameraMotionState& s,std::uint32_t delta,const CameraFieldParameters& p) noexcept {
    auto n=s;if(!UpdateCameraDistanceExact(n)||!Finite(p.stereo_depth)||!Finite(p.stereo_intensity))return false;
    auto& d=n.distance;
    float ratio;
    if(!Advance(d.elapsed,Delta(delta,n.mode,true),ratio)||!Interpolate(d.origin,d.target,ratio,n.view_distance))return false;
    n.stereo_depth=Add(p.stereo_depth,n.view_distance);n.stereo_intensity=p.stereo_intensity;
    if(!Finite(n.stereo_depth))return false;s=n;return true;
}
bool UpdateCameraAngleExact(CameraMotionState& s) noexcept {
    auto n=s;auto& pitch=n.pitch;auto& yaw=n.yaw;
    if(!Finite(n.rotation_radians)||!Finite(pitch.destination)||!Finite(pitch.target)||!Finite(yaw.destination)||!Finite(yaw.target))return false;
    // ICamera::GetRotateDeg uses an independently rounded original constant.
    const auto k=std::bit_cast<float>(std::uint32_t{0x42652ee0});
    if(pitch.destination!=pitch.target){pitch.elapsed=0.0f;pitch.origin=Mul(n.rotation_radians.x,k);pitch.target=pitch.destination;}
    if(yaw.destination!=yaw.target){yaw.elapsed=0.0f;yaw.origin=Mul(n.rotation_radians.y,k);yaw.target=yaw.destination;}
    if(!Finite(pitch.origin)||!Finite(yaw.origin))return false;
    s=n;return true;
}
bool TickCameraAngleExact(CameraMotionState& s,std::uint32_t delta) noexcept {
    auto n=s;if(!UpdateCameraAngleExact(n))return false;auto& pitch=n.pitch;auto& yaw=n.yaw;
    float rp,ry,x,y;const auto dt=Delta(delta,n.mode,false);
    if(!Advance(pitch.elapsed,dt,rp)||!Advance(yaw.elapsed,dt,ry)||
        !Interpolate(pitch.origin,pitch.target,rp,x)||!Interpolate(yaw.origin,yaw.target,ry,y)||!Rotate({x,y,0.0f},n.rotation_radians))return false;
    s=n;return true;
}
bool InstantCameraExact(CameraMotionState& s,float height,CameraPlayArea a,const CameraFieldParameters& p) noexcept {
    auto n=s;if(!Finite(height)||!Finite(n.distance.destination)||!Finite(p.stereo_depth)||!Finite(p.stereo_intensity))return false;
    n.destination.y=height;
    // Instant limits against the PREVIOUS ICamera distance, then publishes the
    // new destination distance below. This differs from UpdateTarget.
    if(!LimitCameraTargetExact(n.destination,n.view_distance,a,p,n.limited))return false;
    n.current=n.origin=n.target=n.position=n.limited;n.elapsed=60.0f;
    n.pitch.origin=n.pitch.target=n.pitch.destination;n.pitch.elapsed=60.0f;
    n.yaw.origin=n.yaw.target=n.yaw.destination;n.yaw.elapsed=60.0f;
    if(!Rotate({n.pitch.destination,n.yaw.destination,0.0f},n.rotation_radians))return false;
    n.distance.origin=n.distance.target=n.view_distance=n.distance.destination;n.distance.elapsed=60.0f;
    n.stereo_depth=Add(p.stereo_depth,n.distance.destination);n.stereo_intensity=p.stereo_intensity;
    if(!Finite(n.stereo_depth))return false;s=n;return true;
}
bool TickUnblendedCameraExact(CameraMotionState& s,std::uint32_t delta,CameraPlayArea a,const CameraFieldParameters& p) noexcept {
    auto n=s;
    if(n.mode!=2 && n.mode!=3) {
        if(!TickCameraDistanceExact(n,delta,p) || !TickCameraTargetExact(n,delta,a,p) || !TickCameraAngleExact(n,delta))return false;
        // TickBlend at3E0F6C returns immediately for the admitted duration0.
    }
    n.mode=0;s=n;return true;
}
bool SetCameraDestinationIntExact(CameraMotionState& s,std::int32_t x,std::int32_t y,float height,
    CameraPlayArea a,const CameraFieldParameters& p) noexcept {
    if(!Finite(height))return false;auto n=s;
    n.destination={Add(static_cast<float>(x),0.5f),height,Add(static_cast<float>(y),0.5f)};
    if(!UpdateCameraTargetExact(n,a,p))return false;s=n;return true;
}
bool SetCameraDestinationFloatExact(CameraMotionState& s,float x,float y,float height,
    CameraPlayArea a,const CameraFieldParameters& p) noexcept {
    if(!Finite(x)||!Finite(y)||!Finite(height))return false;
    // The executable falls through from3E024C into UpdateTarget at3E0250;
    // the exported function boundary is not a return instruction.
    auto n=s;n.destination={x,height,y};
    if(!UpdateCameraTargetExact(n,a,p))return false;s=n;return true;
}
}
