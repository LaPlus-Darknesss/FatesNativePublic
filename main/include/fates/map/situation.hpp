#pragma once

#include <cstdint>
#include "fates/game/force.hpp"

class Stream;
class Unit;

namespace map {

// Tier A chapter/tactical outcome and rule-state policy.
// The public names are exact StackTrace identity. Internal flag-manager,
// versus-config, save-packet and chapter-state storage remains OPAQUE until
// those owners are reconstructed.
class Situation {
public:
    struct WinLoseResult { enum class Type : int {}; };

    static void Initialize();
    static void Finalize();

    void Deserialize(Stream* stream);
    void Serialize(Stream* stream) const;

    void SetComplete(WinLoseResult::Type result);
    void SetGameOver(WinLoseResult::Type result);
    void GameEndCheck();
    void ResetRestTime();
    void SetHumanForceFirst();
    void GameEndCheckUnitDead(const Unit* unit);
    void UpdateValidLinkExchange();
    void CalculateCastleEnemyBattleScore();
    void TurnEnd();

    bool CanGainExp(Force::Type force) const;
    bool IsComplete() const;
    bool IsGameOver() const;
    bool IsShowTurn() const;
    int GetRestTime() const;
    bool IsEntrustAI() const;
    bool IsRecordDead(Force::Type force) const;
    bool IsRecordKill(Force::Type force) const;
    bool CanGainReliance(Force::Type force) const;
    bool IsErrorOperation() const;
    bool IsCancelOperation() const;
    int GetMessageWaitFrame() const;
    bool IsValidLinkExchange() const;

    // Exact retail spelling is preserved.
    int CalculateDoragonVein() const;

    bool CanDual(Force::Type force) const;
    bool IsCasual(bool includeMapRestriction) const;
    bool IsVersus() const;
    bool IsPhoenix() const;
};

} // namespace map
