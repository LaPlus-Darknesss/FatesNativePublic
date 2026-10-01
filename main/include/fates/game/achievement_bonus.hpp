#pragma once

#include "fates/detail/arm32_address.hpp"

#include <cstdint>

struct AchievementBonusTable {
    fates::decomp_detail::Arm32Address entries{};
    std::uint32_t count{};
};

static_assert(sizeof(AchievementBonusTable) == 8);

class AchieveBonus {
public:
    static void Initialize(const void* data);
    static const AchievementBonusTable* GetRoute();
    static const AchievementBonusTable* GetVisit();
    static const AchievementBonusTable* GetBattle();
};

using AchievementBonus = AchieveBonus;
