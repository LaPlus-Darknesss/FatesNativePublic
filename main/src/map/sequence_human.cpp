#include "fates/map/sequence.hpp"
#include "fates/map/sequence_steps.hpp"
#include "fates/detail/map_sequence_runtime.hpp"

namespace map {
SequenceHuman* SequenceHuman::GetInstance() {
    return fates::decomp_detail::MapSequenceValue<SequenceHuman*>("SequenceHuman.GetInstance");
}
void SequenceHuman::SetJobIntroTrigger() {
    fates::decomp_detail::MapSequenceCall("SequenceHuman.SetJobIntroTrigger");
}
void SequenceHuman::Jump(Label label) {
    // PROVEN: no-op when the human sequence process is not active.
    fates::decomp_detail::MapSequenceCall("SequenceHuman.Jump",label);
}
void SequenceHuman::Create(ProcInst* parent) {
    fates::decomp_detail::MapSequenceCall("SequenceHuman.Create",parent);
}
void SequenceHuman::SetCannon(const trick::Data* cannon) {
    // PROVEN: retail stores the selected cannon only on the live human process.
    fates::decomp_detail::MapSequenceCall("SequenceHuman.SetCannon",cannon);
}
}

namespace map::sequence_detail {
namespace { inline void Human(const char* n){fates::decomp_detail::MapSequenceCall(n);} }

#define HUMAN_STEP(Name) void HumanProcess::Name(){Human("SequenceHuman." #Name);}
HUMAN_STEP(UnitListMenu)
HUMAN_STEP(ConfigMenu)
HUMAN_STEP(CreateJobIntro)
HUMAN_STEP(ClassChange)
HUMAN_STEP(FreeCursorPrepare)
HUMAN_STEP(FixedAfterEvent)
HUMAN_STEP(ShowBalloon)
HUMAN_STEP(DoubleTraded)
HUMAN_STEP(UnitMoveWait)
HUMAN_STEP(CreateMapSave)
HUMAN_STEP(DoubleChanged)
HUMAN_STEP(FreeCursorTick)
HUMAN_STEP(PickCursorTick)
HUMAN_STEP(BattleInfoEvent)
HUMAN_STEP(SortieDoubleOff)
HUMAN_STEP(TrickCursorTick)
HUMAN_STEP(CannonCursorTick)
HUMAN_STEP(PickCursorCancel)
HUMAN_STEP(ClassChangeBranch)
HUMAN_STEP(PickCursorPrepare)
HUMAN_STEP(SortieShowBalloon)
HUMAN_STEP(SortieDoubleChange)
HUMAN_STEP(TrickCursorPrepare)
HUMAN_STEP(UnitListMenuBranch)
HUMAN_STEP(CannonCursorPrepare)
HUMAN_STEP(ItemUsedForMedicine)
HUMAN_STEP(JumpForCursorResume)
HUMAN_STEP(TransporterFinalize)
HUMAN_STEP(SortiePositionChanged)
HUMAN_STEP(TransporterInitialize)
HUMAN_STEP(PickCursorResumePrepare)
HUMAN_STEP(SortieCursorDecideDouble)
HUMAN_STEP(TrickCursorResumePrepare)
HUMAN_STEP(CannonCursorResumePrepare)
HUMAN_STEP(Mind)
HUMAN_STEP(Fixed)
HUMAN_STEP(UnitMove)
HUMAN_STEP(PickEvent)
HUMAN_STEP(Talked)
HUMAN_STEP(UnitCommandCancel)
HUMAN_STEP(TransporterMenu)
#undef HUMAN_STEP

void HumanProcess::AutoTurnEnd() {
    // PROVEN: when either retail auto-end enable bit is active, the current
    // force is scanned for any still-actionable Unit. If none remains, the
    // process jumps to label 0x28; otherwise normal human control continues.
    Human("SequenceHuman.AutoTurnEnd");
}
void HumanProcess::GameEndBranch() {
    // PROVEN: IsGameOver -> Sequence label 7; IsComplete -> Sequence label 6;
    // either condition redirects the human process to label 0x29.
    Human("SequenceHuman.GameEndBranch");
}
bool HumanProcess::IsCannonAttack(int x,int y) {
    return fates::decomp_detail::MapSequenceValue<bool>("SequenceHuman.IsCannonAttack",x,y);
}
void HumanProcess::SaveMenuBranch() {
    // PROVEN: a false retail save-menu result redirects to local label 7.
    Human("SequenceHuman.SaveMenuBranch");
}
void HumanProcess::TurnEnd() {
    // PROVEN: disables cursor/panel interaction, stores cursor position for the
    // current force, restores Situation-view game info, hides terrain info,
    // disables gradation, then returns control through Mind/turn machinery.
    Human("SequenceHuman.TurnEnd");
}
void HumanProcess::Destroy() {
    // PROVEN: clears the human-sequence activity bit and singleton process slot
    // before destroying the process. This is retained as source responsibility.
    Human("SequenceHuman.Destroy");
}

} // namespace map::sequence_detail
