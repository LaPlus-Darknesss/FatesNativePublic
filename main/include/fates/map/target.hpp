#pragma once
#include <cstdint>
class Unit;

namespace map {

// Tier A action-target enumerator. Retail maintains a bounded target-entry
// buffer plus selected unit/position and item masks. The concrete host layout
// is deliberately hidden; individual action eligibility rules remain named.
class Target {
public:
    struct Attack { enum class Type : int {}; };
    struct Data {
        void Set(Unit* unit, unsigned short itemOrActionId, int value);
        void Set(int x, int y, unsigned short itemOrActionId, int value);
    };

    static void Initialize();
    static Target* Get();
    static void Finalize();

    void EnumerateRod();
    void EnumerateTalk();
    void EnumerateWarp(int x, int y);
    void SetSelectUnit(const Unit* unit);
    void EnumerateClone();
    void EnumerateDance();
    void EnumerateTrade();
    void EnumerateAttack(Attack::Type type);
    void EnumerateAttack(int x, int y, unsigned short itemId, Attack::Type type);
    void EnumerateCharge();
    void EnumerateRescue();
    void EnumerateDestroy();
    void EnumerateDoubleOn();
    void SetSelectPosition(int x, int y);
    void EnumerateDoubleOff();
    void EnumerateTransform();
    void EnumerateTranspose();
    std::uint32_t PreCheckItemMaskRod();
    void EnumerateDoubleTrade();
    std::uint32_t PreCheckItemMaskAttack(Attack::Type type);
    void EnumerateObstacleAcquisition();
    void EnumerateObstacleInstallation();
    void Reset();
    void Enumerate();
    bool IsTrade(const Unit* unit) const;
};

} // namespace map
