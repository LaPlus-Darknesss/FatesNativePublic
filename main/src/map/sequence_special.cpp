#include "fates/map/sequence.hpp"
#include "fates/detail/map_sequence_runtime.hpp"

namespace map {
void SequenceFixed::Create(ProcInst* p){fates::decomp_detail::MapSequenceCall("SequenceFixed.Create",p);}
void SequenceFixed::Create2(ProcInst* p){fates::decomp_detail::MapSequenceCall("SequenceFixed.Create2",p);}
void SequenceItem::Create(ProcInst* p){fates::decomp_detail::MapSequenceCall("SequenceItem.Create",p);}
void SequenceLink::Create(ProcInst* p){fates::decomp_detail::MapSequenceCall("SequenceLink.Create",p);}
void SequenceCannon::CreateForAuto(ProcInst* p,Unit* u,int x,int y){fates::decomp_detail::MapSequenceCall("SequenceCannon.CreateForAuto",p,u,x,y);}
void SequenceCannon::Create(ProcInst* p){fates::decomp_detail::MapSequenceCall("SequenceCannon.Create",p);}
void SequencePhoenix::Create(ProcInst* p){
    // PROVEN: only active in Phoenix mode on the qualifying human-turn state;
    // retail scans numeric force 4 for a Unit carrying the resurrection flag
    // before creating the Phoenix sequence.
    fates::decomp_detail::MapSequenceCall("SequencePhoenix.Create",p);
}
void SequenceWinRule::Create(ProcInst* p){
    // PROVEN: loads the system font, obtains the current win-rule message,
    // splits it into lines, and uses a 300-frame presentation wait in versus.
    fates::decomp_detail::MapSequenceCall("SequenceWinRule.Create",p);
}
bool SequenceInformal::Check(int x,int y){
    // PROVEN: uses the Informal map-trick enumerator and returns whether at
    // least one qualifying trick exists at the requested cell.
    return fates::decomp_detail::MapSequenceValue<bool>("SequenceInformal.Check",x,y);
}
void SequenceInformal::Create(ProcInst* p,Unit* u,int x,int y){fates::decomp_detail::MapSequenceCall("SequenceInformal.Create",p,u,x,y);}
void SequenceTurnEffect::Create(ProcInst* p){fates::decomp_detail::MapSequenceCall("SequenceTurnEffect.Create",p);}
void SequenceTargetSelect::Create(ProcInst* p){fates::decomp_detail::MapSequenceCall("SequenceTargetSelect.Create",p);}
void SequenceTemporarySave::Create(ProcInst* p){
    // PROVEN: owns a GameBackup object configured with retail temporary-save
    // mode 5 before attaching the process to the parent.
    fates::decomp_detail::MapSequenceCall("SequenceTemporarySave.Create",p);
}
} // namespace map
