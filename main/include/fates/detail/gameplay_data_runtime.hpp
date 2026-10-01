#pragma once
#include <cstdint>
#include <utility>

class Item;
class Job;
class Person;
class Unit;
namespace unit { class Item; }
namespace map { class BattleInfo; class BattleCalculator; }

namespace fates::decomp_detail {
// These adapters mark facts whose source-level intent is known but whose owning
// data layouts are not yet source-owned. They deliberately take semantic names
// instead of exposing retail offsets in durable gameplay code.
template<class... A> inline void GameplayDataCall(const char*, A&&...) {}
template<class T, class... A> inline T GameplayDataValue(const char*, A&&...) { return T{}; }

std::uint8_t UnitRawDisplayLevel(const Unit* unit);
unit::Item* UnitInventorySlot(Unit* unit, int index);
const unit::Item* UnitInventorySlot(const Unit* unit, int index);
std::int16_t* UnitEquipSkillSlot(Unit* unit, int index);
int UnitEquippedItemIndex(const Unit* unit);
std::uint16_t StaticItemId(const ::Item* item);
int StaticItemDefaultEndurance(const ::Item* item);
}
