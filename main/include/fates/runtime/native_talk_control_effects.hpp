#pragma once
#include "fates/runtime/native_talk_control_scanner.hpp"
#include "fates/runtime/native_talk_control_arguments.hpp"
#include "fates/runtime/native_talk_key_wait.hpp"
#include "fates/runtime/native_talk_wait.hpp"
#include "fates/presentation/native_talk_background.hpp"
#include <map>
#include "fates/presentation/native_talk_font_effects.hpp"
#include "fates/runtime/native_shift_jis.hpp"
namespace fates::runtime::native {
class NativeTalkWindowControls;
class NativeTalkModeFade;
enum class TalkControlStatus:std::uint8_t {
    Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,InvalidParent,
    InvalidHandle,Unavailable,InvalidSource,Unsupported,IdentityExhausted
};
// Original manager fields reached by these controls. Every optional is explicit
// carried state; no ProcTalkManager constructor, default current window, completed
// face loading or auxiliary-child absence is invented by the control owner.
struct TalkControlManagerView {
    ProcessHandle manager;
    std::optional<presentation::native::TalkWindowHandle> window;
    std::optional<presentation::native::TalkBackgroundHandle> background;
    std::optional<ProcessHandle> auxiliary;
    std::optional<std::uint8_t> skip; // +12E5, same field used by effects/key wait.
    std::optional<std::uint8_t> alternate_wait_clock; // +14FB, NOT the skip byte.
    // Three embedded manager windows in original address order. Carried until
    // ProcTalkManager construction is owned; never inferred from Handles().
    std::array<std::optional<presentation::native::TalkWindowHandle>,3> window_slots{};
    std::optional<std::uint8_t> free_window_mode{}; // Original +1294, not manager skip.
    std::optional<std::uint8_t> selection_pending{}; // Original +1295.
    std::optional<std::uint8_t> delete_in_skip{}; // Original +12E6, NOT skip+12E5.
    std::optional<presentation::native::TalkWindowSource> message_cursor{}; // Original +38.
    // Read/write in original mode/fade controls, not aliases of manager skip.
    std::optional<presentation::native::TalkWindowHandle> system_window{}; // +12F8.
    std::optional<std::uint32_t> screen_fade_duration{}; // +12E0, signed duration bits.
    std::optional<std::uint8_t> fade_second_target{}; // +14FA.
    std::optional<std::uint32_t> reveal_counter{}; // +1300, NOT character pacing below.
    std::optional<std::uint32_t> character_delay{}; // +1298, signed arithmetic bits.
    std::optional<std::uint8_t> character_parity{},letter_pulse{}; // +129C/+129D.
    // Original inline pending voice name +129E..+12DF. Untouched bytes remain
    // unknown until a constructor/control supplies them. No shadow voice buffer.
    std::array<std::uint8_t,66> pending_voice{};
    std::bitset<66> pending_voice_known{};
    // Existing manager bytes reached by the resource/skip/end callbacks. These
    // remain explicit until the full constructor/InitializeDirect is owned.
    std::optional<std::uint8_t> render_enabled{}; // +12E4, not held-input skip.
    std::optional<std::uint8_t> suppress_face_fade{}; // +12EC.
    std::optional<std::uint8_t> drawer_acquired{}; // +12ED, written after Initialize.
    std::optional<std::uint8_t> ending_fade{}; // +12FC (0/black/white).
    std::optional<std::uint32_t> saved_skip_flags{}; // +12E8, constructor zero then current flags &~4 when present.
    std::optional<std::uint8_t> shadow_enabled{}; // +12FD, not render_enabled.
    std::optional<std::uint8_t> initialize_face_color{}; // +14F8.
    std::optional<std::uint8_t> initialize_face_motion{}; // +14F9.

};
struct TalkLifecycleUpdate {
    std::optional<std::uint8_t> render,drawer,delete_in_skip,ending_fade;
    std::optional<ProcessHandle> auxiliary;
};
struct TalkCharacterUpdate {
    std::optional<presentation::native::TalkWindowSource> cursor;
    std::optional<std::uint32_t> delay;
    std::optional<std::uint8_t> selection,parity,pulse,skip,voice_first;
};
class NativeTalkControlContext final:public TalkKeyWaitManagerState {
public:
    NativeTalkControlContext(std::shared_ptr<NativeProcessScheduler>,std::shared_ptr<presentation::native::NativeTalkWindow>,std::shared_ptr<presentation::native::NativeTalkBackground>);
    TalkControlStatus RestoreCarried(const TalkControlManagerView&,ProcessAccess* =nullptr);
    TalkControlStatus BeginWindowSelection(ProcessHandle,ProcessAccess&);
    TalkControlStatus SelectCurrentWindow(ProcessHandle,presentation::native::TalkWindowHandle,ProcessAccess&);
    TalkControlStatus ClearAuxiliary(ProcessHandle,ProcessAccess&);
    std::optional<TalkControlManagerView> Observe(ProcessHandle) const;
    // Constructor-owned fields use the SAME existing row. No carried defaults
    // are inserted into callbacks that never ran construction.
    TalkControlStatus ConstructMembers(ProcessHandle,const std::array<presentation::native::TalkWindowHandle,3>&,
        presentation::native::TalkBackgroundHandle,ProcessAccess&);
    TalkControlStatus WriteSavedSkipFlags(ProcessHandle,std::uint32_t,ProcessAccess&);
    // Destructor-only access: the scheduler has unlinked, but not freed, this
    // allocation. Ordinary service/row mutation admission remains unchanged.
    TalkControlStatus ClearOwnedAuxiliary(ProcessHandle,ProcessAccess&);
    TalkControlStatus RetireOwnedMembers(ProcessHandle,ProcessAccess&);

