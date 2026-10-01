#include "fates/map/sequence.hpp"
#include "fates/map/sequence_steps.hpp"
#include "fates/detail/map_sequence_runtime.hpp"

namespace map {
void SequenceCompleteEffect::Create(ProcInst* parent,const Mvp* mvp){
    // PROVEN: normal completion creates the StageClear UI/effect sequence,
    // formats localized chapter title data, and uses a 300-frame wait in versus.
    // A castle-specific path instead updates Dragon Vein completion score using
    // Situation::CalculateDoragonVein with retail clamping.
    fates::decomp_detail::MapSequenceCall("SequenceCompleteEffect.Create",parent,mvp);
}
}

namespace map::sequence_detail {
namespace { inline void Complete(const char* n){fates::decomp_detail::MapSequenceCall(n);} }
void CompleteEffectProcess::Persistent(){Complete("SequenceCompleteEffect.Persistent");}
void CompleteEffectProcess::Open(){Complete("SequenceCompleteEffect.Open");}
void CompleteEffectProcess::Tick(){Complete("SequenceCompleteEffect.Tick");}
void CompleteEffectProcess::Close(){Complete("SequenceCompleteEffect.Close");}
void CompleteEffectProcess::Prepare(){Complete("SequenceCompleteEffect.Prepare");}
void CompleteEffectProcess::WaitAsync(){Complete("SequenceCompleteEffect.WaitAsync");}
void CompleteEffectProcess::Destroy(){
    // PROVEN source-owned cleanup: releases the completion TexFile/FileBase
    // resource and destroys the process object.
    Complete("SequenceCompleteEffect.Destroy");
}
} // namespace map::sequence_detail
