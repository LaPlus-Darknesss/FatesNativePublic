#pragma once
#include "fates/headless/b007_dispos_unit_pool.hpp"
#include "fates/map/native_deployment_semantics.hpp"
#include "fates/runtime/native_game_state.hpp"
#include "fates/runtime/native_runtime.hpp"
#include "fates/ai/native_ai_phase.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace fates::ai::native {

enum class B007DeploymentDifficulty : std::uint8_t { Normal=0, Hard=1, Lunatic=2 };
enum class B007PairRole : std::uint8_t { None, Lead, Partner };
enum class B007DeploymentTopologyStatus : std::uint8_t {
    Ok,
    MissingEnemyGroup,
    EnemyCountMismatch,
    MalformedPairTopology,
    InvalidStandalonePosition,
};

struct B007DeploymentRecordTopology {
    std::uint16_t source_record{};
    bool enabled{};
    B007PairRole pair_role{B007PairRole::None};
    std::int16_t effective_x{-1};
    std::int16_t effective_y{-1};
};

struct B007PairTopology {
    std::uint16_t lead_record{};
    std::uint16_t partner_record{};
    std::int16_t x{};
    std::int16_t y{};
};

struct B007OriginalEnemyDeploymentTopology {
    B007DeploymentTopologyStatus status{B007DeploymentTopologyStatus::Ok};
    B007DeploymentDifficulty difficulty{B007DeploymentDifficulty::Normal};
    std::uint16_t source_enemy_records{};
    std::uint16_t enabled_records{};
    std::uint16_t phase_actor_records{};
    std::uint16_t disabled_by_difficulty{};
    std::uint16_t paired_partner_records{};
    std::uint16_t unpositioned_enabled_records{};
    std::vector<B007DeploymentRecordTopology> records;
    std::vector<std::uint16_t> phase_actor_source_records;
    std::vector<B007PairTopology> pairs;
};

std::uint32_t B007DifficultySpawnMask(B007DeploymentDifficulty difficulty) noexcept;
B007OriginalEnemyDeploymentTopology BuildB007OriginalEnemyDeploymentTopology(
    const fates::headless::Fe14DisposFileProjection& dispos,
    B007DeploymentDifficulty difficulty);

// The original B007 deployment always contains at least one Guard-Stance pair
// on every retail difficulty. Pass92 closes deployment topology only; paired
// battle calculation remains an explicit next semantic boundary.
bool B007TopologyRequiresPairedBattleSupport(
    const B007OriginalEnemyDeploymentTopology& topology) noexcept;

enum class B007PairBindStatus : std::uint8_t {
    Ok, InvalidTopology, InvalidSlot, MissingUnit, InvalidPairForce,
};

// source_record_to_unit_slot maps all 19 original Enemy Dispos records to the
// native UnitPool slots created from them. Pair partners are bound as subordinate
// Units and inherit the lead position exactly as Unit::DoubleOn/map::Dispos do.
B007PairBindStatus BindB007GuardStancePairs(
    fates::runtime::native::NativeGameState& state,
    const B007OriginalEnemyDeploymentTopology& topology,
    const std::array<std::uint16_t,19>& source_record_to_unit_slot) noexcept;

// Re-project the current B007 phase roster from original enabled source-record order.
// Unlike the immutable deployment topology, this consults live PairRole state so a
// surviving partner released by battle death becomes a standalone actor next phase.
std::vector<std::uint16_t> BuildB007CurrentPhaseActorSlots(
    const fates::runtime::native::NativeGameState& state,
    const B007OriginalEnemyDeploymentTopology& topology,
    const std::array<std::uint16_t,19>& source_record_to_unit_slot);


enum class B007OriginalPhaseExecutionStatus : std::uint8_t {
    Ok, InvalidTopology, InvalidSourceMap, MissingJobDefinition, PhaseRejected,
};

struct B007OriginalPhaseExecutionResult {
    B007OriginalPhaseExecutionStatus status{B007OriginalPhaseExecutionStatus::Ok};
    std::vector<std::uint16_t> actor_slots;
    AiPhaseResult phase{};
};

// Execute the current difficulty-aware B007 enemy phase from live runtime state.
// Actor membership comes from BuildB007CurrentPhaseActorSlots; movement power is
// resolved from each Unit's current JobDefinition rather than duplicated by callers.
B007OriginalPhaseExecutionResult ExecuteB007OriginalEnemyPhase(
    fates::runtime::native::NativeRuntime& runtime,
    const B007OriginalEnemyDeploymentTopology& topology,
    const std::array<std::uint16_t,19>& source_record_to_unit_slot,
    std::uint16_t current_turn,
    bool retail_enumeration_order_exact=false);

enum class B007OriginalDataImportStatus : std::uint8_t {
    Ok, InvalidTopology, MissingEnemyGroup, EnemyCountMismatch, MissingDefinition,
    UnsupportedAiDescriptor, InvalidAiValue, PairBindRejected,
    UnsupportedDeploymentContext,ForceOrderRejected,OccupiedDestination,
};

struct B007OriginalDataImportResult {
    B007OriginalDataImportStatus status{B007OriginalDataImportStatus::Ok};
    std::array<std::uint16_t,19> source_record_to_unit_slot{};
    std::uint16_t enabled_records{};
    std::uint16_t imported_units{};
    std::uint16_t combat_ready_units{};
    std::uint16_t default_job_fallback_units{};
};

B007OriginalDataImportResult ImportB007OriginalEnemyDisposState(
    fates::runtime::native::NativeRuntime& runtime,
    const fates::headless::Fe14DisposFileProjection& dispos,
    const B007OriginalEnemyDeploymentTopology& topology);


enum class B007CombatInitStatus : std::uint8_t {
    Ok, MissingDefinition, UnsupportedLevel, InvalidItem, NoEquipableAuthoredWeapon,
};

struct B007CombatInitResult {
    B007CombatInitStatus status{B007CombatInitStatus::Ok};
    std::uint16_t initialized_units{};
    std::uint16_t equipped_units{};
};

// Pass98: bounded retail Unit::CreateFromDispos/CreateImpl combat initialization
// for original B007 enemy records. This uses original Person/Job/Item definitions
// and refuses unsupported authored equipment rather than fabricating combat state.
B007CombatInitResult InitializeB007OriginalEnemyCombatState(
    fates::runtime::native::NativeRuntime& runtime,
    B007DeploymentDifficulty difficulty,
    const B007OriginalDataImportResult& imported);

enum class B007OriginalPhaseProbeStatus : std::uint8_t {
    ReadyToExecute, ImportRejected, MissingCombatInitialization, CombatInitializationRejected,
};

struct B007OriginalPhaseProbeResult {
    B007OriginalPhaseProbeStatus status{B007OriginalPhaseProbeStatus::ImportRejected};
    B007OriginalDataImportResult imported{};
    B007CombatInitResult combat_init{};
    std::vector<std::uint16_t> actor_slots;
    std::uint16_t missing_combat_state{};
    std::uint64_t game_rng_draws{};
    std::uint64_t ai_rng_draws{};
};

B007OriginalPhaseProbeResult ProbeB007OriginalEnemyPhase(
    fates::runtime::native::NativeRuntime& runtime,
    const fates::headless::Fe14DisposFileProjection& dispos,
    B007DeploymentDifficulty difficulty);

} // namespace fates::ai::native
