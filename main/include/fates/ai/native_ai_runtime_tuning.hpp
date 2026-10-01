#pragma once
#include "fates/runtime/native_game_state.hpp"
#include <cstdint>

namespace fates::ai::native {

struct AiRuntimeTuningInput {
    std::uint32_t policy_flags{};
    std::uint8_t priority{};
    std::uint8_t battle_rate{};
    std::uint8_t move_limit_mode{0xFF};
    std::uint8_t move_limit_x1{0xFF};
    std::uint8_t move_limit_y1{0xFF};
    std::uint8_t move_limit_x2{0xFF};
    std::uint8_t move_limit_y2{0xFF};
};

enum class AiRuntimeTuningStatus : std::uint8_t {
    Ok,
    InvalidBattleRate
};

// Retail Unit::SetDispos copies these authored values independently from the
// four AI descriptor programs. The native layer keeps the same separation.
AiRuntimeTuningStatus BindAiRuntimeTuning(
    fates::runtime::native::UnitState&,
    AiRuntimeTuningInput) noexcept;

} // namespace fates::ai::native
