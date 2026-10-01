#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace fates::runtime::native {
// The five original descriptor words. Only the low byte of command is dispatched.
struct ProcessDescriptor {
    std::uint32_t command{}, argument{}, argument2{}, target{}, adjustment{};
};
static_assert(sizeof(ProcessDescriptor)==20);
class ProcessProgram final {
public:
    static std::shared_ptr<const ProcessProgram> Create(std::vector<ProcessDescriptor>);
    static std::shared_ptr<const ProcessProgram> Default();
    const std::vector<ProcessDescriptor>& descriptors() const noexcept { return descriptors_; }
private:
    explicit ProcessProgram(std::vector<ProcessDescriptor>);
    const std::vector<ProcessDescriptor> descriptors_;
};
struct ProcessIdentity final { const std::uint64_t serial; };
using ProcessHandle=std::shared_ptr<const ProcessIdentity>;
enum class ProcessStatus : std::uint8_t {
    Ready, Complete, WorkBudget, MissingCallback, CallbackBlocked, InvalidProgram,
    InvalidHandle, InvalidThread, Busy, InvalidName, IdentityExhausted, Retired, InvalidCall
};
enum class ProcessCallKind : std::uint8_t { Descriptor, Persistent, Dispose, Destroy, Service };
struct ProcessCall {
    ProcessHandle process;
    ProcessCallKind kind{};
    std::uint8_t command{};
    std::uint32_t target{};
    std::int32_t this_adjustment{};
    bool has_self{};
    std::array<std::uint32_t,4> arguments{};
    std::uint8_t argument_count{};
};
struct ProcessVirtualMethod {
    std::int32_t this_adjustment{};
    std::uint32_t offset{}, target{};
};
// Derived owners must supply their real virtual methods. An unresolved method
// stops at MissingCallback; it is never replaced with the base implementation.
struct ProcessType {
    std::vector<ProcessVirtualMethod> methods;
    static ProcessType Base();
};
struct ProcessObservation {
    ProcessHandle identity, parent, child, newer, older;
    std::shared_ptr<const ProcessProgram> program;
    std::optional<std::string> name;
    std::uint32_t name_hash{}, persistent_target{}, persistent_adjustment{};
    std::size_t pc{};
    std::uint8_t flags{}, blocking_children{};
    std::int16_t wait_frames{};
    bool linked{}, root{};
};
class NativeProcessScheduler;
class ProcessAccess;
struct ProcessCallbackStep {
    enum class Kind : std::uint8_t { Return, Blocked, Delete, Call, Continue };
    Kind kind{Kind::Blocked};
    std::uint32_t value{};
    ProcessHandle deletion;
    std::optional<ProcessCall> call;
    static ProcessCallbackStep Return(std::uint32_t value=0) {return {Kind::Return,value,{},{}};}
    static ProcessCallbackStep Blocked() {return {};}
    static ProcessCallbackStep Delete(ProcessHandle h) {return {Kind::Delete,0,std::move(h),{}};}
    static ProcessCallbackStep Call(ProcessCall c) {return {Kind::Call,0,{},std::move(c)};}
    // Retain this callback for another host work quantum. This is neither a
    // retail yield nor a missing-service barrier; Run still owns the same tick.
    static ProcessCallbackStep Continue() {return {Kind::Continue,0,{},{}};}
};
// A native service owns its continuation. Step must retain its own progress when
// blocked. Delete suspends it until recursive Dispose finishes; only then is Step
// entered again. No external API can acknowledge a missing service as complete.
class ProcessContinuation {
public:
    virtual ~ProcessContinuation()=default;
    virtual ProcessCallbackStep Step(ProcessAccess&)=0;
};
class ProcessCallbacks {
public:
    virtual ~ProcessCallbacks()=default;
    // A null continuation means unavailable, with no side effects. The scheduler
    // retries Begin only in that case, never after a continuation has started.
    virtual std::unique_ptr<ProcessContinuation> Begin(const ProcessCall&)=0;
};
// One callback address has one native owner in a scheduler domain. Registration
// is atomic and cannot replace an earlier owner, even after its weak lifetime
// expires. A missing module remains a barrier, never another module's fallback.
class ProcessCallbackRegistry final:public ProcessCallbacks {
public:
    bool Register(std::span<const std::uint32_t>,const std::shared_ptr<ProcessCallbacks>&);
    std::unique_ptr<ProcessContinuation> Begin(const ProcessCall&) override;
    std::size_t size() const noexcept {return entries_.size();}
private:
    std::vector<std::pair<std::uint32_t,std::weak_ptr<ProcessCallbacks>>> entries_;
};
struct ProcessRunObservation {
    ProcessStatus status{ProcessStatus::Ready};
    std::uint64_t transitions{};
    std::optional<ProcessCall> callback;
};
// Original Proc root/tree, descriptor dispatch and deferred removal. Exec and
// Sweep are separate operations. Run's budget suspends the SAME operation, not
// another retail frame. Destruction is published only after the actual callback.
class NativeProcessScheduler final {
public:
    explicit NativeProcessScheduler(std::shared_ptr<ProcessCallbacks> callbacks={});
    ~NativeProcessScheduler();
    NativeProcessScheduler(const NativeProcessScheduler&)=delete;
    NativeProcessScheduler& operator=(const NativeProcessScheduler&)=delete;
    ProcessHandle root(std::uint32_t thread) const;
    std::optional<ProcessObservation> Observe(ProcessHandle) const;
    bool HasType(ProcessHandle,const ProcessType&) const noexcept;
    ProcessStatus Create(ProcessHandle parent,std::shared_ptr<const ProcessProgram>,
        std::optional<std::string> name,bool blocking,const ProcessType&,ProcessHandle&);
    ProcessStatus Next(ProcessHandle,bool immediate=false);
    ProcessStatus Jump(ProcessHandle,std::uint32_t label,bool immediate=false);
    ProcessStatus WaitFrame(ProcessHandle,std::uint32_t frames);
    ProcessStatus WaitMilliseconds(ProcessHandle,std::int32_t milliseconds);
    ProcessHandle FindNext(ProcessHandle,bool search_children=true) const;
    ProcessHandle FindByProgram(std::shared_ptr<const ProcessProgram>) const;
    // Original signed-byte name hash and first-match tree order. Hash collisions
    // intentionally match; an empty C string searches the zero name hash.
    ProcessHandle FindByName(std::string_view) const;
    ProcessStatus BeginExec(std::uint32_t thread,std::uint32_t frame_delta);
    ProcessStatus BeginSweep();
    ProcessStatus BeginDelete(ProcessHandle);
    // Invoke a concrete registered native service, retaining nested calls and
    // deletion exactly as during Exec. This cannot impersonate lifecycle calls.
    ProcessStatus BeginServiceCall(ProcessCall);
    bool UsesCallbacks(const ProcessCallbacks*) const noexcept;
    ProcessRunObservation Run(std::size_t work_budget);
    bool busy() const noexcept;
    // Host teardown is not a successful retail Sweep; retained handles go stale.
    void Retire() noexcept;
private:
    friend class ProcessAccess;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
// Borrowed only for the duration of Step. This is the mutation capability for a
// running callback; external operations cannot interleave a suspended Exec/Sweep.
class ProcessAccess final {
public:
    ProcessAccess(const ProcessAccess&)=delete;
    ProcessAccess& operator=(const ProcessAccess&)=delete;
    std::optional<ProcessObservation> Observe(ProcessHandle) const;
    bool BelongsTo(const NativeProcessScheduler&) const noexcept;
    std::uint32_t frame_delta() const noexcept;
    std::uint32_t call_result() const noexcept;
    ProcessStatus Next(ProcessHandle,bool immediate=false);
    ProcessStatus Jump(ProcessHandle,std::uint32_t,bool immediate=false);
    ProcessStatus WaitFrame(ProcessHandle,std::uint32_t);
    ProcessStatus Create(ProcessHandle,std::shared_ptr<const ProcessProgram>,
        std::optional<std::string>,bool,const ProcessType&,ProcessHandle&);
private:
    friend class NativeProcessScheduler;
    explicit ProcessAccess(NativeProcessScheduler& scheduler):scheduler_(scheduler){}
    NativeProcessScheduler& scheduler_;
};
}
