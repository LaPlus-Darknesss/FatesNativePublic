#include "fates/runtime/native_initial_weapon_exp.hpp"
#include <algorithm>

namespace fates::runtime::native {
std::array<std::uint8_t,8> CalculateFirstWeaponExp(
    const std::array<std::uint8_t,8>& authored,
    const std::array<std::uint8_t,8>& resolved_limits,
    const std::uint8_t person_enemy_flag,
    const std::uint8_t difficulty_byte,
    const std::array<std::uint8_t,6>& rank_thresholds) noexcept {
    std::array<std::uint8_t,8> result{};
    for (std::size_t group=0; group<result.size(); ++group) {
        // Retail stores 1 even for a disabled group. GetWeaponExp/CanItemEquip
        // cap that stored value against the actual (possibly zero) limit.
        const auto ceiling=std::max<std::uint8_t>(1,resolved_limits[group]);
        std::uint8_t minimum=1;
        if (person_enemy_flag!=1 && person_enemy_flag!=3 && resolved_limits[group]>0) {
            if (difficulty_byte==1) minimum=rank_thresholds[3];
            else if (difficulty_byte==2) minimum=rank_thresholds[0];
            minimum=std::min(minimum,ceiling);
        }
        // Do not replace this with a rank guessed from the equipped weapon:
        // accepted Dispos equipment raises stored EXP in a later retail step.
        result[group]=std::max(minimum,std::min(authored[group],ceiling));
    }
    return result;
}
} // namespace fates::runtime::native
