#include "fates/progression/progression_support_dependencies.hpp"

namespace fates::progression {

// Retail ownership is preserved here while unresolved object layouts stay behind DependencyRuntime.
DependencyWord game__packet__Unit__Deserialize(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x003DE0CCu, args, argc); }
DependencyWord game__packet__Unit__Copy(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x003DE458u, args, argc); }
DependencyWord game__packet__Unit__Clear(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x003DE46Cu, args, argc); }
DependencyWord unit__Edit__Clear(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x00419644u, args, argc); }
DependencyWord unit__Cloth__operator(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x0041ACB4u, args, argc); }
DependencyWord unit__Record__operator(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x0041B1A0u, args, argc); }
DependencyWord unit__Enhance__operator(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x0041B660u, args, argc); }
DependencyWord game__packet__Unit__Serialize(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x0053592Cu, args, argc); }
DependencyWord unit__Identifier__IsEqual(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x0053A960u, args, argc); }

} // namespace fates::progression
