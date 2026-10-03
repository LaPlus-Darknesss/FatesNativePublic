#pragma once
#include "fates/io/native_file_base.hpp"
#include "fates/io/native_file_store.hpp"
#include <functional>

namespace fates::io::native {
struct FileEntryIdentity final {const std::uint32_t serial;};
using FileEntryHandle=std::shared_ptr<const FileEntryIdentity>;
enum class FileHeapQuery:std::uint8_t {Total,Free};
struct FileReadRequest {
    std::string_view path;
    std::uint32_t allocator{},alignment{};
    bool try_read{};
};
struct FileReadResult {
    // Logical allocation identity and actual final byte extent. Null data is a
    // known failed read, distinct from an unavailable/pending host operation.
    std::uint32_t data{},size{};
    std::shared_ptr<const NativeFileImage> image;
};
struct FileEntryHostServices {
    // Host resource/transport boundaries, never cache or FileBase decisions.
    // read supplies final decoded data, as at the established NativeFileSource
    // seam. This does not port TryDirectRead's CTR mounting/decompression body.
    // nullopt/false keeps the current continuation suspended. Callbacks receive
    // the scheduler mutation capability; they must not recursively run it.
    std::function<std::optional<std::uint32_t>(std::uint32_t,FileHeapQuery,runtime::native::ProcessAccess&)> heap;
    std::function<std::optional<FileReadResult>(const FileReadRequest&,runtime::native::ProcessAccess&)> read;
    std::function<bool(FileControllerHandle,runtime::native::ProcessAccess&)> signal_async;
};
enum class FileEntryStatus:std::uint8_t {
    Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,InvalidHandle,
    InvalidAllocator,ConflictingAllocator,IdentityExhausted
};
// Original cold EntryFile and its FileObject::Sweep dependency. Uses the same
// lower cache, subtype callbacks and file-base attachments as warm Open/Close.
// Actual async worker/completion remains a separate service, never a fake read.
class NativeFileEntry final:public runtime::native::ProcessCallbacks {
public:
    static FileEntryStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<NativeFileController>,
        std::shared_ptr<NativeFileBase>,FileEntryHostServices,std::shared_ptr<NativeFileEntry>&);
    ~NativeFileEntry();
    NativeFileEntry(const NativeFileEntry&)=delete;
    NativeFileEntry& operator=(const NativeFileEntry&)=delete;
    FileEntryStatus RegisterAllocator(std::uint32_t logical_word,std::uint32_t free_target,runtime::native::ProcessAccess* =nullptr);
    // Unknown before publication; zero publishes the original explicit null.
    FileEntryStatus PublishRawAllocator(std::uint32_t logical_word,runtime::native::ProcessAccess* =nullptr);
    FileEntryStatus Prepare(FileBaseHandle,FileObjectHandle,FilePathHandle,std::uint32_t flags,
        std::uint32_t read_type,FileEntryHandle&,runtime::native::ProcessAccess* =nullptr);
    std::optional<runtime::native::ProcessCall> EntryCall(runtime::native::ProcessHandle,FileEntryHandle) const;
    // Called after an actual asynchronous transport result, while the original
    // lower object is marked reading. Publishes bytes/data generation only;
    // queue removal, completion flags and resource Setup are separate steps.
    FileEntryStatus PublishAsyncResult(FileObjectHandle,const FileReadResult&,
        runtime::native::ProcessAccess&);
    std::optional<FileReadResult> ReadResult(FileObjectHandle) const;
    bool UsesOwners(const NativeFileController&,const NativeFileBase&) const noexcept;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    struct State;struct Continuation;
    explicit NativeFileEntry(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
