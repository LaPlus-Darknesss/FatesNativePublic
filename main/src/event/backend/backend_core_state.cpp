#include "fates/event/backend/backend_spine.hpp"

namespace fates::event::backend {

// CoreState backend ownership. Complex object layouts remain behind BackendRuntime;
// exact retail identity/ranges are retained in the registry and evidence.

BackendWord GameUserData__Get(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x001B3AFCu, args, argc);
}

BackendWord GameSkip__IsBlackOut(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x004EB4D8u, args, argc);
}

BackendWord GameSkip__IsSkip(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x004EB830u, args, argc);
}

BackendWord VariableManager__Set(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x001E800Cu, args, argc);
}

BackendWord ChapterSequence__GetInstance(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x001DA0D8u, args, argc);
}

BackendWord FlagNameManager__EntryGlobal(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x001E086Cu, args, argc);
}

BackendWord FlagNameManager__Entry(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x001E08ACu, args, argc);
}

BackendWord FlagManager__Set(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00199D50u, args, argc);
}

BackendWord ChapterSequence__GetStatus(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x001DBE1Cu, args, argc);
}

BackendWord VariableManager__Set_2(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x001E8074u, args, argc);
}

BackendWord VariableManager__Add(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x001E7F74u, args, argc);
}

BackendWord Random__GetValue_2(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0044AE14u, args, argc);
}

BackendWord GameLinkData__Dump(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0050BB84u, args, argc);
}

BackendWord GameConfigData__IsSpeedFast(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0050AA68u, args, argc);
}

BackendWord FlagManager__Clr(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00199CD4u, args, argc);
}

BackendWord GameSkip__Trigger(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x004EB9FCu, args, argc);
}

BackendWord GameSkipSequenceHelper__EscapeSkip(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0021DCC0u, args, argc);
}

BackendWord GameSkipSequenceHelper__EnableSkip(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0021DC9Cu, args, argc);
}

BackendWord GameSkipSequenceHelper__DisableSkip(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0021DCF8u, args, argc);
}

BackendWord GameSkip__IsDisable(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x004EBAD0u, args, argc);
}

BackendWord VariableManager__Get(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0050BCCCu, args, argc);
}

BackendWord ContentsUtil__IsOwnedRoute(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x001AC9B8u, args, argc);
}

BackendWord Random__Initialize(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0044ABC4u, args, argc);
}

BackendWord FlagManagerNoName__Set(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00202BDCu, args, argc);
}

BackendWord FlagManagerNoName__SetAll(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0012A0C0u, args, argc);
}

BackendWord GameLinkGlobalData__Dump(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0050BB18u, args, argc);
}

BackendWord DeliveryMessage__SetMessage(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x001DE188u, args, argc);
}

BackendWord DeliveryMessage__Reset(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x001DE1E4u, args, argc);
}

BackendWord ContentsLocal__GetElapse(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0050A314u, args, argc);
}

BackendWord ContentsLocal__UpdateTime(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x001C7124u, args, argc);
}

BackendWord ContentsLocal__Dump(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0050A300u, args, argc);
}

} // namespace fates::event::backend
