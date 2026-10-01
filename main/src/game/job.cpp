#include "fates/game/job.hpp"

#include "fates/detail/archive_table_runtime.hpp"
#include "fates/detail/arm32_address.hpp"
#include "fates/detail/file_runtime.hpp"
#include "fates/engine/ident_hash.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {

constexpr std::size_t kJobCountOffset = 0x06;
constexpr std::size_t kJobRecordsOffset = 0x08;
constexpr std::size_t kJobIdOffset = 0x18;
constexpr std::size_t kJobRecordSize = 0x80;
constexpr const char* kJobIdentifierPrefix = "JID_";

std::uint16_t ReadU16(const std::byte* address) {
    std::uint16_t value{};
    std::memcpy(&value, address, sizeof(value));
    return value;
}

Job* JobAt(std::byte* table, std::uint32_t index) {
    return reinterpret_cast<Job*>(
        table + kJobRecordsOffset + index * kJobRecordSize);
}

fates::decomp_detail::LoadedTableArchive* FindArchiveById(
    std::uint16_t classId) {
    for (auto* archive = fates::decomp_detail::gLoadedJobArchives;
         archive != nullptr;
         archive = archive->next) {
        const std::uint32_t firstId =
            ReadU16(archive->data + kJobRecordsOffset + kJobIdOffset);
        const std::uint32_t count =
            ReadU16(archive->data + kJobCountOffset);
        if (classId >= firstId && classId < firstId + count) {
            return archive;
        }
    }
    return nullptr;
}

Job* FindById(std::uint16_t classId) {
    auto* const archive = FindArchiveById(classId);
    if (archive == nullptr) {
        return nullptr;
    }
    const std::uint32_t firstId =
        ReadU16(archive->data + kJobRecordsOffset + kJobIdOffset);
    return JobAt(archive->data, classId - firstId);
}

void RemoveJobArchive(const char* archiveName, bool destroyArchive) {
    using namespace fates::decomp_detail;
    LoadedTableArchive* const node =
        UnlinkAdditionalArchiveByName(gLoadedJobArchives, archiveName);
    if (node == nullptr) {
        return;
    }
    if (destroyArchive) {
        DestroyArchiveNodeAndOwner(node);
    } else {
        delete node;
    }
}

} // namespace

void Job::Initialize(const void* data) {
    fates::decomp_detail::gLoadedJobArchives =
        fates::decomp_detail::CreateLoadedTableArchive(data);
}

void Job::LoadArchive(const ArchiveFile* archive) {
    using namespace fates::decomp_detail;
    auto* const node = CreateLoadedTableArchive(
        GetArchiveTablePayload(archive),
        const_cast<ArchiveFile*>(archive));
    AppendLoadedTableArchive(gLoadedJobArchives, node);
}

ArchiveFile* Job::GetArchive(
    char* archiveName,
    int archiveNameCapacity,
    std::uint16_t classId) {
    auto* const node = FindArchiveById(classId);
    if (node == nullptr) {
        return nullptr;
    }
    fates::decomp_detail::CopyArchiveName(
        node,
        archiveName,
        archiveNameCapacity);
    return node->archive;
}

Job* Job::GetFromNoPrefix(const char* identifierWithoutPrefix) {
    if (identifierWithoutPrefix == nullptr) {
        return nullptr;
    }
    char identifier[0x48]{};
    std::snprintf(
        identifier,
        0x40,
        "%s%s",
        kJobIdentifierPrefix,
        identifierWithoutPrefix);
    return static_cast<Job*>(
        fates::decomp_detail::gJobIdentHash->GetSurely(identifier));
}

Job* Job::Get(const char* identifier) {
    return static_cast<Job*>(
        fates::decomp_detail::gJobIdentHash->GetSurely(identifier));
}

Job* Job::Get(std::uint16_t classId) {
    Job* const candidate = FindById(classId);
    if (candidate != nullptr) {
        return candidate;
    }
    return JobAt(fates::decomp_detail::gLoadedJobArchives->data, 0);
}

Job* Job::TryGet(const char* identifier) {
    IdentHashEntry* const entry =
        fates::decomp_detail::gJobIdentHash->GetIdent(identifier);
    return entry != nullptr ? static_cast<Job*>(entry->value) : nullptr;
}

Job* Job::TryGet(std::uint16_t classId) {
    return FindById(classId);
}

bool Job::IsExist(const char* identifier) {
    return TryGet(identifier) != nullptr;
}

bool Job::IsExist(std::uint16_t classId) {
    return TryGet(classId) != nullptr;
}

void Job::FreeArchive(const char* archiveName) {
    RemoveJobArchive(archiveName, false);
}

void Job::Free(const char* archiveName) {
    RemoveJobArchive(archiveName, true);
}

void Job::Load(const char* archiveName) {
    if (archiveName == nullptr) {
        return;
    }

    char path[0x40]{};
    std::snprintf(
        path,
        sizeof(path),
        "GameData/Job/%s.bin.lz",
        archiveName);

    ArchiveFile* const archive =
        fates::decomp_detail::OpenArchiveFile(path);
    LoadArchive(archive);
}

bool Job::IsLoad(const char* archiveName) {
    return fates::decomp_detail::FindLoadedArchiveByName(
               fates::decomp_detail::gLoadedJobArchives,
               archiveName) != nullptr;
}

void Job::Finalize() {
    using namespace fates::decomp_detail;

    LoadedTableArchive*& head = gLoadedJobArchives;
    if (head == nullptr) {
        return;
    }

    while (head->next != nullptr) {
        LoadedTableArchive* const node = head->next;
        head->next = node->next;
        DestroyArchiveNodeAndOwner(node);
    }

    delete head;
    head = nullptr;
}
