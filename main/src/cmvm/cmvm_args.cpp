#include "fates/cmvm/cmvm.hpp"
namespace fates::cmvm {
const char* function_event_arg_string(const CmArchiveView& archive, const std::uint32_t* offsets, std::size_t index) {
    if (!archive.base || !offsets || offsets[index] >= archive.size) return nullptr;
    return reinterpret_cast<const char*>(archive.base + offsets[index]);
}
// Retail CmFunction::EvArgAsStr and CmEvArg::AsStr are archive-string-base + stored offset.
// CmContext::GetStringArg selects function event-arg storage for function types >3,
// otherwise the current VM stack argument. GetIntArg uses the parallel integer path.
}
