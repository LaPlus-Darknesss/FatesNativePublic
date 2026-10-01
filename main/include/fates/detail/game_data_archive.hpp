#pragma once

#include "fates/detail/arm32_address.hpp"

#include <cstddef>
#include <cstdint>

namespace fates::decomp_detail {

// Exact 25-word inline root consumed by retail GameData::Initialize.
// Pointer-like fields are represented as 32-bit target addresses so this
// layout stays correct even when the reconstruction is syntax-checked on x64.
struct GameDataArchiveRoot32 {
    Arm32Address chapterTable;                 // +0x00
    std::int32_t chapterCount;                 // +0x04
    Arm32Address personTable;                  // +0x08
    Arm32Address classTable;                   // +0x0C (retail Job)
    Arm32Address skillTable;                   // +0x10
    std::int32_t normalSkillCount;             // +0x14
    std::int32_t totalSkillCount;               // +0x18
    Arm32Address personalityTable;             // +0x1C
    std::int32_t personalityCount;             // +0x20
    Arm32Address affiliationTable;             // +0x24 (retail Belong)
    std::int32_t affiliationCount;             // +0x28
    Arm32Address itemTable;                    // +0x2C
    Arm32Address weaponProficiencyTable;       // +0x30 (retail ItemKind)
    Arm32Address weaponProficiencyDataTable;   // +0x34 (retail ItemSubKind)
    Arm32Address forgeTable;                   // +0x38 (retail ItemRefine)
    Arm32Address weaponRanks;                  // +0x3C (retail WeaponLevel)
    Arm32Address weaponBonuses;                // +0x40
    Arm32Address weaponInteractions;           // +0x44
    Arm32Address experienceTable;              // +0x48
    Arm32Address movementCostTable;            // +0x4C (retail TerrainCost)
    Arm32Address unknown_50;                   // +0x50
    Arm32Address tutorialTable;                // +0x54
    std::int32_t tutorialCount;                // +0x58
    Arm32Address achievementBonusTable;        // +0x5C
    Arm32Address arenaTable;                   // +0x60
};

static_assert(sizeof(GameDataArchiveRoot32) == 0x64);
static_assert(offsetof(GameDataArchiveRoot32, chapterTable) == 0x00);
static_assert(offsetof(GameDataArchiveRoot32, personTable) == 0x08);
static_assert(offsetof(GameDataArchiveRoot32, classTable) == 0x0C);
static_assert(offsetof(GameDataArchiveRoot32, skillTable) == 0x10);
static_assert(offsetof(GameDataArchiveRoot32, personalityTable) == 0x1C);
static_assert(offsetof(GameDataArchiveRoot32, affiliationTable) == 0x24);
static_assert(offsetof(GameDataArchiveRoot32, itemTable) == 0x2C);
static_assert(offsetof(GameDataArchiveRoot32, weaponRanks) == 0x3C);
static_assert(offsetof(GameDataArchiveRoot32, movementCostTable) == 0x4C);
static_assert(offsetof(GameDataArchiveRoot32, unknown_50) == 0x50);
static_assert(offsetof(GameDataArchiveRoot32, tutorialTable) == 0x54);
static_assert(offsetof(GameDataArchiveRoot32, arenaTable) == 0x60);

} // namespace fates::decomp_detail
