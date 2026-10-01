#pragma once
#include <cstdint>
#include "fates/game/capability.hpp"
#include "fates/game/force.hpp"

class Stream;
class Random;
class Reliance;
class Person;
class Job;
class Item;
class Unit;
namespace BattleCapability { enum class Type : int {}; }
namespace map {
class Actor;
namespace BattleSide { enum class Type : int {}; }
}
namespace game::packet { struct Unit; struct ExtendedUnit; struct FamilyParent; }
namespace unit {
struct Edit;
struct Identifier;
namespace detail { struct EnhanceDefinition { enum class Type : int {}; }; }
}
