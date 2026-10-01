#pragma once
#include "fates/ai/native_ai_action.hpp"
#include "fates/ai/native_ai_semantics.hpp"
#include "fates/ai/native_ai_runtime_tuning.hpp"
#include <cstdint>
#include <span>
#include <vector>
namespace fates::ai::native {
struct AiPhaseActorInput { std::uint16_t unit_slot{}; std::uint8_t priority_byte{}; int move_power{}; };
struct AiPhaseRuntimeActorInput { std::uint16_t unit_slot{}; int move_power{}; };
enum class AiPhaseActorBindStatus : std::uint8_t { Ok, InvalidUnit, MissingRuntimeTuning };
struct AiPhaseActorBindResult {
    AiPhaseActorBindStatus status{AiPhaseActorBindStatus::Ok};
    std::vector<AiPhaseActorInput> actors;
};
struct AiPhaseOrderEntry { std::uint16_t unit_slot{}; std::uint32_t score{}; };
std::vector<AiPhaseOrderEntry> BuildStablePhaseOrder(std::span<const AiPhaseActorInput>);
AiPhaseActorBindResult BindPhaseActorsFromRuntimeAi(
    const fates::runtime::native::NativeRuntime&,
    std::span<const AiPhaseRuntimeActorInput>);
struct AiPhaseContext { std::uint16_t current_turn{}; const AiCandidatePreviewProvider* candidate_preview{}; };
enum class AiPhaseStatus : std::uint8_t { Ok, ActionRejected, PhaseContextRejected, InvalidActorSet };
enum class AiPhaseAccessStatus : std::uint8_t { Allowed, AwaitingServices, HumanControlled, StaleChapter, TurnMismatch, Terminal, WrongForce };
struct AiPhaseStep { std::uint16_t unit_slot{}; AiActionResult action{}; };
struct AiPhaseResult { AiPhaseAccessStatus phase_access{AiPhaseAccessStatus::Allowed}; AiPhaseStatus status{AiPhaseStatus::Ok}; AiPhaseActorBindStatus actor_binding_status{AiPhaseActorBindStatus::Ok}; std::vector<AiPhaseStep> steps; std::vector<std::uint16_t> inactive_unit_slots; std::uint64_t game_rng_draws{}; std::uint64_t ai_rng_draws{}; };
AiPhaseResult ExecuteOrderedAttackPhase(fates::runtime::native::NativeRuntime&,std::span<const AiPhaseActorInput>,AiPhaseContext);
AiPhaseResult ExecuteOrderedAttackPhaseWithSharedOrdinaryPreview(
    fates::runtime::native::NativeRuntime&,std::span<const AiPhaseActorInput>,std::uint16_t current_turn,
    bool retail_enumeration_order_exact=false);
AiPhaseResult ExecuteOrderedAttackPhaseFromRuntimeAiWithSharedOrdinaryPreview(
    fates::runtime::native::NativeRuntime&,std::span<const AiPhaseRuntimeActorInput>,std::uint16_t current_turn,
    bool retail_enumeration_order_exact=false);
AiPhaseResult ExecuteOrderedEverytimePhase(fates::runtime::native::NativeRuntime&,std::span<const AiPhaseActorInput>);
} // namespace fates::ai::native
