#include "fates/map/sequence.hpp"
#include "fates/map/sequence_steps.hpp"
#include "fates/detail/map_sequence_runtime.hpp"

namespace map {
#define MIND_CREATE(Name) void SequenceMind::Name(ProcInst* p){fates::decomp_detail::MapSequenceCall("SequenceMind." #Name,p);}
MIND_CREATE(CreateTalk)
MIND_CREATE(CreateWarp)
MIND_CREATE(CreateItemUse)
MIND_CREATE(CreateDoubleOn)
MIND_CREATE(CreateDoubleOff)
MIND_CREATE(CreateDoubleTrade)
MIND_CREATE(CreateDoubleChange)
MIND_CREATE(Create)
#undef MIND_CREATE
}

namespace map::sequence_detail {
void MindUnitProcess::Persistent(){fates::decomp_detail::MapSequenceCall("SequenceMindHelper.Persistent");}
void MindUnitProcess::Tick(){fates::decomp_detail::MapSequenceCall("SequenceMindHelper.Tick");}
void MindUnitProcess::Wait(){fates::decomp_detail::MapSequenceCall("SequenceMindHelper.Wait");}
void MindUnitProcess::Construct(){
    // PROVEN: this internal process wraps one Unit's AI/mind action lifecycle.
    // The two retail destructor entries at 0x0036B904/0x0036B914 are ABI-only
    // veneers and intentionally do not become additional semantic functions.
    fates::decomp_detail::MapSequenceCall("SequenceMindHelper.Construct");
}
} // namespace map::sequence_detail
