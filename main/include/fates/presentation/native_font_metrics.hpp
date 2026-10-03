#pragma once
#include "fates/io/native_file_entry.hpp"
#include <array>
#include <functional>

namespace fates::runtime::native {class NativeTalkMessageWidth;}
namespace fates::presentation::native {
class NativeFontObjects;
class NativeFontFiles;
class NativeFontStartup;
enum class FontMetricStatus:std::uint8_t {
    Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,InvalidObject,
    DataUnavailable,InvalidData,StaleData,Unavailable,InvalidRequest,ReadLimit,StackUnderflow,IdentityExhausted
};
struct FontMetricView {
    io::native::FileObjectHandle object;
    std::uint64_t data_revision{},setup_generation{};
    std::uint32_t offset{};
};
struct FontGlyphResult {FontMetricStatus status{FontMetricStatus::Unavailable};FontMetricView view;};
struct FontGlyphRecord {
    // Exact retained16-byte record. advance is SIGNED byte12, not box width.
    std::array<std::uint8_t,16> bytes{};
    std::uint16_t code{};
    std::int32_t advance{};
};
struct FontMetricObservation {
    io::native::FileObjectHandle object;
    std::uint64_t data_revision{},setup_generation{};
    std::uint32_t physical_records{},glyph_count{},cached_index{};
    std::uint16_t absent{},maximum_width{};
    std::uint32_t height{};
    // Original header fields used by Draw, distinct from total line height and
    // the append-only number of sheet objects after repeated FontObject Setup.
    std::uint16_t baseline_height{},declared_sheets{};
};
struct FontSelectionState {
    // Slots and selected pointer are independently carried: SetCurrent snapshots
    // a slot, so replacing the slot does not update the selected pointer itself.
    std::array<std::optional<io::native::FileObjectHandle>,4> slots;
    std::optional<std::uint8_t> current_type;
    std::optional<io::native::FileObjectHandle> current;
    std::optional<std::vector<std::uint8_t>> stack;
    // Optional carried retail capacity, independent of the host vector's
    // allocation policy. Older metric-only fixtures may leave it unknown.
    std::optional<std::uint32_t> stack_capacity;
};
struct FontTextIdentity final {const std::uint32_t serial;};
using FontTextRequest=std::shared_ptr<const FontTextIdentity>;
enum class FontTextOperation:std::uint8_t {Width,Lines};
struct FontTextObservation {
    FontTextRequest identity;FontMetricStatus status{FontMetricStatus::Ready};
    bool completed{};std::optional<std::uint32_t> result;std::size_t read_attempts{};
};
// ISFont metric view and process-wide Font selection/width state. Reads the
// existing FileEntry image and binds to the same lower allocation revision.
// SetupMetrics alone does NOT complete FontObject::Setup. NativeFontObjects owns
// external sheet construction separately. Neither method changes ready flags;
// the existing FileBase completion path owns that flag. Font::Load is separate.
// No host font APIs, independent file cache, text parser or handle registry.
class NativeFontMetrics final:public runtime::native::ProcessCallbacks {
public:
    using Reader=std::function<std::optional<char16_t>(std::size_t)>;
    static FontMetricStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<io::native::NativeFileController>,
        std::shared_ptr<io::native::NativeFileBase>,std::shared_ptr<io::native::NativeFileEntry>,std::shared_ptr<NativeFontMetrics>&);
    ~NativeFontMetrics();
    NativeFontMetrics(const NativeFontMetrics&)=delete;
    NativeFontMetrics& operator=(const NativeFontMetrics&)=delete;
    FontMetricStatus SetupMetrics(io::native::FileObjectHandle,runtime::native::ProcessAccess* =nullptr);
    FontMetricStatus SetAbsentCode(io::native::FileObjectHandle,std::optional<char16_t>,runtime::native::ProcessAccess* =nullptr);
    FontGlyphResult GetGlyph(io::native::FileObjectHandle,char16_t,runtime::native::ProcessAccess* =nullptr);
    FontMetricStatus Describe(const FontMetricView&,FontGlyphRecord&) const;
    std::optional<FontMetricObservation> Observe(io::native::FileObjectHandle) const;
    FontMetricStatus RestoreSelection(const FontSelectionState&,runtime::native::ProcessAccess* =nullptr);
    std::optional<FontSelectionState> Selection() const;
    FontMetricStatus PrepareText(Reader,FontTextOperation,FontTextRequest&,runtime::native::ProcessAccess* =nullptr);
    std::optional<runtime::native::ProcessCall> TextCall(runtime::native::ProcessHandle,FontTextRequest) const;
    std::optional<FontTextObservation> Observe(FontTextRequest) const;
    FontMetricStatus Release(FontTextRequest,runtime::native::ProcessAccess* =nullptr);
    static runtime::native::ProcessCall SetCurrentCall(runtime::native::ProcessHandle,std::uint32_t);
    static runtime::native::ProcessCall GetCurrentCall(runtime::native::ProcessHandle);
    static runtime::native::ProcessCall MaxWidthCall(runtime::native::ProcessHandle);
    static runtime::native::ProcessCall MaxHeightCall(runtime::native::ProcessHandle);
    static runtime::native::ProcessCall PushCall(runtime::native::ProcessHandle,std::optional<std::uint32_t> =std::nullopt);
    static runtime::native::ProcessCall PopCall(runtime::native::ProcessHandle);
    static runtime::native::ProcessCall ResetStackCall(runtime::native::ProcessHandle);
    bool UsesScheduler(const runtime::native::NativeProcessScheduler&) const noexcept;
    bool UsesOwners(const io::native::NativeFileController&,const io::native::NativeFileEntry&) const noexcept;
    bool UsesFileBaseOwners(const io::native::NativeFileController&,const io::native::NativeFileBase&) const noexcept;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    friend class NativeTalkDrawerDrawing;
    friend class NativeTalkFontEffects;
    friend class NativeFontObjects;
    friend class NativeFontFiles;
    friend class NativeFontStartup;
    bool CanConstructStaticSelection() const;
    FontMetricStatus ReserveStartupStack(runtime::native::ProcessAccess&);
    // Bind actual static FileBase slots. The existing current pointer is not
    // automatically refreshed when a slot attachment later changes.
    FontMetricStatus BindFileSlots(const std::array<io::native::FileBaseHandle,4>&,
        runtime::native::ProcessAccess*);
    FontMetricStatus RetireMetrics(io::native::FileObjectHandle,runtime::native::ProcessAccess&);
    friend class runtime::native::NativeTalkMessageWidth;
    friend class NativeTalkNamePlateDrawing;
    // Host request scratch only: continuation cancellation releases it without
    // touching original font cache, selector, stack or lower file references.
    void ForgetText(FontTextRequest) noexcept;
    struct State;struct Continuation;
    explicit NativeFontMetrics(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
