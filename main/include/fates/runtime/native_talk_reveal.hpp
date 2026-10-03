#pragma once
#include "fates/runtime/native_talk_message_width.hpp"
#include "fates/presentation/native_talk_speaker.hpp"

namespace fates::runtime::native {
// Original ProcTalkManager::AppendCharacter + Tick over the existing manager
// context, text, window/log, width/effect, control, GameSkip and scheduler owners.
// Does not construct/publish ProcTalkManager or pretend faces/fonts/audio are loaded.
// Tick can advance the supplied manager descriptor at the actual end of message;
// this is NOT event-VM completion or a full Talk process/delivery claim.
//
// Input uses the SAME GameSkipInputSource: held+0C and triggered+14 are distinct.
// Pending voice services 41FFE4 then41FAD4 go through the existing callback registry.
// Their sole native argument is inline-field tag129E; call.process is the owner.
// A provider reads context.pending_voice with its knownness. Missing providers
// block without repeating character/log/counter work; no silent audio success.
class NativeTalkReveal final:public ProcessCallbacks {
public:
    static TalkControlStatus Create(std::shared_ptr<NativeProcessScheduler>,std::shared_ptr<ProcessCallbackRegistry>,
        std::shared_ptr<NativeTalkControlContext>,std::shared_ptr<presentation::native::NativeTalkWindow>,
        std::shared_ptr<presentation::native::NativeTalkWindowEffects>,std::shared_ptr<presentation::native::NativeTalkMotion>,
        std::shared_ptr<NativeTalkMessageWidth>,
        std::shared_ptr<presentation::native::NativeTalkSpeaker>,std::shared_ptr<NativeTalkControlEffects>,
        std::shared_ptr<NativeTalkLog>,std::shared_ptr<NativeTalkTokens>,std::shared_ptr<NativeGameSkip>,
        std::shared_ptr<GameSkipInputSource>,std::shared_ptr<NativeTalkReveal>&);
    ~NativeTalkReveal();
    NativeTalkReveal(const NativeTalkReveal&)=delete;NativeTalkReveal& operator=(const NativeTalkReveal&)=delete;
    static ProcessCall AppendCharacterCall(ProcessHandle);
    static ProcessCall TickCall(ProcessHandle);
    bool UsesScheduler(const NativeProcessScheduler&) const noexcept;
    std::unique_ptr<ProcessContinuation> Begin(const ProcessCall&) override;
private:
    struct State;struct Continuation;explicit NativeTalkReveal(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
