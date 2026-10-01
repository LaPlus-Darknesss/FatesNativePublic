#include "fates/game/game_data.hpp"

#include "fates/detail/game_data_archive.hpp"
#include "fates/detail/game_data_runtime.hpp"
#include "fates/game/person.hpp"

namespace {

constexpr char kGameDataArchivePath[] = "GameData/GameData.bin.lz";

} // namespace

void GameData::Initialize() {
    using fates::decomp_detail::GameDataArchiveRoot32;
    using fates::decomp_detail::TargetPointer;

    fates::decomp_detail::gGameDataArchive =
        fates::decomp_detail::CreateArchiveFile(kGameDataArchivePath);

    const GameDataArchiveRoot32& data =
        *fates::decomp_detail::GetGameDataArchiveRoot(
            fates::decomp_detail::gGameDataArchive);

    Chapter::Initialize(TargetPointer(data.chapterTable), data.chapterCount);
    Person::Initialize(TargetPointer(data.personTable));
    ClassData::Initialize(TargetPointer(data.classTable));
    EquipSkill::Initialize(
        TargetPointer(data.skillTable),
        data.normalSkillCount,
        data.totalSkillCount);
    Personality::Initialize(
        TargetPointer(data.personalityTable),
        data.personalityCount);
    Belong::Initialize(
        TargetPointer(data.affiliationTable),
        data.affiliationCount);
    Item::Initialize(TargetPointer(data.itemTable));
    ItemKind::Initialize(TargetPointer(data.weaponProficiencyTable));
    ItemSubKind::Initialize(TargetPointer(data.weaponProficiencyDataTable));
    ItemRefine::Initialize(TargetPointer(data.forgeTable));
    WeaponLevel::Initialize(TargetPointer(data.weaponRanks));
    WeaponBonus::Initialize(TargetPointer(data.weaponBonuses));
    WeaponInteract::Initialize(TargetPointer(data.weaponInteractions));
    ExpTable::Initialize(TargetPointer(data.experienceTable));
    TerrainCost::Initialize(TargetPointer(data.movementCostTable));

    // Retail intentionally does not consume the +0x50 root word here.
    AchieveBonus::Initialize(TargetPointer(data.achievementBonusTable));
    Arena::Initialize(TargetPointer(data.arenaTable));
    Tutorial::Initialize(TargetPointer(data.tutorialTable), data.tutorialCount);

    EffectData::Initialize();
    CameraData::Initialize();
    GeoAttr::Initialize();
}

void GameData::Finalize() {
    // Retail 0x004E6A50 is explicitly idempotent at the GameData-owner level:
    // if the archive global is null, no subsystem finalizers are called.
    if (fates::decomp_detail::gGameDataArchive == nullptr) {
        return;
    }

    GeoAttr::Finalize();
    CameraData::Finalize();
    EffectData::Finalize();
    Tutorial::Finalize();
    TerrainCost::Finalize();
    ExpTable::Finalize();
    WeaponInteract::Finalize();
    WeaponBonus::Finalize();
    WeaponLevel::Finalize();
    ItemRefine::Finalize();
    ItemSubKind::Finalize();
    ItemKind::Finalize();
    Item::Finalize();
    Belong::Finalize();
    Personality::Finalize();
    EquipSkill::Finalize();
    ClassData::Finalize();
    Person::Finalize();
    Chapter::Finalize();
    fates::decomp_detail::DestroyGameDataArchive();
}
