#pragma once
#include "fates/presentation/native_talk_drawer_drawing.hpp"
#include "fates/presentation/native_talk_window.hpp"

namespace fates::presentation::native {
enum class TalkNamePlateDrawStatus:std::uint8_t {
    Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,InvalidRequest,
    InvalidWindow,InvalidRecord,InvalidPosition,Unavailable,IdentityExhausted,Cancelled
};
enum class TalkNamePlateDrawEntry:std::uint8_t {Location,Absolute};
struct TalkNamePlateDrawIdentity final {const std::uint32_t serial;};
using TalkNamePlateDrawRequest=std::shared_ptr<const TalkNamePlateDrawIdentity>;
struct TalkNamePlateDrawObservation {
    TalkNamePlateDrawRequest identity;TalkNamePlateDrawStatus status{TalkNamePlateDrawStatus::Ready};
    bool active{},completed{},cancelled{};std::size_t commands{};
    // Actual reached pointer loads, including known-null. Width and drawing may
    // borrow distinct allocations; a suspended child never silently rebinds.
    std::vector<TalkNameHandle> name_reads;
    TalkDrawerDrawRequest frame;FontTextRequest measurement;FontDrawRequest glyphs;
    std::optional<std::uint32_t> width,height;std::optional<TalkVectorBits> drawn_position;
};
// Both original TalkNamePlate draws, using actual name storage, layout, frame,
// font selection/metrics and glyph owners. No copied name cache or host font API.
class NativeTalkNamePlateDrawing final:public runtime::native::ProcessCallbacks {
public:
    // Read only the reached caller component: xy after frame setup, z after
    // metrics/layout and the third name-pointer load. Absolute ignores this.
    using PositionReader=std::function<std::optional<std::uint32_t>(std::size_t)>;
    static TalkNamePlateDrawStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<NativeTalkWindow>,
        std::shared_ptr<NativeTalkDrawerDrawing>,std::shared_ptr<io::native::NativeCoordinateResources>,
        std::shared_ptr<NativeTalkLayout>,std::shared_ptr<NativeFontMetrics>,std::shared_ptr<NativeFontDrawing>,
        std::shared_ptr<NativeFontPrimitiveAdapter>,std::shared_ptr<PrimitiveDrawSink>,std::shared_ptr<NativeTalkNamePlateDrawing>&);
    ~NativeTalkNamePlateDrawing();
    NativeTalkNamePlateDrawing(const NativeTalkNamePlateDrawing&)=delete;
    NativeTalkNamePlateDrawing& operator=(const NativeTalkNamePlateDrawing&)=delete;
    TalkNamePlateDrawStatus Prepare(TalkNamePlateDrawEntry,TalkWindowHandle,std::uint32_t location,
        std::uint32_t priority,PositionReader,TalkNamePlateDrawRequest&,runtime::native::ProcessAccess* =nullptr);
    std::optional<runtime::native::ProcessCall> DrawCall(runtime::native::ProcessHandle,TalkNamePlateDrawRequest) const;
    std::optional<TalkNamePlateDrawObservation> Observe(TalkNamePlateDrawRequest) const;
    TalkNamePlateDrawStatus Release(TalkNamePlateDrawRequest,runtime::native::ProcessAccess* =nullptr);
    bool UsesScheduler(const runtime::native::NativeProcessScheduler&) const noexcept;
    bool UsesComposition(const NativeTalkWindow&,const NativeTalkDrawerDrawing&,const io::native::NativeCoordinateResources&,
        const NativeTalkLayout&,const NativeFontMetrics&,const NativeFontDrawing&,const PrimitiveDrawSink*) const noexcept;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    friend class NativeTalkWindowDrawing;
    void ForgetDraw(TalkNamePlateDrawRequest);
    struct State;struct Continuation;
    explicit NativeTalkNamePlateDrawing(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
