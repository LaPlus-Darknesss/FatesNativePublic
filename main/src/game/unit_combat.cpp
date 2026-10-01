#include "fates/game/unit.hpp"

#include "fates/detail/gameplay_data_runtime.hpp"



map::Actor* Unit::ReplaceActor(map::Actor* a0) {
    return fates::decomp_detail::GameplayDataValue<map::Actor*>("Unit.ReplaceActor", this, a0);
}

void Unit::SortEquipSkill() {
    fates::decomp_detail::GameplayDataCall("Unit.SortEquipSkill", this);
}

void Unit::SetCapabilityJust(int a0, int a1) {
    fates::decomp_detail::GameplayDataCall("Unit.SetCapabilityJust", this, a0, a1);
}

void Unit::UpdateLimitOffset() {
    fates::decomp_detail::GameplayDataCall("Unit.UpdateLimitOffset", this);
}

int Unit::CollectiveLevelUpCapability(int a0, bool a1) {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.CollectiveLevelUpCapability", this, a0, a1);
}

const Job* Unit::GetCollectiveLevelUpClassChangeJob() {
    return fates::decomp_detail::GameplayDataValue<const Job*>("Unit.GetCollectiveLevelUpClassChangeJob", this);
}

bool Unit::CanKeyDoor() const {
    // PROVEN: retail returns true if the unit has the door-opening skill OR if
    // GetItemIndexKeyDoor() finds a matching key item; otherwise false. The
    // skill-table identity is still opaque, so the combined query remains a
    // semantic adapter rather than exposing a guessed skill id.
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.CanKeyDoor.skill_or_key_item", this);
}

const char* Unit::GetDeadBGM() const {
    return fates::decomp_detail::GameplayDataValue<const char*>("Unit.GetDeadBGM", this);
}

int Unit::GetDefImpl(const unit::Item* a0, bool a1, bool a2, unit::detail::EnhanceDefinition::Type a3) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetDefImpl", this, a0, a1, a2, a3);
}

int Unit::GetMHPImpl(bool a0, bool a1, unit::detail::EnhanceDefinition::Type a2) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetMHPImpl", this, a0, a1, a2);
}

int Unit::GetPowImpl(const unit::Item* a0, bool a1, bool a2, unit::detail::EnhanceDefinition::Type a3) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetPowImpl", this, a0, a1, a2, a3);
}

int Unit::GetStrImpl(const unit::Item* a0, bool a1, bool a2, unit::detail::EnhanceDefinition::Type a3) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetStrImpl", this, a0, a1, a2, a3);
}

bool Unit::IsCostFree() const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.IsCostFree", this);
}

bool Unit::CanCaptured() const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.CanCaptured", this);
}

bool Unit::CanCheerUse(const Unit* a0, int a1) const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.CanCheerUse", this, a0, a1);
}

bool Unit::CanDanceUse(const Unit* a0) const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.CanDanceUse", this, a0);
}

bool Unit::CanDoubleOn(const Unit* a0) const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.CanDoubleOn", this, a0);
}

std::uint16_t Unit::GetCategory() const {
    return fates::decomp_detail::GameplayDataValue<std::uint16_t>("Unit.GetCategory", this);
}

int Unit::GetCritical(int a0) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetCritical", this, a0);
}

int Unit::GetCritical(const unit::Item* a0) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetCritical", this, a0);
}

int Unit::GetExpDance(Unit::ExpMode::Type a0, const Unit* a1) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetExpDance", this, a0, a1);
}

int Unit::GetLowLevel() const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetLowLevel", this);
}

int Unit::GetLuckImpl(const unit::Item* a0, bool a1, bool a2, unit::detail::EnhanceDefinition::Type a3) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetLuckImpl", this, a0, a1, a2, a3);
}

int Unit::GetMdefImpl(const unit::Item* a0, bool a1, bool a2, unit::detail::EnhanceDefinition::Type a3) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetMdefImpl", this, a0, a1, a2, a3);
}

int Unit::GetTechImpl(const unit::Item* a0, bool a1, bool a2, unit::detail::EnhanceDefinition::Type a3) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetTechImpl", this, a0, a1, a2, a3);
}

bool Unit::IsInvisible() const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.IsInvisible", this);
}

int Unit::ExpNormalize(int a0) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.ExpNormalize", this, a0);
}

int Unit::GetDualExpDestroy(Unit::ExpMode::Type a0, const Unit* a1) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetDualExpDestroy", this, a0, a1);
}

int Unit::GetExpDamage(Unit::ExpMode::Type a0, const Unit* a1) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetExpDamage", this, a0, a1);
}

int Unit::GetMovePower(bool a0, unit::detail::EnhanceDefinition::Type a1) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetMovePower", this, a0, a1);
}

int Unit::GetQuickImpl(const unit::Item* a0, bool a1, bool a2, unit::detail::EnhanceDefinition::Type a3) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetQuickImpl", this, a0, a1, a2, a3);
}

int Unit::GetShowLevel() const {
    // Retail stores the raw visible level separately from combat level and
    // clamps the value presented by this accessor to 99.
    const int raw = fates::decomp_detail::UnitRawDisplayLevel(this);
    return raw > 99 ? 99 : raw;
}

bool Unit::IsDontAttack(const Unit* a0, bool a1) const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.IsDontAttack", this, a0, a1);
}

bool Unit::CanDragonVein() const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.CanDragonVein", this);
}

int Unit::GetCannonArea() const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetCannonArea", this);
}

int Unit::GetExpDestroy(Unit::ExpMode::Type a0, const Unit* a1, bool a2) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetExpDestroy", this, a0, a1, a2);
}

