#pragma once
#include <array>
#include <cstdint>
#include <optional>

namespace fates::runtime::native {
// Semantic fields of Situation, not an ABI mirror. Control value 1 is Human;
// retain other retail byte values without inventing additional enum meanings.
struct SituationPhaseState {
    std::array<std::uint8_t,3> control{{1,2,2}};
    std::uint8_t active_force{};
    std::uint8_t human_force{};
    std::uint16_t turn{1};
    std::int16_t turn_limit{};
    // Situation's signed-byte turn cursor positions. Unknown is distinct from
    // a carried x=-1 sentinel. Phase advancement preserves these same fields.
    std::array<std::optional<std::array<std::int8_t,2>>,3> remembered_cursor{};
};
enum class PhaseAccessStage : std::uint8_t {
    Unbound, AwaitingEntryServices, Ready, AwaitingExitServices, Terminal
};
struct TacticalPhaseContext {
    SituationPhaseState situation{};
    PhaseAccessStage stage{PhaseAccessStage::Unbound};
    std::uint64_t revision{};
    std::uint8_t chapter_index{};
};
} // namespace fates::runtime::native
