#pragma once
#include <cstdint>
#include "fates/game/force.hpp"
class Person; class Stream; class Unit;
namespace game::packet { struct Unit; }
namespace unit { struct Identifier; }

class UnitPool {
public:
    static constexpr int kUnitCapacity = 250;
    static constexpr int kForceCount = 10;

    static void Initialize();
    static void Deserialize(Stream* stream);
    static Unit* GetFromSkill(unsigned long long skillMask, unsigned int forceMask);
    static Unit* GetFromPerson(const Person* person);
    static Unit* GetFromPerson(const Person* person, unsigned int forceMask);
    static Unit* GetFromPerson(const char* identifier);
    static Unit* GetFromPerson(const char* identifier, unsigned int forceMask);
    static Unit* GetFromPerson(std::uint16_t personId, unsigned int forceMask);
    static Unit* GetFromPersonOnlyGuest(const Person* person, unsigned int forceMask);
    static Unit* GetFromPersonIdentifier(const game::packet::Unit* identifier, unsigned int forceMask);
    static Unit* GetFromPersonIdentifierOnlyGuest(const Person* person, const unit::Identifier* identifier, unsigned int forceMask);
    static Unit* Get(int index);
    static void Reset();
    static Unit* GetLast(unsigned int forceMask);
    static void Finalize();
    static int GetCount(unsigned int forceMask);
    static Unit* GetFirst(unsigned int forceMask);
    static Force* GetForce(int forceIndex);
    static Unit* GetDirect(int index);
    static Unit* GetPlayer();
    static void Serialize(Stream* stream, unsigned int forceMask);
};
