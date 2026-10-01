#include "fates/services/core_services.hpp"

namespace fates::services {

// MapHandle ownership: retail identity is exact; unresolved object layout stays behind ServiceRuntime.

ServiceWord GameHandle__Release(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0017BCD0u, args, argc);
}

ServiceWord GameHandle__GameHandle_3(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0017BDB0u, args, argc);
}

ServiceWord GameHandle__operator(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0017BDBCu, args, argc);
}

ServiceWord Map__Initialize(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x003422B0u, args, argc);
}

ServiceWord Map__Deserialize(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00342348u, args, argc);
}

ServiceWord Map__Finalize(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00342848u, args, argc);
}

ServiceWord Map__IsLoaded(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x003428BCu, args, argc);
}

ServiceWord GameHandle__operator_2(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x005067D8u, args, argc);
}

ServiceWord GameHandle__operator_3(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x005067F0u, args, argc);
}

ServiceWord Map__Serialize(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00524B78u, args, argc);
}

} // namespace fates::services
