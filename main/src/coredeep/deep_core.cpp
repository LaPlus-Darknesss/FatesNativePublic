#include "fates/coredeep/deeper_core_impl.hpp"

namespace fates::coredeep {

// CoreDeep: exact retail identity owned; unresolved concrete layout/backend state remains behind DeepRuntime.

DeepWord MapDataFile__TryRead(DeepRuntime& runtime, const DeepWord* args, std::size_t argc) {
    return InvokeDeep(runtime, 0x0019EC9Cu, args, argc);
}

DeepWord FieldWeatherImpl__Draw(DeepRuntime& runtime, const DeepWord* args, std::size_t argc) {
    return InvokeDeep(runtime, 0x001EA8F0u, args, argc);
}

DeepWord castle__JukeBox__GetBgmLabel(DeepRuntime& runtime, const DeepWord* args, std::size_t argc) {
    return InvokeDeep(runtime, 0x00494620u, args, argc);
}

DeepWord MapDataFile__GetPartsList(DeepRuntime& runtime, const DeepWord* args, std::size_t argc) {
    return InvokeDeep(runtime, 0x00508740u, args, argc);
}

} // namespace fates::coredeep
