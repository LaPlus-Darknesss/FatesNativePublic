#pragma once
#include "fates/runtime/native_unit_stat_state.hpp"
#include "fates/runtime/native_person_archives.hpp"
#include "fates/runtime/native_unit_transfer_state.hpp"
#include "fates/runtime/native_unit_map_end.hpp"
#include "fates/runtime/native_turn_state.hpp"
#include "fates/runtime/native_game_user_settings.hpp"
#include "fates/runtime/native_scenario_trick_state.hpp"
#include "fates/campaign/native_campaign_continuity.hpp"
#include "fates/chapter/native_completion_handoff.hpp"
#include "fates/runtime/native_phase_state.hpp"
#include "fates/runtime/native_support_inputs.hpp"
#include "fates/runtime/native_item_state.hpp"
#include "fates/map/native_tactical_image_state.hpp"
#include "fates/map/native_deployment_workspace_state.hpp"
#include "fates/map/native_actor_position_state.hpp"
#include "fates/map/native_actor_visual_state.hpp"
#include "fates/map/native_height_state.hpp"
#include "fates/map/native_field_scene.hpp"
#include <array>
#include <cstdint>
#include <vector>
#include <optional>

namespace fates::runtime::native {

// These are host-native semantic states. They intentionally do not mirror the
// retail Unit/GameUserData/Spot/Transporter object layouts.
struct ItemState {
    bool present{};
    std::uint32_t stack_key{};       // host definition/equality key, not a retail pointer
    std::uint16_t quantity{1};
    bool chapter_limited{};
    bool dead_disposal_primary_excluded{};
    bool dead_disposal_secondary_match{};
};

enum class PairRole : std::uint8_t { None, Lead, Partner };
// The three tactical forces share the map/phase domain. Forces3..8 can retain
// coordinates across transfer but must not become map occupants or targets.
constexpr bool IsTacticalForce(std::uint8_t force) noexcept {return force<3;}

struct UnitState {
    NativeUnitTransferState transfer{};
    NativeUnitLineageState lineage{};
    NativeUnitCapabilityState capabilities{};
    NativeUnitMovementState movement{};
    NativeUnitEnhanceState enhance{};
    NativeUnitMapEndState map_end{};
    NativeUnitTurnState turn{};
    std::uint64_t map_end_revision{}; // invalidates retained selections after lifecycle writes
    // Unit-owned private flags, separate from Person/Job flags and public Unit flags.
    // Providers of carried campaign units must preserve these, not infer them from PID.
    std::array<std::uint8_t,8> private_skill_bits{};
    // Current AI activity and band are separate from authored declaration IDs.
    // Unbound carried state must remain unknown until its owning writer supplies it.
    std::optional<std::uint8_t> ai_activity;
    std::optional<std::uint8_t> ai_band;
    // Current Unit+136, supplied by its creating/carried owner. Unknown is not
    // a default zero capture-name index; Unit clear discards this observation.
    std::optional<std::uint8_t> capture_name_index;
    struct AttackRestrictions {
        bool bound{};
        std::optional<std::uint16_t> excluded_person; // null reference differs from valid Person ID zero
        std::uint8_t excluded_forces{};
    } attack_restrictions{};
    bool occupied{};
    std::uint8_t force_type{9};
    std::uint32_t flags{};
    std::uint32_t chapter_scratch{};
    bool leader_private_skill{};
    std::uint16_t sortie_order_key{};
    std::array<ItemState,5> items{};
    bool dead_record_valid{};
    std::uint8_t dead_record_chapter_id{};
    std::uint8_t dead_record_context{};
    bool accessories_moved_to_box{};
    // Native tactical-state fields introduced by Pass75. These are semantic
    // host values, not retail Unit offsets.
    std::uint16_t person_id{};
    // Exact record selected by the creating/carried-state owner. Legacy numeric
    // readers are unchanged; record-sensitive services refuse an unbound value.
    PersonRecordReference person_record;
    std::uint16_t job_id{};
    bool has_position{};
    std::int16_t x{};
    std::int16_t y{};
    bool action_committed{};

    // Host-native current combat state. These are semantic current values and
    // intentionally do not expose retail Unit offsets.
    bool combat_state_valid{};
    bool complex_battle_rules{true};
    bool defeated{};
    std::int16_t max_hp{};
    std::int16_t current_hp{};
    std::int16_t strength{};
    std::int16_t magic{};
    std::int16_t skill{};
    std::int16_t speed{};
    std::int16_t luck{};
    std::int16_t defense{};
    std::int16_t resistance{};
    std::array<std::uint8_t,8> weapon_exp{};
    std::uint16_t equipped_item_id{}; // compatibility view of bound current inventory
    UnitInventoryState inventory{};
    // Pass98: bounded retail CreateFromDispos combat initialization provenance.
    bool create_from_dispos_combat_init_bound{};
    std::uint8_t create_level{};
    // Constructor provenance only; current mutable stored lanes use capabilities.
    std::array<std::int8_t,8> auto_growth_delta{};

