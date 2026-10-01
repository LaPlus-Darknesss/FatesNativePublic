#include "fates/game/achievement_bonus.hpp"

namespace {
const AchievementBonusTable* gRouteBonuses = nullptr;
const AchievementBonusTable* gBattleBonuses = nullptr;
const AchievementBonusTable* gVisitBonuses = nullptr;
const AchievementBonusTable* gUnknownBonuses = nullptr;
}

void AchieveBonus::Initialize(const void* data) {
    const auto* bytes = static_cast<const std::byte*>(data);
    gRouteBonuses = reinterpret_cast<const AchievementBonusTable*>(bytes + 0x00);
    gBattleBonuses = reinterpret_cast<const AchievementBonusTable*>(bytes + 0x08);
    gVisitBonuses = reinterpret_cast<const AchievementBonusTable*>(bytes + 0x10);
    gUnknownBonuses = reinterpret_cast<const AchievementBonusTable*>(bytes + 0x18);
    (void)gUnknownBonuses;
}

const AchievementBonusTable* AchieveBonus::GetRoute() {
    return gRouteBonuses;
}

const AchievementBonusTable* AchieveBonus::GetVisit() {
    return gVisitBonuses;
}

const AchievementBonusTable* AchieveBonus::GetBattle() {
    return gBattleBonuses;
}
