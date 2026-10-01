#pragma once
#include "fates/runtime/native_game_state.hpp"
#include "fates/campaign/native_chapter_complete_advance.hpp"
#include <cstdint>

namespace fates::runtime::native {

struct CompletionChapterDefinition {
    std::uint8_t index{};
    std::uint8_t type{};
    std::uint8_t next_birthright{};
    std::uint8_t next_conquest{};
    std::uint8_t next_revelation{};
    std::uint8_t requirement_birthright{};
    std::uint8_t requirement_conquest{};
    std::uint8_t requirement_revelation{};
    std::uint8_t route_mask{};
    std::int8_t offspring_seal_level_retail_signed{};
    std::uint8_t raw_chapter_0x15{};
    bool cid_ending{};
};

struct CompletionLifecycleContext {
    std::uint8_t raw_sequence_state{};
    bool map_missing{};
    bool special_dispos_sortie_mode{};
    bool encounter_context{};
    bool has_versus_config{};
    bool purchase_route_failed{};
    bool global_flag_0x400{};
    bool has_global_process{};
    std::uint8_t chapter_context_byte{};
    std::uint16_t current_turn{};
    std::int32_t total_counter{};
    std::int32_t chapter_start_counter{};
};

enum class CompletionLifecycleStatus : std::uint8_t {
    NoCompletion,
    SpecialSequenceRoute,
    NeedsSpecialSortieRestore,
    NeedsTransporterReplacementPolicy,
    NeedsWorldMobUpdate,
    GameOverCommitted,
    CompleteCampaignExit,
    CompleteSaveRedirect,
    CompleteSaveFacing,
};

struct CompletionLifecycleResult {
    CompletionLifecycleStatus status{CompletionLifecycleStatus::NoCompletion};
    fates::chapter::native::MapSequenceEndRoute map_route{fates::chapter::native::MapSequenceEndRoute::Continue};
    fates::chapter::native::ChapterMapEndRoute chapter_route{fates::chapter::native::ChapterMapEndRoute::NormalLabel9};
    fates::campaign::native::ChapterCompleteRoute campaign_route{fates::campaign::native::ChapterCompleteRoute::ReturnNoAdvance};
    bool persistent_commit_applied{};
    bool chapter_record_appended{};
    std::uint8_t next_chapter_index{};
    int save_jump_label{-1};
    bool save_menu_requested{};
    bool backup_write_boundary_exposed{};
};

// Applies the already-proven Pass70-72 semantics in retail lifecycle order.
// The function is atomic with respect to unsupported Transporter replacement,
// special-sortie restore, and world-Mob subtransactions: those conditions are
// detected before state mutation and returned as explicit boundaries.
CompletionLifecycleResult ApplyCompletionLifecycle(
    NativeGameState& state,
    const CompletionChapterDefinition& current_chapter,
    const CompletionChapterDefinition* selected_next_chapter,
    const CompletionLifecycleContext& context);

} // namespace fates::runtime::native
