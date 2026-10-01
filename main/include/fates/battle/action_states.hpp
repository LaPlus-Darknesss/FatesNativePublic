#pragma once
#include "fates/battle/battle_support_models.hpp"

class ActionSync {
public:
    void Tick();
    void Enter();
};

class ActionDance {
public:
    void Tick();
    void Enter();
    AnimClip::Type GetAnim() const;
};

class ActionRound {
public:
    void Enter();
};

class ActionSkill {
public:
    void Tick();
    ActionSkill(BattleUnit* a0, const BattlePhase* a1);
    AnimClip::Type GetAnim() const;
};

class ActionAttack {
public:
    void RealImpact();
    void DummyImpact();
    void ImpactCommon(AnimClip::Type a0);
    AnimClip::Type SuggestProperAttack(const BattleUnit* a0, BattlePhase* a1);
    void Tick();
    void Enter();
    void Leave();
    bool IsAttackState() const;
    AnimClip::Type GetAnim() const;
};

class ActionDeform {
public:
    void Tick();
    void Enter();
};

class ActionEvWait {
public:
    void Tick();
};

class IActionState {
public:
    bool IsAnimFinished();
    void Tick();
    void Enter();
    IActionState(BattleUnit* a0, const BattlePhase* a1);
    bool DoUpdateDir() const;
    bool IsAttackState() const;
    bool IsAllowedToEnd() const;
    AnimClip::Type GetAnim() const;
};

class ActionPostAttackBackStep {
public:
    void Tick();
    void Enter();
    AnimClip::Type GetAnim() const;
};

class ActionDualRun {
public:
    void Tick();
    bool DoUpdateDir() const;
    AnimClip::Type GetAnim() const;
};

class ActionEvEmote {
public:
    void Enter();
};

class ActionEvRunTo {
public:
    void Tick();
    void Enter();
    void Leave();
    const char* GetAnimName() const;
    float GetTurnRate() const;
    float GetDefaultAcc() const;
    bool IsAllowedToEnd() const;
    bool RemainAtTalkEnd() const;
    float GetDefaultMaxVel() const;
};

class ActionStealth {
public:
    void Enter();
};

class ActionApproach {
public:
    void Tick();
    void Enter();
};

class ActionEvGazeAt {
public:
    void Enter();
    ActionEvGazeAt(BattleUnit* a0, BattleUnit* a1);
};

class ActionEvMotion {
public:
    void Tick();
    void Enter();
    bool RemainAtTalkEnd() const;
};

class ActionEvTurnTo {
public:
    void Tick();
    void Enter();
    bool RemainAtTalkEnd() const;
};

class ActionGreeting {
public:
    void Tick();
    void Enter();
    ActionGreeting(BattleUnit* a0, const BattlePhase* a1);
    AnimClip::Type GetAnim() const;
};

class ActionDualGuard {
public:
    void Tick();
    void Enter();
    void Leave();
    ActionDualGuard(BattleUnit* a0, const BattlePhase* a1);
};

class ActionBattleToEv {
public:
    void Tick();
    void Enter();
};

class ActionWaitDeform {
public:
    void Tick();
};

class ActionDualSupport {
public:
    void Enter();
    void Leave();
    AnimClip::Type GetAnim() const;
};

class ActionDualWaiting {
public:
    void Tick();
    void Enter();
    bool IsAllowedToEnd() const;
    AnimClip::Type GetAnim() const;
};

class ActionDualBackStep {
public:
    void Enter();
    bool DoUpdateDir() const;
    AnimClip::Type GetAnim() const;
};

class ActionEvFrameShift {
public:
    void Enter();
};

class ActionEvGazeLocked {
public:
    void Enter();
};

class ActionShadowAttack {
public:
    void Tick();
    void Enter();
    ActionShadowAttack(BattleUnit* a0, AnimClip::Type a1);
};

class ActionStateMachine {
public:
    void Tick();
    void Clear();
};

class ActionCounterAttack {
public:
    void Tick();
    void Enter();
};

class ActionDualChildEscape {
public:
    void Tick();
    void Enter();
};

class ActionDualAttackRequest {
public:
    void Tick();
    void Enter();
};

class ActionEvMotionDirectEnd {
public:
    void Enter();
};

class ActionAny {
public:
    void Tick();
    AnimClip::Type GetAnim() const;
};

class ActionRod {
public:
    void Tick();
    ActionRod(BattleUnit* a0, const BattlePhase* a1);
    AnimClip::Type GetAnim() const;
};

class ActionWin {
public:
    void Tick();
    void Enter();
    AnimClip::Type GetAnim() const;
};

class ActionEvBase {
public:
    bool DoUpdateDir() const;
    bool RemainAtTalkEnd() const;
};

class ActionEvWalkTo {
public:
    const char* GetAnimName() const;
    float GetTurnRate() const;
    float GetDefaultAcc() const;
    bool IsAllowedToEnd() const;
    float GetDefaultMaxVel() const;
};

