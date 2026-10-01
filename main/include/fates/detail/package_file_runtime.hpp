#pragma once

#include "fates/io/package_file.hpp"

#include <cstddef>

class IdentHash;

namespace fates::decomp_detail {

// Readable view of the package-specific state appended after the common
// FileObject fields in retail. The retail offsets are +0x80 IdentHash,
// +0x84 count, +0x88 entry table, +0x8C data base, +0x90 base path.
struct PackageFileState {
    IdentHash* index{};
    int entryCount{};
    const PackageFileInfo* entries{};
    const std::byte* dataBase{};
    const char* basePath{};
};

const PackageFileState* GetPackageFileState(const PackageFile& package);

} // namespace fates::decomp_detail
