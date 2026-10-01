#pragma once
#include "fates/ai/native_ai_semantics.hpp"
#include "fates/runtime/native_runtime.hpp"
namespace fates::ai::native {
// Represented AI fields from unit::AI::Clear, plus the two attack-exclusion
// fields cleared by Unit::Clear. Use at fresh native construction, not as a
// substitute for the rest of Unit::Clear or for loading a carried Unit.
void InitializeFreshUnitAiState(runtime::native::UnitState& unit) noexcept;
void BindDisposAiBand(runtime::native::UnitState& unit,std::uint8_t band) noexcept;
enum class AiStateWriteStatus { Ok,InvalidUnit,MissingAiState,InvalidPair,InvalidChannel };
// These publish all effects atomically. Force membership comes from occupied
// units' force identity; BandActivate's writes commute and do not establish or
// consume retail Force traversal order. Pair partners remain Force members.
AiStateWriteStatus ActivateUnitAi(runtime::native::NativeGameState&,std::uint16_t slot,bool force_band);
AiStateWriteStatus ActivateUnitAiCauseAttacked(runtime::native::NativeGameState&,std::uint16_t slot,bool second_policy);
AiStateWriteStatus CommitAiThinkUpdate(runtime::native::NativeGameState&,std::uint16_t slot,
    std::uint8_t channel,UpdateCommitInput& pending);
bool IsDontAttackExact(std::uint32_t actor_flags,std::uint32_t target_flags,bool ignore_public_flag,
    bool excluded_person_matches,std::uint8_t excluded_forces,std::uint8_t target_force) noexcept;
// Missing restriction/definition state is unknown, never permission. An absent
// actor follows the original target-only branch. No RNG or world mutation.
std::optional<bool> NativeAttackPermission(const runtime::native::NativeRuntime&,
    std::optional<std::uint16_t> actor_slot,std::uint16_t target_slot,bool ignore_public_flag=false);
} // namespace fates::ai::native
