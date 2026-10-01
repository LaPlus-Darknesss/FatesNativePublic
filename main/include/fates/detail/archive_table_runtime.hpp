#pragma once

#include "fates/detail/arm32_address.hpp"
#include "fates/detail/file_runtime.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>

class ArchiveFile;
class IdentHash;

namespace fates::decomp_detail {

// Shared source-level node used by the Person, retail Job (localized class),
// and Item table chains. Retail ARM32 layout is exactly three words:
//
//   +0x00 table data
//   +0x04 associated ArchiveFile*
//   +0x08 next node
//
// This is deliberately a readable host-source model rather than a native
// overlay. The exact 12-byte retail representation remains in evidence/tests.
struct LoadedTableArchive {
    std::byte* data{};
    ArchiveFile* archive{};
    LoadedTableArchive* next{};
};

inline LoadedTableArchive* CreateLoadedTableArchive(
    const void* data,
    ArchiveFile* archive = nullptr) {
    auto* node = new LoadedTableArchive{};
    node->data = const_cast<std::byte*>(static_cast<const std::byte*>(data));
    node->archive = archive;
    return node;
}

inline void AppendLoadedTableArchive(
    LoadedTableArchive*& head,
    LoadedTableArchive* node) {
    if (head == nullptr) {
        head = node;
        return;
    }

    LoadedTableArchive* tail = head;
    while (tail->next != nullptr) {
        tail = tail->next;
    }
    tail->next = node;
}

inline const char* GetArchiveTableName(const LoadedTableArchive* node) {
    if (node == nullptr || node->data == nullptr) {
        return nullptr;
    }
    return TargetPointer<const char>(ReadArm32Address(node->data, 0));
}

inline bool ArchiveTableNameEquals(
    const LoadedTableArchive* node,
    const char* name) {
    const char* const tableName = GetArchiveTableName(node);
    return tableName != nullptr && name != nullptr &&
           std::strcmp(tableName, name) == 0;
}

inline LoadedTableArchive* FindLoadedArchiveByName(
    LoadedTableArchive* head,
    const char* name) {
    for (auto* node = head; node != nullptr; node = node->next) {
        if (ArchiveTableNameEquals(node, name)) {
            return node;
        }
    }
    return nullptr;
}

// Retail Free/FreeArchive wrappers begin at head->next; the base GameData node
// is never removable through those APIs.
inline LoadedTableArchive* UnlinkAdditionalArchiveByName(
    LoadedTableArchive*& head,
    const char* name) {
    if (head == nullptr) {
        return nullptr;
    }

    LoadedTableArchive* previous = head;
    for (LoadedTableArchive* node = head->next;
         node != nullptr;
         node = node->next) {
        if (ArchiveTableNameEquals(node, name)) {
            previous->next = node->next;
            node->next = nullptr;
            return node;
        }
        previous = node;
    }
    return nullptr;
}

inline void DestroyArchiveNodeAndOwner(LoadedTableArchive* node) {
    if (node == nullptr) {
        return;
    }
    ArchiveFile* const archive = node->archive;
    delete node;
    if (archive != nullptr) {
        DestroyArchiveFileOwner(archive);
    }
}

inline void CopyArchiveName(
    const LoadedTableArchive* node,
    char* destination,
    int destinationSize) {
    if (destination == nullptr || destinationSize <= 0) {
        return;
    }

    const char* const name = GetArchiveTableName(node);
    if (name == nullptr) {
        destination[0] = '\0';
        return;
    }

    const std::size_t capacity = static_cast<std::size_t>(destinationSize);
    std::strncpy(destination, name, capacity);
    destination[capacity - 1] = '\0';
}

// Provisional readable views of retail globals/storage fields. Exact storage
// addresses/forms remain in provenance. They are intentionally kept behind
// detail namespace until the owning manager objects are reconstructed.
extern IdentHash* gPersonIdentHash;
extern IdentHash* gJobIdentHash;
extern IdentHash* gItemIdentHash;

extern LoadedTableArchive* gLoadedPersonArchives;
extern LoadedTableArchive* gLoadedJobArchives;
extern LoadedTableArchive* gLoadedItemArchives;

} // namespace fates::decomp_detail
