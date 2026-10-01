#include "fates/runtime/native_item_inventory.hpp"
namespace fates::runtime::native {
bool ItemIsWeaponExact(std::uint8_t group) noexcept {return group!=6 && group!=8;}
std::uint8_t ItemRefineRankExact(UnitItemState item,bool weapon) noexcept {return weapon?std::uint8_t((item.state>>8)&63):0;}
std::uint8_t ItemEnduranceExact(UnitItemState item,bool weapon) noexcept {return weapon?0:std::uint8_t((item.state>>8)&63);}
namespace {
UnitItemState SetSixBits(UnitItemState item,std::int32_t value) noexcept {
    item.state=std::uint16_t((item.state&0xc0ffu)|((std::uint32_t(value)&63u)<<8));return item;
}
void PublishEquippedId(UnitState& unit) noexcept {
    const auto index=EquippedInventoryIndexExact(unit.inventory.items);
    unit.equipped_item_id=index<0?0:unit.inventory.items[std::size_t(index)].item_id;
}
}
UnitItemState SetItemRefineRankExact(UnitItemState item,bool weapon,std::int32_t value) noexcept {return weapon?SetSixBits(item,value):item;}
UnitItemState SetItemEnduranceExact(UnitItemState item,bool weapon,std::int32_t value) noexcept {return weapon?item:SetSixBits(item,value);}
UnitItemState NewItemInstanceExact(std::uint16_t id,bool weapon,std::uint8_t uses) noexcept {return SetItemEnduranceExact({id,0},weapon,uses);}
std::int32_t EquippedInventoryIndexExact(const std::array<UnitItemState,5>& items) noexcept {
    for(std::size_t i=0;i<items.size();++i)if(items[i].state&0x4000u)return std::int32_t(i);
    return -1;
}
bool EquipInventoryExact(std::array<UnitItemState,5>& items,std::int32_t index,const std::array<bool,5>& eligible) noexcept {
    if(index==-1) {for(std::size_t i=0;i<items.size();++i)if(eligible[i])return EquipInventoryExact(items,std::int32_t(i),eligible);return false;}
    if(index<0 || index>=std::int32_t(items.size()) || !eligible[std::size_t(index)])return false;
    const auto old=EquippedInventoryIndexExact(items);
    if(old>=0)items[std::size_t(old)].state&=std::uint16_t(~0x4000u);
    const auto chosen=items[std::size_t(index)];
    for(auto i=index;i>0;--i)items[std::size_t(i)]=items[std::size_t(i-1)];
    items[0]=chosen;items[0].state|=0x4000u;return true;
}
std::optional<ItemCombatValues> ProjectItemCombatValues(const DefinitionStore& defs,UnitItemState instance) {
    const auto* item=defs.FindItem(instance.item_id);if(!item)return std::nullopt;
    const auto* sub=defs.FindItemSubKind(item->weapon_category);if(!sub)return std::nullopt;
    const auto rank=ItemRefineRankExact(instance,ItemIsWeaponExact(sub->weapon_exp_group));
    ItemCombatValues out{std::uint8_t(item->might),item->hit,item->crit,false,0,rank};
    const auto variant=instance.state&0x7fu;
    if((rank!=0 || variant>=2) && !(instance.state&0x80u)) {
        out.table=variant==1?item->raw_0x49:item->forge_table_index;
        const auto* row=defs.FindForgeEntry(out.table,rank);if(!row)return std::nullopt;
        out.power+=row->power;out.hit+=row->hit;out.critical+=row->critical;out.refinement_applied=true;
    }
    return out;
}
InventoryStatus ValidateCurrentInventory(const UnitState& unit) noexcept {
    if(!unit.occupied)return InventoryStatus::InvalidUnit;
    if(!unit.inventory.bound)return InventoryStatus::Unbound;
    const auto index=EquippedInventoryIndexExact(unit.inventory.items);
    const auto equipped=index<0?0:unit.inventory.items[std::size_t(index)].item_id;
    return unit.inventory.owner_person==unit.person_id && equipped==unit.equipped_item_id?InventoryStatus::Ok:InventoryStatus::StaleIdentity;
}
InventoryStatus RestoreCurrentInventory(const DefinitionStore& defs,UnitState& unit,const std::array<UnitItemState,5>& items) {
    if(!unit.occupied)return InventoryStatus::InvalidUnit;
    if(!defs.FindPerson(unit.person_id))return InventoryStatus::MissingDefinition;
    for(const auto& item:items)if(item.item_id && !defs.FindItem(item.item_id))return InventoryStatus::MissingDefinition;
    unit.inventory={true,unit.person_id,items};PublishEquippedId(unit);return InventoryStatus::Ok;
}
InventoryStatus EquipCurrentInventory(UnitState& unit,std::int32_t index,const std::array<bool,5>& eligible) {
    const auto status=ValidateCurrentInventory(unit);if(status!=InventoryStatus::Ok)return status;
    if(index<-1 || index>=5)return InventoryStatus::InvalidIndex;
    auto items=unit.inventory.items;if(!EquipInventoryExact(items,index,eligible))return InventoryStatus::Rejected;
    unit.inventory.items=items;PublishEquippedId(unit);return InventoryStatus::Ok;
}
std::optional<std::array<UnitItemState,5>> ReadCalculationInventory(const UnitState& unit) {
    if(unit.inventory.bound) {if(ValidateCurrentInventory(unit)!=InventoryStatus::Ok)return std::nullopt;return unit.inventory.items;}
    std::array<UnitItemState,5> items{};
    for(std::size_t i=0;i<items.size();++i)items[i].item_id=unit.initial_inventory_item_ids[i];
    return items;
}
std::optional<UnitItemState> ReadCalculationEquippedItem(const UnitState& unit) {
    if(!unit.inventory.bound)return UnitItemState{unit.equipped_item_id,0};
    if(ValidateCurrentInventory(unit)!=InventoryStatus::Ok)return std::nullopt;
    const auto index=EquippedInventoryIndexExact(unit.inventory.items);
    return index<0?UnitItemState{}:unit.inventory.items[std::size_t(index)];
}
}
