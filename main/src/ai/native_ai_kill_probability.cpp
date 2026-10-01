#include "fates/ai/native_ai_kill_probability.hpp"
#include <bit>
#include <cstdint>
namespace fates::ai::native {
namespace {
float MulExact(const float a, const float b) noexcept {
    volatile float out = a * b;
    return out;
}
float AddExact(const float a, const float b) noexcept {
    volatile float out = a + b;
    return out;
}
float SubExact(const float a, const float b) noexcept {
    volatile float out = a - b;
    return out;
}
float MulAddExact(const float accumulator, const float a, const float b) noexcept {
    return AddExact(accumulator, MulExact(a, b));
}
std::int32_t WrapSub32(const std::int32_t a, const std::int32_t b) noexcept {
    const std::uint32_t raw = static_cast<std::uint32_t>(a) - static_cast<std::uint32_t>(b);
    return std::bit_cast<std::int32_t>(raw);
}
std::int32_t ArithmeticHalfExact(const std::int32_t v) noexcept { return v >> 1; }

struct KillAccumulator {
    const std::array<AiIndicationValueFrame, kAiIndicationCount>& indications;
    const std::array<AiKillProbabilityStateSlot, kAiIndicationCount>& states;
    std::uint8_t side{};
    float primary{};
    float secondary{};

    int NextType(const std::uint8_t type) const noexcept {
        const auto& s = states[side];
        switch(type) {
        case 0:
            if ((s.flags & kAiIndicationDoubleWeightFlag) != 0u) return 1;
            if (s.special_branch) return 2;
            if (s.sequence_count >= 2) return 4;
            return -1;
        case 1:
            if (s.special_branch) return 2;
            if (s.sequence_count >= 2) return 4;
            return -1;
        case 2:
            if ((states[static_cast<std::size_t>(side) + 2u].flags & kAiIndicationDoubleWeightFlag) != 0u) return 3;
            if (s.sequence_count >= 2) return 4;
            return -1;
        case 3:
            if (s.sequence_count >= 2) return 4;
            return -1;
        case 4:
            if ((s.flags & kAiIndicationDoubleWeightFlag) != 0u) return 5;
            return -1;
        default:
            return -1;
        }
    }

    void TerminalAdd(const std::uint8_t type, const float probability) noexcept {
        if (type <= 3u) primary = AddExact(primary, probability);
        else secondary = AddExact(secondary, probability);
    }

    void Calculate2(std::uint8_t type, std::int32_t threshold, float probability) noexcept {
        const std::size_t frame_index = (type == 2u || type == 3u)
            ? static_cast<std::size_t>(side) + 2u
            : static_cast<std::size_t>(side);
        const auto& f = indications[frame_index];

        Calculate3(type, threshold, MulExact(f.probabilities.hit_complement, probability), 0);

        float base = MulExact(f.probabilities.residual, probability);
        float extra = MulExact(f.extra_probability, base);
        Calculate3(type, threshold, SubExact(base, extra), f.magnitude0);
        Calculate3(type, threshold, extra, ArithmeticHalfExact(f.magnitude0));

        base = MulExact(f.probabilities.outcome_a, probability);
        extra = MulExact(f.extra_probability, base);
        const std::int32_t combined = CalculateSignedPercentMagnitudeExact(f.magnitude0, f.magnitude2);
        Calculate3(type, threshold, SubExact(base, extra), combined);
        Calculate3(type, threshold, extra, ArithmeticHalfExact(combined));

        if (f.magnitude1 <= 0) return;

        base = MulExact(f.probabilities.outcome_b, probability);
        if (f.special_secondary_branch) {
            Calculate3(type, threshold, base, f.magnitude1);
        } else {
            extra = MulExact(f.extra_probability, base);
            Calculate3(type, threshold, SubExact(base, extra), f.magnitude1);
            Calculate3(type, threshold, extra, ArithmeticHalfExact(f.magnitude1));
        }

        base = MulExact(f.probabilities.overlap, probability);
        if (f.special_secondary_branch) {
            Calculate3(type, threshold, base, combined);
        } else {
            extra = MulExact(f.extra_probability, base);
            Calculate3(type, threshold, SubExact(base, extra), combined);
            Calculate3(type, threshold, extra, ArithmeticHalfExact(combined));
        }
    }

