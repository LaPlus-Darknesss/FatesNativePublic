#include "fates/graphics/camera_param.hpp"
#include <algorithm>
#include <cmath>
namespace { constexpr float kPi=3.14159265358979323846f; constexpr float kDegToRad=kPi/180.0f; constexpr float kRadToDeg=180.0f/kPi; float lerp(float a,float b,float t){return a+(b-a)*t;} nn::math::VEC3 mix(nn::math::VEC3 a,nn::math::VEC3 b,float t){return {lerp(a.x,b.x,t),lerp(a.y,b.y,t),lerp(a.z,b.z,t)};} }
CameraParam::CameraParam(){ ResetParam(); }
CameraParam::~CameraParam()=default;
void CameraParam::ResetParam(){ eye={0.0f,0.0f,10.0f}; at={}; up={0.0f,1.0f,0.0f}; fovyRad=45.0f*kDegToRad; nearClip=1.0f; farClip=10000.0f; depthLevel=0.0f; depthFactor=1.0f; extra=0.0f; }
void CameraParam::SetFovyDeg(float d){ fovyRad=d*kDegToRad; }
float CameraParam::GetFovyDeg() const { return fovyRad*kRadToDeg; }
void CameraParam::BlendParam(const CameraParam* a,const CameraParam* b,float t){ BlendParam(a,b,t,t,t,t); }
void CameraParam::BlendParam(const CameraParam* a,const CameraParam* b,float,float eyeT,float atT,float scalarT){ if(!a||!b) return; eye=mix(a->eye,b->eye,eyeT); at=mix(a->at,b->at,atT); up=mix(a->up,b->up,scalarT); fovyRad=lerp(a->fovyRad,b->fovyRad,scalarT); nearClip=lerp(a->nearClip,b->nearClip,scalarT); farClip=lerp(a->farClip,b->farClip,scalarT); depthLevel=lerp(a->depthLevel,b->depthLevel,scalarT); depthFactor=lerp(a->depthFactor,b->depthFactor,scalarT); extra=lerp(a->extra,b->extra,scalarT); }
void CameraParam::SetLookAtTarget(const nn::math::VEC3& target,const nn::math::VEC3& r,float distance){ at=target; const float cy=std::cos(r.y),sy=std::sin(r.y),cp=std::cos(r.x),sp=std::sin(r.x); eye={target.x-distance*sy*cp,target.y+distance*sp,target.z-distance*cy*cp}; up={0.0f,1.0f,0.0f}; }
void CameraParam::SetLookAtPos(const nn::math::VEC3& pos,const nn::math::VEC3& r,float distance){ eye=pos; const float cy=std::cos(r.y),sy=std::sin(r.y),cp=std::cos(r.x),sp=std::sin(r.x); at={pos.x+distance*sy*cp,pos.y-distance*sp,pos.z+distance*cy*cp}; up={0.0f,1.0f,0.0f}; }
void CameraParam::RotationPos(const nn::math::VEC3& r){ nn::math::VEC3 d{eye.x-at.x,eye.y-at.y,eye.z-at.z}; const float cx=std::cos(r.x),sx=std::sin(r.x),cy=std::cos(r.y),sy=std::sin(r.y); const nn::math::VEC3 dx{d.x,d.y*cx-d.z*sx,d.y*sx+d.z*cx}; const nn::math::VEC3 dy{dx.x*cy+dx.z*sy,dx.y,-dx.x*sy+dx.z*cy}; eye={at.x+dy.x,at.y+dy.y,at.z+dy.z}; }
void CameraParam::CopyParam(const CameraParam* o){ if(o) *this=*o; }
