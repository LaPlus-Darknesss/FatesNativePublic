#include "fates/progression/progression_support_dependencies.hpp"

namespace fates::progression {

// Retail ownership is preserved here while unresolved object layouts stay behind DependencyRuntime.
DependencyWord PersonEnumerator__PersonEnumerator_2(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x001EE304u, args, argc); }
DependencyWord GameUserGlobalData__Get(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x0020C60Cu, args, argc); }
DependencyWord GameUserGlobalData__Viewer__GetRelianceFlagIndexPlayerAqua(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x0020C6C8u, args, argc); }
DependencyWord VariableSizeFlagManager__Get(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x00220F60u, args, argc); }
DependencyWord anonymous_namespace__IsRelianceTalk(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x003CDA04u, args, argc); }
DependencyWord FlagManagerNoName__Get(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x0050C9A4u, args, argc); }

} // namespace fates::progression
