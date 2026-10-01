#pragma once
#include "fates/ai/native_dispos_combat_initialization.hpp"
#include "fates/ai/native_ai_phase.hpp"
#include "fates/headless/b007_dispos_unit_pool.hpp"
#include "fates/runtime/native_runtime.hpp"
#include <cstdint>
#include <vector>

namespace fates::ai::native {

enum class OriginalEnemyImportStatus : std::uint8_t {
    Ok, MissingEnemyGroup, UnitPoolExhausted, InvalidPosition, UnsupportedPairTopology,
    MissingDefinition, UnsupportedAiDescriptor, InvalidAiValue,
    UnsupportedDeploymentContext,ForceOrderRejected,
};

struct OriginalEnemyImportResult {
    OriginalEnemyImportStatus status{OriginalEnemyImportStatus::Ok};
    NativeDisposDifficulty difficulty{NativeDisposDifficulty::Normal};
    std::uint16_t source_enemy_records{};
    std::uint16_t enabled_records{};
    std::uint16_t imported_units{};
    std::uint16_t default_job_fallback_units{};
    std::vector<std::uint16_t> source_record_to_unit_slot;
    std::vector<std::uint16_t> phase_actor_slots;
};

std::uint32_t OriginalEnemyDifficultySpawnMask(NativeDisposDifficulty difficulty) noexcept;
OriginalEnemyImportResult ImportOriginalEnemyDisposState(
    fates::runtime::native::NativeRuntime& runtime,
    const fates::headless::Fe14DisposFileProjection& dispos,
    NativeDisposDifficulty difficulty);

struct OriginalEnemyCombatProbeResult {
    OriginalEnemyImportResult imported{};
    NativeDisposCombatInitResult combat_init{};
};
OriginalEnemyCombatProbeResult ProbeOriginalEnemyCombatState(
    fates::runtime::native::NativeRuntime& runtime,
    const fates::headless::Fe14DisposFileProjection& dispos,
    NativeDisposDifficulty difficulty);

struct OriginalEnemyPhaseExecutionResult {
    OriginalEnemyImportStatus import_status{OriginalEnemyImportStatus::Ok};
    NativeDisposCombatInitStatus combat_status{NativeDisposCombatInitStatus::Ok};
    AiPhaseResult phase{};
    std::vector<std::uint16_t> actor_slots;
};
OriginalEnemyPhaseExecutionResult ExecuteOriginalEnemyPhase(
    fates::runtime::native::NativeRuntime& runtime,
    const fates::headless::Fe14DisposFileProjection& dispos,
    NativeDisposDifficulty difficulty,
    std::uint16_t current_turn,
    bool retail_enumeration_order_exact=false);

} // namespace fates::ai::native
