#pragma once
#include "fates/presentation/native_talk_motion.hpp"
#include "fates/presentation/native_talk_color_fader.hpp"
#include "fates/presentation/native_talk_layout.hpp"

namespace fates::presentation::native {
// Borrowed +12E5 manager byte. Unknown never means no-skip. The eventual actual
// manager supplies this state; this component does not construct or finish Talk.
class TalkEffectManagerState {
public:
    virtual ~TalkEffectManagerState()=default;
    virtual bool UsesScheduler(const runtime::native::NativeProcessScheduler&) const noexcept=0;
    virtual std::optional<std::uint8_t> Skip(runtime::native::ProcessHandle) const=0;
};
enum class TalkWindowEffect:std::uint8_t {StartOpen,OpenFace,OpenStand,StartClose,WaitNextMessage,NameFadeIn,NameFadeOut,SetKeyWait,FadeOutFace,FadeOutFaceInSkip};
enum class TalkEffectStatus:std::uint8_t {Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Retired};
struct TalkEffectStaticColors {std::optional<TalkColorBytes> name_in_shadow,name_out_shadow;};
// Original effect composers over actual carrier/fader/scroll children. The
// admitted NativeTalkWindow has genuine constructor-null face state. Attaching a
// face needs its real owner; no face-derived behavior is synthesized here.
// StartClose calls the original StopAllVoice service via the existing callback
// registry and suspends there until a real service exists. No null sound success.
class NativeTalkWindowEffects final:public runtime::native::ProcessCallbacks {
public:
    static TalkEffectStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<runtime::native::ObjectHandleRegistry>,std::shared_ptr<NativeTalkWindow>,
        std::shared_ptr<NativeTalkMotion>,std::shared_ptr<NativeTalkColorFader>,
        std::shared_ptr<NativeTalkLayout>,std::shared_ptr<TalkEffectManagerState>,std::shared_ptr<NativeTalkWindowEffects>&);
    ~NativeTalkWindowEffects();
    NativeTalkWindowEffects(const NativeTalkWindowEffects&)=delete;
    NativeTalkWindowEffects& operator=(const NativeTalkWindowEffects&)=delete;
    std::optional<runtime::native::ProcessCall> Call(runtime::native::ProcessHandle,TalkWindowHandle,TalkWindowEffect,std::int32_t parameter=0) const;
    std::optional<TalkEffectStaticColors> ObserveStaticColors() const;
    bool UsesOwners(const NativeTalkWindow&,const TalkEffectManagerState&) const noexcept;
    bool UsesScheduler(const runtime::native::NativeProcessScheduler&) const noexcept;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    struct State;struct Continuation;
    explicit NativeTalkWindowEffects(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
