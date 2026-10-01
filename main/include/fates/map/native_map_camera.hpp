#pragma once
#include "fates/map/native_camera_motion.hpp"
#include "fates/map/native_field_scene.hpp"
#include "fates/runtime/native_process.hpp"

namespace fates::map::native {
enum class MapCameraStatus:std::uint8_t {
    Ready,NullRuntime,NullScheduler,UnknownCurrent,Absent,Retired,Busy,MismatchedDomain,
    MissingField,StaleField,MissingTerrain,MissingHeight,InvalidMotion,IdentityExhausted,
    UnknownBlend,ActiveBlendOwnerRequired
};
struct MapCameraIdentity final {const std::uint64_t serial;};
using MapCameraHandle=std::shared_ptr<const MapCameraIdentity>;
struct CameraBlendClock {std::int32_t elapsed{},duration{};};
struct MapCameraSnapshot {MapCameraHandle identity;CameraMotionState motion;std::optional<CameraBlendClock> blend;};
// Owns one live map camera and its motion. Carried admission is explicit: this
// does not silently run a guessed CameraBase constructor or manufacture a
// settled target. It shares the current FieldWorld configuration, Terrain and
// HeightMap, and retains no renderer-owned data or copied tactical image.
class NativeMapCamera final {
public:
    static MapCameraStatus Create(std::shared_ptr<runtime::native::NativeRuntime>,
        std::shared_ptr<runtime::native::NativeProcessScheduler>,std::shared_ptr<NativeMapCamera>&);
    MapCameraStatus RestoreCarried(const CameraMotionState&,MapCameraHandle&,std::optional<CameraBlendClock> blend={});
    MapCameraStatus PublishAbsent();
    std::optional<MapCameraHandle> Current() const;
    std::optional<MapCameraSnapshot> Observe(MapCameraHandle) const;
    MapCameraStatus ScrollProlixity(bool&) const;
    MapCameraStatus Instant();
    MapCameraStatus Instant(runtime::native::ProcessAccess&);
    MapCameraStatus SetDestination(std::int32_t x,std::int32_t y);
    MapCameraStatus SetDestination(runtime::native::ProcessAccess&,std::int32_t x,std::int32_t y);
    // Writes the pitch destination and restarts the original angle tracks;
    // this is not a time advance, Instant, or a complete CMVM command.
    MapCameraStatus SetAngle(std::int32_t degrees);
    MapCameraStatus SetAngle(runtime::native::ProcessAccess&,std::int32_t degrees);
    // Original index0 configuration distance, followed by restart only.
    MapCameraStatus SetDistanceFromNear();
    MapCameraStatus SetDistanceFromNear(runtime::native::ProcessAccess&);
    // Original outer Tick ordering with explicit blend-clock admission. Modes
    // that skip TickBlend need no clock. Active blend is a retained view/param
    // boundary; it cannot be acknowledged by discarding its duration.
    MapCameraStatus Tick(std::uint32_t frame_delta);
    // Individual original motion callbacks. Shake and render-update scheduling
    // are separate owners, not implied by these calls or by Tick.
    MapCameraStatus TickTarget(std::uint32_t frame_delta);
    MapCameraStatus TickTarget(runtime::native::ProcessAccess&);
    MapCameraStatus TickDistance(std::uint32_t frame_delta);
    MapCameraStatus TickAngle(std::uint32_t frame_delta);
    MapCameraStatus Validate() const;
    bool UsesScheduler(const runtime::native::NativeProcessScheduler&) const noexcept;
    const std::shared_ptr<runtime::native::NativeRuntime>& runtime() const noexcept {return runtime_;}
    void Retire() noexcept;
private:
    NativeMapCamera()=default;
    MapCameraStatus Mutable(runtime::native::ProcessAccess*) const;
    MapCameraStatus Environment(CameraPlayArea&,CameraFieldParameters&) const;
    MapCameraStatus FieldParameters(CameraFieldParameters&) const;
    MapCameraStatus InstantImpl(runtime::native::ProcessAccess*);
    MapCameraStatus DestinationImpl(runtime::native::ProcessAccess*,std::int32_t,std::int32_t);
    MapCameraStatus AngleImpl(runtime::native::ProcessAccess*,std::int32_t);
    MapCameraStatus NearDistanceImpl(runtime::native::ProcessAccess*);
    MapCameraStatus TargetImpl(runtime::native::ProcessAccess*,std::uint32_t);
    std::shared_ptr<runtime::native::NativeRuntime> runtime_;
    std::weak_ptr<runtime::native::NativeProcessScheduler> scheduler_;
    std::shared_ptr<const FieldWorldIdentity> world_;
    std::optional<MapCameraSnapshot> current_;
    std::uint64_t serial_{};
    std::uint8_t chapter_{};
    bool known_{},retired_{},map_active_{};
};
}
