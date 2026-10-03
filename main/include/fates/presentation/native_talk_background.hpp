#pragma once
#include "fates/io/native_tex_files.hpp"
#include "fates/runtime/native_object_registry.hpp"
#include "fates/presentation/native_talk_color.hpp"
#include "fates/presentation/native_talk_position.hpp"

namespace fates::presentation::native {
struct TalkBackgroundIdentity final {const std::uint32_t serial;};
using TalkBackgroundHandle=std::shared_ptr<const TalkBackgroundIdentity>;
struct TalkBackgroundNameIdentity final {const std::uint32_t serial;};
using TalkBackgroundName=std::shared_ptr<const TalkBackgroundNameIdentity>;
enum class TalkBackgroundStatus:std::uint8_t {
    Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,
    InvalidHandle,InvalidSelection,InvalidName,IdentityExhausted,Unavailable
};
struct TalkBackgroundState {
    TalkBackgroundHandle identity;
    runtime::native::ObjectHandle movable_handle;
    runtime::native::ObjectIdentity movable_identity;
    std::array<std::uint32_t,3> movable_position{};
    std::array<io::native::FileBaseHandle,2> files;
    std::array<runtime::native::ObjectHandle,2> color_handles;
    std::array<runtime::native::ObjectIdentity,2> color_identities;
    std::array<std::array<std::uint8_t,4>,2> colors;
    std::array<std::array<std::uint32_t,3>,2> positions{};
    std::uint32_t selected{};
    std::uint8_t float_first{1};
};
struct TalkBackgroundRect {
    io::native::NativeTextureView texture;
    std::array<std::uint32_t,3> position;
    std::array<std::uint8_t,4> color;
    std::uint16_t width{},height{};
    std::int16_t priority{};
    std::uint32_t slot{};
};
struct TalkBackgroundDraw {
    std::uint32_t parallax_bits{0xbf666666u}; // Original -0.9 binary32 literal.
    std::vector<TalkBackgroundRect> rectangles;
};
// Renderer boundary only. A false return retains the SAME pending submission.
// Implementations must not partially commit a false-returning batch. A null sink
// is explicit headless output, not a gameplay or process completion provider.
class TalkBackgroundSink {
public:
    virtual ~TalkBackgroundSink()=default;
    virtual bool Submit(const TalkBackgroundDraw&)=0;
};
// Original TalkBg state, selected-layer positions/color handles and two real
// TexFiles. Reuses the existing ObjectHandleRegistry supplied by the composition.
// No second file cache, fader, event VM, GPU implementation or Talk manager.
class NativeTalkBackground final:public runtime::native::ProcessCallbacks,public TalkColorOwner,public TalkPositionOwner {
public:
    static TalkBackgroundStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<runtime::native::ObjectHandleRegistry>,
        std::shared_ptr<io::native::NativeFileController>,std::shared_ptr<io::native::NativeFileBase>,
        std::shared_ptr<io::native::NativeTexFiles>,std::shared_ptr<io::native::NativeTextureObjects>,
        std::shared_ptr<TalkBackgroundSink>,std::shared_ptr<NativeTalkBackground>&);
    ~NativeTalkBackground();
    NativeTalkBackground(const NativeTalkBackground&)=delete;
    NativeTalkBackground& operator=(const NativeTalkBackground&)=delete;
    TalkBackgroundStatus Construct(TalkBackgroundHandle&,runtime::native::ProcessAccess* =nullptr);
    TalkBackgroundStatus RegisterName(std::string_view,TalkBackgroundName&,runtime::native::ProcessAccess* =nullptr);
    TalkBackgroundStatus Select(TalkBackgroundHandle,std::uint32_t,runtime::native::ProcessAccess* =nullptr);
    TalkBackgroundStatus FloatSelected(TalkBackgroundHandle,runtime::native::ProcessAccess* =nullptr);
    TalkBackgroundStatus SetPosition(TalkBackgroundHandle,std::array<std::uint32_t,3>,runtime::native::ProcessAccess* =nullptr);
    TalkBackgroundStatus SetPositionX(TalkBackgroundHandle,std::uint32_t,runtime::native::ProcessAccess* =nullptr);
    TalkBackgroundStatus SetColor(TalkBackgroundHandle,std::size_t,std::array<std::uint8_t,4>,runtime::native::ProcessAccess* =nullptr);
    std::optional<TalkBackgroundState> Observe(TalkBackgroundHandle) const;
    std::optional<std::array<std::uint32_t,3>> Position(TalkBackgroundHandle) const;
    std::optional<runtime::native::ProcessCall> LoadCall(runtime::native::ProcessHandle,TalkBackgroundHandle,TalkBackgroundName) const;
    std::optional<runtime::native::ProcessCall> DrawCall(runtime::native::ProcessHandle,TalkBackgroundHandle,std::uint16_t) const;
    // Retail non-deleting TalkBg destructor. Its caller separately owns allocation
    // deletion; do not insert an extra operator delete or report its completion.
    std::optional<runtime::native::ProcessCall> DestroyCall(runtime::native::ProcessHandle,TalkBackgroundHandle) const;
    bool UsesObjectRegistry(const runtime::native::ObjectHandleRegistry&) const noexcept override;
    bool UsesScheduler(const runtime::native::NativeProcessScheduler&) const noexcept override;
    // ProcCarrier dereferences the registered Movable base fields directly.
    // This is NOT the virtual selected-layer Position/SetPosition interface.
    std::optional<TalkVectorBits> ReadPosition(runtime::native::ObjectIdentity) const override;
    bool WritePosition(runtime::native::ObjectIdentity,TalkVectorBits,runtime::native::ProcessAccess&) override;
    std::optional<TalkColorBytes> ReadColor(runtime::native::ObjectIdentity) const override;
    bool WriteColorChannel(runtime::native::ObjectIdentity,std::uint8_t,std::uint8_t,runtime::native::ProcessAccess&) override;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    struct State;struct Continuation;
    explicit NativeTalkBackground(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
