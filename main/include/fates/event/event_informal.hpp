#pragma once
#include "fates/event/event.hpp"
namespace event::Informal {
enum Type : int {}; namespace ItemTable { enum Type : int {}; enum Route : int {}; }
int GetRandomFood(Random*); int GetRandomItem(Random*,ItemTable::Type,ItemTable::Route); int GetRandomGemstone(Random*);
Type CaclulateTypeForMap(Unit*,Random*); Type CaclulateTypeForCastle(Random*); int GetRank();
void Create(ProcInst*,Unit*,Unit*,Type,unsigned int);
namespace detail { void Enhance(); void GainGemstone(); void ShowMessageNone(); void Wait(); void Escape(); void Rollback(); void Reliance(); void GainWeaponExp(); void ShowWeaponLevelUp(); bool GainItemIsExclusion(const void*); void GainFood(); void GainItem(); void GainMoney(); }
}
