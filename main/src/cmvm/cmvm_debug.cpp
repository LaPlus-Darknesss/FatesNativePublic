#include "fates/cmvm/cmvm.hpp"
#include <array>
namespace fates::cmvm {
namespace {
struct DebugVar { const char* name{}; const std::int32_t* value{}; };
[[maybe_unused]] void clear_debug_vars(std::array<DebugVar,16>& vars) { for (auto& v:vars) v={}; }
}
// Retail debug metadata is real VM-owned state, not gameplay policy:
// CmDebugVar zero-initializes name/value; CmDebugVars owns 16 slots;
// CmDebugSource::NewLine keeps a bounded rolling source-line history.
}
