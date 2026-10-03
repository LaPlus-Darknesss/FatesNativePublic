#pragma once
#include "fates/runtime/native_talk_reveal.hpp"
#include "fates/runtime/native_talk_mode_fade.hpp"
#include "fates/presentation/native_talk_window_drawer.hpp"

namespace fates::runtime::native {
enum class TalkLifecycleOperation:std::uint8_t {Load,WaitLoadAsync,Skip,Resume,StartFade,EndFade,EndFadeForce,Release,StartSkip};
// Original manager descriptor callbacks surrounding the already-owned Tick.
// No manager/global constructor, event binding, FaceManager or GPU provider is
// invented. ReleaseAllFace(1964DC) and StopAllVoice(41F6C4) are explicit services
// in the existing callback registry. Missing services suspend at their real call.
// Resume's ProcBind is a BLOCKING CHILD OF THE MANAGER'S PARENT (its sibling),
// not a blocker on the manager. It uses the original default virtual-Tick program.
class NativeTalkLifecycle final:public ProcessCallbacks {
public:
    static TalkControlStatus Create(std::shared_ptr<NativeProcessScheduler>,std::shared_ptr<ProcessCallbackRegistry>,
        std::shared_ptr<NativeTalkControlContext>,std::shared_ptr<presentation::native::NativeTalkWindow>,
        std::shared_ptr<presentation::native::NativeTalkWindowEffects>,std::shared_ptr<presentation::native::NativeTalkWindowDrawer>,
        std::shared_ptr<presentation::native::NativeTalkSpeaker>,std::shared_ptr<NativeTalkControlEffects>,
        std::shared_ptr<NativeTalkLog>,std::shared_ptr<NativeTalkTokens>,std::shared_ptr<NativeGameSkip>,
        std::shared_ptr<presentation::native::NativeFadeSystem>,std::shared_ptr<NativeTalkModeFade>,
        std::shared_ptr<NativeTalkLifecycle>&);
    ~NativeTalkLifecycle();
    NativeTalkLifecycle(const NativeTalkLifecycle&)=delete;
    NativeTalkLifecycle& operator=(const NativeTalkLifecycle&)=delete;
    static std::optional<ProcessCall> Call(ProcessHandle,TalkLifecycleOperation,bool force=false);
    std::vector<ProcessHandle> AuxiliaryProcesses() const;
    // Shared ProcBind construction used by Resume and the real manager factory.
    // Does not publish a manager field or jump a manager descriptor.
    TalkControlStatus BindAuxiliary(ProcessAccess&,ProcessHandle,ProcessHandle&);
    bool UsesContext(const NativeTalkControlContext&)const noexcept;
    bool UsesOwners(const NativeTalkControlContext&,const presentation::native::NativeTalkWindow&,
        const NativeTalkLog&,const NativeTalkTokens&,const NativeGameSkip&) const noexcept;
    bool UsesScheduler(const NativeProcessScheduler&) const noexcept;
    std::unique_ptr<ProcessContinuation> Begin(const ProcessCall&) override;
private:
    struct State;struct Continuation;
    explicit NativeTalkLifecycle(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