    // Pass97: authored original Dispos source identity. This is provenance/input
    // state only; it does not claim retail Unit::CreateImpl combat initialization.
    bool dispos_source_bound{};
    std::uint16_t dispos_source_record{0xFFFFu};
    std::uint8_t dispos_authored_level{};
    std::array<std::uint16_t,5> dispos_item_ids{};
    std::array<std::uint8_t,5> dispos_item_flags{};
    std::array<std::array<std::int8_t,3>,5> dispos_item_difficulty_adjustments{};
    // Compact, accepted initial inventory identity projection. Source arrays
    // above remain untouched for provenance/re-initialization. Durability,
    // refinement, transfer and drop execution are separate contracts.
    std::array<std::uint16_t,5> initial_inventory_item_ids{};
    std::array<bool,5> initial_inventory_drop_flags{};
    std::array<std::uint16_t,5> dispos_skill_ids{};

    // Pass78 owns the bounded post-combat skill/debuff state needed by B007.
    // IDs are FE14 SkillDefinition IDs, not retail pointers/bitsets.
    std::array<std::uint16_t,8> equipped_skill_ids{};
    // Retail unit::Enhance weakness lanes: [0, Str, Mag, Skl, Spd, Lck, Def, Res].
    // Values are penalty magnitudes and saturate at 99 on additive paths.
    std::array<std::uint8_t,8> weakness{};

    // Pass93: host-native Pair Up / Guard Stance relationship state. Retail
    // Unit::DoubleOn stores reciprocal Unit pointers; native state uses stable
    // UnitPool slots instead. Pair partners remain real Units but are subordinate
    // to the lead for tactical occupancy/phase scheduling.
    std::uint64_t pair_revision{}; // link/unlink invalidates retained command intent
    struct PairState {
        bool bound{};
        PairRole role{PairRole::None};
        std::uint16_t partner_slot{0xFFFFu};
        std::uint8_t guard_progress{};
        // Person GuardStanceBonuses contribution for the current relationship
        // level. Job pair_up_bonuses are read from DefinitionStore at preview time.
        bool person_guard_bonus_bound{};
        std::array<std::int8_t,8> person_guard_bonus{};
    } pair{};

    // Portable AI declaration IDs + resolved AIValue argument slots sourced from
    // original AIData/Dispos. Retail Unit::SetDispos resolves exactly four signed
    // 16-bit AIValue arguments per descriptor channel; missing values are -1.
    struct AiDescriptorState {
        bool configured{};
        std::uint8_t action_id{};
        std::uint8_t mission_id{};
        std::uint8_t attack_id{};
        std::uint8_t movement_id{};
        // Pass90: runtime AI tuning copied by retail Unit::SetDispos.
        // These are semantic native fields, not a retail-layout mirror.
        bool runtime_tuning_bound{};
        std::uint32_t policy_flags{}; // Dispos +0x80 -> Unit+0x54
        std::uint8_t priority{};      // Dispos +0x75 -> Unit+0x5C
        // Dispos +0x74 -> Unit+0x5F. Values 0/1/2 select AIBattleSimulator score strategy.
        // 0xFF means the native importer has not yet bound this authored runtime byte.
        std::uint8_t battle_rate{0xFF};
        // Pass91: retail Unit::SetDispos +0x79..+0x7D -> Unit+0x60..+0x64.
        // Mode 0 with 0xFF rectangle bytes is the exact unrestricted B007 profile.
        std::uint8_t move_limit_mode{0xFF};
        std::uint8_t move_limit_x1{0xFF};
        std::uint8_t move_limit_y1{0xFF};
        std::uint8_t move_limit_x2{0xFF};
        std::uint8_t move_limit_y2{0xFF};
        std::array<std::int16_t,4> action_args{{-1,-1,-1,-1}};
        std::array<std::int16_t,4> mission_args{{-1,-1,-1,-1}};
        std::array<std::int16_t,4> attack_args{{-1,-1,-1,-1}};
        std::array<std::int16_t,4> movement_args{{-1,-1,-1,-1}};
    } ai{};
};

struct TransporterState {
    std::array<ItemState,500> slots{};
};

struct ChapterRecordState {
    bool valid{};
    std::uint8_t chapter_id{};
    std::uint16_t turn{};
    std::int32_t elapsed{};
};

struct SpotState {
    bool present{};
    std::uint8_t chapter_index{};
    std::uint8_t state{};
    std::int16_t stored_level{};
};

struct CampaignState {
    std::uint32_t game_user_flags{};
    fates::campaign::native::RouteIndex route{fates::campaign::native::RouteIndex::Birthright};
    std::uint8_t current_chapter_index{};       // GameUserData current Chapter
    std::uint8_t spot_current_chapter_index{};  // strategic Spot current Chapter
    int current_level{};

