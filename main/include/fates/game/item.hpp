#pragma once

#include <cstdint>

class ArchiveFile;

class Item {
public:
    static void Initialize(const void* data);
    static void LoadArchive(const ArchiveFile* archive);

    static ArchiveFile* GetArchive(
        char* archiveName,
        int archiveNameCapacity,
        std::uint16_t itemId);

    static Item* GetFromNoPrefix(const char* identifierWithoutPrefix);
    static Item* Get(const char* identifier);
    static Item* Get(std::uint16_t itemId);
    static Item* TryGet(std::uint16_t itemId);
    static bool IsExist(const char* identifier);
    static bool IsExist(std::uint16_t itemId);

    static void FreeArchive(const char* archiveName);
    static void Free(const char* archiveName);
    static void Load(const char* archiveName);
    static bool IsLoad(const char* archiveName);

    // Recovered Pass-33 instance queries. These describe static item data,
    // not the four-byte per-Unit inventory payload in unit::Item.
    bool IsDownload() const;
    bool IsHiddenEffect() const;
    bool CanUse() const;
    const wchar_t* GetHelp() const;
    const wchar_t* GetName() const;
    bool IsMagic() const;
    bool IsWeapon() const;

    static void Finalize();
};
