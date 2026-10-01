#include "fates/game/item.hpp"
#include "fates/game/job.hpp"
#include "fates/game/person.hpp"
#include "fates/detail/gameplay_data_runtime.hpp"

bool Item::IsDownload() const { return fates::decomp_detail::GameplayDataValue<bool>("Item.IsDownload",this); }
bool Item::IsHiddenEffect() const { return fates::decomp_detail::GameplayDataValue<bool>("Item.IsHiddenEffect",this); }
bool Item::CanUse() const { return fates::decomp_detail::GameplayDataValue<bool>("Item.CanUse",this); }
const wchar_t* Item::GetHelp() const { return fates::decomp_detail::GameplayDataValue<const wchar_t*>("Item.GetHelp",this); }
const wchar_t* Item::GetName() const { return fates::decomp_detail::GameplayDataValue<const wchar_t*>("Item.GetName",this); }
bool Item::IsMagic() const { return fates::decomp_detail::GameplayDataValue<bool>("Item.IsMagic",this); }
bool Item::IsWeapon() const { return fates::decomp_detail::GameplayDataValue<bool>("Item.IsWeapon",this); }

bool Job::IsDownload() const { return fates::decomp_detail::GameplayDataValue<bool>("Job.IsDownload",this); }
bool Job::IsEnemyOnly() const { return fates::decomp_detail::GameplayDataValue<bool>("Job.IsEnemyOnly",this); }
const wchar_t* Job::GetIntroName() const { return fates::decomp_detail::GameplayDataValue<const wchar_t*>("Job.GetIntroName",this); }
std::uint16_t Job::GetEquipSkill(int a0) const { return fates::decomp_detail::GameplayDataValue<std::uint16_t>("Job.GetEquipSkill",this,a0); }
int Job::GetLimitLevel() const { return fates::decomp_detail::GameplayDataValue<int>("Job.GetLimitLevel",this); }
int Job::GetWeaponIcon(ItemKind::Type a0) const { return fates::decomp_detail::GameplayDataValue<int>("Job.GetWeaponIcon",this,a0); }
bool Job::CanEquipFromSubKind(ItemSubKind::Type a0) const { return fates::decomp_detail::GameplayDataValue<bool>("Job.CanEquipFromSubKind",this,a0); }
const Item* Job::GetBasicItemCanEquipForEvent() const { return fates::decomp_detail::GameplayDataValue<const Item*>("Job.GetBasicItemCanEquipForEvent",this); }
bool Job::IsHigh() const { return fates::decomp_detail::GameplayDataValue<bool>("Job.IsHigh",this); }
const wchar_t* Job::GetHelp() const { return fates::decomp_detail::GameplayDataValue<const wchar_t*>("Job.GetHelp",this); }
const wchar_t* Job::GetName() const { return fates::decomp_detail::GameplayDataValue<const wchar_t*>("Job.GetName",this); }
bool Job::IsFlyer() const { return fates::decomp_detail::GameplayDataValue<bool>("Job.IsFlyer",this); }
bool Job::IsGiant() const { return fates::decomp_detail::GameplayDataValue<bool>("Job.IsGiant",this); }
bool Job::IsRider() const { return fates::decomp_detail::GameplayDataValue<bool>("Job.IsRider",this); }
bool Job::IsFemale() const { return fates::decomp_detail::GameplayDataValue<bool>("Job.IsFemale",this); }

bool Person::IsDownload() const { return fates::decomp_detail::GameplayDataValue<bool>("Person.IsDownload",this); }
bool Person::IsRawDownload() const { return fates::decomp_detail::GameplayDataValue<bool>("Person.IsRawDownload",this); }
std::uint16_t Person::GetLowJobIndex() const { return fates::decomp_detail::GameplayDataValue<std::uint16_t>("Person.GetLowJobIndex",this); }
const Person* Person::GetCaptureChange(std::uint16_t a0) const { return fates::decomp_detail::GameplayDataValue<const Person*>("Person.GetCaptureChange",this,a0); }
const wchar_t* Person::GetNameWithCaptureNameIndex(int a0) const { return fates::decomp_detail::GameplayDataValue<const wchar_t*>("Person.GetNameWithCaptureNameIndex",this,a0); }
int Person::GetRelianceRecollectionParentChild(const Person* a0,int a1) const { return fates::decomp_detail::GameplayDataValue<int>("Person.GetRelianceRecollectionParentChild",this,a0,a1); }
int Person::GetRelianceRecollectionBrotherSister(const Person* a0,int a1) const { return fates::decomp_detail::GameplayDataValue<int>("Person.GetRelianceRecollectionBrotherSister",this,a0,a1); }
bool Person::IsBond() const { return fates::decomp_detail::GameplayDataValue<bool>("Person.IsBond",this); }
const wchar_t* Person::GetHelp() const { return fates::decomp_detail::GameplayDataValue<const wchar_t*>("Person.GetHelp",this); }
const wchar_t* Person::GetName() const { return fates::decomp_detail::GameplayDataValue<const wchar_t*>("Person.GetName",this); }
bool Person::IsFemale() const { return fates::decomp_detail::GameplayDataValue<bool>("Person.IsFemale",this); }
bool Person::IsPlayer() const { return fates::decomp_detail::GameplayDataValue<bool>("Person.IsPlayer",this); }
bool Person::IsCapture() const { return fates::decomp_detail::GameplayDataValue<bool>("Person.IsCapture",this); }
