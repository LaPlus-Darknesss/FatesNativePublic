#pragma once
#include "fates/runtime/native_definition_store.hpp"
#include "fates/runtime/native_game_state.hpp"
#include "fates/runtime/native_player_state_provider.hpp"

namespace fates::runtime::native {

// Top-level host-native ownership root. Definitions are immutable original-data
// projections; game is mutable semantic state. Future movement/battle/AI/event
// systems should receive this owner (or narrow views into it), not retail object
// addresses and not pre-resolved scalar facts.
struct NativeRuntime {
    DefinitionStore definitions{};
    NativeGameState game{};
    // Live event ownership stays outside copied tactical transaction images.
    // Only RestorePlayerEventState replaces this carried projection.
    const std::shared_ptr<NativePlayerEventState>& PlayerEvents() const noexcept {return player_events_;}
private:
    friend EventStateStatus RestorePlayerEventState(NativeRuntime&,const PlayerStateProvider&);
    std::shared_ptr<NativePlayerEventState> player_events_=NativePlayerEventState::Unknown();
};

} // namespace fates::runtime::native
