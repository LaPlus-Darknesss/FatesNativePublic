#include "fates/game/effect_data.hpp"

#include "fates/detail/metadata_runtime.hpp"

#include <cstdio>
#include <cstring>

namespace {
char gEffectPath[0x50]{};
}

void EffectData::Initialize() {
    fates::decomp_detail::LoadStructList(
        fates::decomp_detail::gEffectDataList,
        "GameData/GameEffect.bin.lz",
        "EffectTable",
        0,
        0x4C);
    fates::decomp_detail::DumpStructList(
        fates::decomp_detail::gEffectDataList);
}

const char* EffectData::GetFilePath(const char* archiveName) {
    std::snprintf(
        gEffectPath,
        sizeof(gEffectPath),
        "effect/%s.arc.lz",
        archiveName);

    if (archiveName != nullptr &&
        std::strncmp(archiveName, "Tlp_", 4) == 0) {
        fates::decomp_detail::ApplyLanguageFolder(gEffectPath);
    }

    return gEffectPath;
}

void EffectData::Finalize() {
    fates::decomp_detail::FreeStructList(
        fates::decomp_detail::gEffectDataList,
        0);
}

bool EffectData::HasBBox() const {
    return 0.0f < bboxValue0 + bboxValue1 + bboxValue2;
}
