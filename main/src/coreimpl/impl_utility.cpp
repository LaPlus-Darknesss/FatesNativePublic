#include "fates/coreimpl/core_service_impl.hpp"

namespace fates::coreimpl {

// CoreUtility: exact retail identity is owned; unresolved concrete object layout remains behind ImplRuntime.

ImplWord map__Intermediate__Tutorial__IsActive(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x0034CC9Cu, args, argc);
}

ImplWord FieldSound__StopReverb(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x00176D9Cu, args, argc);
}

ImplWord IActor__FreeAll(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004458B0u, args, argc);
}

ImplWord Color8__operator_3(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x0053D58Cu, args, argc);
}

} // namespace fates::coreimpl
