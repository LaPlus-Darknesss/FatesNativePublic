#pragma once
#include "fates/runtime/native_process.hpp"

namespace fates::runtime::native {
// Registered native callback and logical argument words. Resource identities
// use these words instead of executing retail pointers on the host.
struct DelayedResourceCall {
    std::uint32_t target{};
    std::array<std::uint32_t,2> arguments{};
    std::uint8_t argument_count{};
};
enum class ResourceDelayStatus:std::uint8_t {Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,IdentityExhausted};
struct ResourceDelayEntry {std::uint16_t slot{};std::int32_t ticks{};DelayedResourceCall callback;};
struct ResourceDelaySnapshot {
    bool present{};
    std::uint64_t identity{};
    std::size_t free_slots{};
    std::vector<ResourceDelayEntry> entries;
};
struct ResourceWaitSnapshot {ProcessHandle process;std::uint32_t ticks{};std::uint8_t cleanup{};};
// A delay tick is explicit; Exec does not secretly advance it. Cleanup calls
// actual registered UnitIconManager/FrameManager/File services, never an ack.
class NativeResourceDelay final:public ProcessCallbacks {
public:
    static ResourceDelayStatus Create(std::shared_ptr<NativeProcessScheduler>,
        std::shared_ptr<ProcessCallbackRegistry>,std::shared_ptr<NativeResourceDelay>&);
    ~NativeResourceDelay();
    NativeResourceDelay(const NativeResourceDelay&)=delete;
    NativeResourceDelay& operator=(const NativeResourceDelay&)=delete;
    ResourceDelayStatus Initialize();
    ResourceDelayStatus Initialize(ProcessAccess&);
    // Explicit caller null publication. Does not finish callbacks or release
    // earlier queues. Unknown publication remains distinct from known absence.
    ResourceDelayStatus PublishAbsent();
    ResourceDelayStatus PublishAbsent(ProcessAccess&);
    bool UsesScheduler(const NativeProcessScheduler&) const noexcept;
    std::optional<ResourceDelaySnapshot> Observe() const;
    std::vector<ResourceWaitSnapshot> Waits() const;
    static std::optional<ProcessCall> EntryCall(ProcessHandle,const DelayedResourceCall&);
    static ProcessCall InitializeCall(ProcessHandle);
    static ProcessCall TickCall(ProcessHandle);
    static ProcessCall IsActiveCall(ProcessHandle);
    static ProcessCall DumpCall(ProcessHandle);
    static ProcessCall BindCall(ProcessHandle parent,std::uint32_t cleanup);
    std::unique_ptr<ProcessContinuation> Begin(const ProcessCall&) override;
private:
    struct State;
    struct Continuation;
    explicit NativeResourceDelay(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
