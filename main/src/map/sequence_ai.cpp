#include "fates/map/sequence.hpp"
#include "fates/detail/map_sequence_runtime.hpp"

namespace map {
void SequenceAI::InitializeThread() {
    // PROVEN: retail owns a dedicated AI worker thread with a 0x2000-byte stack,
    // mutex/light-event synchronization, and a global thread owner. The native
    // port should preserve scheduling semantics, not CTR Thread object layout.
    fates::decomp_detail::MapSequenceCall("SequenceAI.InitializeThread");
}
void SequenceAI::FinalizeThread() {
    // PROVEN: joins/finalizes the worker, releases stack allocation and thread
    // synchronization state, and clears the global owner.
    fates::decomp_detail::MapSequenceCall("SequenceAI.FinalizeThread");
}
void SequenceAI::Create(ProcInst* parent) {
    // PROVEN: marks map AI-active state, creates the AI controller, then creates
    // the SequenceAI process beneath the supplied parent.
    fates::decomp_detail::MapSequenceCall("SequenceAI.Create",parent);
}
} // namespace map
