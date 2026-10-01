#include "fates/coredeep/deeper_core_impl.hpp"

namespace fates::coredeep {

// UtilityDeep: exact retail identity owned; unresolved concrete layout/backend state remains behind DeepRuntime.

DeepWord util__Curve__Accelfloat(DeepRuntime& runtime, const DeepWord* args, std::size_t argc) {
    return InvokeDeep(runtime, 0x0011EF20u, args, argc);
}

DeepWord util__Curve__Decelfloat(DeepRuntime& runtime, const DeepWord* args, std::size_t argc) {
    return InvokeDeep(runtime, 0x0011EF74u, args, argc);
}

DeepWord FieldSound__StartReverb(DeepRuntime& runtime, const DeepWord* args, std::size_t argc) {
    return InvokeDeep(runtime, 0x00176DB4u, args, argc);
}

} // namespace fates::coredeep
