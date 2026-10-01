#include "fates/runtime/native_player_state_provider.hpp"
#include "fates/runtime/native_runtime.hpp"

namespace fates::runtime::native {
EventStateStatus RestorePlayerEventState(NativeRuntime& runtime,const PlayerStateProvider& provider) {
    PlayerEventStateSnapshot snapshot;
    const auto status=provider.ReadEventState(snapshot);
    if(status!=EventStateStatus::Ok)return status;
    if(snapshot.route && *snapshot.route>2)return EventStateStatus::InvalidSnapshot;
    auto next=std::shared_ptr<NativePlayerEventState>(new NativePlayerEventState);
    if(snapshot.flags) {
        const auto admitted=NativeEventFlagBank::Restore(*snapshot.flags,next->flags_);
        if(admitted!=EventStateStatus::Ok)return admitted;
    }
    if(snapshot.variables) {
        const auto admitted=NativeEventVariableBank::Restore(*snapshot.variables,next->variables_);
        if(admitted!=EventStateStatus::Ok)return admitted;
    }
    const auto previous=runtime.player_events_;
    next->route_bound_=snapshot.route.has_value();
    if(snapshot.route)runtime.game.campaign.route=static_cast<campaign::native::RouteIndex>(*snapshot.route);
    runtime.player_events_=std::move(next);
    if(previous) {
        previous->active_=false;
        if(previous->flags())previous->flags()->Retire();
        if(previous->variables())previous->variables()->Retire();
    }
    return EventStateStatus::Ok;
}
std::optional<std::uint8_t> CurrentCarriedRoute(const NativeRuntime& runtime) noexcept {
    const auto& state=runtime.PlayerEvents();
    if(!state || !state->active() || !state->route_bound())return {};
    const auto route=static_cast<std::uint8_t>(runtime.game.campaign.route);
    return route<=2?std::optional<std::uint8_t>(route):std::nullopt;
}
}
