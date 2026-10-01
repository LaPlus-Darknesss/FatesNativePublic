#include "fates/game/person.hpp"

#include "fates/detail/archive_table_runtime.hpp"
#include "fates/detail/file_runtime.hpp"
#include "fates/detail/support_runtime.hpp"
#include "fates/engine/ident_hash.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {

constexpr std::size_t kPersonCountOffset = 0x04;
constexpr std::size_t kPersonResidentFlagOffset = 0x07;
constexpr std::size_t kPersonRecordsOffset = 0x10;
constexpr std::size_t kPersonFidOffset = 0x0C;
constexpr std::size_t kPersonIdOffset = 0x24;
constexpr std::size_t kPersonRecordSize = 0x98;
constexpr const char* kPersonIdentifierPrefix = "PID_";

std::uint16_t ReadU16(const std::byte* address) {
    std::uint16_t value{};
    std::memcpy(&value, address, sizeof(value));
    return value;
}

Person* PersonAt(std::byte* archiveData, std::uint32_t index) {
    return reinterpret_cast<Person*>(
        archiveData + kPersonRecordsOffset + index * kPersonRecordSize);
}

std::uint16_t PersonId(const Person* person) {
    return ReadU16(
        reinterpret_cast<const std::byte*>(person) + kPersonIdOffset);
}

fates::decomp_detail::LoadedTableArchive* FindArchiveById(
    std::uint16_t personId) {
    for (auto* archive = fates::decomp_detail::gLoadedPersonArchives;
         archive != nullptr;
         archive = archive->next) {
        const std::uint32_t firstId =
            ReadU16(archive->data + kPersonRecordsOffset + kPersonIdOffset);
        const std::uint32_t count =
            ReadU16(archive->data + kPersonCountOffset);
        const std::uint32_t requestedId = personId;
        if (requestedId >= firstId && requestedId < firstId + count) {
            return archive;
        }
    }
    return nullptr;
}

Person* FindById(std::uint16_t personId) {
    auto* const archive = FindArchiveById(personId);
    if (archive == nullptr) {
        return nullptr;
    }

    const std::uint32_t firstId =
        ReadU16(archive->data + kPersonRecordsOffset + kPersonIdOffset);
    return PersonAt(archive->data, personId - firstId);
}

void FreeSupportIfPresent(const void* personTable) {
    const void* const supportTable =
        fates::decomp_detail::GetPersonSupportTable(personTable);
    if (supportTable != nullptr) {
        fates::decomp_detail::FreeSupportTable(supportTable);
    }
}

void RemovePersonArchive(const char* archiveName, bool destroyArchive) {
    using namespace fates::decomp_detail;

    LoadedTableArchive* const node =
        UnlinkAdditionalArchiveByName(gLoadedPersonArchives, archiveName);
    if (node == nullptr) {
        return;
    }

    FreeSupportIfPresent(node->data);
    if (destroyArchive) {
        DestroyArchiveNodeAndOwner(node);
    } else {
        delete node;
    }
}

void PruneNonResidentPersonArchives() {
    using namespace fates::decomp_detail;

    LoadedTableArchive*& head = gLoadedPersonArchives;
    if (head == nullptr) {
        return;
    }

    LoadedTableArchive* previous = head;
    LoadedTableArchive* node = head->next;
    while (node != nullptr) {
        LoadedTableArchive* const next = node->next;

        // Person::IsResident establishes byte +0x07 of the PersonTable header
        // as the retail residency flag.
        const bool resident =
            node->data != nullptr &&
            static_cast<std::uint8_t>(node->data[kPersonResidentFlagOffset]) != 0;

        if (!resident) {
            previous->next = next;
            node->next = nullptr;
            FreeSupportIfPresent(node->data);
            DestroyArchiveNodeAndOwner(node);
        } else {
            previous = node;
        }

        node = next;
    }
}

} // namespace

void Person::Initialize(const void* data) {
    using namespace fates::decomp_detail;

    gLoadedPersonArchives = CreateLoadedTableArchive(data);
    InitializeSupportSystem();

    const void* const supportTable = GetPersonSupportTable(data);
    if (supportTable != nullptr) {
        LoadSupportTable(supportTable);
    }
}

void Person::LoadArchive(const ArchiveFile* archive) {
    using namespace fates::decomp_detail;

    std::byte* const tableData = GetArchiveTablePayload(archive);
    auto* const node = CreateLoadedTableArchive(
        tableData,
        const_cast<ArchiveFile*>(archive));

    const void* const supportTable = GetPersonSupportTable(tableData);
    if (supportTable != nullptr) {
        LoadSupportTable(supportTable);
    }

    AppendLoadedTableArchive(gLoadedPersonArchives, node);
}

ArchiveFile* Person::GetArchive(
    char* archiveName,
    int archiveNameCapacity,
    std::uint16_t personId) {
    auto* const node = FindArchiveById(personId);
    if (node == nullptr) {
        return nullptr;
    }

    fates::decomp_detail::CopyArchiveName(
        node,
        archiveName,
        archiveNameCapacity);
    return node->archive;
}

