#include "fates/services/core_services.hpp"

namespace fates::services {

// Field ownership: retail identity is exact; unresolved object layout stays behind ServiceRuntime.

ServiceWord FieldWorld__ChangeFree(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0017794Cu, args, argc);
}

ServiceWord FieldWorld__ChangeLoad(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001779C4u, args, argc);
}

ServiceWord FieldWorld__ChangeTime(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001779D4u, args, argc);
}

ServiceWord FieldWorld__FindObject_2(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00177A54u, args, argc);
}

ServiceWord FieldWorld__FindObject_3(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00177A74u, args, argc);
}

ServiceWord FieldWorld__Initialize(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00177AA4u, args, argc);
}

ServiceWord FieldWorld__RemoveData(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00177ACCu, args, argc);
}

ServiceWord FieldWorld__Deserialize(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00177AE4u, args, argc);
}

ServiceWord FieldWorld__GetGeometry(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00177B8Cu, args, argc);
}

ServiceWord FieldWorld__SetCallback(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00177BA0u, args, argc);
}

ServiceWord FieldWorld__SetKeepTime(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00177BB4u, args, argc);
}

ServiceWord FieldWorld__SetLevelPos(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00177BCCu, args, argc);
}

ServiceWord FieldWorld__UpdateRange(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00177BECu, args, argc);
}

ServiceWord FieldWorld__ChangeResume(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00177C4Cu, args, argc);
}

ServiceWord FieldWorld__DeleteObject_2(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00177D20u, args, argc);
}

ServiceWord FieldWorld__GetCollision(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00177D44u, args, argc);
}

ServiceWord FieldWorld__UpdateDispos(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00177D6Cu, args, argc);
}

ServiceWord FieldWorld__ChangeSuspend(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00177D84u, args, argc);
}

ServiceWord FieldWorld__GetObjectList(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00177E18u, args, argc);
}

ServiceWord FieldWorld__GetEffectColor(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00177E30u, args, argc);
}

ServiceWord FieldWorld__CalcGeometryHit(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00177E98u, args, argc);
}

ServiceWord FieldWorld__GetFaceLowerColor(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00177EACu, args, argc);
}

ServiceWord FieldWorld__GetFaceUpperColor(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00177EDCu, args, argc);
}

ServiceWord FieldWorld__GetGeometryHeight(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00177F0Cu, args, argc);
}

ServiceWord FieldWorld__GetUnitLowerColor(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00177F4Cu, args, argc);
}

ServiceWord FieldWorld__GetUnitUpperColor(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00177F7Cu, args, argc);
}

ServiceWord FieldWorld__Draw(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00177FB0u, args, argc);
}

ServiceWord FieldWorld__Free(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001780A8u, args, argc);
}

ServiceWord FieldWorld__Load(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001780C4u, args, argc);
}

ServiceWord FieldWorld__Tick(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001780DCu, args, argc);
}

ServiceWord FieldWorld__Resume(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001780ECu, args, argc);
}

ServiceWord FieldWorld__Update(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00178158u, args, argc);
}

ServiceWord FieldWorld__GetArea(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00178210u, args, argc);
}

ServiceWord FieldWorld__SetTime(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00178238u, args, argc);
}

ServiceWord FieldWorld__Finalize(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0017826Cu, args, argc);
}

ServiceWord FieldWorld__IsLoaded(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001782ACu, args, argc);
}

ServiceWord FieldWorld__IsLoaded_2(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001782C4u, args, argc);
}

ServiceWord FieldWorld__EntryData(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001782DCu, args, argc);
}

ServiceWord FieldWorld__GetCamera(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001782F8u, args, argc);
}

ServiceWord FieldWorld__Serialize(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0017830Cu, args, argc);
}

ServiceWord FieldWorld__UpdateAll(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00178320u, args, argc);
}

ServiceWord FieldObject__PlayAccess(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00196D60u, args, argc);
}

ServiceWord FieldObject__SkipAccess(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00196FC8u, args, argc);
}

ServiceWord FieldObject__UpdateColor(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001970BCu, args, argc);
}

ServiceWord FieldObject__SetTransform(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001971C8u, args, argc);
}

ServiceWord FieldObject__UpdateAccess(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00197314u, args, argc);
}

ServiceWord FieldObject__UpdateDispos(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0019746Cu, args, argc);
}

ServiceWord FieldObject__UpdateEffect(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00197700u, args, argc);
}

ServiceWord FieldObject__SetTranslate(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00197910u, args, argc);
}

ServiceWord FieldObject__UpdateTransform(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00197948u, args, argc);
}

ServiceWord FieldObject__Free(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00197B9Cu, args, argc);
}

ServiceWord FieldObject__Load(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00197D5Cu, args, argc);
}

ServiceWord FieldObject__Update(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00198074u, args, argc);
}

ServiceWord FieldObject__FadeOut(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00198474u, args, argc);
}

ServiceWord FieldObject__SetPose(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00198488u, args, argc);
}

ServiceWord FieldObject__SetAlpha(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001984C0u, args, argc);
}

ServiceWord FieldObject__SetColor(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001984D4u, args, argc);
}

ServiceWord FieldObject__SetLevel(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001984ECu, args, argc);
}

ServiceWord FieldObject__FieldObject(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001988FCu, args, argc);
}

ServiceWord FieldObject__FieldObject_2(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00198D44u, args, argc);
}

ServiceWord FieldWorld__FindObject_4(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001C9B74u, args, argc);
}

ServiceWord FieldObject__GetChangeMap(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00508004u, args, argc);
}

ServiceWord FieldObject__GetLocatorPos(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00508170u, args, argc);
}

ServiceWord FieldObject__IsPlayingAccess(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x005082A4u, args, argc);
}

ServiceWord FieldObject__GetRange(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x005082B8u, args, argc);
}

ServiceWord FieldObject__IsInside(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00508318u, args, argc);
}

ServiceWord FieldObject__GetCenter(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x005083A8u, args, argc);
}

ServiceWord FieldObject__IsVisible(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00549808u, args, argc);
}

} // namespace fates::services