int Unit::GetLimitLevel() const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetLimitLevel", this);
}

int Unit::GetCannonRangeI() const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetCannonRangeI", this);
}

int Unit::GetCannonRangeO() const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetCannonRangeO", this);
}

int Unit::GetEntrustForAI() const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetEntrustForAI", this);
}

int Unit::GetRodHealPower(const unit::Item* a0, const Unit* a1) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetRodHealPower", this, a0, a1);
}

bool Unit::IsSpecialBattle() const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.IsSpecialBattle", this);
}

int Unit::GetDualExpDamage(Unit::ExpMode::Type a0, const Unit* a1) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetDualExpDamage", this, a0, a1);
}

void Unit::ToExtendedPacket(game::packet::ExtendedUnit* a0, bool a1) const {
    fates::decomp_detail::GameplayDataCall("Unit.ToExtendedPacket", this, a0, a1);
}

bool Unit::CanKeyTreasureBox() const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.CanKeyTreasureBox", this);
}

int Unit::GetCapabilityImpl(int a0, const unit::Item* a1, bool a2, bool a3, unit::detail::EnhanceDefinition::Type a4) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetCapabilityImpl", this, a0, a1, a2, a3, a4);
}

bool Unit::IsUniqueWithClone() const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.IsUniqueWithClone", this);
}

int Unit::GetDoubleMovePower(const Unit* a0) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetDoubleMovePower", this, a0);
}

int Unit::GetEntrustForCount() const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetEntrustForCount", this);
}

Force::Type Unit::GetForceAbsentType() const {
    return fates::decomp_detail::GameplayDataValue<Force::Type>("Unit.GetForceAbsentType", this);
}

int Unit::GetUnderContinuous(const unit::Item* a0) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetUnderContinuous", this, a0);
}

bool Unit::IsDetailBattleMust() const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.IsDetailBattleMust", this);
}

int Unit::GetAttackForTerrain(int a0) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetAttackForTerrain", this, a0);
}

int Unit::GetCapabilityRating() const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetCapabilityRating", this);
}

int Unit::GetDoubleCapability(int a0, const Unit* a1) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetDoubleCapability", this, a0, a1);
}

bool Unit::IsDeadProductionHigh() const {
    return GetDeadProductionLevel() > 1;
}

int Unit::GetDeadProductionLevel() const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetDeadProductionLevel", this);
}

bool Unit::IsDetailBattleNotAllow() const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.IsDetailBattleNotAllow", this);
}

int Unit::GetLevelForCalculateExp() const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetLevelForCalculateExp", this);
}

void Unit::CaclulateAutoGrowCapability(Capability* a0, int a1) const {
    fates::decomp_detail::GameplayDataCall("Unit.CaclulateAutoGrowCapability", this, a0, a1);
}

int Unit::GetLearnFromDictionaryPrice(short a0) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetLearnFromDictionaryPrice", this, a0);
}

int Unit::GetNoWeaknessCapabilityRating() const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetNoWeaknessCapabilityRating", this);
}

int Unit::GetHit(int a0, bool a1) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetHit", this, a0, a1);
}

int Unit::GetHit(const unit::Item* a0, bool a1) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetHit", this, a0, a1);
}

int Unit::GetGrow(int a0) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetGrow", this, a0);
}

const wchar_t* Unit::GetName() const {
    return fates::decomp_detail::GameplayDataValue<const wchar_t*>("Unit.GetName", this);
}

Unit* Unit::GetNext(unsigned int a0) const {
    return fates::decomp_detail::GameplayDataValue<Unit*>("Unit.GetNext", this, a0);
}

Unit* Unit::GetPrev(unsigned int a0) const {
    return fates::decomp_detail::GameplayDataValue<Unit*>("Unit.GetPrev", this, a0);
}

bool Unit::IsWrong() const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.IsWrong", this);
}

int Unit::GetAvoid(int a0) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetAvoid", this, a0);
}

int Unit::GetAvoid(const unit::Item* a0) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetAvoid", this, a0);
}

int Unit::GetLimit(int a0) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetLimit", this, a0);
}

bool Unit::IsFemale() const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.IsFemale", this);
}

bool Unit::IsLeader() const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.IsLeader", this);
}

bool Unit::IsUnique() const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.IsUnique", this);
}

void Unit::ToPacket(game::packet::Unit* a0, bool a1) const {
    fates::decomp_detail::GameplayDataCall("Unit.ToPacket", this, a0, a1);
}

bool Unit::CanCannon() const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.CanCannon", this);
}

int Unit::GetAttackForTerrain(const unit::Item* a0) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetAttackForTerrain", this, a0);
}

int Unit::GetAttack(const unit::Item* a0, bool a1) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetAttack", this, a0, a1);
}

int Unit::GetAttack(int a0, bool a1) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetAttack", this, a0, a1);
}

int Unit::GetExpRod(Unit::ExpMode::Type a0, const unit::Item* a1, const Unit* a2) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetExpRod", this, a0, a1, a2);
}

int Unit::GetRangeI(const unit::Item* a0) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetRangeI", this, a0);
}

int Unit::GetRangeO(const unit::Item* a0) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetRangeO", this, a0);
}

int Unit::GetSecure(const unit::Item* a0) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetSecure", this, a0);
}

bool Unit::IsNotIcon() const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.IsNotIcon", this);
}

void Unit::Serialize(Stream* a0) const {
    fates::decomp_detail::GameplayDataCall("Unit.Serialize", this, a0);
}

const wchar_t* Unit::GetHelp() const {
    return fates::decomp_detail::GameplayDataValue<const wchar_t*>("Unit.GetHelp", this);
}
