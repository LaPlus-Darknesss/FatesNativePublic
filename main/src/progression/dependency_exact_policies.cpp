#include "fates/progression/progression_support_dependencies.hpp"
#include <algorithm>

namespace fates::progression {

int DecodeClassChangeType(std::int8_t raw_type_code) {
    if (raw_type_code == 0) return 0;
    if (raw_type_code == 4) return 2;
    if (raw_type_code == 6) return 3;
    return 1;
}

int SelectLearnEquipSkill(const JobHandle* candidates, std::size_t candidate_count,
                          int resolved_unit_level, int current_job_limit, bool current_job_is_high,
                          LearnSkillCallbacks callbacks) {
    if (candidates == nullptr || callbacks.job_is_high == nullptr ||
        callbacks.job_get_equip_skill == nullptr || callbacks.unit_has_equip_skill == nullptr) return 0;
    int virtual_limit = std::min(resolved_unit_level, current_job_limit);
    if (current_job_is_high) virtual_limit += 20;
    int result = 0;
    for (std::size_t i = 0; i < candidate_count; ++i) {
        const JobHandle job = candidates[i];
        const bool high = callbacks.job_is_high(callbacks.user, job);
        const int first_virtual_level = high ? 21 : 1;
        for (int virtual_level = first_virtual_level; virtual_level <= virtual_limit; ++virtual_level) {
            const int job_level = high ? virtual_level - 20 : virtual_level;
            const int skill = callbacks.job_get_equip_skill(callbacks.user, job, job_level);
            if (skill != 0 && !callbacks.unit_has_equip_skill(callbacks.user, skill)) {
                result = skill;
                virtual_limit = virtual_level - 1;
                break;
            }
        }
    }
    return result;
}

bool VariableSizeFlagGet(const std::uint8_t* bytes, std::size_t bit_count, std::size_t index) {
    if (bytes == nullptr || index >= bit_count) return false;
    return (bytes[index >> 3] & static_cast<std::uint8_t>(1u << (index & 7u))) != 0;
}
bool PackedFlagGet(const std::uint8_t* bytes, std::size_t index) {
    if (bytes == nullptr) return false;
    return (bytes[index >> 3] & static_cast<std::uint8_t>(1u << (index & 7u))) != 0;
}
int RelianceFlagIndexPlayerAqua(bool alternate_block, int person_group, int reliance_level) {
    return reliance_level - 1 + (alternate_block ? 4 : 0) + person_group * 8;
}
bool IdentifierEqual(const std::array<std::uint32_t,4>& lhs, const std::array<std::uint32_t,4>& rhs) {
    return lhs == rhs;
}

bool IsRelianceTalkEligible(const RelianceTalkInputs& in) {
    if (in.first_invalid || in.second_invalid) return false;
    if (!in.first_route_enabled || !in.second_route_enabled) return false;
    if (in.first_unit_blocked || in.second_unit_blocked) return false;
    if (in.current_level < in.max_level) return in.next_rank_talk_exists;
    if (!in.allow_post_max_special || in.max_level != 3) return false;
    if (in.first_is_player || in.second_is_player || in.first_is_download || in.second_is_download) return false;
    if (!in.same_sex || in.is_family) return false;
    return in.first_secondary_person_id_is_zero;
}

SkipExpResult ApplySkippedExperienceLane(std::uint8_t level, std::uint8_t exp, int gain) {
    SkipExpResult out{level, exp, false};
    if (gain <= 0) return out;
    int total = static_cast<int>(exp) + gain;
    if (total > 99) {
        total %= 100;
        out.level = static_cast<std::uint8_t>(level + 1);
        out.level_advanced = true;
    }
    out.exp = static_cast<std::uint8_t>(total);
    return out;
}

std::array<std::uint16_t,4> EncodeGameFontIcon(std::uint16_t icon_id) {
    return {0x24u, 0x69u, static_cast<std::uint16_t>(icon_id | 0x4000u), 0u};
}
std::array<std::uint16_t,4> EncodeGameFontSkill(std::uint16_t skill_id) {
    return {0x24u, 0x69u, static_cast<std::uint16_t>((skill_id & 0x0fffu) | 0x3000u), 0u};
}
std::int16_t WaitTicksFromMilliseconds(float milliseconds) {
    return static_cast<std::int16_t>(static_cast<int>(milliseconds * 0.06f));
}
int TransitionTicksFromMilliseconds(float milliseconds) {
    const int ticks = static_cast<int>(milliseconds * 0.06f);
    return ticks < 1 ? 0 : ticks;
}

} // namespace fates::progression
