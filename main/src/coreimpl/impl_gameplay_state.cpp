#include "fates/coreimpl/core_service_impl.hpp"

namespace fates::coreimpl {

// GameplayStateCore: exact retail identity is owned; unresolved concrete object layout remains behind ImplRuntime.

ImplWord versus__Config__Get(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004C5344u, args, argc);
}

ImplWord CastleDefendSettingSequence__GetInstance(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x00222BDCu, args, argc);
}

ImplWord GP__GetInt(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x00223E40u, args, argc);
}

ImplWord ContentsUtil__IsNeedMount(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x001AC8A0u, args, argc);
}

ImplWord GameUserGlobalData__IsUsedJapaneseVoice(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x0020C5E0u, args, argc);
}

} // namespace fates::coreimpl
