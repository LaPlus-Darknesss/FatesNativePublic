#pragma once
#include "fates/presentation/native_font_metrics.hpp"
#include "fates/io/native_texture_objects.hpp"

namespace fates::presentation::native {
enum class FontObjectStatus:std::uint8_t {
    Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,InvalidObject,
    DataUnavailable,InvalidData,StaleView,InvalidIndex,IdentityExhausted
};
struct FontSheetView {
    io::native::FileObjectHandle object;
    std::uint32_t index{};
    std::uint64_t sheet_identity{};
};
struct FontObjectObservation {
    io::native::FileObjectHandle object;
    std::uint32_t sheet_count{},sheet_capacity{},destroyed_sheets{};
    std::uint64_t completed_setups{};
    bool members_retired{};
    FontObjectStatus last_status{FontObjectStatus::Ready};
    std::vector<std::uint64_t> destruction_order;
};
// FontObject's original member construction, Setup, Cleanup, and destruction.
// Each sheet is an ITexture of target External: its pixels belong to the same
// FileEntry allocation, never to another texture cache or upload provider.
// Setup appends; Cleanup is the original no-op. The destructor retires sheets
// forward and ISFont metrics before invoking the existing RawFileObject dtor.
// FileBase::Close/Finish, not this owner, commits the lower ready flag.
class NativeFontObjects final:public runtime::native::ProcessCallbacks {
public:
    static FontObjectStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<io::native::NativeFileController>,
        std::shared_ptr<io::native::NativeFileBase>,std::shared_ptr<io::native::NativeFileEntry>,
        std::shared_ptr<NativeFontMetrics>,std::shared_ptr<NativeFontObjects>&);
    ~NativeFontObjects();
    NativeFontObjects(const NativeFontObjects&)=delete;
    NativeFontObjects& operator=(const NativeFontObjects&)=delete;
    static io::native::FileObjectMethods Methods() noexcept;
    FontObjectStatus Construct(io::native::FileObjectHandle&,runtime::native::ProcessAccess* =nullptr);
    // Explicit carried state only, for an already constructed lower object with
    // original methods and known empty sheet vector. Not automatic adoption.
    FontObjectStatus RestoreEmptyConstructed(io::native::FileObjectHandle,runtime::native::ProcessAccess* =nullptr);
    std::optional<FontObjectObservation> Observe(io::native::FileObjectHandle) const;
    std::optional<FontSheetView> GetSheet(io::native::FileObjectHandle,std::uint32_t) const;
    FontObjectStatus Describe(const FontSheetView&,io::native::NativeTextureDescription&) const;
    FontObjectStatus PackedBytes(const FontSheetView&,std::vector<std::uint8_t>&) const;
    static runtime::native::ProcessCall SetupCall(runtime::native::ProcessHandle,io::native::FileObjectHandle);
    static runtime::native::ProcessCall CleanupCall(runtime::native::ProcessHandle,io::native::FileObjectHandle);
    static runtime::native::ProcessCall DestructorCall(runtime::native::ProcessHandle,io::native::FileObjectHandle);
    bool UsesScheduler(const runtime::native::NativeProcessScheduler&) const noexcept;
    bool UsesOwners(const io::native::NativeFileController&,const io::native::NativeFileEntry&,const NativeFontMetrics&) const noexcept;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    struct State;struct Continuation;
    explicit NativeFontObjects(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
