#include "fates/event/native_commands.hpp"

namespace fates::event::native {
// Script-visible inventory, rewards, accessories, guest/card/social and
// content-facing commands.  Original table IDs/localized names may be resolved
// through the project's E2 Paragon/original-data authority where applicable.

#define FATES_EVENT_NATIVE_WRAPPER(Name) \
NativeWord Name(NativeCommandRuntime& runtime, cmvm::CmContext* context, const NativeWord* args, std::size_t argc) { \
    return InvokeNative(runtime, "ev::" #Name, context, args, argc); \
}

FATES_EVENT_NATIVE_WRAPPER(ItemGain)
FATES_EVENT_NATIVE_WRAPPER(ItemGainNoSound)
FATES_EVENT_NATIVE_WRAPPER(ItemGainSilent)
FATES_EVENT_NATIVE_WRAPPER(ItemReplaceForYatonokami)
FATES_EVENT_NATIVE_WRAPPER(ItemGainMessageOnly)
FATES_EVENT_NATIVE_WRAPPER(ItemGainMessageOnlyNoSound)
FATES_EVENT_NATIVE_WRAPPER(ItemGetKind)
FATES_EVENT_NATIVE_WRAPPER(ItemGetOrigin)
FATES_EVENT_NATIVE_WRAPPER(ItemGetRandom)
FATES_EVENT_NATIVE_WRAPPER(GoldGain)
FATES_EVENT_NATIVE_WRAPPER(GoldGain2)
FATES_EVENT_NATIVE_WRAPPER(GoldGainSilent)
FATES_EVENT_NATIVE_WRAPPER(GoldGet)
FATES_EVENT_NATIVE_WRAPPER(AccessoryGain)
FATES_EVENT_NATIVE_WRAPPER(AccessoryGainSilent)
FATES_EVENT_NATIVE_WRAPPER(JoinGuest)
FATES_EVENT_NATIVE_WRAPPER(CardEntry)
FATES_EVENT_NATIVE_WRAPPER(CardIsExist)
FATES_EVENT_NATIVE_WRAPPER(AnnaMessageSet)
FATES_EVENT_NATIVE_WRAPPER(AnnaMessageClr)
FATES_EVENT_NATIVE_WRAPPER(ComebackCastle)
FATES_EVENT_NATIVE_WRAPPER(AmiiboUpdate)

#undef FATES_EVENT_NATIVE_WRAPPER
} // namespace fates::event::native
