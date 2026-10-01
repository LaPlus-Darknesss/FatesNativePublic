#pragma once
#include <utility>
namespace fates::decomp_detail {

// OPAQUE backend boundary for Tier-A tactical-map code.
// These adapters are intentionally named by semantic operation rather than by retail offsets.
// A later native/runtime pass can replace each operation independently after layout and
// differential-test evidence is sufficient.
template<class... A>
inline void TacticalMapCall(const char*, A&&...) {}

template<class T, class... A>
inline T TacticalMapValue(const char*, A&&...) { return T{}; }

} // namespace fates::decomp_detail
