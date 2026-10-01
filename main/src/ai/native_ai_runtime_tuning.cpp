#include "fates/ai/native_ai_runtime_tuning.hpp"

namespace fates::ai::native {

AiRuntimeTuningStatus BindAiRuntimeTuning(
    fates::runtime::native::UnitState& unit,
    const AiRuntimeTuningInput input) noexcept {
    if(input.battle_rate > 2u) return AiRuntimeTuningStatus::InvalidBattleRate;
    unit.ai.policy_flags = input.policy_flags;
    unit.ai.priority = input.priority;
    unit.ai.battle_rate = input.battle_rate;
    unit.ai.move_limit_mode = input.move_limit_mode;
    unit.ai.move_limit_x1 = input.move_limit_x1;
    unit.ai.move_limit_y1 = input.move_limit_y1;
    unit.ai.move_limit_x2 = input.move_limit_x2;
    unit.ai.move_limit_y2 = input.move_limit_y2;
    unit.ai.runtime_tuning_bound = true;
    return AiRuntimeTuningStatus::Ok;
}

} // namespace fates::ai::native
