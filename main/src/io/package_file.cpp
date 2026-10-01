#include "fates/io/package_file.hpp"

#include "fates/detail/package_file_runtime.hpp"
#include "fates/engine/ident_hash.hpp"

#include <cstdio>

namespace {
const fates::decomp_detail::PackageFileState* State(const PackageFile& package) {
    return fates::decomp_detail::GetPackageFileState(package);
}
}

PackageFile::~PackageFile() {
    Free();
}

void PackageFile::Dump() const {
    // The shipped retail body only walks the entry count; debug output is not
    // present in this release. Preserve the observable no-op behavior.
    const auto* const state = State(*this);
    if (state == nullptr) {
        return;
    }
    for (int index = 0; index < state->entryCount; ++index) {
        (void)index;
    }
}

const PackageFileInfo* PackageFile::GetInfo(const char* identifier) const {
    const auto* const state = State(*this);
    if (state == nullptr || state->index == nullptr || identifier == nullptr) {
        return nullptr;
    }
    return static_cast<const PackageFileInfo*>(state->index->GetSurely(identifier));
}

const void* PackageFile::GetData(const char* identifier) const {
    const auto* const state = State(*this);
    const PackageFileInfo* const info = GetInfo(identifier);
    if (state == nullptr || info == nullptr || state->dataBase == nullptr) {
        return nullptr;
    }
    return state->dataBase + info->dataOffset;
}

const void* PackageFile::GetData(int index) const {
    const auto* const state = State(*this);
    if (state == nullptr || state->entries == nullptr || state->dataBase == nullptr) {
        return nullptr;
    }
    return state->dataBase + state->entries[index].dataOffset;
}

char* PackageFile::GetPath(
    char* destination,
    int destinationCapacity,
    const char* identifier) const {
    const auto* const state = State(*this);
    if (destination == nullptr || destinationCapacity <= 0) {
        return destination;
    }
    if (state == nullptr || state->basePath == nullptr) {
        destination[0] = '\0';
        return destination;
    }
    std::snprintf(
        destination,
        static_cast<std::size_t>(destinationCapacity),
        "%s/%s",
        state->basePath,
        identifier != nullptr ? identifier : "");
    return destination;
}

unsigned int PackageFile::GetSize(const char* identifier) const {
    const PackageFileInfo* const info = GetInfo(identifier);
    return info != nullptr ? info->size : 0;
}

unsigned int PackageFile::GetSize(int index) const {
    const auto* const state = State(*this);
    return state != nullptr && state->entries != nullptr
        ? state->entries[index].size
        : 0;
}

bool PackageFile::IsExist(const char* identifier) const {
    return GetInfo(identifier) != nullptr;
}
