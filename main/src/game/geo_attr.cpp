#include "fates/game/geo_attr.hpp"

#include "fates/detail/metadata_runtime.hpp"

void GeoAttr::Initialize() {
    using namespace fates::decomp_detail;

    LoadStructList(
        gGeoAttrList,
        "GameData/GeoAttr.bin.lz",
        "GeoAttrTable",
        0,
        0x14);

    const int count = GetStructListCount(gGeoAttrList);
    for (int index = 0; index < count; ++index) {
        auto* attr = static_cast<GeoAttr*>(
            GetStructListSurely(gGeoAttrList, index));
        const char* const name = TargetPointer<const char>(attr->name);
        attr->nameSum16 = GetStringSum16(name);
    }
}

GeoAttr* GeoAttr::GetAttr(std::uint16_t nameSum) {
    using namespace fates::decomp_detail;

    const int count = GetStructListCount(gGeoAttrList);
    for (int index = 0; index < count; ++index) {
        auto* attr = static_cast<GeoAttr*>(
            GetStructListSurely(gGeoAttrList, index));
        if (attr->nameSum16 == nameSum) {
            return attr;
        }
    }

    return static_cast<GeoAttr*>(
        GetStructListSurely(gGeoAttrList, 0));
}

void GeoAttr::Finalize() {
    fates::decomp_detail::FreeStructList(
        fates::decomp_detail::gGeoAttrList,
        0);
}
