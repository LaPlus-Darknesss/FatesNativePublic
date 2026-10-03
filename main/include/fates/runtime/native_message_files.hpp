#pragma once
#include "fates/io/native_global_files.hpp"
#include "fates/io/native_language_paths.hpp"
#include "fates/runtime/native_identifier_holder.hpp"
#include "fates/runtime/native_runtime.hpp"

namespace fates::runtime::native {
struct MessageArchiveNameIdentity final {const std::uint32_t serial;const std::string text;};
using MessageArchiveName=std::shared_ptr<const MessageArchiveNameIdentity>;
enum class MessageFileStatus:std::uint8_t {
    Ready,NullOwner,MismatchedDomain,DuplicateBinding,Busy,Retired,InvalidName,IdentityExhausted
};
struct MessageFileObservation {
    bool initialized{};
    std::optional<IdentifierHolderObservation> bindings;
    std::array<char,0x80> language_buffer{};
    std::array<char,0x50> message_buffer{};
};
// Mess archive binding over shared GlobalFile ownership, never a second cache.
// Create installs a fresh, known-null Mess global; Initialize constructs the
// holder. Current route comes only from the existing carried player provider.
// Source existence is distinct from registry membership and may be unavailable.
class NativeMessageFiles final:public ProcessCallbacks {
public:
    static MessageFileStatus Create(std::shared_ptr<NativeRuntime>,std::shared_ptr<NativeProcessScheduler>,
        std::shared_ptr<ProcessCallbackRegistry>,std::shared_ptr<io::native::NativeFileBase>,
        std::shared_ptr<io::native::NativeGlobalFiles>,std::function<io::native::FileSourceStatus(std::string_view)>,
        std::shared_ptr<NativeMessageFiles>&);
    ~NativeMessageFiles();
    NativeMessageFiles(const NativeMessageFiles&)=delete;
    NativeMessageFiles& operator=(const NativeMessageFiles&)=delete;
    MessageFileStatus RegisterName(std::string_view,MessageArchiveName&,ProcessAccess* =nullptr);
    MessageFileStatus PublishLanguage(io::native::NativeLanguageState,ProcessAccess* =nullptr);
    std::optional<MessageFileObservation> Observe() const;
    static ProcessCall InitializeCall(ProcessHandle);
    static ProcessCall FinalizeCall(ProcessHandle);
    std::optional<ProcessCall> LoadCall(ProcessHandle,MessageArchiveName,bool route=false) const;
    std::optional<ProcessCall> FreeCall(ProcessHandle,MessageArchiveName,bool route=false) const;
    std::optional<ProcessCall> IsFileLoadCall(ProcessHandle,MessageArchiveName) const;
    std::optional<ProcessCall> IsFileExistRouteCall(ProcessHandle,MessageArchiveName) const;
    std::unique_ptr<ProcessContinuation> Begin(const ProcessCall&) override;
private:
    struct State;struct Continuation;
    explicit NativeMessageFiles(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
