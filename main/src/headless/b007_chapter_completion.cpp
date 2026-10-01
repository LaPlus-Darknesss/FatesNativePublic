#include "fates/headless/b007_chapter_completion.hpp"

namespace fates::headless {

B007EnemyCountCompletion EvaluateB007EnemyCountCompletion(
    const int non_allied_units_in_forces_0_to_2,
    const bool completion_guard_already_set,
    const bool versus_mode) noexcept {
    B007EnemyCountCompletion out{};
    // B007 sets ChapterSetWinRuleEnemyNumberLessThanOrEqualTo(0).
    if (!versus_mode && non_allied_units_in_forces_0_to_2 <= 0 &&
        !completion_guard_already_set) {
        out.triggers = true;
        out.retail_result_code = 4;
        out.completion_guard_becomes_set = true;
    }
    return out;
}

B007CompletionSnapshot ComposeB007Completion(
    const B007PlayerSex sex,
    const B007Event14PlacementCase placement) noexcept {
    B007CompletionSnapshot out{};
    out.exact = true;
    out.player_sex = sex;
    out.companion = sex == B007PlayerSex::Male ? B007Companion::Felicia : B007Companion::Jakob;
    out.begin_cleanup_forces = {0u, 1u, 2u};
    out.begin_cleanup_preserves_unit_pool_identity = true;
    out.event11_mode = 0x804u;
    out.companion_event_mode = 0x804u;
    out.event14_mode = 0u;
    out.player_final = {4, 20};
    out.elise_final = {4, 21};
    out.silas_final = {4, 21};
    out.companion_final = {4, 21};
    out.system_rng_draws = 11u;
    out.game_rng_draws = 0u;
    out.placement_rng_draws = 0u;
    if (placement == B007Event14PlacementCase::PreferredFree) {
        out.event14_final = {15, 12};
        out.event14_actor_start = {20, 14};
    } else if (placement == B007Event14PlacementCase::StrictFallback) {
        out.event14_final = {15, 13};
        out.event14_actor_start = {20, 14};
    } else {
        out.event14_final = {15, 13};
        out.event14_actor_start = {15, 13};
    }
    out.dispos_wait_after_gameplay_commit = true;
    out.map_event_end_restores_hp_gauge = true;
    out.map_event_end_restores_tricks = true;
    out.map_event_end_ends_event_camera = true;
    out.function_terminal_reached = true;
    return out;
}

} // namespace fates::headless
