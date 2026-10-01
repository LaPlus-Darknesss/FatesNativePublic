#include "fates/coreimpl/core_service_impl.hpp"

namespace fates::coreimpl {

// MovieCore: exact retail identity is owned; unresolved concrete object layout remains behind ImplRuntime.

ImplWord MovieData__Initialize(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x00502694u, args, argc);
}

ImplWord MovieData__Finalize(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x005026D8u, args, argc);
}

ImplWord is__app__movie__MovieThread__MovieLeave(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x002247A8u, args, argc);
}

ImplWord is__app__movie__MovieThread__MovieThreadInitialize(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x00225164u, args, argc);
}

} // namespace fates::coreimpl
