#pragma once
#include "fates/io/native_archive_objects.hpp"
#include "fates/io/native_file_entry.hpp"

namespace fates::io::native {
enum class GlobalFileMode:std::uint8_t {Plain,Archive};
struct GlobalFileEntryIdentity final {const std::uint32_t serial;};
using GlobalFileEntryHandle=std::shared_ptr<const GlobalFileEntryIdentity>;
enum class GlobalFileStatus:std::uint8_t {
    Ready,Missing,StaleValue,NullScheduler,MismatchedDomain,DuplicateBinding,
    Busy,Retired,InvalidHandle,InvalidPath,IdentityExhausted
};
struct GlobalFileEntryObservation {
    GlobalFileEntryHandle identity;
    GlobalFileMode mode{};
    FileBaseHandle base;
    std::uint32_t references{},allocation_word{};
    bool live{},linked{};
};
struct GlobalFileRegistryObservation {
    std::vector<GlobalFileEntryHandle> entries;
    std::uint32_t count{},index_accounting_count{};
    std::size_t index_size{};
};
struct GlobalFileFindResult {GlobalFileStatus status{GlobalFileStatus::Missing};GlobalFileEntryHandle entry;};
// Fresh, explicitly initialized higher registries (127 buckets each) over ONE
// lower cache. Native record allocation is ordinary host C++ storage; this does
// not port the original allocator or infer an existing game's global entries.
// The second Load word is passed to cold Entry flags exactly as the executable
// does. No subtype is replaced on a warm hit and Close's boolean is ignored.
class NativeGlobalFiles final:public runtime::native::ProcessCallbacks {
public:
    static GlobalFileStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<NativeFileController>,
        std::shared_ptr<NativeFileBase>,std::shared_ptr<NativeFileEntry>,std::shared_ptr<NativeArchiveObjects>,
        std::shared_ptr<NativeGlobalFiles>&);
    ~NativeGlobalFiles();
    NativeGlobalFiles(const NativeGlobalFiles&)=delete;
    NativeGlobalFiles& operator=(const NativeGlobalFiles&)=delete;
    GlobalFileFindResult Find(GlobalFileMode,std::string_view) const;
    std::optional<GlobalFileEntryObservation> Observe(GlobalFileEntryHandle) const;
    std::optional<GlobalFileRegistryObservation> Observe(GlobalFileMode) const;
    // Explicit imported counter state, with the same guarded mutation domain
    // as carried FileObject fields. Ordinary callers use Load/Free below.
    GlobalFileStatus WriteReferences(GlobalFileEntryHandle,std::uint32_t,runtime::native::ProcessAccess* =nullptr);
    bool UsesOwners(const runtime::native::NativeProcessScheduler&,const NativeFileBase&) const noexcept;
    std::optional<runtime::native::ProcessCall> LoadCall(runtime::native::ProcessHandle,GlobalFileMode,
        FilePathHandle,std::uint32_t allocation_word=0) const;
    std::optional<runtime::native::ProcessCall> FreeCall(runtime::native::ProcessHandle,GlobalFileMode,FilePathHandle) const;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    struct State;struct Continuation;
    explicit NativeGlobalFiles(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
