#pragma once
#include <cstdint>
#include "fates/game/force.hpp"
class Unit;
namespace Dir { enum class Type : int {}; }

namespace map {

// Reachability/range image producer used by movement, AI, attacks, rods and
// map gimmicks. Four 64-bit image words are preserved as semantic bitfields;
// their backing retail object layout remains OPAQUE.
class Deploy {
public:
    struct MoveImage { int Get(int x, int y) const; };

    static void Initialize();
    static void Finalize();

    void DisposMove(int startX, int startY, int goalX, int goalY, unsigned int flags);
    void DoubleFill(const Unit* unit, unsigned long long a, unsigned long long b,
                    unsigned long long c, unsigned long long d, int x, int y);
    void FillAttack(unsigned long long itemMask, int rangeMode);
    void UnitMoveXY(const Unit* unit, int x, int y, int move, unsigned int flags, unsigned int limit);
    void RangeAround(int radius);
    void UnitAIMoveXY(const Unit* unit, int x, int y, int move, unsigned int flags, unsigned int limit);
    void DoubleFillRod(const Unit* unit, unsigned long long a, unsigned long long b, int rangeMode);
    void RangeImmobile(int x, int y, int minRange, int maxRange);
    void UnitAIMoveLimit(const Unit* unit, unsigned int limit);
    void DoubleFillAttack(const Unit* unit, unsigned long long a, unsigned long long b, int rangeMode);
    void UnitAIMove(const Unit* unit, int move, unsigned int flags, unsigned int limit);
    void UnitAIAttackLimit(const Unit* unit, unsigned int limit);
    void Move(int startX, int startY, int minRange, int maxRange, unsigned int flags);
    bool Cannon(int x, int y, int minRange, int maxRange, int radius);
    void FillRod(unsigned long long itemMask, int rangeMode);
    void UnitFill(const Unit* unit, unsigned int flags);
    void UnitMove(const Unit* unit, int move, unsigned int flags, unsigned int limit);
    void EventMove(int startX, int startY, int goalX, int goalY, unsigned int flags);
    void SearchDir(Dir::Type direction);
    void TurnReset(Force::Type force);

    void GetRangeBit(const Unit* unit, unsigned long long* out0, unsigned long long* out1,
                     unsigned long long* out2, unsigned long long* out3,
                     int* minRange, int* maxRange, unsigned int flags) const;
    bool IsFill(int x, int y) const;
};

} // namespace map
