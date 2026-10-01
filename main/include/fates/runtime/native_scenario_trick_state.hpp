#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace fates::runtime::native {
struct ScenarioScriptEvidence {
    std::string identity;
    std::vector<std::uint8_t> original_bytes;
};
struct ScenarioTrickRecord {
    // Exact constructor byte order for retail +0x08..+0x16. The repeated
    // initial/current value is preserved; no guessed names for unknown fields.
    std::array<std::uint8_t,15> fields{};
    std::uint16_t archive_index{},function_index{};
    std::uint8_t event_type{};
    std::string label;
};
struct NativeScenarioTrickState {
    bool bound{};
    std::uint8_t chapter{},difficulty{};
    std::uint64_t phase_revision{},difficulty_revision{};
    std::uint32_t user_flags{};
    std::array<std::uint8_t,3> control{};
    std::array<std::uint32_t,6> map_bounds{};
    std::array<std::uint8_t,1024> terrain_grid{};
    std::vector<ScenarioScriptEvidence> attached_archives;
    std::vector<ScenarioTrickRecord> registry; // head-to-tail retail list order
};
} // namespace fates::runtime::native
