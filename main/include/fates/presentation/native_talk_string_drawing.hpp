#pragma once
#include "fates/presentation/native_font_drawing.hpp"
#include "fates/presentation/native_talk_window.hpp"

namespace fates::presentation::native {
enum class TalkStringDrawStatus:std::uint8_t {
    Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,
    InvalidRequest,InvalidWindow,Unavailable,InvalidPosition,IdentityExhausted,Cancelled
};
struct TalkStringDrawIdentity final {const std::uint32_t serial;};
using TalkStringDrawRequest=std::shared_ptr<const TalkStringDrawIdentity>;
struct TalkStringDrawObservation {
    TalkStringDrawRequest identity;TalkStringDrawStatus status{TalkStringDrawStatus::Ready};
    bool active{},completed{},cancelled{};
    std::optional<TalkColorBytes> copied_color;
    std::optional<TalkVectorBits> drawn_position;
    FontDrawRequest child;
};
// Original TalkString::Draw over actual window-owned storage. It neither
// selects a font nor consults display/length/location: those are caller duties.
// Color and position are captured before calling Font; UTF16 storage remains
// live through the existing window Read API and the actual glyph continuation.
class NativeTalkStringDrawing final:public runtime::native::ProcessCallbacks {
public:
    using PositionReader=std::function<std::optional<TalkVectorBits>()>;
    static TalkStringDrawStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<NativeTalkWindow>,
        std::shared_ptr<NativeFontDrawing>,std::shared_ptr<NativeTalkStringDrawing>&);
    ~NativeTalkStringDrawing();
    NativeTalkStringDrawing(const NativeTalkStringDrawing&)=delete;
    NativeTalkStringDrawing& operator=(const NativeTalkStringDrawing&)=delete;
    // The supplied vector is read when the original reaches its argument loads.
    // An unavailable read does not replay the already copied/scaled color.
    TalkStringDrawStatus Prepare(TalkWindowView,PositionReader,std::uint32_t alpha_factor,
        std::uint32_t location,TalkStringDrawRequest&,runtime::native::ProcessAccess* =nullptr);
    std::optional<runtime::native::ProcessCall> DrawCall(runtime::native::ProcessHandle,TalkStringDrawRequest) const;
    std::optional<TalkStringDrawObservation> Observe(TalkStringDrawRequest) const;
    TalkStringDrawStatus Release(TalkStringDrawRequest,runtime::native::ProcessAccess* =nullptr);
    bool UsesScheduler(const runtime::native::NativeProcessScheduler&) const noexcept;
    bool UsesComposition(const NativeTalkWindow&,const NativeFontDrawing&) const noexcept;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    friend class NativeTalkWindowDrawing;
    void ForgetDraw(TalkStringDrawRequest);
    struct State;struct Continuation;
    explicit NativeTalkStringDrawing(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
