#include "fates/map/native_map_camera.hpp"
#include "fates/runtime/native_runtime.hpp"
#include <cmath>
#include <limits>

namespace fates::map::native {
using namespace runtime::native;
using S=MapCameraStatus;
namespace {
bool Finite(const CameraMotionState& s) {
    for(auto v:{s.destination,s.offset,s.current,s.limited,s.origin,s.target,s.position,s.rotation_radians})
        if(!std::isfinite(v.x)||!std::isfinite(v.y)||!std::isfinite(v.z))return false;
    for(auto a:{s.distance,s.pitch,s.yaw})for(auto f:{a.destination,a.origin,a.target,a.elapsed})if(!std::isfinite(f))return false;
    for(auto f:{s.elapsed,s.view_distance,s.stereo_depth,s.stereo_intensity})if(!std::isfinite(f))return false;
    return true;
}
float Center(std::int32_t v) {volatile float r=static_cast<float>(v)+0.5f;return r;}
}
S NativeMapCamera::Create(std::shared_ptr<NativeRuntime> r,std::shared_ptr<NativeProcessScheduler> scheduler,
    std::shared_ptr<NativeMapCamera>& out) {
    if(!r)return S::NullRuntime;if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    auto next=std::shared_ptr<NativeMapCamera>(new NativeMapCamera);next->runtime_=std::move(r);next->scheduler_=scheduler;
    out=std::move(next);return S::Ready;
}
S NativeMapCamera::Mutable(ProcessAccess* a) const {
    auto scheduler=scheduler_.lock();if(retired_ || !scheduler || !scheduler->root(2))return S::Retired;
    if(a)return a->BelongsTo(*scheduler)?S::Ready:S::MismatchedDomain;
    return scheduler->busy()?S::Busy:S::Ready;
}
S NativeMapCamera::Validate() const {
    auto scheduler=scheduler_.lock();if(retired_ || !scheduler || !scheduler->root(2))return S::Retired;
    if(!known_)return S::UnknownCurrent;if(!current_)return S::Absent;
    const auto& g=runtime_->game;
    if(world_!=g.field_scene.owner || chapter_!=g.campaign.current_chapter_index || map_active_!=g.map_active)return S::StaleField;
    return S::Ready;
}
S NativeMapCamera::FieldParameters(CameraFieldParameters& parameters) const {
    const auto& g=runtime_->game;const auto& f=g.field_scene;const auto& snapshot=f.snapshot;
    if(!f.owner || !snapshot)return S::MissingField;
    if(snapshot->world!=f.owner || snapshot->revision!=f.revision || snapshot->terrain_revision!=f.terrain_revision ||
        snapshot->chapter!=g.campaign.current_chapter_index || snapshot->map_active!=g.map_active || snapshot->updates_pending)return S::StaleField;
    // Null config is an admitted original value only when the actual current
    // field snapshot explicitly publishes it. Unknown field never means null.
    parameters=snapshot->configuration?ReadCameraFieldParameters(*snapshot->configuration):CameraFieldParameters{};
    return S::Ready;
}
S NativeMapCamera::Environment(CameraPlayArea& area,CameraFieldParameters& parameters) const {
    if(auto status=FieldParameters(parameters);status!=S::Ready)return status;
    const auto* t=runtime_->definitions.terrain_map();if(!t)return S::MissingTerrain;
    if(t->min_x>255 || t->min_y>255 || t->max_x>255 || t->max_y>255)return S::MissingTerrain;
    area={t->min_x,t->min_y,(t->max_x-t->min_x)&255u,(t->max_y-t->min_y)&255u};
    return S::Ready;
}
S NativeMapCamera::RestoreCarried(const CameraMotionState& motion,MapCameraHandle& out,std::optional<CameraBlendClock> blend) {
    if(auto status=Mutable(nullptr);status!=S::Ready)return status;
    CameraPlayArea a;CameraFieldParameters p;if(auto status=Environment(a,p);status!=S::Ready)return status;
    if(!Finite(motion))return S::InvalidMotion;
    if(serial_==std::numeric_limits<std::uint64_t>::max())return S::IdentityExhausted;
    auto identity=std::make_shared<const MapCameraIdentity>(MapCameraIdentity{++serial_});
    world_=runtime_->game.field_scene.owner;chapter_=runtime_->game.campaign.current_chapter_index;map_active_=runtime_->game.map_active;
    current_=MapCameraSnapshot{identity,motion,blend};known_=true;out=std::move(identity);return S::Ready;
}
S NativeMapCamera::PublishAbsent() {
    if(auto status=Mutable(nullptr);status!=S::Ready)return status;
    current_.reset();world_.reset();known_=true;return S::Ready;
}
std::optional<MapCameraHandle> NativeMapCamera::Current() const {
    const auto status=Validate();if(status==S::Absent)return MapCameraHandle{};
    if(status!=S::Ready)return {};return current_->identity;
}
std::optional<MapCameraSnapshot> NativeMapCamera::Observe(MapCameraHandle h) const {
    if(Validate()!=S::Ready || current_->identity!=h)return {};return current_;
}
S NativeMapCamera::ScrollProlixity(bool& out) const {
    if(auto status=Validate();status!=S::Ready)return status;
    return CameraScrollProlixityExact(current_->motion,out)?S::Ready:S::InvalidMotion;
}
S NativeMapCamera::InstantImpl(ProcessAccess* access) {
    if(auto status=Mutable(access);status!=S::Ready)return status;
    if(auto status=Validate();status!=S::Ready)return status;
    CameraPlayArea a;CameraFieldParameters p;if(auto status=Environment(a,p);status!=S::Ready)return status;
    const auto height=QueryCurrentMapHeight(*runtime_,current_->motion.destination,false);
    if(height.status!=HeightStatus::Ok)return S::MissingHeight;
    return InstantCameraExact(current_->motion,height.height,a,p)?S::Ready:S::InvalidMotion;
}
S NativeMapCamera::DestinationImpl(ProcessAccess* access,std::int32_t x,std::int32_t y) {
    if(auto status=Mutable(access);status!=S::Ready)return status;
    if(auto status=Validate();status!=S::Ready)return status;
    CameraPlayArea a;CameraFieldParameters p;if(auto status=Environment(a,p);status!=S::Ready)return status;
    const auto height=QueryCurrentMapHeight(*runtime_,{Center(x),current_->motion.destination.y,Center(y)},false);
    if(height.status!=HeightStatus::Ok)return S::MissingHeight;
    return SetCameraDestinationIntExact(current_->motion,x,y,height.height,a,p)?S::Ready:S::InvalidMotion;
}
S NativeMapCamera::TargetImpl(ProcessAccess* access,std::uint32_t delta) {
    if(auto status=Mutable(access);status!=S::Ready)return status;
    if(auto status=Validate();status!=S::Ready)return status;
    CameraPlayArea a;CameraFieldParameters p;if(auto status=Environment(a,p);status!=S::Ready)return status;
    return TickCameraTargetExact(current_->motion,delta,a,p)?S::Ready:S::InvalidMotion;
}
S NativeMapCamera::AngleImpl(ProcessAccess* access,std::int32_t degrees) {
    if(auto status=Mutable(access);status!=S::Ready)return status;
    if(auto status=Validate();status!=S::Ready)return status;
    auto motion=current_->motion;motion.pitch.destination=static_cast<float>(degrees);
    if(!UpdateCameraAngleExact(motion))return S::InvalidMotion;
    current_->motion=motion;return S::Ready;
}
S NativeMapCamera::NearDistanceImpl(ProcessAccess* access) {
    if(auto status=Mutable(access);status!=S::Ready)return status;
    if(auto status=Validate();status!=S::Ready)return status;
    CameraFieldParameters p;if(auto status=FieldParameters(p);status!=S::Ready)return status;
    auto motion=current_->motion;motion.distance.destination=p.distances[0];
    if(!UpdateCameraDistanceExact(motion))return S::InvalidMotion;
    current_->motion=motion;return S::Ready;
}
S NativeMapCamera::Instant(){return InstantImpl(nullptr);}
S NativeMapCamera::Instant(ProcessAccess& a){return InstantImpl(&a);}
S NativeMapCamera::SetDestination(std::int32_t x,std::int32_t y){return DestinationImpl(nullptr,x,y);}
S NativeMapCamera::SetDestination(ProcessAccess& a,std::int32_t x,std::int32_t y){return DestinationImpl(&a,x,y);}
S NativeMapCamera::SetAngle(std::int32_t degrees){return AngleImpl(nullptr,degrees);}
S NativeMapCamera::SetAngle(ProcessAccess& a,std::int32_t degrees){return AngleImpl(&a,degrees);}
S NativeMapCamera::SetDistanceFromNear(){return NearDistanceImpl(nullptr);}
S NativeMapCamera::SetDistanceFromNear(ProcessAccess& a){return NearDistanceImpl(&a);}
S NativeMapCamera::TickTarget(std::uint32_t delta){return TargetImpl(nullptr,delta);}
S NativeMapCamera::Tick(std::uint32_t delta) {
    if(auto status=Mutable(nullptr);status!=S::Ready)return status;
    if(auto status=Validate();status!=S::Ready)return status;
    auto& motion=current_->motion;
    if(motion.mode==2 || motion.mode==3){motion.mode=0;return S::Ready;}
    if(!current_->blend)return S::UnknownBlend;
    if(current_->blend->duration)return S::ActiveBlendOwnerRequired;
    CameraPlayArea a;CameraFieldParameters p;if(auto status=Environment(a,p);status!=S::Ready)return status;
    return TickUnblendedCameraExact(motion,delta,a,p)?S::Ready:S::InvalidMotion;
}
S NativeMapCamera::TickTarget(ProcessAccess& a){return TargetImpl(&a,a.frame_delta());}
S NativeMapCamera::TickDistance(std::uint32_t delta) {
    if(auto status=Mutable(nullptr);status!=S::Ready)return status;
    if(auto status=Validate();status!=S::Ready)return status;
    CameraPlayArea a;CameraFieldParameters p;if(auto status=Environment(a,p);status!=S::Ready)return status;
    return TickCameraDistanceExact(current_->motion,delta,p)?S::Ready:S::InvalidMotion;
}
S NativeMapCamera::TickAngle(std::uint32_t delta) {
    if(auto status=Mutable(nullptr);status!=S::Ready)return status;
    if(auto status=Validate();status!=S::Ready)return status;
    return TickCameraAngleExact(current_->motion,delta)?S::Ready:S::InvalidMotion;
}
bool NativeMapCamera::UsesScheduler(const NativeProcessScheduler& s) const noexcept{return scheduler_.lock().get()==&s;}
void NativeMapCamera::Retire() noexcept{retired_=true;current_.reset();world_.reset();}
}
