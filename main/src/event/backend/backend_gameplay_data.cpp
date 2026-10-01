#include "fates/event/backend/backend_spine.hpp"

namespace fates::event::backend {

// GameplayData backend ownership. Complex object layouts remain behind BackendRuntime;
// exact retail identity/ranges are retained in the registry and evidence.

BackendWord Transporter__Get(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x001A744Cu, args, argc);
}

BackendWord ProcGameInfo__SetUnit(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x001BBE40u, args, argc);
}

BackendWord Transporter__Add(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x001A7050u, args, argc);
}

BackendWord ItemSkill__Get(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00500CB0u, args, argc);
}

BackendWord ProcGameInfo__GetUnit(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x001BBE20u, args, argc);
}

BackendWord Transporter__Delete(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x001A74FCu, args, argc);
}

BackendWord JobCategory__Get(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0019B9CCu, args, argc);
}

BackendWord unit__Enhance__AddWeakness(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0041B1CCu, args, argc);
}

BackendWord unit__Enhance__MergeWeakness(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0041B308u, args, argc);
}

BackendWord unit__AI__operator(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x004192F4u, args, argc);
}

BackendWord UnitActor__SetMotion(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00504D14u, args, argc);
}

BackendWord UnitIcon__SetIcon_5(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x004F53B4u, args, argc);
}

BackendWord AIDesc__Get(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x004432A8u, args, argc);
}

BackendWord AIValue__SetValue(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x004D6C8Cu, args, argc);
}

BackendWord Transporter__SetInvalidRefine(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x001A6EB8u, args, argc);
}

BackendWord anonymous_namespace__GimmickMedicineImpl(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x003AC8E4u, args, argc);
}

} // namespace fates::event::backend
