#include "fates/engine/ident_hash.hpp"
#include "fates/detail/ident_hash_runtime.hpp"
#include "fates/runtime/native_identifier.hpp"

#include <cstdint>

using fates::runtime::native::IdentifierHashes;
using fates::runtime::native::HashIdentifierExact;


IdentHash::IdentHash(int bucketCountValue)
    : bucketCount(bucketCountValue > 0 ? static_cast<std::uint32_t>(bucketCountValue) : 0),
      entryCount(0),
      entryPool(fates::decomp_detail::GetGlobalIdentHashEntryPool()),
      buckets(bucketCount != 0 ? new IdentHashEntry*[bucketCount]{} : nullptr) {}

IdentHashEntry* IdentHash::GetIdent(const char* identifier) const {
    if (identifier == nullptr || bucketCount == 0 || buckets == nullptr) {
        return nullptr;
    }

    const IdentifierHashes hashes = HashIdentifierExact(identifier);
    for (IdentHashEntry* entry = buckets[hashes.bucketHash % bucketCount];
         entry != nullptr;
         entry = entry->next) {
        // Retail compares only the stored base-31 hash here. The identifier
        // pointer is retained in the entry but is not strcmp-checked.
        if (entry->nameHash == hashes.nameHash) {
            return entry;
        }
    }
    return nullptr;
}

void* IdentHash::GetSurely(const char* identifier) const {
    IdentHashEntry* const entry = GetIdent(identifier);
    return entry != nullptr ? entry->value : nullptr;
}

void IdentHash::Set(const char* identifier, void* value) {
    if (identifier == nullptr || bucketCount == 0 ||
        buckets == nullptr || entryPool == nullptr) {
        return;
    }

    const IdentifierHashes hashes = HashIdentifierExact(identifier);
    const std::uint32_t bucketIndex = hashes.bucketHash % bucketCount;

    IdentHashEntry* entry = entryPool->freeList;
    // Retail assumes the pool has an available entry.
    if (entry == nullptr) {
        return;
    }
    entryPool->freeList = entry->next;

    entry->next = nullptr;
    entry->nameHash = hashes.nameHash;
    entry->identifier = identifier;
    entry->value = value;

    if (buckets[bucketIndex] == nullptr) {
        buckets[bucketIndex] = entry;
    } else {
        IdentHashEntry* tail = buckets[bucketIndex];
        while (tail->next != nullptr) {
            tail = tail->next;
        }
        tail->next = entry;
    }
    ++entryCount;
}

void IdentHash::Delete(const char* identifier) {
    if (identifier == nullptr || bucketCount == 0 ||
        buckets == nullptr || entryPool == nullptr) {
        return;
    }

    const IdentifierHashes hashes = HashIdentifierExact(identifier);
    const std::uint32_t bucketIndex = hashes.bucketHash % bucketCount;

    IdentHashEntry* previous = nullptr;
    IdentHashEntry* entry = buckets[bucketIndex];
    while (entry != nullptr && entry->nameHash != hashes.nameHash) {
        previous = entry;
        entry = entry->next;
    }

    if (entry != nullptr) {
        if (previous == nullptr) {
            buckets[bucketIndex] = entry->next;
        } else {
            previous->next = entry->next;
        }

        entry->value = nullptr;
        entry->next = entryPool->freeList;
        entryPool->freeList = entry;
    }

    // This intentionally preserves the exact retail quirk at 0x0013080C:
    // a non-null identifier decrements the count even if no matching entry
    // was found. It may be a caller-contract assumption, but hiding it would
    // make the reconstruction behaviorally cleaner than the game.
    --entryCount;
}

void IdentHash::Clear() {
    if (buckets == nullptr || entryPool == nullptr) {
        entryCount = 0;
        return;
    }

    for (std::uint32_t index = 0; index < bucketCount; ++index) {
        IdentHashEntry* entry = buckets[index];
        while (entry != nullptr) {
            IdentHashEntry* const next = entry->next;
            entry->next = entryPool->freeList;
            entry->value = nullptr;
            entryPool->freeList = entry;
            entry = next;
        }
        buckets[index] = nullptr;
    }
    entryCount = 0;
}
