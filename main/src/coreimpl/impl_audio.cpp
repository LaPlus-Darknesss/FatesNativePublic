#include "fates/coreimpl/core_service_impl.hpp"

namespace fates::coreimpl {

// AudioCore: exact retail identity is owned; unresolved concrete object layout remains behind ImplRuntime.

ImplWord SoundPrivate__GetSoundPlayer(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x001BF138u, args, argc);
}

ImplWord SoundPrivate__GetFreeHandleSE(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x001BF20Cu, args, argc);
}

ImplWord MultiSound__GetSoundItem(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x0017D7F0u, args, argc);
}

ImplWord SoundPrivate__Initialize(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x001BEF8Cu, args, argc);
}

ImplWord SoundPrivate__GetHandleLoopSE(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x001BF270u, args, argc);
}

ImplWord SoundPrivate__GetPreparedHandleSE(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x001BF334u, args, argc);
}

ImplWord DLCSound__AddArchive(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004E4FD8u, args, argc);
}

ImplWord DLCSound__RemoveArchive(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004E52B8u, args, argc);
}

ImplWord SoundActor3D__StopAll(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x001BEF30u, args, argc);
}

ImplWord SoundPrivate__GetHandleSE(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x001BF0D8u, args, argc);
}

ImplWord SoundPrivate__LoadGroupAsync(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x001BF168u, args, argc);
}

ImplWord SoundPrivate__GetFreeHandleME(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x001BF1F4u, args, argc);
}

ImplWord SoundPrivate__StopAll(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x001BF39Cu, args, argc);
}

ImplWord SoundPrivate__Construct(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x001BF5F8u, args, argc);
}

ImplWord SoundPrivate__LoadGroup(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x001BF90Cu, args, argc);
}

ImplWord SoundPrivate__SoundPrivate_3(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x001BFC08u, args, argc);
}

ImplWord IndirectSound__Initialize(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x001C88B4u, args, argc);
}

ImplWord IndirectSound__Finalize(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x001C8950u, args, argc);
}

ImplWord ProcDelaySound__DeleteAll(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x001D4840u, args, argc);
}

ImplWord SoundHandleBGM__PlayPosition(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x001D5210u, args, argc);
}

ImplWord DLCSound__StopAllVoice(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004E521Cu, args, argc);
}

ImplWord DLCSound__SetBiquadFilter(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004E5438u, args, argc);
}

ImplWord DLCSound__SetPlayerVolumeSe(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004E54C8u, args, argc);
}

ImplWord DLCSound__SetPlayerVolumeBGM(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004E5554u, args, argc);
}

ImplWord DLCSound__UpdateSoundSetting(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004E55E0u, args, argc);
}

ImplWord DLCSound__SetPlayerVolumeVoice(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004E5838u, args, argc);
}

ImplWord DLCSound__GetData(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004E59E0u, args, argc);
}

ImplWord DLCSound__IsVoicing(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004E5AD4u, args, argc);
}

ImplWord DLCSound__StopAllSe(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004E5BA8u, args, argc);
}

ImplWord SoundHandle__IsEqual(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x00508B30u, args, argc);
}

} // namespace fates::coreimpl
