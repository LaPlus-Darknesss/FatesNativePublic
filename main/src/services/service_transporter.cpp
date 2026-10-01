#include "fates/services/core_services.hpp"

namespace fates::services {

// Transporter ownership: retail identity is exact; unresolved object layout stays behind ServiceRuntime.

ServiceWord Transporter__Initialize(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001A6D04u, args, argc);
}

ServiceWord Transporter__Deserialize(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001A6D74u, args, argc);
}

ServiceWord Transporter__GetExistNum(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001A6E70u, args, argc);
}

ServiceWord Transporter__ClearInvalidRefine(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001A6F0Cu, args, argc);
}

ServiceWord Transporter__anonymous_namespace__GetPrice(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001A6F4Cu, args, argc);
}

ServiceWord Transporter__Sub(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001A7460u, args, argc);
}

ServiceWord Transporter__Data__Data(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001A74ACu, args, argc);
}

ServiceWord Transporter__Reset(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001A74C0u, args, argc);
}

ServiceWord Transporter__Finalize(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001A7534u, args, argc);
}

ServiceWord Transporter__GetIndex(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001A755Cu, args, argc);
}

ServiceWord Transporter__Serialize(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001A75C4u, args, argc);
}

} // namespace fates::services
