#include "fates/game/item.hpp"

#include "fates/detail/archive_table_runtime.hpp"
#include "fates/detail/file_runtime.hpp"
#include "fates/engine/ident_hash.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {

constexpr std::size_t kItemCountOffset = 0x06;
constexpr std::size_t kItemRecordsOffset = 0x08;
constexpr std::size_t kItemIdOffset = 0x14;
constexpr std::size_t kItemRecordSize = 0x68;
constexpr const char* kItemIdentifierPrefix = "IID_";

std::uint16_t ReadU16(const std::byte* address) {
    std::uint16_t value{};
    std::memcpy(&value, address, sizeof(value));
    return value;
}

Item* ItemAt(std::byte* table, std::uint32_t index) {
    return reinterpret_cast<Item*>(
        table + kItemRecordsOffset + index * kItemRecordSize);
}

fates::decomp_detail::LoadedTableArchive* FindArchiveById(
    std::uint16_t itemId) {
    for (auto* archive = fates::decomp_detail::gLoadedItemArchives;
         archive != nullptr;
         archive = archive->next) {
        const std::uint32_t firstId =
            ReadU16(archive->data + kItemRecordsOffset + kItemIdOffset);
        const std::uint32_t count =
            ReadU16(archive->data + kItemCountOffset);
        if (itemId >= firstId && itemId < firstId + count) {
            return archive;
        }
    }
    return nullptr;
}

Item* FindById(std::uint16_t itemId) {
    auto* const archive = FindArchiveById(itemId);
    if (archive == nullptr) {
        return nullptr;
    }
    const std::uint32_t firstId =
        ReadU16(archive->data + kItemRecordsOffset + kItemIdOffset);
    return ItemAt(archive->data, itemId - firstId);
}

void RemoveItemArchive(const char* archiveName, bool destroyArchive) {
    using namespace fates::decomp_detail;
    LoadedTableArchive* const node =
        UnlinkAdditionalArchiveByName(gLoadedItemArchives, archiveName);
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

void Item::Initialize(const void* data) {
    fates::decomp_detail::gLoadedItemArchives =
        fates::decomp_detail::CreateLoadedTableArchive(data);
}

void Item::LoadArchive(const ArchiveFile* archive) {
    using namespace fates::decomp_detail;
    auto* const node = CreateLoadedTableArchive(
        GetArchiveTablePayload(archive),
        const_cast<ArchiveFile*>(archive));
    AppendLoadedTableArchive(gLoadedItemArchives, node);
}

ArchiveFile* Item::GetArchive(
    char* archiveName,
    int archiveNameCapacity,
    std::uint16_t itemId) {
    auto* const node = FindArchiveById(itemId);
    if (node == nullptr) {
        return nullptr;
    }
    fates::decomp_detail::CopyArchiveName(
        node,
        archiveName,
        archiveNameCapacity);
    return node->archive;
}

Item* Item::GetFromNoPrefix(const char* identifierWithoutPrefix) {
    if (identifierWithoutPrefix == nullptr) {
        return nullptr;
    }
    char identifier[0x48]{};
    std::snprintf(
        identifier,
        0x40,
        "%s%s",
        kItemIdentifierPrefix,
        identifierWithoutPrefix);
    return static_cast<Item*>(
        fates::decomp_detail::gItemIdentHash->GetSurely(identifier));
}

Item* Item::Get(const char* identifier) {
    return static_cast<Item*>(
        fates::decomp_detail::gItemIdentHash->GetSurely(identifier));
}

Item* Item::Get(std::uint16_t itemId) {
    Item* const candidate = FindById(itemId);
    if (candidate != nullptr) {
        return candidate;
    }
    return ItemAt(fates::decomp_detail::gLoadedItemArchives->data, 0);
}

Item* Item::TryGet(std::uint16_t itemId) {
    return FindById(itemId);
}

bool Item::IsExist(const char* identifier) {
    IdentHashEntry* const entry =
        fates::decomp_detail::gItemIdentHash->GetIdent(identifier);
    return entry != nullptr && entry->value != nullptr;
}

bool Item::IsExist(std::uint16_t itemId) {
    return TryGet(itemId) != nullptr;
}

void Item::FreeArchive(const char* archiveName) {
    RemoveItemArchive(archiveName, false);
}

void Item::Free(const char* archiveName) {
    RemoveItemArchive(archiveName, true);
}

void Item::Load(const char* archiveName) {
    if (archiveName == nullptr) {
        return;
    }

    char path[0x40]{};
    std::snprintf(
        path,
        sizeof(path),
        "GameData/Item/%s.bin.lz",
        archiveName);

    ArchiveFile* const archive =
        fates::decomp_detail::OpenArchiveFile(path);
    LoadArchive(archive);
}

bool Item::IsLoad(const char* archiveName) {
    return fates::decomp_detail::FindLoadedArchiveByName(
               fates::decomp_detail::gLoadedItemArchives,
               archiveName) != nullptr;
}

void Item::Finalize() {
    using namespace fates::decomp_detail;

    LoadedTableArchive*& head = gLoadedItemArchives;
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
