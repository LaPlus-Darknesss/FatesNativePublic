#include "fates/event/backend/backend_spine.hpp"

namespace fates::event::backend {

// Infrastructure backend ownership. Complex object layouts remain behind BackendRuntime;
// exact retail identity/ranges are retained in the registry and evidence.

BackendWord GameHandle__GameHandle_2(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0017BD60u, args, argc);
}

BackendWord Proc__ResDelayBind(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x003D2940u, args, argc);
}

BackendWord GameHandle__GameHandle(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0017BD20u, args, argc);
}

} // namespace fates::event::backend
