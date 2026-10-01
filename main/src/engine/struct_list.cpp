#include "fates/engine/struct_list.hpp"

#include "fates/detail/metadata_runtime.hpp"
#include "fates/detail/struct_list_runtime.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

StructList::StructList() = default;

bool StructList::Load(
    const char* path,
    const char* objectName,
    int baseIndex,
    unsigned int recordSize) {
    auto* entry = new Entry{};
    entry->archiveHandle =
        fates::decomp_detail::OpenStructListArchive(path);

    if (entry->archiveHandle == nullptr) {
        delete entry;
        return false;
    }

    const char* selectedName = objectName;
    if (selectedName == nullptr) {
        selectedName =
            fates::decomp_detail::GetDefaultStructListObjectName(
                entry->archiveHandle);
    }

    entry->table = static_cast<const Table32*>(
        fates::decomp_detail::GetSurelyUniqueArchiveObject(
            entry->archiveHandle,
            selectedName));
    entry->baseIndex = baseIndex;

    if (entry->table == nullptr ||
        entry->table->recordSize != recordSize) {
        fates::decomp_detail::DestroyStructListArchive(
            entry->archiveHandle);
        delete entry;
        return false;
    }

    entry->owner = this;
    entry->previous = tail_;
    if (tail_ != nullptr) {
        tail_->next = entry;
    }
    if (head_ == nullptr) {
        head_ = entry;
    }
    tail_ = entry;
    ++archiveCount_;

    Update();
    return true;
}

void StructList::Free(int baseIndex) {
    bool changed = false;
    Entry* entry = head_;

    while (entry != nullptr) {
        Entry* const next = entry->next;
        if (entry->baseIndex == baseIndex) {
            if (entry->previous != nullptr) {
                entry->previous->next = entry->next;
            }
            if (entry->next != nullptr) {
                entry->next->previous = entry->previous;
            }
            if (head_ == entry) {
                head_ = entry->next;
            }
            if (tail_ == entry) {
                tail_ = entry->previous;
            }

            entry->previous = nullptr;
            entry->next = nullptr;
            entry->owner = nullptr;
            --archiveCount_;

            fates::decomp_detail::DestroyStructListArchive(
                entry->archiveHandle);
            delete entry;
            changed = true;
        }
        entry = next;
    }

    if (changed) {
        Update();
    }
}

void StructList::Update() {
    delete[] index_;
    index_ = nullptr;
    indexedCount_ = 0;

    for (Entry* entry = head_; entry != nullptr; entry = entry->next) {
        if (entry->table == nullptr) {
            continue;
        }
        indexedCount_ = std::max(
            indexedCount_,
            entry->baseIndex + static_cast<int>(entry->table->count));
    }

    if (indexedCount_ <= 0) {
        return;
    }

    index_ = new void*[static_cast<std::size_t>(indexedCount_)]{};

    for (Entry* entry = head_; entry != nullptr; entry = entry->next) {
        const Table32* const table = entry->table;
        if (table == nullptr || table->count == 0) {
            continue;
        }

        auto* records = fates::decomp_detail::MutableTargetPointer<std::byte>(
            table->records);
        for (std::uint32_t i = 0; i < table->count; ++i) {
            index_[entry->baseIndex + static_cast<int>(i)] =
                records + static_cast<std::size_t>(table->recordSize) * i;
        }
    }
}

void StructList::Dump() const {
    // The retail release body only walks the list; debug printing was removed.
    for (Entry* entry = head_; entry != nullptr; entry = entry->next) {
        (void)entry;
    }
}

void* StructList::TryGet(const char* name) const {
    for (Entry* entry = head_; entry != nullptr; entry = entry->next) {
        if (entry->archiveHandle == nullptr) {
            continue;
        }
        if (void* value =
                fates::decomp_detail::TryGetUniqueArchiveObject(
                    entry->archiveHandle,
                    name)) {
            return value;
        }
    }
    return nullptr;
}

void* StructList::TryGet(int index) const {
    if (index < 0 || index >= indexedCount_ || index_ == nullptr) {
        return nullptr;
    }
    return index_[index];
}

std::uint32_t StructList::GetIndex(const void* record) const {
    for (int i = 0; i < indexedCount_; ++i) {
        if (index_ != nullptr && index_[i] == record) {
            return static_cast<std::uint32_t>(i);
        }
    }
    return 0xFFFFFFFFu;
}

void* StructList::GetSurely(const char* name) const {
    // Retail release code is identical to TryGet; assertion behavior is absent.
    return TryGet(name);
}

void* StructList::GetSurely(int index) const {
    // Retail release code is identical to TryGet; assertion behavior is absent.
    return TryGet(index);
}

namespace fates::decomp_detail {

void LoadStructList(
    StructList*& list,
    const char* path,
    const char* tableName,
    int flags,
    unsigned int recordSize) {
    if (list == nullptr) {
        list = new StructList();
    }
    list->Load(path, tableName, flags, recordSize);
}

void FreeStructList(StructList* list, int flags) {
    if (list != nullptr) {
        list->Free(flags);
    }
}

int GetStructListCount(const StructList* list) {
    return list != nullptr ? list->GetCount() : 0;
}

void* GetStructListSurely(StructList* list, int index) {
    return list != nullptr ? list->GetSurely(index) : nullptr;
}

void DumpStructList(const StructList* list) {
    if (list != nullptr) {
        list->Dump();
    }
}

} // namespace fates::decomp_detail
