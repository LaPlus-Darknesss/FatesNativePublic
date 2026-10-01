#include "fates/map/sequence.hpp"
#include "fates/map/sequence_steps.hpp"
#include "fates/detail/map_sequence_runtime.hpp"

namespace map {
void SequenceDance::Create(ProcInst* parent){fates::decomp_detail::MapSequenceCall("SequenceDance.Create",parent);}
}

namespace map::sequence_detail {
namespace { inline void Dance(const char* n){fates::decomp_detail::MapSequenceCall(n);} }
void DanceBattleProcess::AfterDetailBattle(){Dance("SequenceDance.Battle.AfterDetailBattle");}
void DanceBattleProcess::CreateDetailBattle(){Dance("SequenceDance.Battle.CreateDetailBattle");}
void DanceBattleProcess::BeforeDetailBattle(){Dance("SequenceDance.Battle.BeforeDetailBattle");}
void DanceBattleProcess::Grow(){Dance("SequenceDance.Battle.Grow");}
void DanceBattleProcess::FocusEnd(){Dance("SequenceDance.Battle.FocusEnd");}
void DanceBattleProcess::Reliance(){Dance("SequenceDance.Battle.Reliance");}
void DanceBattleProcess::Destroy(){
    // PROVEN source-owned cleanup: unbinds battle sound info, destroys the
    // owned battle calculator, releases the owned auxiliary object/process.
    Dance("SequenceDance.Battle.Destroy");
}
void DanceProcess::End(){Dance("SequenceDance.End");}
void DanceProcess::Grow(){Dance("SequenceDance.Grow");}
void DanceProcess::Begin(){Dance("SequenceDance.Begin");}
void DanceProcess::Action(){Dance("SequenceDance.Action");}
void DanceProcess::Reliance(){Dance("SequenceDance.Reliance");}
// The derived ProcDance deleting-destructor veneer at 0x00350984 has no
// distinct Fates policy beyond base destruction + delete and remains ABI-only.
} // namespace map::sequence_detail
