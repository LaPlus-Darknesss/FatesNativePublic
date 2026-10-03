#pragma once
#include "fates/presentation/native_talk_drawing_statics.hpp"
#include "fates/presentation/native_talk_name_plate_drawing.hpp"
#include "fates/presentation/native_talk_string_drawing.hpp"

namespace fates::presentation::native {
enum class TalkWindowDrawStatus:std::uint8_t {
    Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,InvalidRequest,
    InvalidWindow,InvalidRecord,InvalidGeometry,Unavailable,IdentityExhausted,Cancelled
};
struct TalkWindowDrawInputs {
    // Original game globals6DCA9C,6DCF40,6DCAA0. Sample only at reached sites.
    std::function<std::optional<std::uint32_t>()> shout_delta,next_icon_suppressed,elapsed_ticks;
};
struct TalkWindowDrawIdentity final {const std::uint32_t serial;};
using TalkWindowDrawRequest=std::shared_ptr<const TalkWindowDrawIdentity>;
struct TalkWindowStringCall {
    std::uint8_t slot{};TalkVectorBits argument{};std::uint32_t alpha{},location{};
};
struct TalkWindowDrawObservation {
    TalkWindowDrawRequest identity;TalkWindowDrawStatus status{TalkWindowDrawStatus::Ready};
    bool active{},completed{},cancelled{};std::size_t commands{};
    TalkDrawerDrawRequest frame;TalkDrawingOffsetRequest offset;
    TalkStringDrawRequest string;TalkNamePlateDrawRequest name;
    std::optional<std::int32_t> initial_shout;std::optional<std::uint8_t> written_shout;
    std::optional<TalkVectorBits> text_offset;std::vector<TalkWindowStringCall> string_calls;
    bool name_called{};std::uint32_t name_location{},name_priority{};
};
// Original18F6C8 over the existing admitted face-null NativeTalkWindow domain.
// Frames, font state/glyphs, strings and names remain with their actual owners.
// Primitive traces are not a renderer completion claim.
class NativeTalkWindowDrawing final:public runtime::native::ProcessCallbacks {
public:
    static TalkWindowDrawStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<NativeTalkWindow>,
        std::shared_ptr<NativeTalkDrawerDrawing>,std::shared_ptr<io::native::NativeCoordinateResources>,
        std::shared_ptr<NativeTalkLayout>,std::shared_ptr<NativeFontMetrics>,std::shared_ptr<NativeFontDrawing>,
        std::shared_ptr<NativeTalkStringDrawing>,std::shared_ptr<NativeTalkNamePlateDrawing>,
        std::shared_ptr<NativeTalkDrawingStatics>,std::shared_ptr<PrimitiveDrawSink>,std::shared_ptr<NativeTalkWindowDrawing>&);
    ~NativeTalkWindowDrawing();
    NativeTalkWindowDrawing(const NativeTalkWindowDrawing&)=delete;
    NativeTalkWindowDrawing& operator=(const NativeTalkWindowDrawing&)=delete;
    TalkWindowDrawStatus Prepare(TalkWindowHandle,std::uint32_t priority,TalkWindowDrawInputs,TalkWindowDrawRequest&,runtime::native::ProcessAccess* =nullptr);
    std::optional<runtime::native::ProcessCall> DrawCall(runtime::native::ProcessHandle,TalkWindowDrawRequest) const;
    std::optional<TalkWindowDrawObservation> Observe(TalkWindowDrawRequest) const;
    TalkWindowDrawStatus Release(TalkWindowDrawRequest,runtime::native::ProcessAccess* =nullptr);
    bool UsesScheduler(const runtime::native::NativeProcessScheduler&) const noexcept;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    struct State;struct Continuation;
    explicit NativeTalkWindowDrawing(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
