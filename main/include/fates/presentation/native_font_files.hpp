#pragma once
#include "fates/presentation/native_font_objects.hpp"

namespace fates::presentation::native {
enum class FontFileStatus:std::uint8_t {
    Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,InvalidSlot,Unavailable
};
// Original Font::Load/Free/FinishAsync/IsAsyncLoading over four real FileBases.
// Slots are the existing metrics owner's live attachment sources; selected font
// remains its independent captured pointer. No second cache, selector or worker.
// BindSlots explicitly supplies already-constructed static bases. It is not the
// full Font::Initialize/bootstrap and does not set the initial selector/stack.
class NativeFontFiles final:public runtime::native::ProcessCallbacks {
public:
    static FontFileStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<io::native::NativeFileController>,
        std::shared_ptr<io::native::NativeFileBase>,std::shared_ptr<io::native::NativeFileEntry>,
        std::shared_ptr<NativeFontMetrics>,std::shared_ptr<NativeFontObjects>,std::shared_ptr<NativeFontFiles>&);
    ~NativeFontFiles();
    NativeFontFiles(const NativeFontFiles&)=delete;
    NativeFontFiles& operator=(const NativeFontFiles&)=delete;
    FontFileStatus BindSlots(const std::array<io::native::FileBaseHandle,4>&,runtime::native::ProcessAccess* =nullptr);
    std::optional<io::native::FileBaseObservation> ObserveSlot(std::uint32_t) const;
    // name is an existing path capability for the complete filename relative to
    // font/, e.g. Talk.bfnt.lz. A null name is the original no-op, not an empty name.
    std::optional<runtime::native::ProcessCall> LoadCall(runtime::native::ProcessHandle,std::uint32_t,
        io::native::FilePathHandle name,bool asynchronous=false) const;
    static runtime::native::ProcessCall FreeCall(runtime::native::ProcessHandle,std::uint32_t);
    static runtime::native::ProcessCall FinishAsyncCall(runtime::native::ProcessHandle,std::uint32_t);
    static runtime::native::ProcessCall IsAsyncLoadingCall(runtime::native::ProcessHandle,std::uint32_t);
    bool UsesOwners(const io::native::NativeFileBase&,const NativeFontMetrics&,const NativeFontObjects&) const noexcept;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    struct State;struct Continuation;
    explicit NativeFontFiles(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
