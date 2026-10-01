#include "fates/runtime/native_turn_upkeep.hpp"

namespace fates::runtime::native {
bool BeginUnitTurnEndExact(UnitTurnSnapshot& state) noexcept {
    state.flags &= 0xf9e7fffeu;
    state.secondary_flags &= ~8u;
    if (state.flags & 0x40000u) return false;
    AdvanceEnhanceTurnEndExact(state.enhance);
    return true;
}
void FinishUnitTurnEndExact(UnitTurnSnapshot& state) noexcept {
    state.counter_12d = 0;
}
bool BeginUnitTurnBeginExact(UnitTurnSnapshot& state, bool recovery_skill) noexcept {
    if (state.flags & 0x40000u) return false;
    AdvanceEnhanceTurnBeginExact(state.enhance, recovery_skill);
    return true;
}
void FinishUnitTurnBeginExact(UnitTurnSnapshot& state) noexcept {
    if (state.flags & 0x800000u) state.flags &= ~0x800000u;
    else if (state.counter_135) --state.counter_135;
}
TurnControlRoute SelectTurnControlRouteExact(std::uint8_t control) noexcept {
    switch (control) {
    case 1: return TurnControlRoute::Human;
    case 2: return TurnControlRoute::Ai;
    case 3: return TurnControlRoute::Link;
    default: return TurnControlRoute::Exit;
    }
}
bool SkipEmptyTurnExact(std::uint8_t control, std::uint32_t force_count) noexcept {
    return control != 1 && force_count == 0;
}
}
