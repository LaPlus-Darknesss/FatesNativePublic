#pragma once

#include <cstddef>

class FileBase;
class IdentHash;

namespace fates::decomp_detail {

// Lower FileBase/FileObject construction/destruction remains behind a narrow
// boundary until that cache/async layer is promoted as a coherent system.
FileBase* OpenSharedFileBase(
    const char* path,
    unsigned int priority,
    bool archiveMode);
void DestroySharedFileBase(FileBase* file);

// File::GetFullPath uses the current mounted ROM root and the original Shift-JIS
// to UTF-16 path converter.
const wchar_t* GetRomRootPath();
bool IsRootPath(const char* path);
void ShiftJisToUtf16(
    wchar_t* destination,
    unsigned int destinationCapacity,
    const char* source);

} // namespace fates::decomp_detail
