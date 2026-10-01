#include "fates/progression/progression_support.hpp"
#include <algorithm>

namespace fates::progression {

GrowthResolution ResolveRetailLevelGrowth(const std::array<int, kLevelStatCount>& growth_percent, GrowthCallbacks callbacks) {
    GrowthResolution out{};
    // Retail repeats the whole eight-stat pass while no qualifying growth event occurred,
    // with at most four passes. A successful roll still counts as an event when a cap blocks the raise.
    for (unsigned attempt = 0; attempt < 4 && !out.qualifying_growth_event; ++attempt) {
        out.attempts = attempt + 1;
        for (std::size_t stat = 0; stat < kLevelStatCount; ++stat) {
            int rate = growth_percent[stat];
            if (rate <= 0) continue;
            while (rate > 99) {
                rate -= 100;
                out.qualifying_growth_event = true;
                if (callbacks.can_raise != nullptr && callbacks.raise != nullptr && callbacks.can_raise(callbacks.user, stat)) {
                    callbacks.raise(callbacks.user, stat);
                    ++out.applied_points[stat];
                }
            }
            // Retail consumes one 0..99 roll for every positive growth rate, including an exact 100/200/... remainder of zero.
            const int roll = callbacks.next_percent_0_99 != nullptr ? callbacks.next_percent_0_99(callbacks.user) : 100;
            if (roll < rate) {
                out.qualifying_growth_event = true;
                if (callbacks.can_raise != nullptr && callbacks.raise != nullptr && callbacks.can_raise(callbacks.user, stat)) {
                    callbacks.raise(callbacks.user, stat);
                    ++out.applied_points[stat];
                }
            }
        }
    }
    return out;
}

bool SupportKeyMatches(std::uint16_t stored_person_id, std::uint16_t query_person_id,
                       bool query_person_is_download, bool identifier_equal) {
    if (stored_person_id != query_person_id) return false;
    return query_person_is_download || identifier_equal;
}

std::array<std::uint8_t,2> SupportPoolSerializedHeader(std::size_t node_count) {
    return {0u, static_cast<std::uint8_t>(node_count & 0xffu)};
}

DualSupportState ClearedDualSupportState() { return {}; }

bool HasNewLevel(std::uint8_t calculated_level, std::uint8_t committed_level) {
    return calculated_level < committed_level;
}

bool HasNewWeaponRank(int before_rank, int after_rank) { return before_rank < after_rank; }

bool ShouldBranchClassChangeSkill(bool has_class_change, int class_change_type,
                                  unsigned branch_index, unsigned branch_count) {
    return has_class_change && class_change_type == 3 && branch_index < branch_count;
}

} // namespace fates::progression
