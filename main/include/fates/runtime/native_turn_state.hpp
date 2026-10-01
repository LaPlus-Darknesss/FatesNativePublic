#pragma once
#include <array>
#include <cstdint>
#include <optional>

namespace fates::runtime::native {
enum class UnitClonePresence : std::uint8_t { Unknown, Absent, Present };
// These are carried fields, not defaults inferred from chapter or Person IDs.
// Present clone synchronization remains a separate Unit owner.
struct NativeUnitTurnState {
    std::optional<std::uint8_t> counter_135;
    UnitClonePresence clone{UnitClonePresence::Unknown};
};
struct NativeForceUpkeepProgress {
    bool bound{}, begin_done{}, end_done{};
    std::uint64_t phase_revision{};
    std::uint8_t chapter{}, force{}, human_force{};
    std::uint16_t turn{};
    std::int16_t turn_limit{};
    std::array<std::uint8_t,3> control{};
};
}
