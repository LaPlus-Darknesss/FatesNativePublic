#include "fates/services/core_services.hpp"

namespace fates::services {

// VisualMedia ownership: retail identity is exact; unresolved object layout stays behind ServiceRuntime.

ServiceWord Fade__Initialize(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00105050u, args, argc);
}

ServiceWord Fade__GetGoalColor(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x003CF504u, args, argc);
}

ServiceWord Fade__IsBlackOutAll(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x003CF564u, args, argc);
}

ServiceWord Fade__IsActiveFadeIn(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x003CF5A8u, args, argc);
}

ServiceWord Fade__BlackIn(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x003CF600u, args, argc);
}

ServiceWord Fade__WhiteIn(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x003CF614u, args, argc);
}

ServiceWord Fade__BlackOut(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x003CF620u, args, argc);
}

ServiceWord Fade__GetAlpha(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x003CF62Cu, args, argc);
}

ServiceWord Fade__GetColor(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x003CF644u, args, argc);
}

ServiceWord Fade__WaitBind(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x003CF6C4u, args, argc);
}

ServiceWord Fade__WhiteOut(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x003CF734u, args, argc);
}

ServiceWord Movie__Initialize(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041EEA4u, args, argc);
}

ServiceWord Movie__SetSkipped(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041EEBCu, args, argc);
}

ServiceWord Movie__GetFilePath(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041EED0u, args, argc);
}

ServiceWord Movie__GetFileRoute(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041EFD0u, args, argc);
}

ServiceWord Movie__IsPauseAtEnd(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041F004u, args, argc);
}

ServiceWord Movie__EnablePauseAtEnd(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041F018u, args, argc);
}

ServiceWord Movie__GetNowVideoFrame(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041F02Cu, args, argc);
}

ServiceWord Movie__CreateOpeningBind(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041F050u, args, argc);
}

ServiceWord Movie__DisablePauseAtEnd(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041F0A4u, args, argc);
}

ServiceWord Movie__Destroy(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041F10Cu, args, argc);
}

ServiceWord Movie__IsSkipped(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041F1ACu, args, argc);
}

ServiceWord ProcMovie__ThreadStart(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0050296Cu, args, argc);
}

ServiceWord ProcMovie__Tick(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00502A64u, args, argc);
}

ServiceWord ProcMovie__ThreadEnd(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00502B00u, args, argc);
}

ServiceWord ProcMovie__ProcMovie(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00502BCCu, args, argc);
}

ServiceWord ProcMovie__ProcMovie_3(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00502E50u, args, argc);
}

} // namespace fates::services
