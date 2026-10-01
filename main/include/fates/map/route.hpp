#pragma once

#include <cstdint>
class Unit;

namespace map {

// Tier A tactical path/AI-route state.
// PROVEN: one-byte route steps use direction bits 0x01/0x02/0x04/0x08 and
// 0x80 is the route terminator/sentinel. Retail movement-cost accumulation
// walks those steps and queries terrain cost for the active Unit.
class Route {
public:
    static void Initialize();
    static Route* Get();
    static void Finalize();

    void SetForEvent(int startX, int startY, int goalX, int goalY);
    static void SetOneRoute(unsigned char* route, unsigned char step);
    static void SetOneRoute(unsigned char* route, int startX, int startY, int goalX, int goalY);
    static int GetRouteCost(Unit* unit, int startX, int startY, const unsigned char* route);

    bool TickMindHere(int x, int y, unsigned int flags);
    bool TickMindSeek(int x, int y, unsigned int flags);
    void Back(int amount);
    void Seek(int x, int y, bool alternateCostImage);
    void Tick(int x, int y);
    void Reset(int x, int y);
    void Reset();

    int GetGoalX(int index, const unsigned char* route) const;
    int GetGoalY(int index, const unsigned char* route) const;
    void SetForAI(int startX, int startY, int goalX, int goalY);

    // INFERRED return semantics from predicate naming/call use; the retail
    // StackTrace does not encode return types. Keep behind the adapter until
    // a caller/differential probe earns a stronger public ABI claim.
    bool CanMindSeek(int x, int y, unsigned int flags);
    void TickMind(int x, int y, unsigned int flags);
    void TickMove(int x, int y);
    int GetCross(int x, int y) const;
};

} // namespace map
