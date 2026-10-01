#include "fates/progression/progression_support.hpp"

namespace fates::progression {

// These source-facing functions preserve retail responsibility boundaries while unresolved object layouts remain behind ProgressionRuntime.
ProgressionWord map__DualSupportCalculator__Clear(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x00370A8Cu, args, argc); }
ProgressionWord map__DualSupportCalculator__Calculate(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x00370AB8u, args, argc); }

} // namespace fates::progression
