#include "fates/coreimpl/core_service_impl.hpp"

namespace fates::coreimpl {

// ProcessCore: exact retail identity is owned; unresolved concrete object layout remains behind ImplRuntime.

ImplWord ProcInst__ProcInst_3(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004EE318u, args, argc);
}

ImplWord Proc__KillByName(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x0010FD20u, args, argc);
}

ImplWord Proc__FindByDesc(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x003D277Cu, args, argc);
}

ImplWord ProcInst__Jump(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004EDFCCu, args, argc);
}

ImplWord ProcInst__Next(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004EE098u, args, argc);
}

} // namespace fates::coreimpl
