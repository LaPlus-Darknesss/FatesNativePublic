#pragma once
#include <cstdint>
#include <span>
#include <vector>
#include "fates/ai/native_ai_indication.hpp"
#include "fates/ai/native_ai_kill_probability.hpp"
#include "fates/runtime/native_game_state.hpp"
namespace fates::ai::native {
// Fields proven to feed AIBattleSimulator from retail BattleInfo::CalculateSimple.
// Unresolved skill-specific indication inputs deliberately remain outside this struct.
struct AiBattleInfoOrdinaryFacts {
    std::int32_t simple_power{};                  // BattleInfo::Side +0x2C
    std::int32_t simple_hit{};                    // BattleInfo::Side +0x30
    std::int32_t hit_denominator10000{};          // exact helper result from simple_hit
    std::int32_t simple_damage_rate{};            // BattleInfo::Side +0x38
    std::int32_t simple_attack_count{};           // BattleInfo::Side +0x3C
    std::int32_t opposing_current_hp{};            // Unit current HP; kill recursion threshold
    std::int32_t simple_critical{}; // Side +0x34, ordinary critical outcome-A probability
};
struct AiOrdinaryFrameBinding {
    AiIndicationValueFrame indication{};
    AiKillProbabilityStateSlot state_slot{};
    std::int32_t kill_threshold_current_hp{};
    std::int32_t simple_hit{};
    std::int32_t hit_denominator10000{};
};
// Exact for the ordinary lane where no skill-specific indication probability/magnitude
// is present. This is intentionally narrower than generic CalculateIndication.
AiOrdinaryFrameBinding BindOrdinaryNoSpecialBattleInfoExact(
    const AiBattleInfoOrdinaryFacts& facts, std::uint32_t indication_flags = 0u) noexcept;

// Retail AI owns a local 16-byte Random state. AI::AI seeds it from one System RNG
// raw draw, expands the seed using Random(seed), and performs 20 local warm-up draws.
bool InitializeAiLocalRandomFromSystem(fates::runtime::native::NativeGameState& state) noexcept;
std::uint32_t DrawAiLocalRandomBoundedExact(fates::runtime::native::NativeGameState& state,
                                            std::uint32_t max_exclusive) noexcept;

enum class AiCandidateSelectionStatus : std::uint8_t { Empty, Selected, MissingAiRandom };
struct AiTacticalScoredCandidate {
    std::uint16_t candidate_index{};
    std::uint16_t target_slot{};
    std::int16_t attack_x{};
    std::int16_t attack_y{};
    std::int32_t score{};
};
struct AiTacticalCandidateSelectionResult {
    AiCandidateSelectionStatus status{AiCandidateSelectionStatus::Empty};
    AiTacticalScoredCandidate selected{};
    std::uint16_t equal_score_encounters{};
    std::uint16_t tie_rng_draws{};
};
// Exact AIThink::GetAttackScore replacement policy: first candidate establishes best;
// higher replaces without RNG; equal consumes AI-local Random(...,2) and replaces on 0.
AiTacticalCandidateSelectionResult SelectRetailScoredCandidateExact(
    std::span<const AiTacticalScoredCandidate> candidates,
    fates::runtime::native::NativeGameState& state) noexcept;

// A present local source is distinct from an active assist. Inactive source
// indication fields may exist, but the primary +0x94 gate prevents visiting them.
// These are resolved simulator facts, not a relationship/skill provider.
struct AiOrdinaryAssistFacts {
    bool present{};
    bool active{};
    std::uint16_t unit_slot{0xFFFFu};
    AiBattleInfoOrdinaryFacts side{};
    std::uint32_t indication_flags{};
};

// Pass113 extends the existing ordinary frame to all four simulator participants.
// The established indication, kill recursion and score owners remain shared.
// Pass87: complete ordinary/no-special tactical-candidate scoring input.
// Geometry identity is preserved through retail score selection so the winner can
// later be committed by the existing movement/battle transaction without rescoring.
struct AiOrdinaryTacticalCandidateInput {
    std::uint16_t candidate_index{};
    std::uint16_t target_slot{};
    std::int16_t attack_x{};
    std::int16_t attack_y{};
    AiBattleInfoOrdinaryFacts side0{};
    AiBattleInfoOrdinaryFacts side1{};
    std::int32_t side0_current_hp{};
    std::int32_t side1_current_hp{};
    std::int8_t side0_score_cap{};
    std::int8_t side1_score_cap{};
    bool process_side1_first{};
    std::uint8_t strategy_mode{};
    bool clever{};
    std::uint32_t side0_flags{};
    std::uint32_t side1_flags{};
    bool requires_special_indication{};
    std::array<AiOrdinaryAssistFacts,2> assists{};
};

enum class AiOrdinaryMultiCandidateStatus : std::uint8_t {
    Empty, Selected, UnsupportedSpecialIndication, MissingAiRandom
};
struct AiOrdinaryMultiCandidateResult {
    AiOrdinaryMultiCandidateStatus status{AiOrdinaryMultiCandidateStatus::Empty};
    AiTacticalScoredCandidate selected{};
    std::vector<AiTacticalScoredCandidate> scored_candidates;
    std::uint16_t equal_score_encounters{};
    std::uint16_t tie_rng_draws{};
};

AiResolvedBattleScoreInput BindOrdinaryResolvedBattleScoreInputExact(
    const AiOrdinaryTacticalCandidateInput& candidate) noexcept;
std::int32_t ScoreOrdinaryTacticalCandidateExact(
    const AiOrdinaryTacticalCandidateInput& candidate) noexcept;
AiOrdinaryMultiCandidateResult ScoreAndSelectOrdinaryTacticalCandidatesExact(
    std::span<const AiOrdinaryTacticalCandidateInput> candidates,
    fates::runtime::native::NativeGameState& state);

} // namespace fates::ai::native
