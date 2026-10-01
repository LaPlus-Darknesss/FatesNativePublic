#pragma once
class ProcInst;
class Unit;
class Capability;
namespace unit { class Item; }

namespace map {
class BattleCalculator;

namespace SequenceBattle {
// EventData is source-owned as a semantic request type, but its retail packet
// layout intentionally remains opaque until all producers/consumers converge.
struct EventData;
void Create(ProcInst* parent);
void CreateEvent(ProcInst* parent, const EventData* eventData);
const char* GetAdditionalEffectName(int equipSkillId);
}

namespace battle_sequence_detail {
void MergeSeal(Capability* capability, const Unit* unit, int componentIndex, int incomingValue);

class BattleProcess {
public:
    void ActionWait(); void CommonWait(); void DestroyEnd(); void FocusBegin();
    void GaleEffect(); void ItemExpend(); void PlayEffect(); void TransBegin();
    void AddReliance(Unit* a, Unit* b, int points); void AfterBattle();
    void AttackBegin(); void BranchEvent(); void BranchFirst(); void BranchScene();
    void CursorBegin(); void DestroyRuin(); void GaleReflect(); void ResetMotion();
    void Craftmanship(); void DestroyBegin(); void FormationEnd(); void FreezeEffect();
    void GetRichQuick(); void HpWindowOpen(); void HpWindowWait(); void LinkExchange();
    void PoisonEffect(); void Resurrection(); void RodMotionEnd();
    void SetHiddenOne(BattleCalculator* calculator, int side);
    void DeadActionEnd(); void FreezeReflect(); void GetSealEffect(int side);
    void HpWindowClose(); void PoisonOfWitch(); void PoisonReflect();
    void RodMotionWait(); void CaptureReflect(); void DeadSkipEscape();
    void FormationBegin(); void FormationStand(); void HpWindowCreate();
    void HpWindowDelete(); void LifeAbsorption(); void RelianceEffect();
    void ResurrectionIn(); void RodMotionBegin(); void WeaknessEffect();
    void DeadActionBegin(); void DrainCapability(); void FormationAttack();
    void GetPoisonEffect(int side); void KillingInstinct(); void InstantDeadEvent();
    void AfterDetailBattle(); void BeforeDetailBattle(); void CheerAfterBattleOf();
    void CraftmanshipEffect(); void CreateDetailBattle(); void GetRichQuickEffect();
    void ActionWaitForBattle(); void CraftmanshipReflect(); void GetRichQuickReflect();
    void HpChangeAfterBattle(); void PoisonOfWitchEffect(); void CheerAfterBattleDf();
    void CheerAfterBattleImpl(int side); void LifeAbsorptionEffect();
    void PoisonOfWitchReflect(); void DrainCapabilityEffect();
    void EnhanceAfterBattleOne(Unit* unit, const unit::Item* item, int side, bool actor, bool partner);
    void KillingInstinctEffect(); void LifeAbsorptionReflect();
    void CheerAfterBattleDualDf(); void CheerAfterBattleDualOf();
    void DrainCapabilityReflect(); void EnhanceAfterBattle();
    void EnhanceAfterBattleImpl(bool includePartner); void KillingInstinctReflect();
    void WeaknessCalculateEffect(); void CalculateEffectHiddenOne(BattleCalculator* calculator, int side);
    void GetPoisonEffectReverseSide(int side); void EnhanceAfterBattleCalculateEffect();
    void Rod(); void Gale(); void Grow(); void Skill(); void Freeze(); void Poison();
    void Rescue(); void Capture(); void Destroy(); void WeaknessReflect();
    void SetSeal(int side); void FocusEnd(); void ItemDrop(); void PopEvent();
    void RescueIn(); void TransEnd(); void Attack(); void UpdateHp(bool commitPresentation);
    void AttackEnd(); void CursorEnd(); void DeadEvent(); void FocusWait();
    void HitEffect(); void PushEvent(); void RescueOut(); void TransWait();
    void Construct(const SequenceBattle::EventData* eventData);
    void DestroyOwnedState();
    void BattleEvent(); void InstantBattleEvent();
    Unit* GetSceneTargetUnit() const; bool IsShow() const;
    Unit* GetLoser() const; Unit* GetWinner() const;
};

} // namespace battle_sequence_detail
} // namespace map
