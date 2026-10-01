#include "fates/event/backend/backend_spine.hpp"

namespace fates::event::backend {

// Presentation backend ownership. Complex object layouts remain behind BackendRuntime;
// exact retail identity/ranges are retained in the registry and evidence.

BackendWord Sound__IsEnableBGMCommand(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0041FB28u, args, argc);
}

BackendWord Sound__EnableBGMCommand(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0041FAB0u, args, argc);
}

BackendWord Sound__SetEnableBGMCommand(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0041FB60u, args, argc);
}

BackendWord Fade__IsActive(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x003CF6ACu, args, argc);
}

BackendWord Fade__FadeIn(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0033A2C0u, args, argc);
}

BackendWord Fade__FadeOut(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x003CF60Cu, args, argc);
}

BackendWord map__sound__Se__DangerOn(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00398268u, args, argc);
}

BackendWord map__sound__Se__DangerOff(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x003982C0u, args, argc);
}

BackendWord map__sound__Se__ItemGain(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00398284u, args, argc);
}

BackendWord Movie__GetProc(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0041F12Cu, args, argc);
}

BackendWord Sound__BGMStop(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00420244u, args, argc);
}

BackendWord Fade__IsBlackOut(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x003CF4D8u, args, argc);
}

BackendWord Movie__CreateBind(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0041EE54u, args, argc);
}

BackendWord Movie__Create(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0041F0B8u, args, argc);
}

BackendWord Movie__IsPaused(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0041F13Cu, args, argc);
}

BackendWord Sound__BGMPlay(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x004201A0u, args, argc);
}

BackendWord Sound__BGMTrackVolume(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0041F730u, args, argc);
}

BackendWord Sound__RBGMPlay(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00420418u, args, argc);
}

BackendWord Sound__RBGMStop(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00420500u, args, argc);
}

BackendWord Sound__RBGMEffect(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0041F334u, args, argc);
}

BackendWord Sound__BGMVolume(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00420858u, args, argc);
}

BackendWord map__sound__Bgm__UpdateBGM(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00399188u, args, argc);
}

BackendWord map__sound__Bgm__Map__Resume(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00398FBCu, args, argc);
}

BackendWord Sound__SEPlay(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x004200FCu, args, argc);
}

BackendWord Sound3D__Play(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x004E01E8u, args, argc);
}

BackendWord Sound__LSEPlay(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00420318u, args, argc);
}

BackendWord Sound__Voice(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0041FFE4u, args, argc);
}

BackendWord Sound__EnvStop(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x004202FCu, args, argc);
}

BackendWord Sound__EnvPlay(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0042028Cu, args, argc);
}

} // namespace fates::event::backend
