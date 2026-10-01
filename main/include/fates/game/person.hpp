#pragma once

#include <cstdint>

class ArchiveFile;

class Person {
public:
    static void Initialize(const void* data);
    static void LoadArchive(const ArchiveFile* archive);

    static ArchiveFile* GetArchive(
        char* archiveName,
        int archiveNameCapacity,
        std::uint16_t personId);

    static Person* GetFromFid(const char* faceIdentifier);
    static Person* Get(const char* identifier);
    static Person* Get(std::uint16_t personId);
    static Person* GetFromNoPrefix(const char* identifierWithoutPrefix);
    static Person* TryGet(const char* identifier);
    static Person* TryGet(std::uint16_t personId);
    static bool IsExist(const char* identifier);
    static bool IsExist(std::uint16_t personId);

    static bool IsResident(const Person* person);
    static bool IsResidentFirst(const Person* person);

    static void FreeArchive(const char* archiveName);
    static void Free(const char* archiveName);
    static void Load(const char* archiveName);
    static bool IsLoad(const char* archiveName);

    static void InitializeChapter();
    static void FinalizeChapter();
    // Recovered Pass-33 person metadata and localized-label queries.
    bool IsDownload() const;
    bool IsRawDownload() const;
    std::uint16_t GetLowJobIndex() const;
    const Person* GetCaptureChange(std::uint16_t personId) const;
    const wchar_t* GetNameWithCaptureNameIndex(int index) const;
    int GetRelianceRecollectionParentChild(const Person* other, int level) const;
    int GetRelianceRecollectionBrotherSister(const Person* other, int level) const;
    bool IsBond() const;
    const wchar_t* GetHelp() const;
    const wchar_t* GetName() const;
    bool IsFemale() const;
    bool IsPlayer() const;
    bool IsCapture() const;

    static void Finalize();
};
