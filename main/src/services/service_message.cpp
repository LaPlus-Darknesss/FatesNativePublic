#include "fates/services/core_services.hpp"

namespace fates::services {

// Message ownership: retail identity is exact; unresolved object layout stays behind ServiceRuntime.

ServiceWord GameMessage__SetSound(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0011B0D0u, args, argc);
}

ServiceWord GameMessage__Persistent(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0019A550u, args, argc);
}

ServiceWord GameMessage__WindowOpen(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0019A764u, args, argc);
}

ServiceWord GameMessage__CreateFatal(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0019A7E4u, args, argc);
}

ServiceWord GameMessage__WindowClose(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0019A814u, args, argc);
}

ServiceWord GameMessage__CreateSystem(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0019A884u, args, argc);
}

ServiceWord GameMessage__DarknessStop(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0019A8B0u, args, argc);
}

ServiceWord GameMessage__CreateKeyWait(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0019A908u, args, argc);
}

ServiceWord GameMessage__CreateWarning(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0019A928u, args, argc);
}

ServiceWord GameMessage__DarknessStart(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0019A95Cu, args, argc);
}

ServiceWord GameMessage__RecoverParentMenu(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0019A994u, args, argc);
}

ServiceWord GameMessage__CreateKeyTouchWaitLower(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0019A9F8u, args, argc);
}

ServiceWord GameMessage__Tick(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0019AA20u, args, argc);
}

ServiceWord GameMessage__Setup(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0019AB6Cu, args, argc);
}

ServiceWord GameMessage__Create(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0019AC98u, args, argc);
}

ServiceWord GameMessage__IsWait(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0019ACDCu, args, argc);
}

ServiceWord GameMessage__SEOpen(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0019ACF8u, args, argc);
}

ServiceWord GameMessage__SEClose(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0019AD08u, args, argc);
}

ServiceWord GameMessage__DrawFont(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0019AD0Cu, args, argc);
}

ServiceWord GameMessage__GameMessage(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0019AD64u, args, argc);
}

ServiceWord GameMessage__GameMessage_2(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0019AE08u, args, argc);
}

ServiceWord GameMessage__GameMessage_3(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0019AED8u, args, argc);
}

} // namespace fates::services
