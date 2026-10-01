#include "fates/coreimpl/core_service_impl.hpp"

namespace fates::coreimpl {

// FieldGeometryCore: exact retail identity is owned; unresolved concrete object layout remains behind ImplRuntime.

ImplWord MapPose__GetMatrixSRT(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x00544964u, args, argc);
}

ImplWord AABB__Reset(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x0011B264u, args, argc);
}

ImplWord ColsTree__Remove(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004E4D40u, args, argc);
}

ImplWord MapRange__Reset_5(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004ED9F0u, args, argc);
}

ImplWord util__MoveTime__SetTime(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x0041C58Cu, args, argc);
}

ImplWord util__MoveTime__Evaluate(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x00111B78u, args, argc);
}

ImplWord util__MoveTime__GetRate(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x0011389Cu, args, argc);
}

ImplWord HeightList__Copy(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x0017C6C8u, args, argc);
}

ImplWord HeightList__Transform(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x0017C758u, args, argc);
}

ImplWord FieldWorldImpl__UpdateAll(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x001CFED4u, args, argc);
}

ImplWord AABB__Transform(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x003CF3A4u, args, argc);
}

ImplWord ColsTree__Clear(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004E4894u, args, argc);
}

ImplWord ColsTree__Entry(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004E4940u, args, argc);
}

ImplWord MapRange__Reset(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004ED8B4u, args, argc);
}

ImplWord PolyList__Copy(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004EDB84u, args, argc);
}

ImplWord PolyList__Transform(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004EDDECu, args, argc);
}

ImplWord HeightMap__Remove(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x00500604u, args, argc);
}

ImplWord FieldWorldImpl__CalcGeometryHit(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x0050A7ECu, args, argc);
}

ImplWord FieldData__IsLoaded(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x005486C8u, args, argc);
}

ImplWord FieldActor__Setup(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x00175930u, args, argc);
}

ImplWord FieldActor__Update(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x00175B1Cu, args, argc);
}

ImplWord FieldActor__Cleanup(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x00175B44u, args, argc);
}

ImplWord FieldTrick__Free(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x00176E2Cu, args, argc);
}

ImplWord FieldTrick__Load(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x00176EB4u, args, argc);
}

ImplWord FieldTrick__Update(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x00177134u, args, argc);
}

ImplWord ObjectBase__EntryHandle(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x0017D878u, args, argc);
}

ImplWord ObjectBase__RemoveHandle(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x0017D950u, args, argc);
}

ImplWord FieldShadow__Draw(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x00199358u, args, argc);
}

ImplWord FieldShadow__Update(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x001993C0u, args, argc);
}

ImplWord FieldWeather__Draw(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x001B1CB4u, args, argc);
}

ImplWord ObjectManager__Get(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x001C9B78u, args, argc);
}

ImplWord OwnHeightList__OwnHeightList(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x001C9D10u, args, argc);
}

ImplWord FieldWorldImpl__ChangeLoad(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x001CEBECu, args, argc);
}

ImplWord FieldWorldImpl__UpdateParam(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x001CF1B8u, args, argc);
}

ImplWord FieldWorldImpl__Free(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x001CF348u, args, argc);
}

ImplWord FieldWorldImpl__Load(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x001CF414u, args, argc);
}

ImplWord FieldWorldImpl__Tick(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x001CF69Cu, args, argc);
}

ImplWord FieldWorldImpl__Resume(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x001CF978u, args, argc);
}

ImplWord FieldWorldImpl__FreeScene(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x001CFAB4u, args, argc);
}

ImplWord FieldWorldImpl__Serialize(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x001CFD34u, args, argc);
}

ImplWord MTX__GetTranslate(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x00341F04u, args, argc);
}

ImplWord GfxUtil__GetModDeg(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004DA804u, args, argc);
}

ImplWord MapPose__Copy(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004DC4ECu, args, argc);
}

ImplWord MapPose__Round(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004DC518u, args, argc);
}

ImplWord MapRange__Add_2(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004ED6E0u, args, argc);
}

ImplWord MapRange__Add_3(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004ED78Cu, args, argc);
}

ImplWord HeightMap__Clear(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004ED984u, args, argc);
}

ImplWord PolyList__Round(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004EDC38u, args, argc);
}

ImplWord FieldArea__Update(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004FEB8Cu, args, argc);
}

ImplWord FieldData__RemoveData(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004FF23Cu, args, argc);
}

ImplWord FieldData__DeleteObject(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004FF384u, args, argc);
}

ImplWord FieldData__Resume(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004FF454u, args, argc);
}

ImplWord FieldData__Update(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004FF480u, args, argc);
}

ImplWord FieldData__Suspend(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004FF4B8u, args, argc);
}

ImplWord FieldData__EntryData(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004FF4E4u, args, argc);
}

ImplWord FieldData__FreeParam(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x004FF620u, args, argc);
}

ImplWord HeightMap__Entry(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x00500510u, args, argc);
}

ImplWord HeightMap__Update(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x00500634u, args, argc);
}

ImplWord FieldData__FreeField(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x005009E0u, args, argc);
}

ImplWord AABB__IsIntersect(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x00528BC0u, args, argc);
}

ImplWord MapPose__IsIdentity(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x00544828u, args, argc);
}

ImplWord MapPose__GetMatrixRT(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x005448F8u, args, argc);
}

ImplWord FieldData__FindObject_2(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x005484C4u, args, argc);
}

ImplWord FieldData__FindObject_3(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x00548528u, args, argc);
}

ImplWord PartsData__GetModelName(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x005495BCu, args, argc);
}

ImplWord PartsData__GetAccessAnim(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x005495FCu, args, argc);
}

ImplWord ReferList__GetIndex(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x0054969Cu, args, argc);
}

} // namespace fates::coreimpl
