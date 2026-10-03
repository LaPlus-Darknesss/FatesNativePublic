#pragma once
#include "fates/io/native_coordinate_resources.hpp"

namespace fates::presentation::native {
enum class TalkDrawerStatus:std::uint8_t {Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired};
struct TalkDrawerObservation {
    std::uint32_t references{};
    bool instance_present{},instance_live{};
    std::array<io::native::FileBaseHandle,2> files;
    std::array<io::native::CoordinateSceneHandle,3> scenes;
};
// The actual shared drawer resource lifecycle, not a renderer or Talk process.
// Initialize constructs the three scenes but does NOT TextureLoad them. GPU
// SetState/drawing, font/window/control/input and ProcTalkManager remain separate.
class NativeTalkWindowDrawer final:public runtime::native::ProcessCallbacks {
public:
    static TalkDrawerStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<io::native::NativeFileController>,
        std::shared_ptr<io::native::NativeFileBase>,std::shared_ptr<io::native::NativeTexFiles>,
        std::shared_ptr<io::native::NativeCoordinateResources>,std::shared_ptr<NativeTalkWindowDrawer>&);
    ~NativeTalkWindowDrawer();
    NativeTalkWindowDrawer(const NativeTalkWindowDrawer&)=delete;
    NativeTalkWindowDrawer& operator=(const NativeTalkWindowDrawer&)=delete;
    // Explicit carried projection of game::graphics::SharedTexture's first word.
    // Never infer it from chapter, asset presence or a default route. The original
    // Initialize reads its low byte only when it reaches the second file.
    TalkDrawerStatus PublishSharedTextureWord(std::optional<std::uint32_t>,runtime::native::ProcessAccess* =nullptr);
    static runtime::native::ProcessCall InitializeCall(runtime::native::ProcessHandle,bool asynchronous);
    static runtime::native::ProcessCall TickLoadAsyncCall(runtime::native::ProcessHandle);
    static runtime::native::ProcessCall FinalizeCall(runtime::native::ProcessHandle);
    std::optional<TalkDrawerObservation> Observe() const;
    // Read-only equivalent of the two FileBase::IsDone tests reached by Talk's
    // WaitLoadAsync. It neither finishes a read nor advances a process descriptor.
    std::optional<bool> FilesReady() const;
    bool UsesScheduler(const runtime::native::NativeProcessScheduler&) const noexcept;
    bool UsesOwners(const io::native::NativeFileController&,const io::native::NativeFileBase&,
        const io::native::NativeTexFiles&,const io::native::NativeCoordinateResources&) const noexcept;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    struct State;struct Continuation;
    explicit NativeTalkWindowDrawer(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
