#include "fates/coredeep/deeper_core_impl.hpp"

namespace fates::coredeep {

// ContentStateDeep: exact retail identity owned; unresolved concrete layout/backend state remains behind DeepRuntime.

DeepWord GP__GetFloat(DeepRuntime& runtime, const DeepWord* args, std::size_t argc) {
    return InvokeDeep(runtime, 0x00223E70u, args, argc);
}

DeepWord ContentsUtil__GetContentID(DeepRuntime& runtime, const DeepWord* args, std::size_t argc) {
    return InvokeDeep(runtime, 0x001AC940u, args, argc);
}

DeepWord ContentsUtil__Mount(DeepRuntime& runtime, const DeepWord* args, std::size_t argc) {
    return InvokeDeep(runtime, 0x001ACE3Cu, args, argc);
}

} // namespace fates::coredeep
