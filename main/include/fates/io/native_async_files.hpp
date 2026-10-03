#pragma once
#include "fates/io/native_file_entry.hpp"

namespace fates::io::native {
enum class AsyncFileStatus:std::uint8_t {Ready,NullScheduler,MismatchedDomain,DuplicateBinding};
// Cooperative replacement for the CTR worker's platform scheduling. One Pump
// executes one original ThreadFunction iteration: choose the first greatest
// priority, mark reading, obtain real transport bytes, unlink, mark complete.
// It never performs subtype Setup or fabricates FileBase::IsDone.
class NativeAsyncFiles final:public runtime::native::ProcessCallbacks {
public:
    static AsyncFileStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<NativeFileController>,
        std::shared_ptr<NativeFileBase>,std::shared_ptr<NativeFileEntry>,FileEntryHostServices,
        std::shared_ptr<NativeAsyncFiles>&);
    ~NativeAsyncFiles();
    NativeAsyncFiles(const NativeAsyncFiles&)=delete;
    NativeAsyncFiles& operator=(const NativeAsyncFiles&)=delete;
    // This target is explicitly host-native, not a made-up retail function.
    // Return1 means an iteration published a transport result;0 means idle.
    static runtime::native::ProcessCall PumpCall(runtime::native::ProcessHandle);
    bool UsesOwners(const NativeFileController&,const NativeFileEntry&) const noexcept;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    struct State;struct Continuation;
    explicit NativeAsyncFiles(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
