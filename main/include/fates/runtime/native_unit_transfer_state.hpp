#pragma once
#include <cstdint>
#include <optional>
#include <vector>
namespace fates::runtime::native {
// Present records a carried object whose presentation lifetime is not owned by
// this facade. It never acts as permission to discard an unknown map actor.
enum class UnitMapActorPresence : std::uint8_t { Absent, Present };
struct UnitTransferSnapshot {
    UnitMapActorPresence map_actor{UnitMapActorPresence::Absent};
    // Original Unit+120 chapter points, separate from ordinary points at11C.
    std::optional<std::vector<std::uint8_t>> chapter_points;
    std::uint8_t chapter_counter{}; // original Unit+125
    bool operator==(const UnitTransferSnapshot&) const=default;
};
struct NativeUnitTransferState {
    bool bound{};
    std::uint16_t person_id{};
    UnitTransferSnapshot value{};
    std::uint64_t revision{};
};
}
