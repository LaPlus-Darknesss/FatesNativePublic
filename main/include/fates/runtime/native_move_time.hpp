#pragma once
#include <cstdint>

namespace fates::runtime::native {
// Semantic projection of MoveTime+00..08. Only reached curve profiles are
// admitted by Rate; unknown profiles are never silently made linear.
struct MoveTimeState {
    std::uint16_t kind{},subkind{};
    float elapsed{},duration{};
};
void SetMoveTimeExact(MoveTimeState&,std::int32_t milliseconds) noexcept;
bool EvaluateMoveTimeExact(MoveTimeState&,float step,std::uint32_t frame_delta,bool& evaluated) noexcept;
bool MoveTimeRateExact(const MoveTimeState&,float& rate) noexcept;
}
