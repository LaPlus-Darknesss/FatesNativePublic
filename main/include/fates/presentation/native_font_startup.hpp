#pragma once
#include "fates/presentation/native_font_files.hpp"
#include <variant>

namespace fates::presentation::native {
enum class FontStartupStatus:std::uint8_t {
    Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,Unavailable,InvalidState
};
// Explicit carried bytes remain opaque. They are never interpreted as host
// pointers. Drawing writes typed views into this same buffer owner instead.
struct FontStartupBuffer {
    std::vector<std::array<std::uint8_t,8>> records;
    std::uint32_t capacity{};
};
struct FontQueuedGlyph {FontMetricView glyph;std::uint32_t x_bits{};};
using FontBufferRecord=std::variant<std::array<std::uint8_t,8>,FontQueuedGlyph>;
struct FontBufferObservation {
    std::vector<FontBufferRecord> records;
    std::uint32_t capacity{};
};
enum class FontStaticLifetime:std::uint8_t {Carried,Constructed,Destroying,Destroyed};
using FontStaticColors=std::array<std::array<std::uint8_t,4>,12>;
struct FontStartupObservation {
    std::array<std::optional<FontStartupBuffer>,8> buffers;
    std::uint64_t completed_initializations{};
    FontStaticLifetime static_lifetime{FontStaticLifetime::Carried};
    std::optional<FontStaticColors> colors;
    std::optional<std::array<io::native::FileBaseHandle,4>> static_slots;
    std::uint8_t destroyed_slots{};
};
// Original Font startup over the actual loader and existing selector owner.
// Create registers the service only. ConstructFreshStorage establishes a new
// static lifetime; callers may instead supply C25's explicitly carried storage.
class NativeFontStartup final:public runtime::native::ProcessCallbacks {
public:
    static FontStartupStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<io::native::NativeFileBase>,
        std::shared_ptr<NativeFontMetrics>,std::shared_ptr<NativeFontObjects>,
        std::shared_ptr<NativeFontFiles>,std::shared_ptr<NativeFontStartup>&);
    ~NativeFontStartup();
    NativeFontStartup(const NativeFontStartup&)=delete;
    NativeFontStartup& operator=(const NativeFontStartup&)=delete;
    // Fresh executable image plus __sti___8_Font_cpp. Refuses existing carried
    // selection/buffers/bindings. Creates real empty FileBase identities and
    // binds them through the existing loader; performs no font file reads.
    FontStartupStatus ConstructFreshStorage(runtime::native::ProcessAccess* =nullptr);
    FontStartupStatus RestoreBuffer(std::uint32_t,const FontStartupBuffer&,runtime::native::ProcessAccess* =nullptr);
    // Legacy observation exposes a buffer only while all its records are opaque
    // carried bytes. ObserveBuffer is the complete typed/opaque observation.
    std::optional<FontStartupObservation> Observe() const;
    std::optional<FontBufferObservation> ObserveBuffer(std::uint32_t) const;
    bool UsesOwners(const NativeFontMetrics&,const NativeFontObjects&) const noexcept;
    static runtime::native::ProcessCall InitializeCall(runtime::native::ProcessHandle);
    // Original registered FileHandle<FontObject> array destructor. Releases
    // slots 3,2,1,0 through FileBase, without Font::Free's selector refresh.
    static runtime::native::ProcessCall DestroyStaticSlotsCall(runtime::native::ProcessHandle);
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    friend class NativeFontDrawing;
    FontStartupStatus ClearDrawBuffer(std::uint32_t,runtime::native::ProcessAccess&);
    FontStartupStatus AppendDrawGlyph(std::uint32_t,const FontQueuedGlyph&,bool&,runtime::native::ProcessAccess&);
    std::optional<std::size_t> DrawBufferSize(std::uint32_t) const;
    std::optional<FontQueuedGlyph> DrawGlyph(std::uint32_t,std::size_t) const;
    struct State;struct Continuation;
    explicit NativeFontStartup(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
