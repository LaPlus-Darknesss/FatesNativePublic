#include "fates/ai/native_ai_indication.hpp"
namespace fates::ai::native {
namespace {
// Retail VFP uses separate single-precision multiply/add operations (VMLA rather
// than VFMA). Volatile temporaries force host rounding at each arithmetic step.
float MulExact(const float a, const float b) noexcept {
    volatile float product = a * b;
    return product;
}
float AddExact(const float a, const float b) noexcept {
    volatile float sum = a + b;
    return sum;
}
float MulAddExact(const float accumulator, const float a, const float b) noexcept {
    const float product = MulExact(a, b);
    return AddExact(accumulator, product);
}
std::int32_t ArithmeticHalfExact(const std::int32_t value) noexcept {
    // C++20 signed right shift mirrors the retail ASR #1 used here.
    return value >> 1;
}
}
float CalculateHitComplementExact(const std::int32_t denominator10000) noexcept {
    return 1.0f - static_cast<float>(denominator10000) * 0.0001f;
}
float ProbabilityPercentExact(const std::int32_t probability_percent) noexcept {
    return static_cast<float>(probability_percent) * 0.01f;
}
float WeightProbabilityByHitExact(const std::int32_t probability_percent,
                                  const float hit_complement) noexcept {
    return ProbabilityPercentExact(probability_percent) * (1.0f - hit_complement);
}
AiIndicationProbabilityPartition ComposeIndicationProbabilityPartitionExact(
    const float hit_complement, const float outcome_a_raw, const float outcome_b_raw) noexcept {
    AiIndicationProbabilityPartition out{};
    out.hit_complement = hit_complement;
    out.overlap = outcome_a_raw * outcome_b_raw;
    out.outcome_a = outcome_a_raw - out.overlap;
    out.outcome_b = outcome_b_raw - out.overlap;
    out.residual = 1.0f - out.hit_complement - out.outcome_a - out.outcome_b - out.overlap;
    return out;
}
float ApplyIndicationDoubleWeightExact(const float value, const std::uint32_t flags) noexcept {
    return (flags & kAiIndicationDoubleWeightFlag) != 0u ? value * 2.0f : value;
}
float AccumulateIndicationExtraProbabilityExact(const float accumulated,
                                                const std::int32_t probability_percent) noexcept {
    const float remaining = 1.0f - accumulated;
    const float weighted = MulExact(static_cast<float>(probability_percent), remaining);
    return MulAddExact(accumulated, weighted, 0.01f);
}
float ComposeIndicationExpectedValueExact(const AiIndicationValueFrame& f) noexcept {
    const float residual = f.probabilities.residual;
    const float outcome_a = f.probabilities.outcome_a;
    const float extra = f.extra_probability;
    const float one_minus_extra = 1.0f - extra;
    const std::int32_t half0_i = ArithmeticHalfExact(f.magnitude0);
    const std::int32_t half1_i = ArithmeticHalfExact(f.magnitude1);

    // 0x001F7A48..0x001F7AB4: base weighted expectation.
    const float residual_extra = MulExact(residual, extra);
    const float primary_no_extra = MulExact(one_minus_extra, static_cast<float>(f.magnitude0));
    float value = MulExact(residual, primary_no_extra);
    value = MulAddExact(value, static_cast<float>(half0_i), residual_extra);

    const float primary_tertiary = MulExact(static_cast<float>(f.magnitude0),
                                            static_cast<float>(f.magnitude2));
    const float outcome_no_extra = MulExact(outcome_a, one_minus_extra);
    const float full_outcome_term = MulExact(outcome_no_extra, primary_tertiary);
    value = MulAddExact(value, full_outcome_term, 0.01f);

    const float outcome_extra = MulExact(outcome_a, extra);
    const float half_primary_secondary = MulExact(static_cast<float>(half0_i),
                                                   static_cast<float>(f.magnitude1));
    const float half_outcome_term = MulExact(outcome_extra, half_primary_secondary);
    value = MulAddExact(value, half_outcome_term, 0.01f);

    if (f.magnitude1 > 0) {
        // 0x001F7ABC..0x001F7B30: positive secondary magnitude branch.
        if (f.special_secondary_branch) {
            value = MulAddExact(value, residual, static_cast<float>(f.magnitude1));
        } else {
            const float secondary_no_extra = MulExact(one_minus_extra,
                                                       static_cast<float>(f.magnitude1));
            value = MulAddExact(value, residual, secondary_no_extra);
            value = MulAddExact(value, static_cast<float>(half1_i), residual_extra);
        }

        // 0x001F7B34..0x001F7BCC: outcome-A contribution for the secondary lane.
        if (f.special_secondary_branch) {
            const float tertiary_percent = MulExact(static_cast<float>(f.magnitude2), 0.01f);
            const float full_secondary_outcome = MulExact(outcome_a,
                                                           static_cast<float>(f.magnitude1));
            value = MulAddExact(value, full_secondary_outcome, tertiary_percent);
        } else {
            const float half_secondary_tertiary = MulExact(static_cast<float>(half1_i),
                                                            static_cast<float>(f.magnitude2));
            const float no_extra_term = MulExact(outcome_no_extra, half_secondary_tertiary);
            value = MulAddExact(value, no_extra_term, 0.01f);
            const float extra_term = MulExact(outcome_extra, half_secondary_tertiary);
            value = MulAddExact(value, extra_term, 0.01f);
        }
    }

    // 0x001F7BD0..0x001F7BEC is applied last in retail.
    return ApplyIndicationDoubleWeightExact(value, f.flags);
}
std::array<float, kAiIndicationCount> ComposeFourIndicationExpectedValuesExact(
    const std::array<AiIndicationValueFrame, kAiIndicationCount>& frames) noexcept {
    std::array<float, kAiIndicationCount> out{};
    for (std::size_t i = 0; i < out.size(); ++i) out[i] = ComposeIndicationExpectedValueExact(frames[i]);
    return out;
}
} // namespace fates::ai::native
