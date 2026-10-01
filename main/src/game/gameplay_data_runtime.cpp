#include "fates/detail/gameplay_data_runtime.hpp"
#include "fates/game/unit.hpp"
#include "fates/game/unit_item.hpp"

namespace fates::decomp_detail {
std::uint8_t UnitRawDisplayLevel(const Unit* unit) { return GameplayDataValue<std::uint8_t>("Unit.RawDisplayLevel", unit); }
unit::Item* UnitInventorySlot(Unit* unit, int index) { return GameplayDataValue<unit::Item*>("Unit.InventorySlot", unit, index); }
const unit::Item* UnitInventorySlot(const Unit* unit, int index) { return GameplayDataValue<const unit::Item*>("Unit.InventorySlotConst", unit, index); }
std::int16_t* UnitEquipSkillSlot(Unit* unit, int index) { return GameplayDataValue<std::int16_t*>("Unit.EquipSkillSlot", unit, index); }
int UnitEquippedItemIndex(const Unit* unit) { return GameplayDataValue<int>("Unit.EquippedItemIndex", unit); }
std::uint16_t StaticItemId(const ::Item* item) { return GameplayDataValue<std::uint16_t>("Item.StaticId", item); }
int StaticItemDefaultEndurance(const ::Item* item) { return GameplayDataValue<int>("Item.DefaultEndurance", item); }
}
