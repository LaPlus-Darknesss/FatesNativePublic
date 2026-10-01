#include "fates/graphics/camera_motion.hpp"
#include "fates/detail/principal_camera_runtime.hpp"
float CameraCurve::GetValue(int t,float a,float b,float c,float d){return fates::decomp_detail::CameraCurveValue(t,a,b,c,d);} 
CameraInput::CameraInput(){Reset();} void CameraInput::Reset(){rotation_={};zoom_=0.0f;type_=Type::None;latched_=false;} void CameraInput::Tick(Type t,float dt,float h,float v){type_=t;fates::decomp_detail::CameraInputTick(*this,static_cast<int>(t),dt,h,v);} 
CameraTrack::~CameraTrack()=default;
