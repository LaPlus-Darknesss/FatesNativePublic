#include "fates/coreimpl/core_service_impl.hpp"

namespace fates::coreimpl {

// PresentationUiCore: exact retail identity is owned; unresolved concrete object layout remains behind ImplRuntime.

ImplWord Font__SetCurrent(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x003CFA10u, args, argc);
}

ImplWord anonymous_namespace__FadeCreate(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x0033A2C8u, args, argc);
}

ImplWord Darkness__Stop(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004E5C44u, args, argc);
}

ImplWord GameTime__ResetFrameStep(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004EBB38u, args, argc);
}

ImplWord Font__GetCurrent(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x003CF9A4u, args, argc);
}

ImplWord GameFont__Draw(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004E71F4u, args, argc);
}

ImplWord HomeButton__HomeUnbind(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x0017CA80u, args, argc);
}

ImplWord HomeButton__HomeBind(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x0017CD38u, args, argc);
}

ImplWord Font__PopFont(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x003CFEBCu, args, argc);
}

ImplWord Font__GetLines(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x003CFF00u, args, argc);
}

ImplWord Font__GetWidth(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x003CFF30u, args, argc);
}

ImplWord Font__PushFont_2(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x003D0018u, args, argc);
}

ImplWord Icon__Anime__DrawButtonA(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x003D0568u, args, argc);
}

ImplWord Icon__Anime__DrawWait(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x003D0868u, args, argc);
}

ImplWord game__graphics__Window__DrawDialog(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x003E97B0u, args, argc);
}

ImplWord Darkness__Start(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004E5CB4u, args, argc);
}

ImplWord GameFont__GetWidth(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004E7470u, args, argc);
}

ImplWord GameFont__GetHeight(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004E76D4u, args, argc);
}

ImplWord GameTime__SetFrameSlow(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004EBB20u, args, argc);
}

} // namespace fates::coreimpl
