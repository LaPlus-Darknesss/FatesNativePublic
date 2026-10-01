#include "fates/progression/progression_support.hpp"

namespace fates::progression {

// These source-facing functions preserve retail responsibility boundaries while unresolved object layouts remain behind ProgressionRuntime.
ProgressionWord GameProfile__UpdateReliance(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x0019B1D8u, args, argc); }
ProgressionWord RelianceObj__RelianceObj(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x001A3118u, args, argc); }
ProgressionWord Live2DDefine__GetSupportPoint(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x001B59A8u, args, argc); }
ProgressionWord RelianceTalkSequence__Create(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x0021904Cu, args, argc); }
ProgressionWord RelianceTalkSequence__CanTalk(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x00219218u, args, argc); }
ProgressionWord PersonEnumerator_Reliance__IsExclusion(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x002220B8u, args, argc); }
ProgressionWord PersonEnumerator_Reliance__RelianceObjVectorList__RelianceObjVectorList(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x00222978u, args, argc); }
ProgressionWord PersonEnumerator_Reliance__PersonEnumerator_Reliance(ProgressionRuntime& runtime, const ProgressionWord* args, std::size_t argc) { return InvokeProgression(runtime, 0x002229D0u, args, argc); }

} // namespace fates::progression