    void Calculate3(const std::uint8_t type, const std::int32_t threshold,
                    const float probability, const std::int32_t magnitude) noexcept {
        // Retail VCMPE + BXLS returns for zero/non-positive supported finite probabilities.
        if (!(probability > 0.0f)) return;
        const std::int32_t remaining = WrapSub32(threshold, magnitude);
        if (remaining <= 0) {
            TerminalAdd(type, probability);
            return;
        }
        const int next = NextType(type);
        if (next < 0) return;
        Calculate2(static_cast<std::uint8_t>(next), remaining, probability);
    }
};

void ProcessProjectionSide(AiBattleProjectedScoreLanes& out,
                           const std::uint8_t side,
                           const std::size_t stage,
                           const std::array<float, kAiIndicationCount>& values,
                           const std::array<AiKillProbabilityBuckets, 2>& kill,
                           const std::array<AiKillProbabilityStateSlot, kAiIndicationCount>& states) noexcept {
    if (states[side].sequence_count <= static_cast<std::int32_t>(stage)) return;
    const float survive0 = SubExact(1.0f, out.lane0);
    const float survive1 = SubExact(1.0f, out.lane1);
    const float both = MulExact(survive0, survive1);
    const float kill_stage = stage == 0u ? kill[side].primary : kill[side].total;
    if (side == 0u) {
        out.lane0 = MulAddExact(out.lane0, both, kill_stage);
        out.lane2 = MulAddExact(out.lane2, both, values[0]);
        if (stage == 0u && states[0].special_branch)
            out.lane2 = MulAddExact(out.lane2, both, values[2]);
    } else {
        out.lane1 = MulAddExact(out.lane1, both, kill_stage);
        out.lane3 = MulAddExact(out.lane3, both, values[1]);
        if (stage == 0u && states[1].special_branch)
            out.lane3 = MulAddExact(out.lane3, both, values[3]);
    }
}
} // namespace

std::int32_t CalculateSignedPercentMagnitudeExact(const std::int32_t a,
                                                   const std::int32_t b) noexcept {
    // Retail: MUL (low 32) -> SMULL by 0x51EB851F -> ASR #5 -> subtract sign.
    const std::uint32_t wrapped = static_cast<std::uint32_t>(a) * static_cast<std::uint32_t>(b);
    const std::int32_t product = std::bit_cast<std::int32_t>(wrapped);
    const std::int64_t wide = static_cast<std::int64_t>(product) * static_cast<std::int64_t>(0x51EB851F);
    const std::int32_t high = static_cast<std::int32_t>(wide >> 32);
    return (high >> 5) - (high >> 31);
}

AiKillProbabilityBuckets CalculateKillProbabilityBucketsExact(
    const std::array<AiIndicationValueFrame, kAiIndicationCount>& indications,
    const std::array<AiKillProbabilityStateSlot, kAiIndicationCount>& state_slots,
    const std::uint8_t side_index,
    const std::int32_t threshold) noexcept {
    AiKillProbabilityBuckets out{};
    if (side_index > 1u) return out;
    if (state_slots[side_index].sequence_count <= 0) return out;
    KillAccumulator k{indications, state_slots, side_index};
    k.Calculate2(0u, threshold, 1.0f);
    out.primary = k.primary;
    out.secondary = k.secondary;
    out.total = AddExact(k.primary, k.secondary); // Calculate stores secondary += primary.
    return out;
}

AiBattleProjectedScoreLanes ProjectAIBattleSimulatorScoreLanesExact(
    const std::array<float, kAiIndicationCount>& indication_values,
    const std::array<AiKillProbabilityBuckets, 2>& kill_probability,
    const std::array<AiKillProbabilityStateSlot, kAiIndicationCount>& state_slots,
    const bool process_side1_first) noexcept {
    AiBattleProjectedScoreLanes out{};
    for (std::size_t stage = 0; stage < 2u; ++stage) {
        if (process_side1_first) {
            ProcessProjectionSide(out, 1u, stage, indication_values, kill_probability, state_slots);
            ProcessProjectionSide(out, 0u, stage, indication_values, kill_probability, state_slots);
        } else {
            ProcessProjectionSide(out, 0u, stage, indication_values, kill_probability, state_slots);
            ProcessProjectionSide(out, 1u, stage, indication_values, kill_probability, state_slots);
        }
    }
    return out;
}

AiResolvedBattleScoreResult ComposeResolvedAIBattleSimulatorScoreExact(
    const AiResolvedBattleScoreInput& input) noexcept {
    AiResolvedBattleScoreResult out{};
    out.indication_values = ComposeFourIndicationExpectedValuesExact(input.indications);
    out.kill_probability[0] = CalculateKillProbabilityBucketsExact(
        input.indications, input.state_slots, 0u, input.side1_current_hp);
    out.kill_probability[1] = CalculateKillProbabilityBucketsExact(
        input.indications, input.state_slots, 1u, input.side0_current_hp);
    out.score_lanes = ProjectAIBattleSimulatorScoreLanesExact(
        out.indication_values, out.kill_probability, input.state_slots, input.process_side1_first);
    AiBattleScoreFrame score_frame{};
    score_frame.strategy_mode = input.strategy_mode;
    score_frame.clever = input.clever;
    score_frame.lane0 = out.score_lanes.lane0;
    score_frame.lane1 = out.score_lanes.lane1;
    score_frame.lane2 = out.score_lanes.lane2;
    score_frame.lane3 = out.score_lanes.lane3;
    score_frame.side0_scale = input.side0_scale;
    score_frame.side1_scale = input.side1_scale;
    out.score = ComposeAIBattleSimulatorScoreExact(score_frame);
    return out;
}
} // namespace fates::ai::native
