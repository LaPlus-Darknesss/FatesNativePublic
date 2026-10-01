#include "fates/leafcore/core_leaf_impl.hpp"

namespace fates::leafcore {

// FieldGeometryLeaf: exact retail identity owned; unresolved layout/backend state remains behind LeafRuntime.

LeafWord FieldWeatherImpl__Load(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x001EAC94u, args, argc);
}

LeafWord FieldWeatherImpl__Update(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x001EB07Cu, args, argc);
}

LeafWord FieldUtil__GetRound(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x004FF9A8u, args, argc);
}

LeafWord AABB__GetCenter(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x00528DF8u, args, argc);
}

LeafWord ColsTree__CalcHit_2(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x0054588Cu, args, argc);
}

LeafWord PolyData__GetIntersect(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x00546588u, args, argc);
}

LeafWord PolyData__GetNormal(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x00546728u, args, argc);
}

LeafWord HeightMap__GetPolyData(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x00548784u, args, argc);
}

} // namespace fates::leafcore
