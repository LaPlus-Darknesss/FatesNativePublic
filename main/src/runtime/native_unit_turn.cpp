#include "fates/runtime/native_unit_turn.hpp"

namespace fates::runtime::native {
void AdvanceEnhanceTurnEndExact(UnitEnhanceSnapshot& state) noexcept {
    auto& flags = state.flags[0];
    flags &= static_cast<std::uint8_t>(~((flags & 4u) ? 4u : 2u));
    flags &= static_cast<std::uint8_t>(~((flags & 16u) ? 16u : 8u));
}

void AdvanceEnhanceTurnBeginExact(UnitEnhanceSnapshot& state, bool recovery_skill) noexcept {
    state.flags[4] &= 0xfcu;
    state.flags[2] &= 0x7fu;
    state.flags[3] = 0;
    const int recovery = recovery_skill ? 2 : 1;
    // Retail uses LDRSB. Preserve that even for carried bytes outside the
    // ordinary 0..99 penalty domain. Lane zero is deliberately untouched.
    for (unsigned lane = 1; lane < state.weakness.size(); ++lane) {
        const auto raw = state.weakness[lane];
        const int value = (raw < 128u ? int(raw) : int(raw) - 256) - recovery;
        state.weakness[lane] = static_cast<std::uint8_t>(value > 0 ? value : 0);
    }
}
}
