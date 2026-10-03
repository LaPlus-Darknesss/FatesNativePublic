#pragma once
#include "fates/presentation/native_font_startup.hpp"

namespace fates::presentation::native {
enum class FontDrawStatus:std::uint8_t {
    Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,InvalidRequest,
    Unavailable,StaleData,InvalidData,ReadLimit,IdentityExhausted,Cancelled
};
enum class FontDrawEntry:std::uint8_t {Object,Font2D,Font3D};
enum class FontDrawCommandKind:std::uint8_t {Push,Begin,RectShape,Texture,End,Rectangle,Pop};
using FontDrawColor=std::array<std::uint8_t,4>;
struct FontDrawCommand {
    FontDrawCommandKind kind{};
    std::optional<FontSheetView> sheet;
    // Exact binary32 x,y,z,width,height,u,v passed to the original primitive.
    std::array<std::uint32_t,7> values{};
    FontDrawColor color{};
};
class FontDrawSink {
public:
    virtual ~FontDrawSink()=default;
    // False commits nothing. The same pending command is retried unchanged.
    virtual bool Submit(const FontDrawCommand&)=0;
};
struct FontDrawIdentity final {const std::uint32_t serial;};
using FontDrawRequest=std::shared_ptr<const FontDrawIdentity>;
struct FontDrawObservation {
    FontDrawRequest identity;FontDrawStatus status{FontDrawStatus::Ready};
    bool active{},completed{},cancelled{};
    std::size_t read_attempts{},color_attempts{},commands{},rectangles{},dropped_glyphs{};
};
// FontObject::Draw and the two UTF16 Font::Draw wrappers. Uses the existing
// metric cache, borrowed sheets, selector and eight static glyph buffers.
// Primitive output is an explicit sink boundary, not a rendering completion
// claim. A null sink is allowed for headless execution of this owner.
class NativeFontDrawing final:public runtime::native::ProcessCallbacks {
public:
    using Reader=NativeFontMetrics::Reader;
    using ColorReader=std::function<std::optional<FontDrawColor>()>;
    static FontDrawStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<NativeFontMetrics>,
        std::shared_ptr<NativeFontObjects>,std::shared_ptr<NativeFontStartup>,
        std::shared_ptr<FontDrawSink>,std::shared_ptr<NativeFontDrawing>&);
    ~NativeFontDrawing();
    NativeFontDrawing(const NativeFontDrawing&)=delete;
    NativeFontDrawing& operator=(const NativeFontDrawing&)=delete;
    // Positions are finite binary32 bits. Font2D supplies original positive-zero
    // z. Object uses the explicit object; wrappers capture selection on entry.
    FontDrawStatus Prepare(FontDrawEntry,io::native::FileObjectHandle,Reader,ColorReader,
        std::array<std::uint32_t,3>,FontDrawRequest&,runtime::native::ProcessAccess* =nullptr);
    std::optional<runtime::native::ProcessCall> DrawCall(runtime::native::ProcessHandle,FontDrawRequest) const;
    std::optional<FontDrawObservation> Observe(FontDrawRequest) const;
    FontDrawStatus Release(FontDrawRequest,runtime::native::ProcessAccess* =nullptr);
    bool UsesScheduler(const runtime::native::NativeProcessScheduler&) const noexcept;
    bool UsesMetrics(const NativeFontMetrics&) const noexcept;
    bool UsesSink(const FontDrawSink*) const noexcept;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    friend class NativeTalkStringDrawing;
    friend class NativeTalkNamePlateDrawing;
    // Release only host request scratch during higher-continuation cancellation.
    // Original buffer/cache/output prefixes remain untouched.
    void ForgetDraw(FontDrawRequest) noexcept;
    struct State;struct Continuation;
    explicit NativeFontDrawing(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
