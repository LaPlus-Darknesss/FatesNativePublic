#pragma once

#include "fates/detail/arm32_address.hpp"

#include <cstddef>
#include <cstdint>

class StructList {
public:
    StructList();

    bool Load(
        const char* path,
        const char* objectName,
        int baseIndex,
        unsigned int recordSize);
    void Free(int baseIndex);
    void Update();

    void Dump() const;
    void* TryGet(const char* name) const;
    void* TryGet(int index) const;
    std::uint32_t GetIndex(const void* record) const;
    void* GetSurely(const char* name) const;
    void* GetSurely(int index) const;

    int GetCount() const { return indexedCount_; }

private:
    struct Table32 {
        fates::decomp_detail::Arm32Address unknown00{};
        std::uint16_t count{};
        std::uint16_t recordSize{};
        fates::decomp_detail::Arm32Address records{};
    };

    struct Entry {
        void* archiveHandle{};
        Entry* previous{};
        Entry* next{};
        StructList* owner{};
        const Table32* table{};
        int baseIndex{};
    };

    Entry* head_{};
    Entry* tail_{};
    int archiveCount_{};
    int indexedCount_{};
    void** index_{};
};
