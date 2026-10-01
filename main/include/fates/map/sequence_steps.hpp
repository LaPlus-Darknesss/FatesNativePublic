#pragma once

namespace map::sequence_detail {

// These classes model retail anonymous ProcInst subclasses without claiming
// their 32-bit ARM object layout. Method names preserve the first-party
// StackTrace vocabulary so the source can be navigated directly from evidence.
struct MainTurnProcess {
    static void Disconnect();
    static void Persistent();
    static void TurnBranch();
    static void TurnEffect();
    static void TurnScroll();
    static void ShowWinRule();
    static void GameEndBranch();
    static void GameOverBranch();
    static void GameOverEffect();
    static void TurnScrollAfterTerrainEffect();
    static void Sync();
    static void TurnEnd();
    static void Complete();
    static void GameOver();
    static void TurnSkip();
    static void TurnTime();
    static void TurnBegin();
    static void CompleteEffect();
    static void Destroy();
};

struct SyncProcess {
    static void Open();
    static void Close();
    static void Execute();
};

struct HumanProcess {
    static void UnitListMenu();
    static void ConfigMenu();
    static void CreateJobIntro();
    static void ClassChange();
    static void FreeCursorPrepare();
    static void FixedAfterEvent();
    static void AutoTurnEnd();
    static void ShowBalloon();
    static void DoubleTraded();
    static void UnitMoveWait();
    static void CreateMapSave();
    static void DoubleChanged();
    static void GameEndBranch();
    static void FreeCursorTick();
    static bool IsCannonAttack(int x, int y);
    static void PickCursorTick();
    static void SaveMenuBranch();
    static void BattleInfoEvent();
    static void SortieDoubleOff();
    static void TrickCursorTick();
    static void CannonCursorTick();
    static void PickCursorCancel();
    static void ClassChangeBranch();
    static void PickCursorPrepare();
    static void SortieShowBalloon();
    static void SortieDoubleChange();
    static void TrickCursorPrepare();
    static void UnitListMenuBranch();
    static void CannonCursorPrepare();
    static void ItemUsedForMedicine();
    static void JumpForCursorResume();
    static void TransporterFinalize();
    static void SortiePositionChanged();
    static void TransporterInitialize();
    static void PickCursorResumePrepare();
    static void SortieCursorDecideDouble();
    static void TrickCursorResumePrepare();
    static void CannonCursorResumePrepare();
    static void Mind();
    static void Fixed();
    static void TurnEnd();
    static void UnitMove();
    static void PickEvent();
    static void Talked();
    static void UnitCommandCancel();
    static void TransporterMenu();
    static void Destroy();
};

struct DanceBattleProcess {
    static void AfterDetailBattle();
    static void CreateDetailBattle();
    static void BeforeDetailBattle();
    static void Grow();
    static void FocusEnd();
    static void Reliance();
    static void Destroy();
};

struct DanceProcess {
    static void End();
    static void Grow();
    static void Begin();
    static void Action();
    static void Reliance();
};

struct MindUnitProcess {
    static void Persistent();
    static void Tick();
    static void Wait();
    static void Construct();
};

struct CompleteEffectProcess {
    static void Persistent();
    static void Open();
    static void Tick();
    static void Close();
    static void Prepare();
    static void WaitAsync();
    static void Destroy();
};

} // namespace map::sequence_detail
