#pragma once
#include <array>
#include <cstdint>

namespace fates::runtime::native {
// Unit::CaclulateFirstWeaponExp, 0x00531C5C..0x00531D30. Limits are the
// resolved GetWeaponExpLimit results, NOT necessarily the Job bytes. This pure
// transform owns stored EXP only; effective EXP is separately capped on use.
std::array<std::uint8_t,8> CalculateFirstWeaponExp(
    const std::array<std::uint8_t,8>& authored,
    const std::array<std::uint8_t,8>& resolved_limits,
    std::uint8_t person_enemy_flag,
    std::uint8_t difficulty_byte,
    const std::array<std::uint8_t,6>& rank_thresholds) noexcept;
} // namespace fates::runtime::native
