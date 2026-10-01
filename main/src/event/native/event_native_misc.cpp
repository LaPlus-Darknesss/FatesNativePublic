#include "fates/event/native_commands.hpp"

namespace fates::event::native {
// Remaining registered commands that do not belong cleanly to a larger
// family.  They remain explicit named entry points rather than disappearing
// into a generic catch-all dispatcher.

#define FATES_EVENT_NATIVE_WRAPPER(Name) \
NativeWord Name(NativeCommandRuntime& runtime, cmvm::CmContext* context, const NativeWord* args, std::size_t argc) { \
    return InvokeNative(runtime, "ev::" #Name, context, args, argc); \
}

FATES_EVENT_NATIVE_WRAPPER(BMapBGMChnageFlagOn)
FATES_EVENT_NATIVE_WRAPPER(BMapBGMResume)
FATES_EVENT_NATIVE_WRAPPER(LSEPlay)
FATES_EVENT_NATIVE_WRAPPER(LSEStop)
FATES_EVENT_NATIVE_WRAPPER(LSEVolume)

#undef FATES_EVENT_NATIVE_WRAPPER
} // namespace fates::event::native
