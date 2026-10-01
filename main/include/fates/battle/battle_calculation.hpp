#pragma once
#include <cstdint>
#include "fates/game/gameplay_types.hpp"

namespace map {
class BattleInfo {
public:
    class Flag;
    class Side {
    public:
        Side();
        void CalculateDetail(const Side* opponent);
        void CalculateEfficacy(const Side* opponent);
        void CalculateDetailHit(const Side* opponent);
        void CalculateDetailAvoid(const Side* opponent);
        void ComplementConditions(const Flag* flags);
        void CalculateDetailAttack(const Side* opponent);
        void CalculateDetailDefense(const Side* opponent);
        int GetSimpleHit(const Side* opponent) const;
        int GetSimplePower(const Side* opponent, int attackType, int distance) const;
        int GetSimpleTimes(const Side* opponent, int distance, const Flag* flags) const;
    };

    void CalculateDual();
    void ComplementDual();
    void CalculateDetail();
    void CalculateSimple();
    void ComplementConditions();
    void CalculateFaceExpression();
    void Clear();
    void Calculate();
};

class BattleCalculator {
public:
    struct Scene;
    struct Status {
        Status();
        std::uint32_t value{};
    };

    explicit BattleCalculator(BattleInfo* info);
    ~BattleCalculator();
    void CalculateRod();
    Scene* CreateNewScene();
    bool CalculateAttack(BattleSide::Type side);
    void InitializeProgress();
    bool CalculateAttackSingle(BattleSide::Type side, int attackIndex);
    void Calculate();
    Unit* GetDeadUnit(const Scene* scene) const;
    int GetAttackContinuousCount(const Scene* scene) const;
};
}
