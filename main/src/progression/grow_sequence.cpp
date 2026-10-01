#include "fates/progression/progression_support.hpp"

namespace fates::progression {

// These source-facing functions preserve retail responsibility boundaries while unresolved object layouts remain behind ProgressionRuntime.
ProgressionWord GrowSequence__BranchSkill(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x001B3D84u, args, argc); }
ProgressionWord GrowSequence__ClassChange(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x001B3DD8u, args, argc); }
ProgressionWord GrowSequence__DualLevelUp(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x001B3DF0u, args, argc); }
ProgressionWord GrowSequence__ForgetSkill(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x001B3E1Cu, args, argc); }
ProgressionWord GrowSequence__LevelUpShow(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x001B3F74u, args, argc); }
ProgressionWord GrowSequence__DualGainSkill(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x001B3F94u, args, argc); }
ProgressionWord GrowSequence__GainWeaponExp(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x001B40C8u, args, argc); }
ProgressionWord GrowSequence__WeaponLevelUp(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x001B40ECu, args, argc); }
ProgressionWord GrowSequence__ClassChangeShow(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x001B42D0u, args, argc); }
ProgressionWord GrowSequence__DualForgetSkill(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x001B42F0u, args, argc); }
ProgressionWord GrowSequence__DualLevelUpShow(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x001B4454u, args, argc); }
ProgressionWord GrowSequence__DualGainWeaponExp(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x001B4474u, args, argc); }
ProgressionWord GrowSequence__ClassChangeReflect(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x001B4610u, args, argc); }
ProgressionWord GrowSequence__DualLevelUpReflect(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x001B46A8u, args, argc); }
ProgressionWord GrowSequence__ClassChangeCalculate(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x001B46E4u, args, argc); }
ProgressionWord GrowSequence__DualLevelUpCalculate(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x001B4768u, args, argc); }
ProgressionWord GrowSequence__Create(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x001B47C0u, args, argc); }
ProgressionWord GrowSequence__GainExp(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x001B4888u, args, argc); }
ProgressionWord GrowSequence__LevelUp(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x001B48C4u, args, argc); }
ProgressionWord GrowSequence__Calculate(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x001B48E8u, args, argc); }
ProgressionWord GrowSequence__GainSkill(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x001B4A08u, args, argc); }
ProgressionWord GrowSequence__GrowSequence(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x001B4B9Cu, args, argc); }
ProgressionWord GrowSequence__ResumeGameInfo(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x001BBB04u, args, argc); }
ProgressionWord GrowSequence__LevelUpReflect(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x003D4FD0u, args, argc); }
ProgressionWord GrowSequence__LevelUpCalculate(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x003D80FCu, args, argc); }

} // namespace fates::progression
