#include "fates/leafcore/core_leaf_impl.hpp"

namespace fates::leafcore {

// ContentIoLeaf: exact retail identity owned; unresolved layout/backend state remains behind LeafRuntime.

LeafWord CreateGlobalFile(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x001660BCu, args, argc);
}

LeafWord ContentsReader__MountContent(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x001CD04Cu, args, argc);
}

LeafWord ContentsReader__Load(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x001CD22Cu, args, argc);
}

LeafWord ContentsManager__LoadGlobal(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x001DD950u, args, argc);
}

LeafWord ContentsManager__LoadLocal(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x001DDDC4u, args, argc);
}

LeafWord castle__Food__GetData(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x0048BE3Cu, args, argc);
}

LeafWord castle__Gemstone__GetData(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x00497534u, args, argc);
}

LeafWord MapDataFile__GetFileList(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x00508724u, args, argc);
}

LeafWord MapDataFile__GetReferList(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x0050875Cu, args, argc);
}

LeafWord ContentsReader__GetListIndex(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x0050A6CCu, args, argc);
}

LeafWord castle__Food__Data__GetName(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x00541650u, args, argc);
}

LeafWord castle__Gemstone__Data__GetName(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x00542C44u, args, argc);
}

} // namespace fates::leafcore
