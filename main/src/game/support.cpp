#include "fates/game/support.hpp"

#include "fates/detail/support_runtime.hpp"
#include "fates/game/person.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace fates::decomp_detail {
namespace {

struct SupportBucketNode {
    const std::byte* table{};
    SupportBucketNode* next{};
};

std::array<SupportBucketNode*, kSupportBucketCount> gSupportBuckets{};

std::int16_t ReadI16(const void* base, std::size_t offset) {
    std::int16_t value{};
    std::memcpy(
        &value,
        static_cast<const std::byte*>(base) + offset,
        sizeof(value));
    return value;
}

std::int32_t ReadI32(const void* base, std::size_t offset) {
    std::int32_t value{};
    std::memcpy(
        &value,
        static_cast<const std::byte*>(base) + offset,
        sizeof(value));
    return value;
}

std::uint16_t ReadU16(const void* base, std::size_t offset) {
    std::uint16_t value{};
    std::memcpy(
        &value,
        static_cast<const std::byte*>(base) + offset,
        sizeof(value));
    return value;
}

const std::byte* GetSupportSubtable(
    const void* mainSupportTable,
    std::int32_t index) {
    const Arm32Address address = ReadArm32Address(
        mainSupportTable,
        4 + static_cast<std::size_t>(index) * sizeof(Arm32Address));
    return TargetPointer<const std::byte>(address);
}

std::size_t SupportBucketFor(const std::byte* table) {
    return static_cast<std::size_t>(ReadU16(table, 0) & 0x7Fu);
}

std::int16_t PersonSupportId(const Person* person) {
    // Person.yml calls this exact +0x30 field support_id.
    return ReadI16(person, 0x30);
}

std::uint16_t PersonId(const Person* person) {
    // Person.yml and Person::Get(unsigned short) independently constrain +0x24.
    return ReadU16(person, 0x24);
}

std::uint16_t SupportTableOwner(const std::byte* table) {
    return ReadU16(table, 0x00);
}

std::uint16_t SupportTableCount(const std::byte* table) {
    return ReadU16(table, 0x02);
}

const SupportRecord* SupportAt(
    const std::byte* table,
    std::ptrdiff_t index) {
    return reinterpret_cast<const SupportRecord*>(
        table + 0x04 + index * static_cast<std::ptrdiff_t>(sizeof(SupportRecord)));
}

SupportBucketNode* FirstBucketNode(std::int16_t supportId) {
    if (supportId < 0) {
        return nullptr;
    }
    return gSupportBuckets[
        static_cast<std::uint16_t>(supportId) & 0x7Fu];
}

} // namespace

void InitializeSupportSystem() {
    // Retail Reliance::Initialize allocates 0x200 bytes: exactly 128 ARM32
    // bucket-head pointers, then clears all of them.
    gSupportBuckets.fill(nullptr);
}

void LoadSupportTable(const void* supportTable) {
    if (supportTable == nullptr) {
        return;
    }

    const std::int32_t count = ReadI32(supportTable, 0);
    for (std::int32_t index = 0; index < count; ++index) {
        const std::byte* const table = GetSupportSubtable(supportTable, index);
        if (table == nullptr) {
            continue;
        }

        auto* node = new SupportBucketNode{table, nullptr};
        SupportBucketNode*& head = gSupportBuckets[SupportBucketFor(table)];
        if (head == nullptr) {
            head = node;
            continue;
        }

        SupportBucketNode* tail = head;
        while (tail->next != nullptr) {
            tail = tail->next;
        }
        tail->next = node;
    }
}

void FreeSupportTable(const void* supportTable) {
    if (supportTable == nullptr) {
        return;
    }

    const std::int32_t count = ReadI32(supportTable, 0);
    for (std::int32_t index = 0; index < count; ++index) {
        const std::byte* const table = GetSupportSubtable(supportTable, index);
        if (table == nullptr) {
            continue;
        }

        SupportBucketNode*& head = gSupportBuckets[SupportBucketFor(table)];
        SupportBucketNode* previous = nullptr;
        SupportBucketNode* node = head;
        while (node != nullptr) {
            if (node->table == table) {
                if (previous == nullptr) {
                    head = node->next;
                } else {
                    previous->next = node->next;
                }
                delete node;
                break;
            }
            previous = node;
            node = node->next;
        }
    }
}

void FinalizeSupportSystem() {
    for (SupportBucketNode*& head : gSupportBuckets) {
        while (head != nullptr) {
            SupportBucketNode* const next = head->next;
            delete head;
            head = next;
        }
    }
}

int GetSupportCountForPerson(const Person* person, bool selfOnly) {
    const std::int16_t supportId = PersonSupportId(person);
    if (supportId < 0) {
        return 0;
    }

    int total = 0;
    for (SupportBucketNode* node = FirstBucketNode(supportId);
         node != nullptr;
         node = node->next) {
        const std::byte* const table = node->table;
        if (SupportTableOwner(table) !=
            static_cast<std::uint16_t>(supportId)) {
            continue;
        }

        // GetSelfNum checks the first byte of the first Support record's tag
        // at table +0x0C. Paragon independently identifies Support::tag as the
        // final 4 bytes of each 12-byte record.
        if (!selfOnly ||
            static_cast<std::uint8_t>(table[0x0C]) != 0) {
            total += SupportTableCount(table);
        }
    }
    return total;
}

const SupportRecord* GetSupportByIndex(
    const Person* person,
    int requestedIndex) {
    const std::int16_t supportId = PersonSupportId(person);
    if (supportId < 0) {
        return nullptr;
    }

    int index = requestedIndex;
    for (SupportBucketNode* node = FirstBucketNode(supportId);
         node != nullptr;
         node = node->next) {
        const std::byte* const table = node->table;
        if (SupportTableOwner(table) !=
            static_cast<std::uint16_t>(supportId)) {
            continue;
        }

        const int count = SupportTableCount(table);
        if (index < count) {
            return SupportAt(table, static_cast<std::ptrdiff_t>(index));
        }
        index -= count;
    }
    return nullptr;
}

const SupportRecord* GetSupportBetweenPeople(
    const Person* owner,
    const Person* other) {
    const std::int16_t supportId = PersonSupportId(owner);
    if (supportId < 0) {
        return nullptr;
    }

    const std::uint16_t targetPersonId = PersonId(other);

    for (SupportBucketNode* node = FirstBucketNode(supportId);
         node != nullptr;
         node = node->next) {
        const std::byte* const table = node->table;
        if (SupportTableOwner(table) !=
            static_cast<std::uint16_t>(supportId)) {
            continue;
        }

        const std::uint32_t count = SupportTableCount(table);
        // Retail emits an odd-first / paired unrolled search. A linear walk
        // preserves the same observable first-match ordering.
        for (std::uint32_t index = 0; index < count; ++index) {
            const SupportRecord* const support = SupportAt(table, static_cast<std::ptrdiff_t>(index));
            if (support->characterId == targetPersonId) {
                return support;
            }
        }
    }

    return nullptr;
}

} // namespace fates::decomp_detail

int Reliance::GetSelfNum(const Person* person) {
    return fates::decomp_detail::GetSupportCountForPerson(person, true);
}

const SupportRecord* Reliance::GetFromPerson(
    const Person* owner,
    const Person* other) {
    return fates::decomp_detail::GetSupportBetweenPeople(owner, other);
}

const SupportRecord* Reliance::Get(
    const Person* person,
    int index) {
    return fates::decomp_detail::GetSupportByIndex(person, index);
}

int Reliance::GetNum(const Person* person) {
    return fates::decomp_detail::GetSupportCountForPerson(person, false);
}
