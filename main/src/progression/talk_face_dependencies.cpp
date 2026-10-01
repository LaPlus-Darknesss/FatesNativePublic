#include "fates/progression/progression_support_dependencies.hpp"

namespace fates::progression {

// Retail ownership is preserved here while unresolved object layouts stay behind DependencyRuntime.
DependencyWord FaceInstance__LoadByUnit(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x001AE730u, args, argc); }
DependencyWord FaceInstance__SetDirection(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x001AEC00u, args, argc); }
DependencyWord FaceInstance__ChangeExpression(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x001AFA5Cu, args, argc); }
DependencyWord FaceInstance__FaceInstance(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x001B1A54u, args, argc); }
DependencyWord FrameManager__Layout__Show(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x001B2924u, args, argc); }
DependencyWord FrameManager__Layout__SetTitle_2(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x001B2CC8u, args, argc); }
DependencyWord FrameManager__Layout__AddButton(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x001B2D74u, args, argc); }
DependencyWord ProcTalkManager__InitializeDirect(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x001E5474u, args, argc); }
DependencyWord TalkCodeFactory__WindowActive(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x001E72B0u, args, argc); }
DependencyWord TalkCodeFactory__WindowDelete(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x001E7328u, args, argc); }
DependencyWord TalkCodeFactory__WindowMakeGrow(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x001E73A0u, args, argc); }
DependencyWord TalkCodeFactory__AddMess(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x001E753Cu, args, argc); }
DependencyWord TalkCodeFactory__TalkType(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x001E75D4u, args, argc); }
DependencyWord TalkCodeFactory__AddDirect(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x001E7620u, args, argc); }
DependencyWord TalkCodeFactory__TalkCodeFactory(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x001E7798u, args, argc); }

} // namespace fates::progression
