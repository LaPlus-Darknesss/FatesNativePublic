#include "fates/leafcore/core_leaf_impl.hpp"

namespace fates::leafcore {

// AudioPlaybackLeaf: exact retail identity owned; unresolved layout/backend state remains behind LeafRuntime.

LeafWord SoundHandle__PlayCore(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x001A5964u, args, argc);
}

LeafWord MultiSound__Play(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x0017D7FCu, args, argc);
}

LeafWord RandomSound__Play(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x001A2F94u, args, argc);
}

LeafWord ParameterSound__Play(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x0050B250u, args, argc);
}

} // namespace fates::leafcore