    // Read-only original manager selection helpers. Existing admitted windows
    // have their actual constructor-null attached face, not a forged null face.
    std::optional<presentation::native::TalkWindowHandle> FreeWindow(ProcessHandle,std::uint32_t) const;
    std::optional<presentation::native::TalkWindowHandle> WindowForWidth(ProcessHandle,std::span<const std::uint8_t>) const;
    TalkControlStatus WriteShadowEnabled(ProcessHandle,std::uint8_t,ProcessAccess&);
    TalkControlStatus WriteTalkMode(ProcessHandle,std::uint8_t,ProcessAccess&);
    TalkControlStatus WriteFadeDuration(ProcessHandle,std::uint32_t,ProcessAccess&);
    TalkControlStatus WriteRevealCounter(ProcessHandle,std::uint32_t,ProcessAccess&);
    TalkControlStatus WriteCharacterState(ProcessHandle,const TalkCharacterUpdate&,ProcessAccess&);
    TalkControlStatus WriteLifecycleState(ProcessHandle,const TalkLifecycleUpdate&,ProcessAccess&);
    // Only the original InitializeDirect post-expansion write prefix. Does not
    // construct manager/windows, reset mode/log, or infer auxiliary absence.
    TalkControlStatus CommitInitializedText(ProcessHandle,std::shared_ptr<const NativeTalkText>,ProcessAccess&);

