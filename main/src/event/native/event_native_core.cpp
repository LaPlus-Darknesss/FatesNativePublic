#include "fates/event/native_commands.hpp"

namespace fates::event::native {
// Retail semantics represented here include event time conversion/speed,
// named flags/variables, separate game/system RNG streams, skip state,
// content/difficulty/turn queries and other global event state.  State storage
// remains behind NativeCommandRuntime unless its owning subsystem is already
// source-owned; exact retail signatures are preserved in the registry.

#define FATES_EVENT_NATIVE_WRAPPER(Name) \
NativeWord Name(NativeCommandRuntime& runtime, cmvm::CmContext* context, const NativeWord* args, std::size_t argc) { \
    return InvokeNative(runtime, "ev::" #Name, context, args, argc); \
}

FATES_EVENT_NATIVE_WRAPPER(Warning)
FATES_EVENT_NATIVE_WRAPPER(ArgsGetInt)
FATES_EVENT_NATIVE_WRAPPER(ArgsGetString)
FATES_EVENT_NATIVE_WRAPPER(TimeGetSpeed)
FATES_EVENT_NATIVE_WRAPPER(TimeGetConfigSpeed)
FATES_EVENT_NATIVE_WRAPPER(TimeMSecToFrame)
FATES_EVENT_NATIVE_WRAPPER(TimeFrameToMSec)
FATES_EVENT_NATIVE_WRAPPER(FlagEntryGlobal)
FATES_EVENT_NATIVE_WRAPPER(FlagEntry)
FATES_EVENT_NATIVE_WRAPPER(FlagGet)
FATES_EVENT_NATIVE_WRAPPER(FlagSet)
FATES_EVENT_NATIVE_WRAPPER(FlagClr)
FATES_EVENT_NATIVE_WRAPPER(VariableEntryGlobal)
FATES_EVENT_NATIVE_WRAPPER(VariableEntry)
FATES_EVENT_NATIVE_WRAPPER(VariableGet)
FATES_EVENT_NATIVE_WRAPPER(VariableSet)
FATES_EVENT_NATIVE_WRAPPER(VariableAdd)
FATES_EVENT_NATIVE_WRAPPER(VariableInc)
FATES_EVENT_NATIVE_WRAPPER(VariableDec)
FATES_EVENT_NATIVE_WRAPPER(RandomGetGame)
FATES_EVENT_NATIVE_WRAPPER(RandomGetSystem)
FATES_EVENT_NATIVE_WRAPPER(SkipTrigger)
FATES_EVENT_NATIVE_WRAPPER(SkipEscape)
FATES_EVENT_NATIVE_WRAPPER(SkipEnable)
FATES_EVENT_NATIVE_WRAPPER(SkipDisable)
FATES_EVENT_NATIVE_WRAPPER(SkipIsActive)
FATES_EVENT_NATIVE_WRAPPER(SkipIsDisable)
FATES_EVENT_NATIVE_WRAPPER(ContentsGetPackage)
FATES_EVENT_NATIVE_WRAPPER(DifficultyGet)
FATES_EVENT_NATIVE_WRAPPER(DifficultyIsCasual)
FATES_EVENT_NATIVE_WRAPPER(DifficultyIsPhoenix)
FATES_EVENT_NATIVE_WRAPPER(ScenarioRankGet)
FATES_EVENT_NATIVE_WRAPPER(TurnGet)
FATES_EVENT_NATIVE_WRAPPER(EnableBGMCommandPG)
FATES_EVENT_NATIVE_WRAPPER(DisableBGMCommandPG)
FATES_EVENT_NATIVE_WRAPPER(ContentsGetFlag)
FATES_EVENT_NATIVE_WRAPPER(ContentsSetFlag)
FATES_EVENT_NATIVE_WRAPPER(ContentsClrFlag)
FATES_EVENT_NATIVE_WRAPPER(ContentsGetIndex)
FATES_EVENT_NATIVE_WRAPPER(ContentsGetElapse)
FATES_EVENT_NATIVE_WRAPPER(ContentsUpdateTime)

#undef FATES_EVENT_NATIVE_WRAPPER
} // namespace fates::event::native
