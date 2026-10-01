#include "fates/event/backend/backend_spine.hpp"

namespace fates::event::backend {

// ProcessUi backend ownership. Complex object layouts remain behind BackendRuntime;
// exact retail identity/ranges are retained in the registry and evidence.

BackendWord anonymous_namespace__EventDialogItem__EventDialogItem(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x003A9E50u, args, argc);
}

BackendWord ProcInst__Create(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0010A1F4u, args, argc);
}

BackendWord BasicMenu__Create(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x004FA880u, args, argc);
}

BackendWord BasicMenu__SetText(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x004FAFACu, args, argc);
}

BackendWord anonymous_namespace__EventDialog__EventDialog(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x003A79ECu, args, argc);
}

BackendWord GameMessage__CreateBind(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0019A4B4u, args, argc);
}

BackendWord GameMessage__SetGameSkipDisable(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0019A9C0u, args, argc);
}

BackendWord ProcTalkManager__CreateInstanceBind(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x001E56A0u, args, argc);
}

BackendWord ProcTalkManager__Initialize(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x001E4830u, args, argc);
}

BackendWord ProcInst__ProcInst(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0010A2A8u, args, argc);
}

BackendWord GameFont__AddItem(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x004E72F0u, args, argc);
}

BackendWord GameDialog__GameDialog(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0017A500u, args, argc);
}

BackendWord BasicMenu__SetSelectFromIndexAsPossible(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x004F96F8u, args, argc);
}

BackendWord ProcInst__Create_2(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x004EE0BCu, args, argc);
}

BackendWord ProcInst__Delete(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0011D300u, args, argc);
}

BackendWord map__NoticeWindow__Create(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0034D08Cu, args, argc);
}

BackendWord map__Intermediate__Tutorial__Create(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0034CC64u, args, argc);
}

} // namespace fates::event::backend
