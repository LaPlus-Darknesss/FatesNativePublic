#pragma once
#include "fates/event/native_band.hpp"
#include "fates/map/native_map_camera.hpp"
#include "fates/runtime/native_chapter_sequence_scope.hpp"

namespace fates::event::native {
enum class EventCameraStatus:std::uint8_t {
    Ready,NullScheduler,MismatchedDomain,Retired,UnknownChapter,CameraUnavailable,BandUnavailable
};
struct EventCameraOutcome {
    // Map absence returns0. A successful tail-void camera/Band helper leaves an
    // incidental register result, which bytecode may drop but not use as int.
    std::optional<std::int32_t> value;
};
// Shared native command composition. Actual current ProcEvent capability is
// supplied by the VM bridge; camera, chapter scope, Fade and Band share one
// scheduler. No script-specific callbacks or chapter IDs participate.
class NativeEventCamera final {
public:
    // Distance commands need only the shared camera domain. Angle additionally
    // requires the explicit Chapter/Band services supplied by the overload.
    static EventCameraStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<map::native::NativeMapCamera>,std::shared_ptr<NativeEventCamera>&);
    static EventCameraStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<map::native::NativeMapCamera>,std::shared_ptr<runtime::native::NativeChapterSequenceScope>,
        std::shared_ptr<NativeBand>,std::shared_ptr<NativeEventCamera>&);
    EventCameraStatus SetAngle(runtime::native::ProcessAccess&,runtime::native::ProcessHandle current_event,
        std::int32_t degrees,EventCameraOutcome&);
    EventCameraStatus SetDistanceFromNear(runtime::native::ProcessAccess&,EventCameraOutcome&);
    bool UsesScheduler(const runtime::native::NativeProcessScheduler&) const noexcept;
    const std::shared_ptr<runtime::native::NativeRuntime>& runtime() const noexcept;
private:
    NativeEventCamera()=default;
    std::weak_ptr<runtime::native::NativeProcessScheduler> scheduler_;
    std::shared_ptr<map::native::NativeMapCamera> camera_;
    std::shared_ptr<runtime::native::NativeChapterSequenceScope> chapter_;
    std::shared_ptr<NativeBand> band_;
};
}
