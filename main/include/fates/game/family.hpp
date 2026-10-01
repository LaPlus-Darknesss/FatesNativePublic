#pragma once
#include <array>
#include <cstddef>
#include "fates/game/gameplay_types.hpp"
class Job; class Person; class Stream; class Unit;
namespace game::packet { struct FamilyParent; }

namespace unit {
class Family {
public:
    class ParentSingle {
    public:
        void SetFromPacket(const game::packet::FamilyParent* packet);
        void Set(Unit* parent);
        void ToPacket(game::packet::FamilyParent* packet) const;
        int GetMaxLevel() const;
        int GetLevelPoint(int level) const;
        int GetGrow(int capabilityIndex) const;
        int GetLevel(int points) const;
    };
    class BrotherSingle {
    public:
        int GetMaxLevel() const;
        int GetLevelPoint(int level) const;
        int GetLevel(int points) const;
    };

    void Deserialize(Stream* stream);
    void ClearChapterReliance();
    void Clear();
    ParentSingle* GetParent(const Unit* unit);
    ParentSingle* GetParent(const Person* person);
    Family& operator=(const Family& other);

    int GetGrow(const Person* person, int capabilityIndex) const;
    int GetDualSupport(BattleCapability::Type capability, int supportLevel) const;
    const Person* GetGrandFather() const;
    const Person* GetGrandMother() const;
    int GetDoubleCapability(Capability::Type capability, int supportLevel) const;
    bool IsWrong() const;
    int GetLimit(int capabilityIndex) const;
    const ParentSingle* GetParent(const Unit* unit) const;
    void Serialize(Stream* stream) const;

private:
    // PROVEN size/slot boundaries from retail access. Pointer-bearing fields
    // stay opaque because copying 32-bit guest pointers into a native layout
    // would be a false claim about the future host representation.
    std::array<std::byte, 0x38> retailPayload_{};
};
static_assert(sizeof(Family) == 0x38);
}
