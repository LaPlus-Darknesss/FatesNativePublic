#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
class Person;

class Reliance {
public:
    std::uint16_t characterId{}; // PROVEN +0x00: partner Person id
    std::uint16_t id{};          // PROVEN +0x02
    // PROVEN: retail reads these four bytes as point thresholds for ranks 1..4
    // and treats >=99 as unavailable. CORROBORATED: FE14 data + localized game
    // terminology map them to Support C/B/A/S progression.
    std::array<std::uint8_t,4> rankPointThresholds{}; // +0x04..+0x07
    std::uint32_t tag{};         // +0x08; tag semantics remain partially opaque

    int GetMaxLevel() const;
    int GetMaxPoint() const;
    int GetLevelPoint(int level) const;
    int GetLevel(int points) const;
    const Person* GetPerson() const;
    bool IsRomance() const;

    static int GetSelfNum(const Person* person);
    static const Reliance* GetFromPerson(const Person* owner,const Person* other);
    static const Reliance* Get(const Person* person,int index);
    static int GetNum(const Person* person);
};
static_assert(sizeof(Reliance)==0x0C);
using SupportRecord=Reliance;
using Support=Reliance; // localized contributor-facing alias; retail symbol is Reliance.
