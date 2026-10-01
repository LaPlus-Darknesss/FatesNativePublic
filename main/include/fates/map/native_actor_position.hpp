#pragma once
#include "fates/map/native_actor_position_state.hpp"
namespace fates::runtime::native {struct NativeRuntime;struct NativeGameState;}
namespace fates::map::native {
enum class ActorPositionStatus : std::uint8_t {
    Ok,MissingUnit,MissingPair,MissingActorPresence,MissingActorBinding,StaleActor,
    MissingOffset,MissingHeight,InvalidValue
};
struct ActorPositionUnit {
    std::uint8_t x{},y{};
    std::uint32_t flags{};
    bool partner_known{};
    std::optional<std::uint16_t> partner;
};
struct ActorPositionServices {
    virtual ~ActorPositionServices()=default;
    virtual std::optional<ActorPositionUnit> ReadUnit(std::uint16_t)=0;
    virtual std::optional<bool> HasActor(std::uint16_t)=0;
    virtual bool WriteCoordinates(std::uint16_t,std::uint8_t,std::uint8_t)=0;
    virtual std::optional<ActorPairOffset> PairOffset(bool partner)=0;
    virtual std::optional<float> MapHeight(ActorPositionVector,bool map_layer)=0;
    virtual bool StorePosition(std::uint16_t,ActorPositionVector)=0;
};
struct ActorPositionResult {ActorPositionStatus status{ActorPositionStatus::Ok};ActorPositionVector position{};};
ActorPositionResult ActorBasePositionExact(ActorPositionUnit,ActorPositionServices&);
ActorPositionStatus UpdateActorPositionExact(std::uint16_t,bool update_partner,ActorPositionServices&);
// Read-only query into the current scene's existing HeightMap boundary. No
// callback may mutate gameplay/actor/RNG state. No missing-height fallback.
struct CurrentActorHeightQuery {
    virtual ~CurrentActorHeightQuery()=default;
    virtual std::optional<float> MapHeight(const fates::runtime::native::NativeRuntime&,
        ActorPositionVector,bool map_layer) const=0;
};
// Restore a carried actor's current coordinate value, not actor construction.
ActorPositionStatus RestoreCurrentActorPosition(fates::runtime::native::NativeRuntime&,
    std::uint16_t,ActorPositionVector);
void InvalidateCurrentActorPositions(fates::runtime::native::NativeGameState&) noexcept;
// Scene-height changes invalidate values; an unobserved actor/map teardown must
// forget identities and require explicit carried actor binding again.
void ForgetCurrentActorPositions(fates::runtime::native::NativeGameState&) noexcept;
struct CurrentActorPositionView {ActorPositionStatus status{ActorPositionStatus::MissingActorBinding};const ActorPositionVector* position{};};
CurrentActorPositionView ReadCurrentActorPosition(const fates::runtime::native::NativeRuntime&,std::uint16_t);
// Internal composition entry: caller must stage game state before invoking this
// mutating operation. The public position publisher supplies that transaction.
// A null query uses the immutable native current scene-height owner. An explicit
// external query retains its caller-owned invalidation responsibility.
ActorPositionStatus UpdateCurrentActorPositionStaged(fates::runtime::native::NativeRuntime&,
    std::uint16_t,bool update_partner,const CurrentActorHeightQuery*);
}
