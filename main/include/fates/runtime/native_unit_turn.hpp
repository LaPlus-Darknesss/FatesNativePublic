#pragma once
#include "fates/runtime/native_unit_map_end.hpp"

namespace fates::runtime::native {
// Original Enhance::TurnEnd and TurnBegin, respectively. These operate on the
// existing value snapshot; persistent weakness remains owned by UnitState.
// The caller resolves the current SEID_心頭滅却 skill through DefinitionStore.
// No Unit clone, force iteration, event or complete phase lifecycle is implied.
void AdvanceEnhanceTurnEndExact(UnitEnhanceSnapshot&) noexcept;
void AdvanceEnhanceTurnBeginExact(UnitEnhanceSnapshot&, bool recovery_skill) noexcept;
}
