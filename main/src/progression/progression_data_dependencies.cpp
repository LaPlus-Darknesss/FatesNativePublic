#include "fates/progression/progression_support_dependencies.hpp"

namespace fates::progression {

// Retail ownership is preserved here while unresolved object layouts stay behind DependencyRuntime.
DependencyWord ClassChange__GetType(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x001951B0u, args, argc); }
DependencyWord ExpSequence__Create(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x001961F0u, args, argc); }
DependencyWord ClassChangeEnumerator__GetLearnEquipSkill(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x0021AC8Cu, args, argc); }
DependencyWord Random__GetValue(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x0044ADF8u, args, argc); }

} // namespace fates::progression
