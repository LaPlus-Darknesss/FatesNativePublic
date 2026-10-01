#pragma once

#include "fates/detail/arm32_address.hpp"

#include <cstddef>
#include <cstdint>

class IdentHash;
class StructList;

namespace fates::decomp_detail {

void LoadStructList(
    StructList*& list,
    const char* path,
    const char* tableName,
    int flags,
    unsigned int recordSize);
void FreeStructList(StructList* list, int flags);
int GetStructListCount(const StructList* list);
void* GetStructListSurely(StructList* list, int index);
void DumpStructList(const StructList* list);

std::uint16_t GetStringSum16(const char* text); // retail sign::GetSum16
void ApplyLanguageFolder(char* path);           // retail Lang::Folder

extern StructList* gCameraDataList;
extern StructList* gEffectDataList;
extern StructList* gGeoAttrList;
extern IdentHash* gTutorialIdentHash;

} // namespace fates::decomp_detail
