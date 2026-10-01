#pragma once
#include "fates/runtime/native_event_flags.hpp"
#include "fates/runtime/native_event_variables.hpp"

namespace fates::runtime::native {
struct NativeRuntime;
// Carried-player event projection: named flags and variables. Missing means unknown,
// never a fresh zero bank. Future roster/progression import extends this shared
// provider; this projection does not claim a complete player/save restoration.
struct PlayerEventStateSnapshot {
    std::optional<EventFlagSnapshot> flags;
    std::optional<EventVariableSnapshot> variables;
    // GameUserData+2E, admitted raw domain0..2. Missing remains unknown even
    // though the older campaign projection has a default RouteIndex value.
    std::optional<std::uint8_t> route;
};
class PlayerStateProvider {
public:
    virtual ~PlayerStateProvider()=default;
    virtual EventStateStatus ReadEventState(PlayerEventStateSnapshot&) const=0;
};
class DeterministicPlayerStateProvider final : public PlayerStateProvider {
public:
    explicit DeterministicPlayerStateProvider(PlayerEventStateSnapshot state):state_(std::move(state)) {}
    EventStateStatus ReadEventState(PlayerEventStateSnapshot& out) const override {out=state_;return EventStateStatus::Ok;}
private:
    PlayerEventStateSnapshot state_;
};
class NativePlayerEventState final {
public:
    NativePlayerEventState(const NativePlayerEventState&)=delete;
    NativePlayerEventState& operator=(const NativePlayerEventState&)=delete;
    const std::shared_ptr<NativeEventFlagBank>& flags() const noexcept {return flags_;}
    const std::shared_ptr<NativeEventVariableBank>& variables() const noexcept {return variables_;}
    bool active() const noexcept {return active_;}
    bool route_bound() const noexcept {return route_bound_;}
private:
    friend struct NativeRuntime;
    friend EventStateStatus RestorePlayerEventState(NativeRuntime&,const PlayerStateProvider&);
    NativePlayerEventState()=default;
    static std::shared_ptr<NativePlayerEventState> Unknown() {
        return std::shared_ptr<NativePlayerEventState>(new NativePlayerEventState);
    }
    std::shared_ptr<NativeEventFlagBank> flags_;
    std::shared_ptr<NativeEventVariableBank> variables_;
    bool active_{true};
    bool route_bound_{};
};
// All allocation/validation happens before publication. Each successful restore
// publishes a new permanent identity, even with identical or unknown contents.
EventStateStatus RestorePlayerEventState(NativeRuntime&,const PlayerStateProvider&);
// CampaignState remains the sole route value owner; the carried identity owns
// only its availability. Loading/freeing reads the current value on each call.
std::optional<std::uint8_t> CurrentCarriedRoute(const NativeRuntime&) noexcept;
}
