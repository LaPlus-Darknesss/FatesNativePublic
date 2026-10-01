#include "fates/event/native_event_camera.hpp"
#include "fates/runtime/native_runtime.hpp"

namespace fates::event::native {
using namespace runtime::native;
using S=EventCameraStatus;
S NativeEventCamera::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<map::native::NativeMapCamera> camera,
    std::shared_ptr<NativeEventCamera>& out) {
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!camera || !camera->UsesScheduler(*scheduler))return S::MismatchedDomain;
    auto next=std::shared_ptr<NativeEventCamera>(new NativeEventCamera());next->scheduler_=scheduler;
    next->camera_=std::move(camera);out=std::move(next);return S::Ready;
}
S NativeEventCamera::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<map::native::NativeMapCamera> camera,
    std::shared_ptr<NativeChapterSequenceScope> chapter,std::shared_ptr<NativeBand> band,std::shared_ptr<NativeEventCamera>& out) {
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!camera || !camera->UsesScheduler(*scheduler) || !chapter || !chapter->UsesScheduler(*scheduler) ||
        !band || !band->UsesScheduler(*scheduler))return S::MismatchedDomain;
    auto next=std::shared_ptr<NativeEventCamera>(new NativeEventCamera());next->scheduler_=scheduler;
    next->camera_=std::move(camera);next->chapter_=std::move(chapter);next->band_=std::move(band);out=std::move(next);return S::Ready;
}
S NativeEventCamera::SetAngle(ProcessAccess& access,ProcessHandle event,std::int32_t degrees,EventCameraOutcome& out) {
    const auto s=scheduler_.lock();if(!s || !s->root(2))return S::Retired;
    if(!access.BelongsTo(*s))return S::MismatchedDomain;
    if(!runtime()->game.map_active){out={0};return S::Ready;}
    // Admission reads before the first mutation. Successful effects retain
    // original order: pitch destination+UpdateAngle, then BandOpenCreate.
    if(!chapter_)return S::UnknownChapter;
    const auto current=chapter_->Current();if(!current)return S::UnknownChapter;
    const auto parent=*current?*current:std::move(event);
    if(!band_)return S::BandUnavailable;
    if(band_->CanOpen(access,parent)!=BandStatus::Ready)return S::BandUnavailable;
    if(camera_->SetAngle(access,degrees)!=map::native::MapCameraStatus::Ready)return S::CameraUnavailable;
    ProcessHandle band;if(band_->Open(access,parent,band)!=BandStatus::Ready)return S::BandUnavailable;
    out={};return S::Ready;
}
S NativeEventCamera::SetDistanceFromNear(ProcessAccess& access,EventCameraOutcome& out) {
    const auto s=scheduler_.lock();if(!s || !s->root(2))return S::Retired;
    if(!access.BelongsTo(*s))return S::MismatchedDomain;
    if(!runtime()->game.map_active){out={0};return S::Ready;}
    if(camera_->SetDistanceFromNear(access)!=map::native::MapCameraStatus::Ready)return S::CameraUnavailable;
    out={};return S::Ready;
}
bool NativeEventCamera::UsesScheduler(const NativeProcessScheduler& s) const noexcept{return scheduler_.lock().get()==&s;}
const std::shared_ptr<NativeRuntime>& NativeEventCamera::runtime() const noexcept{return camera_->runtime();}
}
