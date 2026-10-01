#include "fates/game/unit_item.hpp"
#include "fates/game/item.hpp"
#include "fates/detail/gameplay_data_runtime.hpp"

namespace unit {
Item::Item() { Clear(); }
Item& Item::operator=(const Item& other) { itemId_=other.itemId_; state_=other.state_; return *this; }
const ::Item* Item::ToData() const { return ::Item::Get(itemId_); }
void Item::Clear() { itemId_=0; state_=0; }
void Item::SetEndurance(int endurance) {
    const ::Item* data=ToData();
    if (data!=nullptr && !data->IsWeapon()) state_=static_cast<std::uint16_t>((state_ & 0xC0FFu) | ((endurance & 0x3F) << 8));
}
void Item::SetRefineRank(int rank) {
    const ::Item* data=ToData();
    if (data!=nullptr && data->IsWeapon()) state_=static_cast<std::uint16_t>((state_ & 0xC0FFu) | ((rank & 0x3F) << 8));
}
int Item::GetEndurance() const { const ::Item* data=ToData(); return data!=nullptr && !data->IsWeapon() ? (state_ >> 8) & 0x3F : 0; }
int Item::GetRefineRank() const { const ::Item* data=ToData(); return data!=nullptr && data->IsWeapon() ? (state_ >> 8) & 0x3F : 0; }
void Item::New(const ::Item* data) {
    if (data==nullptr) { Clear(); return; }
    itemId_=fates::decomp_detail::StaticItemId(data); state_=0;
    if (!data->IsWeapon()) SetEndurance(fates::decomp_detail::StaticItemDefaultEndurance(data));
}
void Item::New(const char* identifier) { if(identifier==nullptr){Clear();return;} New(::Item::Get(identifier)); }
void Item::New(std::uint16_t id) { New(::Item::Get(id)); }
void Item::Replace(std::uint16_t id) {
    itemId_=id;
    const ::Item* data=ToData();
    // Retail preserves the variant/flag bits when replacing and refreshes only
    // endurance for non-weapons.
    if (data!=nullptr && !data->IsWeapon()) SetEndurance(fates::decomp_detail::StaticItemDefaultEndurance(data));
}
bool Item::IsEqual(const Item* other) const {
    if (other==nullptr || itemId_!=other->itemId_) return false;
    return (state_ & 0x007Fu)==(other->state_ & 0x007Fu) && (state_ & 0x3F00u)==(other->state_ & 0x3F00u);
}
bool Item::SortCompare(const Item* lhs,const Item* rhs,void*) { return lhs!=nullptr && rhs!=nullptr && rhs->GetSort() < lhs->GetSort(); }
bool Item::Expend() {
    if (!IsExpend()) return false;
    const int endurance=GetEndurance();
    if (endurance==0) return true;
    SetEndurance(endurance-1);
    return endurance==1;
}
void Item::DeserializeIfNotExistKeep(Stream* a0) { fates::decomp_detail::GameplayDataCall("unit::Item.DeserializeIfNotExistKeep",this,a0); }
void Item::DeserializeIfNotExistDelete(Stream* a0) { fates::decomp_detail::GameplayDataCall("unit::Item.DeserializeIfNotExistDelete",this,a0); }
void Item::Serialize(Stream* a0) const { fates::decomp_detail::GameplayDataCall("unit::Item.Serialize",this,a0); }
int Item::GetCritical() const { return fates::decomp_detail::GameplayDataValue<int>("unit::Item.GetCritical",this); }
bool Item::IsIntegrate(const Item* a0) const { return fates::decomp_detail::GameplayDataValue<bool>("unit::Item.IsIntegrate",this,a0); }
int Item::GetSellPrice() const { return fates::decomp_detail::GameplayDataValue<int>("unit::Item.GetSellPrice",this); }
bool Item::IsShopRefine(const Item* a0) const { return fates::decomp_detail::GameplayDataValue<bool>("unit::Item.IsShopRefinePair",this,a0); }
bool Item::IsShopRefine() const { return fates::decomp_detail::GameplayDataValue<bool>("unit::Item.IsShopRefine",this); }
int Item::GetHit() const { return fates::decomp_detail::GameplayDataValue<int>("unit::Item.GetHit",this); }
const wchar_t* Item::GetName() const { return fates::decomp_detail::GameplayDataValue<const wchar_t*>("unit::Item.GetName",this); }
std::uint64_t Item::GetSort() const { return fates::decomp_detail::GameplayDataValue<std::uint64_t>("unit::Item.GetSort",this); }
int Item::GetPower() const { return fates::decomp_detail::GameplayDataValue<int>("unit::Item.GetPower",this); }
bool Item::IsExpend() const { return fates::decomp_detail::GameplayDataValue<bool>("unit::Item.IsExpend",this); }
}
