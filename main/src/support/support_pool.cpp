#include "fates/progression/progression_support.hpp"

namespace fates::progression {

// These source-facing functions preserve retail responsibility boundaries while unresolved object layouts remain behind ProgressionRuntime.
ProgressionWord SupportNode__SupportNode(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x001A5FC4u, args, argc); }
ProgressionWord SupportPool__Deserialize(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x001A6008u, args, argc); }
ProgressionWord SupportPool__Entry(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x001A61D0u, args, argc); }
ProgressionWord SupportPool__GetEmpty(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x001A62B0u, args, argc); }
ProgressionWord SupportNode__GetName(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x00508DB8u, args, argc); }
ProgressionWord SupportPool__GetLockUnitNum(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x00508DF8u, args, argc); }
ProgressionWord SupportPool__Dump(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x00508E28u, args, argc); }
ProgressionWord SupportPool__Search(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x00508E44u, args, argc); }
ProgressionWord SupportPool__Search_2(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x00508ED4u, args, argc); }
ProgressionWord SupportPool__IsLocked(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x00508F64u, args, argc); }
ProgressionWord SupportPool__Serialize(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x00509004u, args, argc); }

} // namespace fates::progression
