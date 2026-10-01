#pragma once
#include <cstdint>

namespace fates::event::native {
struct PhaseEventFilter {
    std::int32_t first_turn{}, last_turn{}, force{};
};
// Shared original Turn/TurnTerrain/TurnAfter/Reinforce Inspector predicate.
// Filtering does not execute a script, infer one-shot state or complete a hook.
bool PhaseEventMatchesExact(PhaseEventFilter, std::uint16_t turn, std::uint8_t active_force) noexcept;
}
