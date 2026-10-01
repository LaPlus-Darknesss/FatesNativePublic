#pragma once
#include <array>
#include <cstdint>

namespace fates::headless {

enum class B007PlayerSex : std::uint8_t { Male, Female };
enum class B007Event14PlacementCase : std::uint8_t { PreferredFree, StrictFallback, RelaxedFallback };
enum class B007Companion : std::uint8_t { Felicia, Jakob };

struct B007Cell { int x{}; int y{}; };

struct B007EnemyCountCompletion {
    bool triggers{};
    std::uint8_t retail_result_code{};
    bool completion_guard_becomes_set{};
};

// Exact B007 subset of Situation::GameEndCheck: non-versus enemy-count rule.
// The chapter installs threshold 0. Result code 4 is preserved as retail
// provenance; no player-facing enum name is invented here.
[[nodiscard]] B007EnemyCountCompletion EvaluateB007EnemyCountCompletion(
    int non_allied_units_in_forces_0_to_2,
    bool completion_guard_already_set,
    bool versus_mode) noexcept;

struct B007CompletionSnapshot {
    bool exact{};
    B007PlayerSex player_sex{};
    B007Companion companion{};
    std::array<std::uint8_t,3> begin_cleanup_forces{};
    bool begin_cleanup_preserves_unit_pool_identity{};
    std::uint32_t event11_mode{};
    std::uint32_t companion_event_mode{};
    std::uint32_t event14_mode{};
    B007Cell player_final{};
    B007Cell elise_final{};
    B007Cell silas_final{};
    B007Cell companion_final{};
    B007Cell event14_final{};
    B007Cell event14_actor_start{};
    std::uint32_t system_rng_draws{};
    std::uint32_t game_rng_draws{};
    std::uint32_t placement_rng_draws{};
    bool dispos_wait_after_gameplay_commit{};
    bool map_event_end_restores_hp_gauge{};
    bool map_event_end_restores_tricks{};
    bool map_event_end_ends_event_camera{};
    bool function_terminal_reached{};
};

// Composes already-proven Pass64-68 semantics into one portable B007
// completion snapshot. It models the bounded completion vertical, not the
// entire chapter's combat/AI/presentation execution.
[[nodiscard]] B007CompletionSnapshot ComposeB007Completion(
    B007PlayerSex sex,
    B007Event14PlacementCase placement) noexcept;

} // namespace fates::headless
