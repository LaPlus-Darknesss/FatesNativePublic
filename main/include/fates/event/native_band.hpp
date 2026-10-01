#pragma once
#include "fates/presentation/native_fade.hpp"
#include "fates/runtime/native_move_time.hpp"

namespace fates::event::native {
struct BandMotionState {
    runtime::native::MoveTimeState time{2,2,0,0};
    float origin{},target{},current{};
};
struct BandRectangle {float x{},y{},width{},height{};};
struct BandDraw {
    runtime::native::ProcessHandle process;
    std::array<BandRectangle,2> rectangles;
    presentation::native::FadeColor color{};
};
// Logical400x240 output. The PC presentation seam owns layout; the sink cannot
// advance time, acknowledge a callback or decide process completion.
class BandDrawSink {
public:
    virtual ~BandDrawSink()=default;
    virtual void Draw(const BandDraw&)=0;
};
class NullBandDrawSink final:public BandDrawSink {
public:
    void Draw(const BandDraw&) override {}
};
enum class BandStatus:std::uint8_t {
    Ready,NullScheduler,MismatchedDomain,MissingSink,DuplicateBinding,Busy,
    Retired,UnknownFade,InvalidParent,UnownedNamedProcess,InvalidState,InvalidHandle
};
struct BandSnapshot {runtime::native::ProcessHandle process;BandMotionState motion;};
// Original shared-name creation/reuse, Default descriptor program, Tick,
// Persistent and actual deferred destruction. There is no private singleton.
class NativeBand final:public runtime::native::ProcessCallbacks {
public:
    static BandStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,
        std::shared_ptr<presentation::native::NativeFadeSystem>,std::shared_ptr<BandDrawSink>,
        std::shared_ptr<NativeBand>&);
    BandStatus Open(runtime::native::ProcessHandle parent,runtime::native::ProcessHandle&);
    BandStatus Open(runtime::native::ProcessAccess&,runtime::native::ProcessHandle parent,runtime::native::ProcessHandle&);
    // Admission check for compound native commands before their first mutation.
    BandStatus CanOpen(runtime::native::ProcessAccess&,runtime::native::ProcessHandle parent) const;
    // Explicit carried state on an already-owned, live process. This does not
    // implement BandClose or manufacture a process/current ChapterSequence.
    BandStatus RestoreCarriedMotion(runtime::native::ProcessHandle,const BandMotionState&);
    std::optional<BandSnapshot> Observe(runtime::native::ProcessHandle) const;
    std::vector<runtime::native::ProcessHandle> Processes() const;
    bool UsesScheduler(const runtime::native::NativeProcessScheduler&) const noexcept;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    struct State;
    struct Continuation;
    explicit NativeBand(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
