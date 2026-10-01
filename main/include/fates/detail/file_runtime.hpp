#pragma once

#include "fates/detail/arm32_address.hpp"

#include <cstddef>

class ArchiveFile;

namespace fates::decomp_detail {

constexpr std::size_t kFileBaseFileObjectOffset = 0x04;
constexpr std::size_t kFileObjectDataOffset = 0x70;
constexpr std::size_t kArchiveTablePayloadOffset = 0x20;

inline void* GetFileDataFromRetailFileBase(const void* fileBase) {
    const Arm32Address fileObjectAddress =
        ReadArm32Address(fileBase, kFileBaseFileObjectOffset);
    if (fileObjectAddress == 0) {
        return nullptr;
    }

    const void* const fileObject = TargetPointer(fileObjectAddress);
    const Arm32Address dataAddress =
        ReadArm32Address(fileObject, kFileObjectDataOffset);
    return MutableTargetPointer(dataAddress);
}

inline std::byte* GetArchiveTablePayload(const ArchiveFile* archive) {
    auto* data = static_cast<std::byte*>(GetFileDataFromRetailFileBase(archive));
    return data != nullptr ? data + kArchiveTablePayloadOffset : nullptr;
}

// Source-facing boundaries for retail file/archive services that are not yet
// promoted as human-owned functions. By-name Person/Class/Item loaders use
// these instead of embedding compiler-shaped FileBase/FileObject machinery.
ArchiveFile* OpenArchiveFile(const char* path);
void DestroyArchiveFileOwner(ArchiveFile* archive);
bool FileExists(const char* path);
const char* GetCurrentRouteDirectoryName();

} // namespace fates::decomp_detail
