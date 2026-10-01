#include "fates/event/backend/backend_spine.hpp"

namespace fates::event::backend {

// Castle backend ownership. Complex object layouts remain behind BackendRuntime;
// exact retail identity/ranges are retained in the registry and evidence.

BackendWord Castle__Get(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0044544Cu, args, argc);
}

BackendWord castle__Accessory__AddAccessory(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00499AE8u, args, argc);
}

BackendWord castle__CastleWorld__Initialize(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0044F5CCu, args, argc);
}

BackendWord castle__CastleWorld__Finalize(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0044FD24u, args, argc);
}

BackendWord castle__SpotSelectSequence__MoveBind(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0045CE0Cu, args, argc);
}

BackendWord castle__AmiiboSequence__UpdateBind(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x004542C8u, args, argc);
}

} // namespace fates::event::backend
