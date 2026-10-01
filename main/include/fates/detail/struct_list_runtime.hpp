#pragma once

#include <cstddef>

namespace fates::decomp_detail {

// Opaque bridge to the still-unpromoted FileBase / FileObject /
// UniqueArchiveObject ownership layer. StructList owns the returned handle and
// releases it through DestroyStructListArchive.
void* OpenStructListArchive(const char* path);
void DestroyStructListArchive(void* archiveHandle);

const char* GetDefaultStructListObjectName(void* archiveHandle);
void* TryGetUniqueArchiveObject(void* archiveHandle, const char* name);
void* GetSurelyUniqueArchiveObject(void* archiveHandle, const char* name);

} // namespace fates::decomp_detail
