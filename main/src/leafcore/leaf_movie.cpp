#include "fates/leafcore/core_leaf_impl.hpp"

namespace fates::leafcore {

// MovieLeaf: exact retail identity owned; unresolved layout/backend state remains behind LeafRuntime.

LeafWord MovieViewer__GetProc(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x0019FD74u, args, argc);
}

LeafWord MovieSubtitle__Draw(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x001C9594u, args, argc);
}

LeafWord MovieSubtitle__FadeIn(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x001C9884u, args, argc);
}

LeafWord MovieSubtitle__FadeOut(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x001C9898u, args, argc);
}

LeafWord MovieSubtitle__SetMessId(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x001C98ACu, args, argc);
}

LeafWord MovieSubtitle__Tick(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x0041BC3Cu, args, argc);
}

} // namespace fates::leafcore
