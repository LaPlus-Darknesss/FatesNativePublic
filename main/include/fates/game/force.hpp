#pragma once

#include <cstdint>
class Person; class Unit;
namespace game::packet { struct Unit; }

class Force {
public:
    enum class Type : int {};

    void Initialize(Type type);
    static std::uint32_t GetMaskSameForce(Type type);
    static std::uint32_t GetMaskSameForceWithLost(Type type);
    static std::uint32_t GetMaskSameForceWithDefection(Type type);
    static std::uint32_t GetMaskSameForceWithDefectionLost(Type type);

    void TransferForSortie(Type destination, bool preserveSortieState);
    void Remove(Unit* unit);
    void JoinLast(Unit* unit);
    void Transfer(Type destination, bool preserveState);
    void JoinFirst(Unit* unit);

    static Force* Get(Type type);
    Unit* GetUnitFromSkill(const char* skillIdentifier) const;
    Unit* GetUnitFromSkill(unsigned long long skillMask) const;
    Unit* GetUnitFromPerson(const Person* person) const;
    Unit* GetUnitFromPerson(const char* personIdentifier) const;
    Unit* GetUnitFromPersonIdentifier(const game::packet::Unit* identifier) const;
    int GetCount() const;
    bool IsAllied(Type other) const;

private:
    // INFERRED/OPAQUE: retail force values 0..9 are proven by UnitPool setup,
    // but localized semantic labels for every value are intentionally held.
    Type type_{};
};
