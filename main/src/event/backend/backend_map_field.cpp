#include "fates/event/backend/backend_spine.hpp"

namespace fates::event::backend {

// MapField backend ownership. Complex object layouts remain behind BackendRuntime;
// exact retail identity/ranges are retained in the registry and evidence.

BackendWord Map__Get(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00342574u, args, argc);
}

BackendWord FieldWorld__FindObject(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00177A3Cu, args, argc);
}

BackendWord FieldWorld__GetHeightMap(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00177D58u, args, argc);
}

BackendWord HeightMap__GetMapHeight(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00548A90u, args, argc);
}

BackendWord FieldWorld__GetScene(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00178298u, args, argc);
}

BackendWord map__Draw__GetInstance(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00388B88u, args, argc);
}

BackendWord FieldWorld__GetParam(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00178280u, args, argc);
}

BackendWord Spot__Get(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x003D30A4u, args, argc);
}

BackendWord Spot__GetData(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x003D3104u, args, argc);
}

BackendWord Map__Load(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x003426F4u, args, argc);
}

BackendWord FieldWorld__UpdateRange_2(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00177C04u, args, argc);
}

BackendWord FieldObject__SetState(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x001985D8u, args, argc);
}

BackendWord Map__Free(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00342584u, args, argc);
}

BackendWord map__Draw__Create(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00388CB4u, args, argc);
}

BackendWord map__Draw__Delete(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00388DC0u, args, argc);
}

BackendWord map__ItemHelper__Rod__GetRescuePosition(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0034A32Cu, args, argc);
}

BackendWord FieldWorld__UpdateRange_3(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00177C20u, args, argc);
}

BackendWord map__Effect__Create_3(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0039FAECu, args, argc);
}

BackendWord map__Draw__SuspendBind(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00388B98u, args, argc);
}

BackendWord map__Draw__ResumeBind(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00388A80u, args, argc);
}

BackendWord HeightMap__GetHeight(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0054931Cu, args, argc);
}

BackendWord FieldObject__SetVisible(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00196F8Cu, args, argc);
}

BackendWord FieldObject__SetEscape(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x001988B8u, args, argc);
}

BackendWord FieldWorld__CreateObject(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00177CF0u, args, argc);
}

BackendWord FieldObject__SetDispos(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00198874u, args, argc);
}

BackendWord FieldWorld__DeleteObject(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00177D08u, args, argc);
}

BackendWord FieldObject__PlayAnime(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00198810u, args, argc);
}

BackendWord FieldObject__GetLocatorIndex(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x00508288u, args, argc);
}

BackendWord FieldObject__GetLocatorPos_2(BackendRuntime& runtime, const BackendWord* args, std::size_t argc) {
    return InvokeBackend(runtime, 0x0050821Cu, args, argc);
}

} // namespace fates::event::backend
