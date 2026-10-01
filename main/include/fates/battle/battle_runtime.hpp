#pragma once
#include "fates/battle/battle_unit.hpp"
class CameraParam; class ICamera;
class BattleWorld {
public:
    ICamera* GetMainCamera() const;
    void Draw();
    void Tick();
    static void Initialize();
    void OnPostDraw();
    BattleUnit* CreateBattleUnit(int a0);
    void DeleteBattleUnit(int a0);
    void DeleteDisplayObjects();
    static BattleWorld* Get();
    static void Finalize();
    void OnPreDraw();
    BattleWorld();
    ~BattleWorld();
    void OnPostTick();
    void OnPreTick();
};
class BattleUtil {
public:
    static void InstantCamera();
    static int GetCameraModeType();
    static void CalcComebackCamera(CameraParam*);
    static bool IsPlayingBattleCamera();
    static void SuspendMap();
    static int GetBossType();
    static void HideMapIcon();
    static void HideMapUnit();
    static void ShowMapIcon();
    static void ShowMapUnit();
    static bool IsBossBattle();
    static bool IsBossUsable();
    static nn::math::VEC3 GetZoomTarget(const nn::math::VEC3& a0);
    static void ResetUnitIcon(bool a0);
    static void BattleFinalize();
    static bool IsChangeBattle();
    static void OpenBattleArea(const Unit* a0);
    static int GetBossUnitSide();
    static void ComebackUnitIcon(float a0, float a1);
    static void UpdateBattleArea();
    static void DeleteBattleUnits();
    static void EncounterUnitIcon(float a0, float a1);
    static void ResetBattleStance();
    static int GetCurrentUnitSide();
    static void ResetDistanceStance(float a0);
    static bool IsLoadingBattleUnits();
    static void BindMap();
    static void ResumeMap();
    static void UnbindMap();
    static void BattleInitialize();
    static void ResetBattleArea();
};
class BattleSeq {
public:
    static void WaitCamera();
    static void ChangeCamera();
    static void PlayArenaCamera();
    static void PlayChangeCamera();
    static void PlayComebackCamera();
    static void PlaySeamlessCamera();
    static void ChangeLoad();
    static void ChangeFree();
    static void Initialize();
    static void Persistent();
    static void BeginBranch();
    static void HideMapIcon();
    static void HideMapUnit();
    static void ResetOnSkip();
    static void ShowMapIcon();
    static void ShowMapUnit();
    static void WaitLoading();
    static void ChangeResume();
    static void ReturnBranch();
    static void ChangeSuspend();
    static void EndFadeMapSound();
    static void UpdateBattleArea();
    static void BeginFadeMapSound();
    static void BuildActionStates();
    static void CreateBattleUnits();
    static void EndFadeBattleSound();
    static void BeginFadeBattleSound();
    static void ResetInitialLocation();
    static void Sleep();
    static void Finalize();
    static void MainLoop();
    static void EndBranch();
};
class BattleController {
public:
    static bool IsBlurring();
    static bool DetectRunning();
    static void Create(ProcInst* a0, map::BattleCalculator* a1);
    static void Wakeup();
    static ProcInst* GetProc();
    static bool IsRunning();
};
