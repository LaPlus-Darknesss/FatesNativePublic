#include "fates/event/backend/backend_spine.hpp"

namespace fates::event::backend {

std::int32_t FlagEntryForward(FlagNameTableView table, const char* name) {
    if (table.names == nullptr) return -1;
    for (std::size_t i = 0; i < table.count; ++i) {
        if (table.names[i] == nullptr) { table.names[i] = name; return static_cast<std::int32_t>(i); }
    }
    return -1;
}

std::int32_t FlagEntryReverse(FlagNameTableView table, const char* name) {
    if (table.names == nullptr) return -1;
    for (std::size_t i = table.count; i > 0; --i) {
        const std::size_t index = i - 1;
        if (table.names[index] == nullptr) { table.names[index] = name; return static_cast<std::int32_t>(index); }
    }
    return -1;
}

bool FlagTest(const std::uint8_t* bits, std::size_t bit_count, std::int32_t index) {
    if (bits == nullptr || index < 0 || static_cast<std::size_t>(index) >= bit_count) return false;
    const auto u = static_cast<std::uint32_t>(index);
    return (bits[u >> 3] & static_cast<std::uint8_t>(1u << (u & 7u))) != 0;
}

bool FlagSet(std::uint8_t* bits, std::size_t bit_count, std::int32_t index) {
    if (bits == nullptr || index < 0 || static_cast<std::size_t>(index) >= bit_count) return false;
    const auto u = static_cast<std::uint32_t>(index);
    bits[u >> 3] |= static_cast<std::uint8_t>(1u << (u & 7u));
    return true;
}

bool FlagClear(std::uint8_t* bits, std::size_t bit_count, std::int32_t index) {
    if (bits == nullptr || index < 0 || static_cast<std::size_t>(index) >= bit_count) return false;
    const auto u = static_cast<std::uint32_t>(index);
    bits[u >> 3] &= static_cast<std::uint8_t>(~(1u << (u & 7u)));
    return true;
}

} // namespace fates::event::backend
