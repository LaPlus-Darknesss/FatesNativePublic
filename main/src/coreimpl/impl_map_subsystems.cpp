#include "fates/coreimpl/core_service_impl.hpp"

namespace fates::coreimpl {

// MapSubsystemCore: exact retail identity is owned; unresolved concrete object layout remains behind ImplRuntime.

ImplWord map__SortiePosition__Initialize(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x00361378u, args, argc);
}

ImplWord map__SortiePosition__Finalize(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x00361740u, args, argc);
}

ImplWord map__Panel__Initialize(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x0038FAA8u, args, argc);
}

ImplWord map__Panel__Finalize(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x00394848u, args, argc);
}

ImplWord map__sound__Initialize(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x003974D8u, args, argc);
}

ImplWord map__sound__Setup(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x003998C4u, args, argc);
}

ImplWord map__sound__Finalize(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x00399A80u, args, argc);
}

ImplWord map__Binder__Initialize(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x0039B600u, args, argc);
}

ImplWord map__Binder__Finalize(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x0039B678u, args, argc);
}

ImplWord map__Sender__Initialize(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x003A0210u, args, argc);
}

ImplWord map__Sender__Finalize(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x003A0248u, args, argc);
}

ImplWord map__Gradation__Initialize(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x003A58B0u, args, argc);
}

ImplWord map__Gradation__Finalize(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x003A59E4u, args, argc);
}

} // namespace fates::coreimpl
