#pragma once
#include "fates/battle/battle_support.hpp"

class BattleArgs {
public:
    static void Initialize();
    void PostProcess();
    void BuildActionState(BattleUnit* a0, BattleUnit* a1);
    void ClearBattlePhase();
    void CreateBattleUnits();
    void Sync(const BattlePhase* a0);
    void Import(map::BattleCalculator* a0);
    void Import(int a0, const Unit* a1);
    static void Finalize();
    int GetLastPhaseIndex() const;
};

class BattlePhase {
public:
    void DecideDamageAuto();
    static int GetDamageEmotion(AnimClip::Type a0);
    static int GetDamageEmotionTime(AnimClip::Type a0);
    BattlePhase();
    AnimClip::Type GetAttackClip() const;
    AnimClip::Type GetDamageClip() const;
    bool IsEventBattle() const;
    bool IsSomeoneDead() const;
    bool NeedImpactGuard() const;
    bool NeedImpactShake() const;
    int GetHitStopFrames() const;
};

class BattleSoundActor {
public:
    bool IsVoiceCanceled();
    void SetSoundPosition(const nn::math::VEC3& a0);
    void PlaySound(const char* a0);
    BattleSoundActor(int a0);
    ~BattleSoundActor();
};

class BattleUnitParams {
public:
    void Import(const Unit* a0);
    void Import(const map::BattleInfo* a0, int a1);
    std::uint64_t IsReverse(const map::BattleInfo* a0);
    BattleUnitParams();
    const void* GetWeaponData() const;
};

class BattleUnitResName {
public:
    void ClearAnims(unsigned int a0);
    void PostProcess(const Unit* a0, const Person* a1);
    void AddAccessory(int a0, const char* a1, const char* a2);
    void ClearEventAnims();
    void ConstructDirect(const char* a0, unsigned int a1, const char* a2);
    void ClearBattleAnims();
    void ConstructHoodMan(bool a0);
    void ModifySwimwearName(const Unit* a0, const Person* a1, const Job* a2);
    void ClearExceptDeformAnims();
    void ModifyForceTextureName(int a0, bool a1);
    void ClearExceptSubjectAnims();
    void Import(const unsigned int* a0);
    void Import(int a0, unsigned int a1, unsigned int a2, const unsigned int* a3);
    void Construct(const Unit* a0, const Person* a1, const char* a2, const Job* a3, const char* a4, unsigned int a5);
    void Construct(const Unit* a0, unsigned int a1, const char* a2);
    void Construct(const char* a0, const Job* a1, unsigned int a2, const char* a3);
    void Construct(const char* a0, unsigned int a1, const char* a2);
    BattleUnitResName();
};

class BattleUnitResHolder {
public:
    const void* GetSignals(AnimClip::Type a0);
    void PreloadModels(const BattleUnitResName& a0, const BattleUnit* a1);
    void PreloadAnims(const BattleUnitResName& a0);
    void loadanim(const BattleUnitResName& a0, AnimClip::Type a1, FilePrior::Level a2);
    bool IsLoading();
    BattleUnitResHolder(BattleUnit* a0);
    ~BattleUnitResHolder();
    AnimClip::Type GetSimilarAnim(AnimClip::Type a0) const;
};

class BattleUnitColorFader {
public:
    void Persistent();
};

class BattleUnitScratchFader {
public:
    void Effect();
};

class BattleUnitStealthFader {
public:
    void Persistent();
    void Effect();
};

