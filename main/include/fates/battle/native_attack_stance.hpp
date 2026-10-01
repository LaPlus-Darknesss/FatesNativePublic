#pragma once
#include <array>
#include <cstdint>
namespace fates::battle::native {
// Resolved ordinary assist kernels. These are not a relationship provider,
// skill calculator, Guard Stance resolver, or authority to execute an AI action.
std::int32_t ResolveAssistPowerExact(std::int32_t pre_assist_power) noexcept;
std::uint8_t ResolveAssistTimesExact(bool equipped, bool silenced_magic,
    std::uint32_t side_flags, std::uint32_t opponent_flags,
    std::uint32_t battle_flags) noexcept;
struct ResolvedAttackSchedule {
    std::array<std::uint8_t,12> sides{};
    std::uint8_t count{};
};
// Side 0/1 are primary combatants; 2/3 assist their respective primaries.
// Ordering/Brave inputs are resolved facts in the instruction differential.
// The live ordinary path supplies no reverse-order or Brave flags. Scene-marker
// entries (retail side 5) are omitted; each retained entry is a strike attempt.
ResolvedAttackSchedule BuildResolvedAttackScheduleExact(
    const std::array<std::int32_t,4>& counts,
    const std::array<std::uint32_t,4>& flags,
    bool defender_first) noexcept;
}
