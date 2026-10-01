#pragma once
#include "fates/map/native_actor_visual_state.hpp"
namespace fates::runtime::native {struct NativeRuntime;struct NativeGameState;}
namespace fates::map::native {
enum class ActorVisualStatus:std::uint8_t {
    Ok,InvalidUnit,MissingPresence,ActorAbsent,MissingActor,StaleActor,
    AlreadyBound,MissingPair,InvalidPair,MissingIcon,UnknownAnimation,
    MissingMapPublication,StaleMapPublication,MissingForceOrder
};
struct ActorVisualResult {
    ActorVisualStatus status{ActorVisualStatus::Ok};
    std::uint16_t unit_slot{0xffffu};
};
// These carry existing objects' fields. They do not construct an actor, infer
// presence, or initialize a map. UnitTransfer remains the presence authority.
ActorVisualStatus RestoreCurrentActorVisual(fates::runtime::native::NativeRuntime&,
    std::uint16_t,const ActorVisualSnapshot&);
ActorVisualStatus RestoreCurrentActorMapPublication(fates::runtime::native::NativeRuntime&,bool present);
void ForgetCurrentActorVisuals(fates::runtime::native::NativeGameState&) noexcept;
ActorVisualStatus ReadCurrentActorVisual(const fates::runtime::native::NativeRuntime&,
    std::uint16_t,ActorVisualSnapshot&);
ActorVisualStatus SetUnitIconAnimationExact(UnitIconAnimationSnapshot&,std::uint32_t,bool force=false) noexcept;
ActorVisualStatus SetActorMotionExact(ActorVisualSnapshot&,std::uint32_t) noexcept;
ActorVisualResult ResetCurrentActorMotion(fates::runtime::native::NativeRuntime&,std::uint16_t,bool partner);
ActorVisualResult ResetCurrentActorAlpha(fates::runtime::native::NativeRuntime&,std::uint16_t,bool partner);
// Actor portion only: after the real process kill, visit Force0..2 live order.
// Preflight/commit only these field writes; no artificial gameplay callbacks.
ActorVisualResult ApplyCurrentActorConsistency(fates::runtime::native::NativeRuntime&);
}
