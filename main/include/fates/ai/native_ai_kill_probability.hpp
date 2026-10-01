#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include "fates/ai/native_ai_indication.hpp"
#include "fates/ai/native_ai_score.hpp"
namespace fates::ai::native {
// Semantic-neutral state consumed by the exact retail kill-probability transition
// logic. These fields correspond to already-resolved AIBattleSimulator side slots;
// durable gameplay names are intentionally deferred until native BattleInfo binding.
struct AiKillProbabilityStateSlot {
    std::uint32_t flags{};       // retail side-slot +0x28; bit 0x800 participates in transitions
    bool special_branch{};       // retail side-slot +0x94 byte
    std::int32_t sequence_count{}; // retail side-slot +0x3C
};

struct AiKillProbabilityBuckets {
    float primary{};    // terminal types 0..3
    float secondary{};  // terminal types 4..5 before retail total fold
    float total{};      // retail post-fold: primary + secondary
};

// Exact result of CalculateScore's pre-score aggregation. Lane identities remain
// aligned with Pass82's semantic-neutral +0xD4/+0xD8/+0xDC/+0xE0 score frame.
struct AiBattleProjectedScoreLanes {
    float lane0{};
    float lane1{};
    float lane2{};
    float lane3{};
};

struct AiResolvedBattleScoreInput {
    std::array<AiIndicationValueFrame, kAiIndicationCount> indications{};
    std::array<AiKillProbabilityStateSlot, kAiIndicationCount> state_slots{};
    // Current HP thresholds used by CalculateKillProbabilityWithoutInterference2/3.
    // Pass86 proved these are distinct from the signed score-normalization caps.
    std::int32_t side0_current_hp{};
    std::int32_t side1_current_hp{};
    std::int8_t side0_scale{};
    std::int8_t side1_scale{};
    bool process_side1_first{}; // exact retail order condition, resolved upstream
    std::uint8_t strategy_mode{};
    bool clever{};
};

struct AiResolvedBattleScoreResult {
    std::array<float, kAiIndicationCount> indication_values{};
    std::array<AiKillProbabilityBuckets, 2> kill_probability{};
    AiBattleProjectedScoreLanes score_lanes{};
    std::int32_t score{};
};

// Exact ARM signed low-32 multiply followed by retail /100 magic-number quotient.
std::int32_t CalculateSignedPercentMagnitudeExact(std::int32_t a, std::int32_t b) noexcept;

// Exact CalculateKillProbabilityWithoutInterference2/3 closure for side indexes 0/1.
AiKillProbabilityBuckets CalculateKillProbabilityBucketsExact(
    const std::array<AiIndicationValueFrame, kAiIndicationCount>& indications,
    const std::array<AiKillProbabilityStateSlot, kAiIndicationCount>& state_slots,
    std::uint8_t side_index,
    std::int32_t threshold) noexcept;

AiBattleProjectedScoreLanes ProjectAIBattleSimulatorScoreLanesExact(
    const std::array<float, kAiIndicationCount>& indication_values,
    const std::array<AiKillProbabilityBuckets, 2>& kill_probability,
    const std::array<AiKillProbabilityStateSlot, kAiIndicationCount>& state_slots,
    bool process_side1_first) noexcept;

// End-to-end exact arithmetic after upstream BattleInfo has resolved the four
// indication frames/state slots. No target heuristic and no tie RNG are invented.
AiResolvedBattleScoreResult ComposeResolvedAIBattleSimulatorScoreExact(
    const AiResolvedBattleScoreInput& input) noexcept;
} // namespace fates::ai::native
