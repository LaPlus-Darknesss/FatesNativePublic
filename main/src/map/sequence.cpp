#include "fates/map/sequence.hpp"
#include "fates/map/sequence_steps.hpp"
#include "fates/detail/map_sequence_runtime.hpp"

namespace map {
Sequence* Sequence::GetInstance() {
    return fates::decomp_detail::MapSequenceValue<Sequence*>("Sequence.GetInstance");
}
void Sequence::Jump(Label label) {
    // PROVEN: retail jumps only when the singleton process instance exists.
    fates::decomp_detail::MapSequenceCall("Sequence.Jump", label);
}
void Sequence::Create(ProcInst* parent) {
    fates::decomp_detail::MapSequenceCall("Sequence.Create", parent);
}
void Sequence::Resume(ProcInst* parent) {
    fates::decomp_detail::MapSequenceCall("Sequence.Resume", parent);
}
}

namespace map::sequence_detail {
namespace {
inline void Step(const char* name) { fates::decomp_detail::MapSequenceCall(name); }
}

void MainTurnProcess::Disconnect() {
    // PROVEN: versus-only disconnect synchronization is suppressed for the
    // retail local/selected versus modes and after an operation error.
    Step("Sequence.Main.Disconnect");
}
void MainTurnProcess::Persistent() { Step("Sequence.Main.Persistent"); }
void MainTurnProcess::TurnBranch() {
    // PROVEN: current turn-code 1/2/3 selects process labels 2/3/4;
    // every other code selects label 5. Force::Type names remain numeric.
    const auto code=fates::decomp_detail::MapSequenceValue<unsigned char>("Sequence.CurrentTurnCode");
    const int label=(code==1u)?2:(code==2u)?3:(code==3u)?4:5;
    fates::decomp_detail::MapSequenceCall("Sequence.Main.JumpProcessLabel",label);
}
void MainTurnProcess::TurnEffect() {
    // PROVEN: retail chooses a force/turn-dependent GameEffect and binds it
    // to the turn process before applying effect status 4.
    Step("Sequence.Main.TurnEffect");
}
void MainTurnProcess::TurnScroll() { Step("Sequence.Main.TurnScroll"); }
void MainTurnProcess::ShowWinRule() { Step("Sequence.Main.ShowWinRule"); }
void MainTurnProcess::GameEndBranch() {
    // PROVEN: chapter completion and game-over state are sourced from
    // Situation and redirect the parent map sequence to dedicated labels.
    Step("Sequence.Main.GameEndBranch");
}
void MainTurnProcess::GameOverBranch() { Step("Sequence.Main.GameOverBranch"); }
void MainTurnProcess::GameOverEffect() {
    // PROVEN: the versus path resets end-of-map Unit state, disables tactical
    // panels/terrain info, updates danger overlays, stops BGM, and binds the
    // retail game-over presentation effect.
    Step("Sequence.Main.GameOverEffect");
}
void MainTurnProcess::TurnScrollAfterTerrainEffect() {
    // PROVEN: if SequenceHelper finds the first cursor cell for the next turn,
    // retail snaps both cursor current/target coordinates there and moves the camera.
    Step("Sequence.Main.TurnScrollAfterTerrainEffect");
}
void MainTurnProcess::Sync() { Step("Sequence.Main.Sync"); }
void MainTurnProcess::TurnEnd() {
    // PROVEN: retail iterates the current force, clears per-turn Unit flags,
    // advances Unit enhancement/clone state, refreshes danger overlays, then
    // calls Situation::TurnEnd and branches to game-over/complete when needed.
    Step("Sequence.Main.TurnEnd");
}
void MainTurnProcess::Complete() {
    // PROVEN: this is chapter-completion gameplay policy, not a visual tail.
    // It commits chapter Support/Reliance gains, selects/records the strongest
    // chapter relationship pair using stable Person-id tie breaking, updates
    // chapter record data, and performs additional completion bookkeeping.
    // The full packet/user-data layout remains opaque until its producers are source-owned.
    Step("Sequence.Main.Complete");
}
void MainTurnProcess::GameOver() { Step("Sequence.Main.GameOver"); }
void MainTurnProcess::TurnSkip() { Step("Sequence.Main.TurnSkip"); }
void MainTurnProcess::TurnTime() { Step("Sequence.Main.TurnTime"); }
void MainTurnProcess::TurnBegin() {
    // PROVEN: turn-begin state is a gameplay lifecycle boundary that prepares
    // force/unit state before human or AI control begins.
    Step("Sequence.Main.TurnBegin");
}
void MainTurnProcess::CompleteEffect() { Step("Sequence.Main.CompleteEffect"); }
void MainTurnProcess::Destroy() {
    // PROVEN source responsibility: frees two map-sequence message archives,
    // clears the live singleton process pointer, then destroys the process.
    Step("Sequence.Main.Destroy");
}

void SyncProcess::Open() {
    // PROVEN: opens the retail common wait/synchronization message.
    Step("Sequence.Sync.Open");
}
void SyncProcess::Close() {
    // PROVEN: closes that wait message with retail's immediate/forced flag.
    Step("Sequence.Sync.Close");
}
void SyncProcess::Execute() { Step("Sequence.Sync.Execute"); }

} // namespace map::sequence_detail
