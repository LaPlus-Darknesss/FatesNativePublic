#pragma once
#include <utility>

namespace fates::decomp_detail {

// OPAQUE boundary for Tier-A tactical action/turn sequence reconstruction.
// The durable source owns Fates' process policy and state transitions, while
// ProcInst storage, UI/backend objects, thread primitives, and still-opaque
// battle/event consumers remain replaceable runtime services.
template<class... A>
inline void MapSequenceCall(const char*, A&&...) {}

template<class T, class... A>
inline T MapSequenceValue(const char*, A&&...) { return T{}; }

} // namespace fates::decomp_detail
