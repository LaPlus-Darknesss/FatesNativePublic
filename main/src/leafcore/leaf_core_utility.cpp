#include "fates/leafcore/core_leaf_impl.hpp"

namespace fates::leafcore {

// CoreUtilityLeaf: exact retail identity owned; unresolved layout/backend state remains behind LeafRuntime.

LeafWord Decimalize(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x0016448Cu, args, argc);
}

} // namespace fates::leafcore
