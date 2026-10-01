#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace fates::headless::native {

inline constexpr std::size_t kMaxCmvmFunctionsInProbe = 32;

struct CmvmFunctionEntryProbe {
    std::uint32_t record_offset{};
    std::uint32_t code_offset{};
    std::uint8_t type{};
    std::uint8_t argc{};
    std::uint16_t local_words{};
    std::uint8_t first_opcode{};
    bool first_opcode_owned{};
};

struct CmvmArchiveProbe {
    bool certified{};
    std::uint32_t version{};
    std::uint32_t function_table_offset{};
    std::size_t function_count{};
    std::size_t native_enterable_function_count{};
    std::array<CmvmFunctionEntryProbe, kMaxCmvmFunctionsInProbe> functions{};
};

[[nodiscard]] bool IsNativeCmvmEntryOpcodeOwned(std::uint8_t opcode) noexcept;
[[nodiscard]] CmvmArchiveProbe ProbeCmvmArchive(std::span<const std::byte> archive) noexcept;

struct SemanticReadinessInput {
    std::uint32_t user_flags{};
    int requested_mode{};
    int x0{};
    int y0{};
    int x1{};
    int y1{};
    std::uint32_t terrain_cost_width{};
    std::uint64_t dragon_vein_private_mask{};
    std::uint64_t unit_category_mask{};
    bool dragon_vein_block_flag{};
    bool person_is_download{};
    std::array<bool,5> attack_can_equip{};
    std::array<bool,5> attack_excluded{};
    std::uint8_t ai_priority{};
    int ai_move_power{};
    int job_hit{};
    int item_hit{};
    int tech{};
    int luck{};
    int weapon_rank_hit_bonus{};
    bool hit_plus_10{};
    int requested_hp{};
    int max_hp{};
    int proc_roll{};
    int proc_probability{};
    std::uint16_t proc_skill_id{};
};

struct SemanticReadinessSnapshot {
    std::uint32_t encoded_mode_flags{};
    int decoded_mode{};
    int manhattan_distance{};
    std::size_t terrain_cost_stride{};
    bool can_use_dragon_vein{};
    std::uint8_t attack_item_mask{};
    std::uint32_t ai_priority_score{};
    int hit{};
    int clamped_hp{};
    bool battle_proc_success{};
};

[[nodiscard]] SemanticReadinessSnapshot RunSemanticReadinessProbe(const SemanticReadinessInput& input) noexcept;

} // namespace fates::headless::native