Person* Person::GetFromFid(const char* faceIdentifier) {
    if (faceIdentifier == nullptr) {
        return nullptr;
    }

    for (auto* archive = fates::decomp_detail::gLoadedPersonArchives;
         archive != nullptr;
         archive = archive->next) {
        const std::uint32_t count =
            ReadU16(archive->data + kPersonCountOffset);

        for (std::uint32_t index = 0; index < count; ++index) {
            Person* const person = PersonAt(archive->data, index);
            const auto* bytes = reinterpret_cast<const std::byte*>(person);
            const char* const fid =
                fates::decomp_detail::TargetPointer<const char>(
                    fates::decomp_detail::ReadArm32Address(
                        bytes,
                        kPersonFidOffset));
            if (fid != nullptr && std::strcmp(fid, faceIdentifier) == 0) {
                return person;
            }
        }
    }

    return nullptr;
}

Person* Person::Get(const char* identifier) {
    return static_cast<Person*>(
        fates::decomp_detail::gPersonIdentHash->GetSurely(identifier));
}

Person* Person::Get(std::uint16_t personId) {
    Person* const candidate = FindById(personId);
    if (candidate != nullptr) {
        return candidate;
    }

    return PersonAt(fates::decomp_detail::gLoadedPersonArchives->data, 0);
}

Person* Person::GetFromNoPrefix(const char* identifierWithoutPrefix) {
    if (identifierWithoutPrefix == nullptr) {
        return nullptr;
    }

    char identifier[0x44]{};
    std::snprintf(
        identifier,
        0x40,
        "%s%s",
        kPersonIdentifierPrefix,
        identifierWithoutPrefix);
    return static_cast<Person*>(
        fates::decomp_detail::gPersonIdentHash->GetSurely(identifier));
}

Person* Person::TryGet(const char* identifier) {
    IdentHashEntry* const entry =
        fates::decomp_detail::gPersonIdentHash->GetIdent(identifier);
    return entry != nullptr ? static_cast<Person*>(entry->value) : nullptr;
}

Person* Person::TryGet(std::uint16_t personId) {
    return FindById(personId);
}

bool Person::IsExist(const char* identifier) {
    return TryGet(identifier) != nullptr;
}

bool Person::IsExist(std::uint16_t personId) {
    return TryGet(personId) != nullptr;
}

bool Person::IsResident(const Person* person) {
    if (person == nullptr) {
        return false;
    }

    auto* const archive = FindArchiveById(PersonId(person));
    return archive != nullptr &&
           static_cast<std::uint8_t>(
               archive->data[kPersonResidentFlagOffset]) != 0;
}

bool Person::IsResidentFirst(const Person* person) {
    if (person == nullptr ||
        fates::decomp_detail::gLoadedPersonArchives == nullptr) {
        return false;
    }

    auto* const archive = fates::decomp_detail::gLoadedPersonArchives;
    const std::uint32_t firstId =
        ReadU16(archive->data + kPersonRecordsOffset + kPersonIdOffset);
    const std::uint32_t count =
        ReadU16(archive->data + kPersonCountOffset);
    const std::uint32_t id = PersonId(person);

    return id >= firstId &&
           id < firstId + count &&
           static_cast<std::uint8_t>(
               archive->data[kPersonResidentFlagOffset]) != 0;
}

void Person::FreeArchive(const char* archiveName) {
    RemovePersonArchive(archiveName, false);
}

void Person::Free(const char* archiveName) {
    RemovePersonArchive(archiveName, true);
}

void Person::Load(const char* archiveName) {
    if (archiveName == nullptr) {
        return;
    }

    char path[0x40]{};
    const char* const routeDirectory =
        fates::decomp_detail::GetCurrentRouteDirectoryName();

    std::snprintf(
        path,
        sizeof(path),
        "GameData/Person/%s/%s.bin.lz",
        routeDirectory != nullptr ? routeDirectory : "",
        archiveName);

    if (!fates::decomp_detail::FileExists(path)) {
        std::snprintf(
            path,
            sizeof(path),
            "GameData/Person/%s.bin.lz",
            archiveName);
    }

    ArchiveFile* const archive =
        fates::decomp_detail::OpenArchiveFile(path);
    LoadArchive(archive);
}

bool Person::IsLoad(const char* archiveName) {
    return fates::decomp_detail::FindLoadedArchiveByName(
               fates::decomp_detail::gLoadedPersonArchives,
               archiveName) != nullptr;
}

void Person::InitializeChapter() {
    PruneNonResidentPersonArchives();
}

void Person::FinalizeChapter() {
    // Retail emits the same pruning body at 0x0044A3F0 and 0x0044A568.
    PruneNonResidentPersonArchives();
}

void Person::Finalize() {
    using namespace fates::decomp_detail;

    LoadedTableArchive*& head = gLoadedPersonArchives;
    if (head == nullptr) {
        return;
    }

    while (head->next != nullptr) {
        LoadedTableArchive* const node = head->next;
        head->next = node->next;
        FreeSupportIfPresent(node->data);
        DestroyArchiveNodeAndOwner(node);
    }

    FreeSupportIfPresent(head->data);
    FinalizeSupportSystem();

    delete head;
    head = nullptr;
}
