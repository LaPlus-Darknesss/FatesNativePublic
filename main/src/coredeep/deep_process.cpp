#include "fates/coredeep/deeper_core_impl.hpp"

namespace fates::coredeep {

// ProcessDeep: exact retail identity owned; unresolved concrete layout/backend state remains behind DeepRuntime.

DeepWord ProcInst__FindNext(DeepRuntime& runtime, const DeepWord* args, std::size_t argc) {
    return InvokeDeep(runtime, 0x0011D318u, args, argc);
}

DeepWord ProcThread__ProcThread(DeepRuntime& runtime, const DeepWord* args, std::size_t argc) {
    return InvokeDeep(runtime, 0x0018D91Cu, args, argc);
}

} // namespace fates::coredeep
