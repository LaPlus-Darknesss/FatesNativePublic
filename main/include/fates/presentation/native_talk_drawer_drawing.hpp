#pragma once
#include "fates/presentation/native_coordinate_drawing.hpp"
#include "fates/presentation/native_talk_layout.hpp"
#include "fates/presentation/native_talk_window_drawer.hpp"

namespace fates::presentation::native {
enum class TalkDrawerDrawStatus:std::uint8_t {
    Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,InvalidRequest,
    InvalidRecord,InvalidTexture,Unavailable,IdentityExhausted,Cancelled,InvalidGeometry,UnimplementedBranch
};
enum class TalkDrawerDrawEntry:std::uint8_t {State,LocationNamePlate,AbsoluteNamePlate,FaceWindow,StandWindow,SystemWindow,NextIcon,StandCharacters,VariableStand};
struct TalkDrawerFrameOptions {
    std::uint32_t location{};std::array<std::uint32_t,2> xy{};
    std::int32_t shout_index{-1};bool next_icon{};
    std::uint32_t character_count{11};
};
struct TalkDrawerFrameInputs {
    // Explicit carried words at0x6dcf40 and0x6dcaa0. Read separately at the
    // original call sites. Scheduler frame_delta is not elapsed game time.
    std::function<std::optional<std::uint32_t>()> next_icon_suppressed,elapsed_ticks;
    // Original global border color0x6dd098, read independently for each tile.
    NativeFontDrawing::ColorReader variable_color{};
};
struct TalkDrawerDrawIdentity final {const std::uint32_t serial;};
using TalkDrawerDrawRequest=std::shared_ptr<const TalkDrawerDrawIdentity>;
struct TalkDrawerDrawObservation {
    TalkDrawerDrawRequest identity,state_child;CoordinateDrawRequest coordinate_child;
    TalkDrawerDrawRequest frame_child;FontTextRequest measurement;
    std::optional<std::uint32_t> measured_width;
    TalkDrawerDrawStatus status{TalkDrawerDrawStatus::Ready};
    bool active{},completed{},cancelled{};std::uint32_t returned{};
    std::size_t commands{},color_attempts{};std::optional<FontDrawColor> copied_color;
};
// Original SetState, nameplate frames, fixed/variable dialogue frames and icon.
// Resource ownership, I/O completion, font measurement, label selection and
// position geometry stay with their shared owners.
// Commands describe original primitive calls; they do not claim rendered pixels.
class NativeTalkDrawerDrawing final:public runtime::native::ProcessCallbacks {
public:
    using ColorReader=NativeFontDrawing::ColorReader;
    static TalkDrawerDrawStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<NativeTalkWindowDrawer>,
        std::shared_ptr<io::native::NativeFileController>,std::shared_ptr<io::native::NativeFileBase>,
        std::shared_ptr<io::native::NativeTexFiles>,std::shared_ptr<io::native::NativeTextureObjects>,
        std::shared_ptr<io::native::NativeCoordinateResources>,std::shared_ptr<NativeTalkLayout>,
        std::shared_ptr<NativeCoordinateDrawing>,std::shared_ptr<PrimitiveDrawSink>,std::shared_ptr<NativeTalkDrawerDrawing>&,
        std::shared_ptr<NativeFontMetrics> ={});
    ~NativeTalkDrawerDrawing();
    NativeTalkDrawerDrawing(const NativeTalkDrawerDrawing&)=delete;
    NativeTalkDrawerDrawing& operator=(const NativeTalkDrawerDrawing&)=delete;
    // For State, selector is the original file index (0 or 1). For Location it
    // is the raw location word. Absolute ignores selector. xy are integer bits.
    TalkDrawerDrawStatus Prepare(TalkDrawerDrawEntry,std::uint32_t selector,std::array<std::uint32_t,2>,
        ColorReader,TalkDrawerDrawRequest&,runtime::native::ProcessAccess* =nullptr);
    // Face/Stand/System frames, the next icon and variable frame helper/wrapper.
    // Variable frames require the actual font owner supplied at Create. Missing
    // composition stops at the reached font dependency; no scalar width seam.
    // Frame color remains borrowed until the nested scaled helper copies it.
    TalkDrawerDrawStatus PrepareFrame(TalkDrawerDrawEntry,TalkDrawerFrameOptions,
        TalkDrawerFrameInputs,ColorReader,TalkDrawerDrawRequest&,runtime::native::ProcessAccess* =nullptr);
    std::optional<runtime::native::ProcessCall> DrawCall(runtime::native::ProcessHandle,TalkDrawerDrawRequest) const;
    std::optional<TalkDrawerDrawObservation> Observe(TalkDrawerDrawRequest) const;
    TalkDrawerDrawStatus Release(TalkDrawerDrawRequest,runtime::native::ProcessAccess* =nullptr);
    bool UsesScheduler(const runtime::native::NativeProcessScheduler&) const noexcept;
    bool UsesComposition(const io::native::NativeCoordinateResources&,const NativeTalkLayout&,const PrimitiveDrawSink*) const noexcept;
    bool UsesMetrics(const NativeFontMetrics&) const noexcept;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    friend class NativeTalkNamePlateDrawing;
    friend class NativeTalkWindowDrawing;
    void ForgetDraw(TalkDrawerDrawRequest);
    struct State;struct Continuation;
    explicit NativeTalkDrawerDrawing(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
