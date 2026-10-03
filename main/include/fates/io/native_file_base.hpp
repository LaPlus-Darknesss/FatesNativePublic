#pragma once
#include "fates/io/native_file_controller.hpp"

namespace fates::io::native {
struct FileBaseIdentity final {const std::uint32_t serial;};
struct FilePathIdentity final {const std::uint32_t serial;const std::string text;};
using FileBaseHandle=std::shared_ptr<const FileBaseIdentity>;
using FilePathHandle=std::shared_ptr<const FilePathIdentity>;
struct FileBaseObservation {FileBaseHandle identity;FileObjectHandle object;};
enum class FileBaseStatus:std::uint8_t {
    Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,
    InvalidHandle,InvalidPath,IdentityExhausted
};
// FileBase attachments over the one shared lower FileController. Restore/Write
// import concrete attachment state; they do not acquire or release references.
// Gameplay ownership changes use Open/Close/Free/Replace services below.
// Cold EntryFile, allocation and FileController FinishAsync are separate owners;
// an unresolved service remains suspended, never accepted as completed I/O.
class NativeFileBase final:public runtime::native::ProcessCallbacks {
public:
    static FileBaseStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<NativeFileController>,
        std::shared_ptr<NativeFileBase>&);
    ~NativeFileBase();
    NativeFileBase(const NativeFileBase&)=delete;
    NativeFileBase& operator=(const NativeFileBase&)=delete;
    FileBaseStatus RestoreAttachment(FileObjectHandle,FileBaseHandle&,runtime::native::ProcessAccess* =nullptr);
    FileBaseStatus WriteAttachment(FileBaseHandle,FileObjectHandle,runtime::native::ProcessAccess* =nullptr);
    // Destroy an embedded base only after its actual Free has detached it.
    FileBaseStatus RetireEmpty(FileBaseHandle,runtime::native::ProcessAccess* =nullptr);
    FileBaseStatus RegisterPath(std::string_view,FilePathHandle&,runtime::native::ProcessAccess* =nullptr);
    std::optional<FileBaseObservation> Observe(FileBaseHandle) const;
    FileBaseHandle ResolveBase(std::uint32_t) const;
    // Resolve only in the path capability domain, never as an object word.
    FilePathHandle ResolvePath(std::uint32_t) const;
    bool UsesController(const NativeFileController&) const noexcept;
    std::optional<runtime::native::ProcessCall> OpenCall(runtime::native::ProcessHandle,FileBaseHandle,
        FilePathHandle,std::uint32_t read_type=0,bool binary=false) const;
    std::optional<runtime::native::ProcessCall> CloseCall(runtime::native::ProcessHandle,FileBaseHandle,
        std::uint32_t file_type,std::uint32_t read_type=0,bool binary=false) const;
    std::optional<runtime::native::ProcessCall> FreeCall(runtime::native::ProcessHandle,FileBaseHandle) const;
    std::optional<runtime::native::ProcessCall> PriorityCall(runtime::native::ProcessHandle,FileBaseHandle,
        std::uint32_t priority) const;
    std::optional<runtime::native::ProcessCall> PathCall(runtime::native::ProcessHandle,FileBaseHandle) const;
    std::optional<runtime::native::ProcessCall> ReplaceCall(runtime::native::ProcessHandle,FileBaseHandle,FileBaseHandle) const;
    std::optional<runtime::native::ProcessCall> FinishCall(runtime::native::ProcessHandle,FileBaseHandle,bool try_finish=false) const;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    struct State;struct Continuation;
    explicit NativeFileBase(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
