#include "fates/graphics/icamera.hpp"
#include "fates/detail/camera_runtime.hpp"
#include "fates/detail/camera_execution_runtime.hpp"
#include "fates/platform/nn_ulcd_ctr.hpp"
#include <algorithm>
#include <cmath>
namespace fates::decomp_detail { nn::ulcd::CTR::StereoCamera* gStereoCamera = nullptr; }
namespace { constexpr float kPi=3.14159265358979323846f; constexpr float kDegToRad=kPi/180.0f; constexpr float kRadToDeg=180.0f/kPi; }
void ICamera::Initialize(){ auto* const camera=new nn::ulcd::CTR::StereoCamera(); fates::decomp_detail::gStereoCamera=camera; camera->Initialize(); }
ICamera::ICamera(){ Reset(); }
ICamera::~ICamera()=default;
void ICamera::Reset(){ fates::decomp_detail::CameraResetRuntime(*this); }
void ICamera::SetupOrtho(DispType::Type display){ projectionMode_=2; fates::decomp_detail::CameraSetupOrthoRuntime(*this,static_cast<int>(display)); Update(); }
void ICamera::SetupOrtho(int width,int height){ projectionMode_=2; aspect_=height?static_cast<float>(width)/static_cast<float>(height):1.0f; Update(); }
void ICamera::SetParallax(float,float,float,float,float depth){ stereoDepth_=depth; fates::decomp_detail::CameraUpdateStereo(*this); }
void ICamera::SetParallax(const nn::math::MTX34& view,const nn::math::MTX44& proj,float depth){ view_=view; proj_=proj; stereoDepth_=depth; fates::decomp_detail::CameraUpdateStereo(*this); }
float ICamera::GetParallax(float depth) const { return IsStereoMode()?fates::decomp_detail::QueryStereoParallax(*this,depth):0.0f; }
bool ICamera::IsStereoMode() const { return stereoEnabled_ && projectionMode_!=2 && fates::decomp_detail::QueryStereoEnabled(*this); }
const nn::math::MTX44& ICamera::GetViewProj(int eye) const { return viewProj_[eye!=0?1:0]; }
void ICamera::SetFrustum(float n,float f,float fy,float a){ nearClip_=n; farClip_=f; fovyRad_=fy; aspect_=a; }
void ICamera::UpdateProj(){ fates::decomp_detail::CameraUpdateProjection(*this); }
void ICamera::UpdateView(){ fates::decomp_detail::CameraUpdateView(*this); }
void ICamera::UpdateWorld(){ fates::decomp_detail::CameraUpdateWorld(*this); }
void ICamera::UpdateStereo(){ fates::decomp_detail::CameraUpdateStereo(*this); }
void ICamera::UpdateFrustum(){ fates::decomp_detail::CameraUpdateFrustum(*this); }
void ICamera::Update(){ UpdateProj(); UpdateView(); UpdateWorld(); UpdateStereo(); UpdateFrustum(); }
void ICamera::SetRotateDeg(const nn::math::VEC3& d){ SetRotateDeg(d.x,d.y,d.z); }
void ICamera::SetRotateDeg(float x,float y,float z){ rotateRad_={x*kDegToRad,y*kDegToRad,z*kDegToRad}; UpdateView(); }
nn::math::VEC3 ICamera::GetRotateDeg() const { return {rotateRad_.x*kRadToDeg,rotateRad_.y*kRadToDeg,rotateRad_.z*kRadToDeg}; }
nn::math::VEC3 ICamera::GetWorldToScreen(const nn::math::VEC3& w) const { return fates::decomp_detail::WorldToScreen(*this,w); }
bool ICamera::IsReject(const nn::math::VEC3& c,float r) const { for(const auto& p:frustum_) if(p[0]*c.x+p[1]*c.y+p[2]*c.z+p[3] < -r) return true; return false; }
bool ICamera::IsReject(const AABB& b) const { for(const auto& p:frustum_){ const nn::math::VEC3 q{p[0]>=0?b.max.x:b.min.x,p[1]>=0?b.max.y:b.min.y,p[2]>=0?b.max.z:b.min.z}; if(p[0]*q.x+p[1]*q.y+p[2]*q.z+p[3] < 0.0f) return true; } return false; }
float ICamera::GetSqDist(const nn::math::VEC3& p) const { const float x=p.x-eye_.x,y=p.y-eye_.y,z=p.z-eye_.z; return x*x+y*y+z*z; }
float ICamera::GetSqDist(const AABB& b) const { const float x=std::max({b.min.x-eye_.x,0.0f,eye_.x-b.max.x}); const float y=std::max({b.min.y-eye_.y,0.0f,eye_.y-b.max.y}); const float z=std::max({b.min.z-eye_.z,0.0f,eye_.z-b.max.z}); return x*x+y*y+z*z; }
void ICamera::CopyFrom(const ICamera* o){ if(o) fates::decomp_detail::CameraCopyRuntime(*this,*o); }
