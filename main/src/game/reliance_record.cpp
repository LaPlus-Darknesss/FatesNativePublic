#include "fates/game/support.hpp"
#include "fates/game/person.hpp"
#include <algorithm>

int Reliance::GetMaxLevel() const {
    for(int level=4; level>=1; --level) if(rankPointThresholds[static_cast<std::size_t>(level-1)]<99) return level;
    return 0;
}
int Reliance::GetMaxPoint() const {
    for(int i=3;i>=0;--i) if(rankPointThresholds[static_cast<std::size_t>(i)]<99) return rankPointThresholds[static_cast<std::size_t>(i)];
    return 0;
}
int Reliance::GetLevelPoint(int level) const {
    if(level<=0 || level>4) return 0;
    return rankPointThresholds[static_cast<std::size_t>(level-1)];
}
int Reliance::GetLevel(int points) const {
    // PROVEN comparison order and boundary behavior, including rank 4 when
    // points reach the fourth threshold. A threshold of 99 naturally makes
    // that rank unavailable for ordinary support-point ranges.
    // Pass115: ARM tests S, A, B, C in that order, including unordered tables.
    for(int level=4;level>0;--level)
        if(points>=rankPointThresholds[static_cast<std::size_t>(level-1)])return level;
    return 0;
}
const Person* Reliance::GetPerson() const { return Person::Get(characterId); }
bool Reliance::IsRomance() const { return rankPointThresholds[3] < 99; }