    std::array<ChapterRecordState,64> chapter_records{};
    std::uint8_t chapter_record_count{};
    int offspring_seal_level_max{};
    std::uint32_t raw_chapter_0x15_max{};
    std::uint8_t castle_post_chapter_counter{};
    bool has_castle_nested_state{};

    std::uint32_t content_id{};
    int nonrecord_completion_counter{};
    std::array<std::uint8_t,256> contents_earliest_turn{};

    std::array<SpotState,256> spots{};
    bool world_mob_update_pending{};
};

struct SaveFacingState {
    bool chapter_save_before_applied{};
    int chapter_save_jump_label{-1};
    std::uint8_t save_chapter_index{};
    bool save_menu_requested{};
    bool purchase_route_failed{};
    bool menu_byte_138{};
    bool menu_byte_139{};
    std::uint32_t menu_flags_50{};
    // Retail GameBackup::Write remains an asynchronous file-I/O boundary.
    bool backup_write_boundary_exposed{};
};

struct NativeRandomState {
    std::array<std::uint32_t,4> words{};
    bool initialized{};
};

struct RngStreamCounters {
    std::uint64_t system{};
    std::uint64_t game{};
    std::uint64_t world_mob{};
    // Pass86: retail AI owns a distinct Random object, seeded once from System RNG.
    std::uint64_t ai{};
    NativeRandomState system_state{};
    NativeRandomState game_state{};
    NativeRandomState world_mob_state{};
    NativeRandomState ai_state{};
};

struct AiPlannerState {
    // AIThink::AIThink zeroes 252 bytes at retail AIThink+0x514. The nearest-enemy
    // planner indexes this array by the Unit's 1-based UnitPool slot and increments
    // the chosen target after a successful MoveTo. Index 0 is intentionally unused.
    std::array<std::uint8_t,252> target_move_counts{};
};

// Writer-owned Force traversal order, checked against complete membership and
// Person identities before consumption. Pool-slot order is never a fallback.
struct NativeForceOrderState {
    bool bound{};
    std::uint16_t count{};
    std::array<std::uint16_t,250> slots{};
    std::array<std::uint16_t,250> person_ids{};
};
struct NativeGameState {
    // Explicitly refreshed/carried occupancy. Existing world writers may make
    // its binding stale; readers detect this rather than silently rebuilding it.
    fates::map::native::NativeTacticalImageState tactical_image{};
    fates::map::native::NativeActorPositionState actor_positions{};
    fates::map::native::NativeActorVisualState actor_visuals{};
    fates::map::native::NativeHeightState scene_height{};
    fates::map::native::NativeFieldSceneState field_scene{};
    fates::map::native::ActorPositionOffsets actor_position_offsets{};
    fates::chapter::native::SituationOutcomeState outcome{};
    bool map_active{true};
    std::array<UnitState,250> units{};
    // Host slot lifetime survives replacement of a Unit value, including reuse
    // by the same Person. It is not the retail 1-based pool index byte.
    std::array<std::uint64_t,250> unit_slot_generations{};
    struct FreeUnitPoolState {
        bool bound{};
        NativeForceOrderState order{};
        std::uint32_t clear_constructor_key{};
        std::array<std::uint32_t,250> cleared_constructor_keys{};
    } free_unit_pool{};
    TransporterState transporter{};
    CampaignState campaign{};
    NativeGameUserDifficultyState game_user_difficulty{};
    // Explicit carried GameConfigData+0x0E. Unknown is not selector zero.
    // This survives tactical-image initialization; save decoding and option UI
    // are separate owners. Danger consumes it only when Add is reached.
    std::optional<std::uint8_t> danger_staff_range_selector;
    // GameConfigData+4, separate from campaign GameUserData flags. Its bit8
    // controls whether TurnScroll uses Situation's remembered cursor position.
    std::optional<std::uint32_t> game_config_flags;
    SaveFacingState save{};
    RngStreamCounters rng{};
    AiPlannerState ai_planner{};
    TacticalPhaseContext phase{};
    NativeForceUpkeepProgress force_upkeep{};
    fates::map::native::NativeDeploymentWorkspaceState deployment_workspace;
    NativeForceOrderState player_force_order{}; // Force 0; retained for existing consumers
    std::array<NativeForceOrderState,8> other_force_orders{}; // Forces 1..8; free-pool order is separate
    ResolvedSupportContext support_context{};
    NativeScenarioTrickState scenario_tricks{};
    std::vector<std::uint16_t> restored_sortie_order_unit_slots{};
};

} // namespace fates::runtime::native
