#include "fates/map/sequence_battle.hpp"
#include "fates/detail/map_battle_resolution_runtime.hpp"

namespace map::SequenceBattle {
void Create(ProcInst* parent){ fates::decomp_detail::MapBattleResolutionCall("SequenceBattle.Create",parent); }
void CreateEvent(ProcInst* parent,const EventData* data){
    // PROVEN: retail allocates one battle process, constructs it from EventData,
    // then attaches it beneath the supplied ProcInst. EventData layout stays opaque.
    fates::decomp_detail::MapBattleResolutionCall("SequenceBattle.CreateEvent",parent,data);
}
const char* GetAdditionalEffectName(int equipSkillId){
    // PROVEN: eight cached EquipSkill IDs map to eight first-party effect-name
    // strings; every other skill returns no additional-effect name.
    return fates::decomp_detail::MapBattleResolutionValue<const char*>("SequenceBattle.GetAdditionalEffectName",equipSkillId);
}
}

namespace map::battle_sequence_detail {
namespace { inline void B(const char* n){fates::decomp_detail::MapBattleResolutionCall(n);} }
void MergeSeal(Capability* cap,const Unit* unit,int index,int incoming){
    // PROVEN: the normal merge keeps max(existing,incoming). Retail checks
    // internal Skill ID SEID_負の連鎖; the original FE14 Skill table + English
    // message data identify it as Inevitable End. With that skill, merge is additive.
    fates::decomp_detail::MapBattleResolutionCall("SequenceBattle.MergeSeal",cap,unit,index,incoming);
}
#define STEP(Name) void BattleProcess::Name(){B("SequenceBattle." #Name);}
STEP(ActionWait) STEP(CommonWait) STEP(DestroyEnd) STEP(FocusBegin) STEP(TransBegin)
STEP(AttackBegin) STEP(BranchEvent) STEP(BranchFirst) STEP(BranchScene) STEP(CursorBegin)
STEP(DestroyRuin) STEP(ResetMotion) STEP(DestroyBegin) STEP(FormationEnd)
STEP(HpWindowOpen) STEP(HpWindowWait) STEP(LinkExchange) STEP(Resurrection) STEP(RodMotionEnd)
STEP(DeadActionEnd) STEP(HpWindowClose) STEP(RodMotionWait) STEP(CaptureReflect)
STEP(DeadSkipEscape) STEP(FormationBegin) STEP(FormationStand) STEP(HpWindowCreate)
STEP(HpWindowDelete) STEP(ResurrectionIn) STEP(RodMotionBegin) STEP(DeadActionBegin)
STEP(FormationAttack) STEP(InstantDeadEvent) STEP(AfterDetailBattle) STEP(BeforeDetailBattle)
STEP(CreateDetailBattle) STEP(ActionWaitForBattle) STEP(Rod) STEP(Skill) STEP(Rescue)
STEP(Capture) STEP(Destroy) STEP(FocusEnd) STEP(PopEvent) STEP(RescueIn) STEP(TransEnd)
STEP(AttackEnd) STEP(CursorEnd) STEP(DeadEvent) STEP(FocusWait) STEP(PushEvent)
STEP(RescueOut) STEP(TransWait) STEP(BattleEvent) STEP(InstantBattleEvent)
#undef STEP
void BattleProcess::SetHiddenOne(BattleCalculator* c,int side){fates::decomp_detail::MapBattleResolutionCall("SequenceBattle.SetHiddenOne",c,side);}
void BattleProcess::Attack(){
    // PROVEN: commits the current BattleCalculator scene's final HP to up to
    // four Unit slots, updates clone HP, updates guard/progress bytes, and emits
    // damage presentation from scene display deltas when presentation is enabled.
    B("SequenceBattle.Attack");
}
void BattleProcess::UpdateHp(bool presentation){
    // PROVEN: HP synchronization is distinct from damage calculation. The scene
    // already contains resolved values; this step commits/animates them.
    fates::decomp_detail::MapBattleResolutionCall("SequenceBattle.UpdateHp",presentation);
}
void BattleProcess::Construct(const SequenceBattle::EventData* data){
    // PROVEN: source-owned process construction wires BattleInfo/BattleCalculator,
    // Unit-side pointers, scene state, event data and owned presentation state.
    fates::decomp_detail::MapBattleResolutionCall("SequenceBattle.Construct",data);
}
void BattleProcess::DestroyOwnedState(){
    // PROVEN: meaningful destructor resets battle-sound info, restores map Actor
    // positions/transfer state, frees the owned BattleCalculator/BattleInfo and
    // releases battle-sequence resources. The 0x0035DC20 delete veneer is ABI-only.
    B("SequenceBattle.DestroyOwnedState");
}
Unit* BattleProcess::GetSceneTargetUnit() const{return fates::decomp_detail::MapBattleResolutionValue<Unit*>("SequenceBattle.GetSceneTargetUnit");}
bool BattleProcess::IsShow() const{return fates::decomp_detail::MapBattleResolutionValue<bool>("SequenceBattle.IsShow");}
Unit* BattleProcess::GetLoser() const{
    // PROVEN: normal map battles use BattleCalculator side status bit 0x400;
    // external battle data follows the external scene-result stream instead.
    return fates::decomp_detail::MapBattleResolutionValue<Unit*>("SequenceBattle.GetLoser");
}
Unit* BattleProcess::GetWinner() const{return fates::decomp_detail::MapBattleResolutionValue<Unit*>("SequenceBattle.GetWinner");}
} // namespace map::battle_sequence_detail
