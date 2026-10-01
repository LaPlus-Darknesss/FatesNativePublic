#pragma once

#include "fates/io/file_base.hpp"

#include <cstdint>

struct PackageFileInfo {
    std::uint32_t unknown00{};
    int entryIndex{};
    std::uint32_t size{};
    std::uint32_t dataOffset{};
};

static_assert(sizeof(PackageFileInfo) == 0x10);

class PackageFile : public FileBase {
public:
    ~PackageFile();

    void Dump() const;
    const void* GetData(const char* identifier) const;
    const void* GetData(int index) const;
    const PackageFileInfo* GetInfo(const char* identifier) const;
    char* GetPath(char* destination, int destinationCapacity, const char* identifier) const;
    unsigned int GetSize(const char* identifier) const;
    unsigned int GetSize(int index) const;
    bool IsExist(const char* identifier) const;
};
