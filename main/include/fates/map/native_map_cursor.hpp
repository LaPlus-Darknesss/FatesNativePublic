#pragma once
#include "fates/map/native_map_camera.hpp"
#include "fates/runtime/native_force_order.hpp"

namespace fates::map::native {
struct CursorPoint {float x{},y{};bool operator==(const CursorPoint&)const=default;};
struct MapCursorMotion {CursorPoint saved{},current{},target{};bool operator==(const MapCursorMotion&)const=default;};
enum class MapCursorStatus:std::uint8_t {
    Ready,NullRuntime,MismatchedDomain,Busy,Retired,UnknownCurrent,StaleField,
    InvalidPhase,UnknownConfig,UnknownRememberedPosition,MissingForceOrder,
    MissingPrivateSkill,UnitStateUnavailable,InvalidCoordinate,CameraUnavailable
};
struct TurnCursorSelection {
    MapCursorStatus status{MapCursorStatus::Ready};
    bool scroll{};
    std::int32_t x{},y{};
    std::optional<runtime::native::ForceSkillLookupResult> lookup;
};
// Shared map cursor position owner. Explicit carried admission does not claim
// the cursor's original factory, input, hook, visual type or Draw lifetime.
// Turn selection reads Situation, GameConfigData and original Force list order.
class NativeMapCursor final {
public:
    static MapCursorStatus Create(std::shared_ptr<runtime::native::NativeRuntime>,
        std::shared_ptr<runtime::native::NativeProcessScheduler>,std::shared_ptr<NativeMapCamera>,
        std::shared_ptr<NativeMapCursor>&);
    MapCursorStatus RestoreCarried(MapCursorMotion);
    std::optional<MapCursorMotion> Observe() const;
    TurnCursorSelection SelectTurnFirst(std::uint8_t force) const;
    MapCursorStatus TurnScroll(runtime::native::ProcessAccess&,TurnCursorSelection&);
    bool UsesRuntime(const runtime::native::NativeRuntime&) const noexcept;
    bool UsesScheduler(const runtime::native::NativeProcessScheduler&) const noexcept;
private:
    MapCursorStatus Validate() const;
    std::shared_ptr<runtime::native::NativeRuntime> runtime_;
    std::weak_ptr<runtime::native::NativeProcessScheduler> scheduler_;
    std::shared_ptr<NativeMapCamera> camera_;
    std::shared_ptr<const FieldWorldIdentity> world_;
    std::uint8_t chapter_{};
    std::optional<MapCursorMotion> current_;
};
}
