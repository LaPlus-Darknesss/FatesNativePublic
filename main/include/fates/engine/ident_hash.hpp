#pragma once

#include <cstddef>
#include <cstdint>

namespace fates::decomp_detail { struct IdentHashEntryPool; }

class IdentHashEntry {
public:
    IdentHashEntry* next{};            // retail +0x00
    std::uint32_t nameHash{};          // retail +0x04, polynomial base 31
    const char* identifier{};          // retail +0x08
    void* value{};                     // retail +0x0C
};

class IdentHash {
public:
    explicit IdentHash(int bucketCount);
    // Exact member signatures come from retail StackTrace data. Return types are
    // reconstructed from ARM behavior because they are not encoded in names.
    IdentHashEntry* GetIdent(const char* identifier) const;
    void* GetSurely(const char* identifier) const;

    void Set(const char* identifier, void* value);
    void Delete(const char* identifier);
    void Clear();

private:
    std::byte unknown_00[0x04]{};
    std::uint32_t bucketCount{};    // retail +0x04
    std::uint32_t entryCount{};     // retail +0x08
    fates::decomp_detail::IdentHashEntryPool* entryPool{}; // retail +0x0C
    IdentHashEntry** buckets{};     // retail +0x10
};
