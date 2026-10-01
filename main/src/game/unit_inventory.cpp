#include "fates/game/unit.hpp"

#include "fates/detail/gameplay_data_runtime.hpp"



void Unit::ItemCloseUp() {
    fates::decomp_detail::GameplayDataCall("Unit.ItemCloseUp", this);
}

void Unit::ItemTakeOff(int a0) {
    fates::decomp_detail::GameplayDataCall("Unit.ItemTakeOff", this, a0);
}

void Unit::AddWeaponExp(ItemKind::Type a0, int a1) {
    fates::decomp_detail::GameplayDataCall("Unit.AddWeaponExp", this, a0, a1);
}

bool Unit::AddEquipSkill(short skill) {
    if (skill == 0) return false;
    // The retail equipped-skill array is five entries. The first zero entry is
    // filled; a full list leaves the Unit unchanged.
    for (int index = 0; index < 5; ++index) {
        std::int16_t* slot = fates::decomp_detail::UnitEquipSkillSlot(this, index);
        if (slot != nullptr && *slot == 0) { *slot = skill; return true; }
    }
    return false;
}

void Unit::ItemPutOffAll() {
    fates::decomp_detail::GameplayDataCall("Unit.ItemPutOffAll", this);
}

void Unit::WeaponBuildup() {
    fates::decomp_detail::GameplayDataCall("Unit.WeaponBuildup", this);
}

unit::Item* Unit::GetItemEquipped() {
    const int index = fates::decomp_detail::UnitEquippedItemIndex(this);
    return index >= 0 && index < 5 ? fates::decomp_detail::UnitInventorySlot(this, index) : nullptr;
}

void Unit::InheritEquipSkill(const short* a0) {
    fates::decomp_detail::GameplayDataCall("Unit.InheritEquipSkill", this, a0);
}

void Unit::LearnFromDictionary(short a0) {
    fates::decomp_detail::GameplayDataCall("Unit.LearnFromDictionary", this, a0);
}

void Unit::DeleteEquipSkillFromIndex(int a0, bool a1) {
    fates::decomp_detail::GameplayDataCall("Unit.DeleteEquipSkillFromIndex", this, a0, a1);
}

void Unit::DeleteEquipSkillUnknownAndEnemyOnlyWithPool() {
    fates::decomp_detail::GameplayDataCall("Unit.DeleteEquipSkillUnknownAndEnemyOnlyWithPool", this);
}

int Unit::ItemAdd(const unit::Item* item) {
    if (item == nullptr) return -1;
    for (int index = 0; index < 5; ++index) {
        unit::Item* slot = fates::decomp_detail::UnitInventorySlot(this, index);
        if (slot != nullptr && slot->IsEmpty()) { *slot = *item; return index; }
    }
    return -1;
}

bool Unit::ItemUse(const unit::Item* a0) {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.ItemUse", this, a0);
}

void Unit::ItemMove(int a0, int a1) {
    fates::decomp_detail::GameplayDataCall("Unit.ItemMove", this, a0, a1);
}

bool Unit::SetDisposItem(const void* a0) {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.SetDisposItem", this, a0);
}

bool Unit::ItemEquip(int a0) {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.ItemEquip", this, a0);
}

void Unit::ItemPutOff(int a0, bool a1) {
    fates::decomp_detail::GameplayDataCall("Unit.ItemPutOff", this, a0, a1);
}

bool Unit::CanItemUse(const unit::Item* a0) const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.CanItemUse", this, a0);
}

bool Unit::CanItemEquip(const unit::Item* a0, bool a1, bool a2) const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.CanItemEquip", this, a0, a1, a2);
}

int Unit::GetWeaponExp(ItemKind::Type a0) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetWeaponExp", this, a0);
}

bool Unit::IsEquipSkill(short a0) const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.IsEquipSkill", this, a0);
}

bool Unit::CanCannonItem(const unit::Item* a0, bool a1) const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.CanCannonItem", this, a0, a1);
}

int Unit::GetContinuous(const unit::Item* a0) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetContinuous", this, a0);
}

int Unit::GetHasItemNum() const {
    // Retail inventory capacity is exactly five slots. This returns the index
    // of the first empty slot, or five when all slots are occupied.
    for (int index = 0; index < 5; ++index) {
        const unit::Item* item = fates::decomp_detail::UnitInventorySlot(this, index);
        if (item == nullptr || item->IsEmpty()) return index;
    }
    return 5;
}

bool Unit::HasCheerSkill() const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.HasCheerSkill", this);
}

bool Unit::HasDanceSkill() const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.HasDanceSkill", this);
}

const unit::Item* Unit::GetItemEquipped() const {
    const int index = fates::decomp_detail::UnitEquippedItemIndex(this);
    return index >= 0 && index < 5 ? fates::decomp_detail::UnitInventorySlot(this, index) : nullptr;
}

bool Unit::IsItemCanCannon(bool a0) const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.IsItemCanCannon", this, a0);
}

int Unit::GetItemHealPower(const unit::Item* a0) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetItemHealPower", this, a0);
}

int Unit::GetItemIndexHold() const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetItemIndexHold", this);
}

int Unit::GetWeaponExpLimit(ItemKind::Type a0) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetWeaponExpLimit", this, a0);
}

bool Unit::HasCheerSkillMove() const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.HasCheerSkillMove", this);
}

bool Unit::IsItemEnhanceHave(const unit::Item* a0, bool a1) const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.IsItemEnhanceHave", this, a0, a1);
}

std::uint16_t Unit::GetIndividualSkill() const {
    return fates::decomp_detail::GameplayDataValue<std::uint16_t>("Unit.GetIndividualSkill", this);
}

int Unit::GetHasEquipSkillNum() const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetHasEquipSkillNum", this);
}

int Unit::GetItemIndexKeyDoor() const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetItemIndexKeyDoor", this);
}

bool Unit::IsItemRangeCanEquip(int a0) const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.IsItemRangeCanEquip", this, a0);
}

bool Unit::IsItemSkillCanEquip(unsigned long long a0) const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.IsItemSkillCanEquip", this, a0);
}

int Unit::GetItemIndexEquipped() const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetItemIndexEquipped", this);
}

int Unit::GetInterferenceRodHit(const unit::Item* a0) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetInterferenceRodHit", this, a0);
}

bool Unit::IsLearnFromDictionary(short a0) const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.IsLearnFromDictionary", this, a0);
}

int Unit::CaclulateFirstWeaponExp(ItemKind::Data* a0) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.CaclulateFirstWeaponExp", this, a0);
}

int Unit::GetInterferenceRodAvoid(const unit::Item* a0) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetInterferenceRodAvoid", this, a0);
}

int Unit::GetItemMaxRangeCanEquip(bool a0) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetItemMaxRangeCanEquip", this, a0);
}

int Unit::GetWeaponExpForJobIntro(ItemKind::Type a0) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetWeaponExpForJobIntro", this, a0);
}

bool Unit::HasCheerSkillCapability() const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.HasCheerSkillCapability", this);
}

int Unit::GetEquipSkillProbability(short a0, bool a1) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetEquipSkillProbability", this, a0, a1);
}

int Unit::GetItemIndexKeyTreasureBox() const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetItemIndexKeyTreasureBox", this);
}

const ::Item* Unit::GetBasicItemCanEquipForArena() const {
    return fates::decomp_detail::GameplayDataValue<const ::Item*>("Unit.GetBasicItemCanEquipForArena", this);
}
