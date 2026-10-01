#include "fates/event/backend/backend_spine.hpp"
#include <bit>

namespace fates::event::backend {
// Shared by the friendly reconstruction and retained native bank.
std::int32_t VariableFind(const NamedVariableTableView table, std::string_view name) {
    if (table.names == nullptr) return -1;
    for (std::size_t i = 0; i < table.count; ++i) {
        const char* candidate = table.names[i];
        if (candidate != nullptr && name == candidate) return static_cast<std::int32_t>(i);
    }
    return -1;
}
bool VariableSet(NamedVariableTableView table, std::string_view name, std::int32_t value) {
    const std::int32_t index = VariableFind(table, name);
    if (index < 0 || table.values == nullptr) return false;
    table.values[index] = value;
    return true;
}
bool VariableAdd(NamedVariableTableView table, std::string_view name, std::int32_t delta) {
    const std::int32_t index = VariableFind(table, name);
    if (index < 0 || table.values == nullptr) return false;
    const auto sum=static_cast<std::uint32_t>(table.values[index])+static_cast<std::uint32_t>(delta);
    table.values[index]=std::bit_cast<std::int32_t>(sum); // ARM ADD wraps at 32 bits
    return true;
}
std::int32_t VariableGet(NamedVariableTableView table, std::string_view name) {
    const std::int32_t index = VariableFind(table, name);
    return (index < 0 || table.values == nullptr) ? 0 : table.values[index];
}
}
