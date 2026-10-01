#pragma once
#include "fates/runtime/native_game_skip.hpp"

namespace fates::runtime::native {
enum class ChapterScopeStatus:std::uint8_t {
    Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,AlreadyBound,InvalidProcess
};
struct ChapterScopeSnapshot {ProcessHandle process;std::optional<std::uint32_t> status_flags;};
// Carried ChapterSequence identity and status, following the existing phase
// sequence admission route. This does not execute Create's gameplay setup or
// the chapter destructor. All88 original descriptors and derived virtuals are
// preserved; unowned callbacks and destruction remain real scheduler barriers.
class NativeChapterSequenceScope final:public ProcessCallbacks {
public:
    static ChapterScopeStatus Create(std::shared_ptr<NativeProcessScheduler>,
        std::shared_ptr<ProcessCallbackRegistry>,std::shared_ptr<NativeGameSkip>,
        std::shared_ptr<NativeChapterSequenceScope>&);
    ChapterScopeStatus BindCarried(ProcessHandle,std::optional<std::uint32_t> status_flags);
    ChapterScopeStatus BindCarriedAbsent();
    // Unknown, known absent and carried positive identity are distinct. Marking
    // or unlinking alone never clears the original global before its destructor.
    std::optional<ProcessHandle> Current() const;
    std::optional<ChapterScopeSnapshot> Observe(ProcessHandle) const;
    bool UsesScheduler(const NativeProcessScheduler&) const noexcept;
    static std::shared_ptr<const ProcessProgram> Program();
    static ProcessType Type();
    std::unique_ptr<ProcessContinuation> Begin(const ProcessCall&) override;
private:
    struct State;
    struct Continuation;
    explicit NativeChapterSequenceScope(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
