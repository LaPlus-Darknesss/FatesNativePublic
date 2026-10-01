#pragma once
#include "fates/map/native_actor_visual.hpp"
#include "fates/runtime/native_process.hpp"
namespace fates::runtime::native {struct NativeRuntime;}
namespace fates::event::native {
// Canonical descriptor identity at752BCC, reconstructed by the original
// ProcTalkManager initializer. This does not implement its talk callbacks.
std::shared_ptr<const runtime::native::ProcessProgram> TalkManagerProcessProgram();
bool EventTypeSkipsActorConsistency(std::uint32_t) noexcept;
enum class EventConsistencyStatus:std::uint8_t {Ready,NullRuntime,NullScheduler,MismatchedDomain,DuplicateBinding};
class NativeEventConsistency final:public runtime::native::ProcessCallbacks {
public:
    static EventConsistencyStatus Create(std::shared_ptr<runtime::native::NativeRuntime>,
        std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<NativeEventConsistency>&);
    ~NativeEventConsistency();
    NativeEventConsistency(const NativeEventConsistency&)=delete;
    NativeEventConsistency& operator=(const NativeEventConsistency&)=delete;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
    std::optional<map::native::ActorVisualResult> last_actor_result() const;
    static runtime::native::ProcessCall Call(runtime::native::ProcessHandle,std::uint32_t type);
private:
    struct State;struct Continuation;
    explicit NativeEventConsistency(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
