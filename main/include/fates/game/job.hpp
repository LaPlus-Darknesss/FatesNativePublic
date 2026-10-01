#pragma once

#include <cstdint>
#include "fates/game/item_kinds.hpp"

class ArchiveFile;
class Item;

class Job {
public:
    static void Initialize(const void* data);
    static void LoadArchive(const ArchiveFile* archive);

    static ArchiveFile* GetArchive(
        char* archiveName,
        int archiveNameCapacity,
        std::uint16_t classId);

    static Job* GetFromNoPrefix(const char* identifierWithoutPrefix);
    static Job* Get(const char* identifier);
    static Job* Get(std::uint16_t classId);
    static Job* TryGet(const char* identifier);
    static Job* TryGet(std::uint16_t classId);
    static bool IsExist(const char* identifier);
    static bool IsExist(std::uint16_t classId);

    static void FreeArchive(const char* archiveName);
    static void Free(const char* archiveName);
    static void Load(const char* archiveName);
    static bool IsLoad(const char* archiveName);

    // Recovered Pass-33 class metadata queries.
    bool IsDownload() const;
    bool IsEnemyOnly() const;
    const wchar_t* GetIntroName() const;
    std::uint16_t GetEquipSkill(int index) const;
    int GetLimitLevel() const;
    int GetWeaponIcon(ItemKind::Type kind) const;
    bool CanEquipFromSubKind(ItemSubKind::Type subKind) const;
    const Item* GetBasicItemCanEquipForEvent() const;
    bool IsHigh() const;
    const wchar_t* GetHelp() const;
    const wchar_t* GetName() const;
    bool IsFlyer() const;
    bool IsGiant() const;
    bool IsRider() const;
    bool IsFemale() const;

    static void Finalize();
};

using ClassData = Job;
