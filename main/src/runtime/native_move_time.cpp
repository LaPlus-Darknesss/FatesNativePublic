#include "fates/runtime/native_move_time.hpp"
#include <bit>
#include <cmath>

namespace fates::runtime::native {
namespace {
float Mul(float a,float b) noexcept {return a*b;}
float Add(float a,float b) noexcept {return a+b;}
}
void SetMoveTimeExact(MoveTimeState& s,std::int32_t ms) noexcept {
    s.elapsed=0.0f;
    const auto frames=Mul(static_cast<float>(ms),std::bit_cast<float>(std::uint32_t{0x3d75c28f}));
    s.duration=static_cast<float>(static_cast<std::int32_t>(frames));
}
bool EvaluateMoveTimeExact(MoveTimeState& s,float step,std::uint32_t delta,bool& evaluated) noexcept {
    if(!std::isfinite(s.duration))return false;
    if(s.duration<=0.0f){evaluated=false;return true;}
    if(!std::isfinite(s.elapsed)||!std::isfinite(step))return false;
    auto elapsed=Add(s.elapsed,Mul(step,static_cast<float>(delta)));
    if(!std::isfinite(elapsed))return false;
    if(elapsed>=s.duration){s.elapsed=0.0f;s.duration=0.0f;}
    else s.elapsed=elapsed;
    evaluated=true;return true;
}
bool MoveTimeRateExact(const MoveTimeState& s,float& rate) noexcept {
    if(!std::isfinite(s.duration))return false;
    if(s.duration<=0.0f){rate=1.0f;return true;}
    // ProcBand's kind2/subkind2 reaches Curve::Decel with degree2. Preserve
    // ordered binary32 subtraction, squares, multiply, divide and subtraction.
    if(s.kind!=2 || s.subkind!=2 || !std::isfinite(s.elapsed))return false;
    const float remaining=s.duration-s.elapsed;
    const float divisor=Mul(s.duration,s.duration);
    const float power=Mul(remaining,remaining);
    const float numerator=Mul(1.0f,power);
    const float quotient=numerator/divisor;
    const float result=1.0f-quotient;
    if(!std::isfinite(result))return false;
    rate=result;return true;
}
}