    std::optional<std::uint8_t> Skip(ProcessHandle) const override;
    std::optional<presentation::native::TalkWindowHandle> CurrentWindow(ProcessHandle) const override;
    bool UsesScheduler(const NativeProcessScheduler&) const noexcept override;
    bool UsesWindows(const presentation::native::NativeTalkWindow& window) const noexcept {return windows_.get()==&window;}
    bool UsesOwners(const presentation::native::NativeTalkWindow&,const presentation::native::NativeTalkBackground&) const noexcept;
private:
    std::weak_ptr<NativeProcessScheduler> scheduler_;
    std::shared_ptr<presentation::native::NativeTalkWindow> windows_;
    std::shared_ptr<presentation::native::NativeTalkBackground> backgrounds_;
    std::map<std::uint64_t,TalkControlManagerView> rows_;
};
struct TalkControlRequestIdentity final {const std::uint32_t serial;};
using TalkControlRequest=std::shared_ptr<const TalkControlRequestIdentity>;
struct TalkControlObservation {
    TalkControlRequest identity;ProcessHandle manager;
    TalkControlStatus status{TalkControlStatus::Ready};
    std::optional<TalkCodeRoute> route;
    bool completed{};
    std::optional<std::uint32_t> result; // Dispose returns2/4/5. Flash is void.
};
struct TalkFaceWaitObservation {ProcessHandle process,manager;std::uint64_t deletion_requests{};};
// Execute the original disposal table for the admitted C,p,w,k,L,B,i,e,m handlers
// and default code5, plus genuine Flash delegation/no-op behavior. Unowned
// specialized handlers stay barriers: parsing/Skip is never substituted for an
// effect. Requests retain existing live TalkWindowSource identity, not copied
// text or another token cache. Caller still owns the manager's subsequent Skip.
//
// ProcWaitFaceLoad uses original 196588 IsAllFaceLoadDone callback through the
// existing registry. A missing provider suspends; it never means faces are done.
// B-m uses the shared exact converter and the original32-byte terminated buffer.
// PublishEncoding admits the original tables; default retains the proven ASCII subset.
class NativeTalkControlEffects final:public ProcessCallbacks {
public:
    static TalkControlStatus Create(std::shared_ptr<NativeProcessScheduler>,std::shared_ptr<ProcessCallbackRegistry>,
        std::shared_ptr<ObjectHandleRegistry>,std::shared_ptr<NativeTalkControlContext>,
        std::shared_ptr<presentation::native::NativeTalkWindow>,std::shared_ptr<presentation::native::NativeTalkBackground>,
        std::shared_ptr<presentation::native::NativeTalkMotion>,std::shared_ptr<presentation::native::NativeTalkColorFader>,
        std::shared_ptr<NativeGameSkip>,std::shared_ptr<NativeTalkLog>,std::shared_ptr<NativeTalkWait>,std::shared_ptr<NativeTalkKeyWait>,
        std::shared_ptr<NativeTalkTokens>,std::shared_ptr<NativeTalkControlEffects>&);
    ~NativeTalkControlEffects();
    NativeTalkControlEffects(const NativeTalkControlEffects&)=delete;
    NativeTalkControlEffects& operator=(const NativeTalkControlEffects&)=delete;
    TalkControlStatus PublishModeFade(std::shared_ptr<NativeTalkModeFade>,ProcessAccess* =nullptr);
    TalkControlStatus PublishWindowControls(std::shared_ptr<NativeTalkWindowControls>,ProcessAccess* =nullptr);
    TalkControlStatus PublishFontEffects(std::shared_ptr<presentation::native::NativeTalkFontEffects>,ProcessAccess* =nullptr);
    TalkControlStatus PublishEncoding(std::shared_ptr<const NativeShiftJis>,ProcessAccess* =nullptr);
    TalkControlStatus Prepare(ProcessHandle,presentation::native::TalkWindowSource,std::size_t start,TalkCodeOperation,TalkControlRequest&,ProcessAccess* =nullptr);
    std::optional<ProcessCall> Call(TalkControlRequest) const;
    std::optional<TalkControlObservation> Observe(TalkControlRequest) const;
    TalkControlStatus Release(TalkControlRequest,ProcessAccess* =nullptr);
    void Forget(TalkControlRequest) noexcept; // Cancel host request only, not committed game effects.
    bool UsesOwners(const NativeTalkControlContext&,const presentation::native::NativeTalkWindow&,
        const NativeTalkLog&,const NativeGameSkip&) const noexcept;
    std::vector<TalkFaceWaitObservation> FaceWaits() const;
    bool UsesTokens(const NativeTalkTokens&) const noexcept;
    std::unique_ptr<ProcessContinuation> Begin(const ProcessCall&) override;
private:
    struct State;struct Continuation;
    explicit NativeTalkControlEffects(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
