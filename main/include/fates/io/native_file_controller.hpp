#pragma once
#include "fates/runtime/native_process.hpp"
namespace fates::io::native {
struct FileControllerIdentity final {const std::uint32_t serial;};
struct FileObjectIdentity final {const std::uint32_t serial;};
using FileControllerHandle=std::shared_ptr<const FileControllerIdentity>;
using FileObjectHandle=std::shared_ptr<const FileObjectIdentity>;
// Logical resource words are resolved by their concrete allocator/subtype owner,
// never dereferenced as host addresses. Zero is an explicitly null field.
struct CarriedFileObject {
    std::string path;
    std::uint32_t allocator{},allocator_free{},data{},size{},flags{};
    std::uint16_t references{};
    std::uint8_t cache_level{},priority{};
    std::uint32_t deleting_destructor{},cleanup{};
};
struct CarriedFileController {
    // Complete cache order, distinct hash insertion order (a permutation), and
    // async order (a subset). No inference from the higher GlobalFile registry.
    std::vector<CarriedFileObject> cache;
    std::vector<std::size_t> index_order,async_order;
};
enum class FileObjectLife:std::uint8_t {Live,Destroying,Destroyed};
struct FileObjectObservation {
    FileObjectHandle identity;
    FileControllerHandle controller;
    CarriedFileObject fields;
    FileObjectLife life{};
    bool cache_linked{},async_linked{},base_destructor_entered{};
};
struct FileControllerObservation {
    bool present{};
    FileControllerHandle identity;
    std::vector<FileObjectHandle> cache,async;
    std::uint32_t index_accounting_count{};
    std::size_t index_size{};
};
enum class FileControllerStatus:std::uint8_t {
    Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,
    InvalidSnapshot,IdentityExhausted,InvalidHandle
};
// Shared lower FileController owner, separate from GlobalFile. Restore carries
// a live controller; it does not construct the worker thread. Virtual methods
// execute through the same scheduler as ProcResDelayBind. Unknown methods block.
class NativeFileController final:public runtime::native::ProcessCallbacks {
public:
    static FileControllerStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<NativeFileController>&);
    ~NativeFileController();
    NativeFileController(const NativeFileController&)=delete;
    NativeFileController& operator=(const NativeFileController&)=delete;
    FileControllerStatus Restore(const CarriedFileController&,FileControllerHandle&,
        std::vector<FileObjectHandle>&,runtime::native::ProcessAccess* =nullptr);
    FileControllerStatus PublishAbsent(runtime::native::ProcessAccess* =nullptr);
    // Carried field writer for an existing live object. Membership, path and
    // virtual identity stay fixed; concrete Setup/Cleanup owners may use it.
    FileControllerStatus WriteFields(FileObjectHandle,const CarriedFileObject&,
        runtime::native::ProcessAccess* =nullptr);
    std::optional<FileControllerObservation> Observe() const;
    std::optional<FileControllerObservation> Observe(FileControllerHandle) const;
    std::optional<FileObjectObservation> Observe(FileObjectHandle) const;
    FileObjectHandle Find(FileControllerHandle,std::string_view path) const;
    FileObjectHandle ResolveObject(std::uint32_t logical_word) const;
    static runtime::native::ProcessCall SweepCall(runtime::native::ProcessHandle);
    std::optional<runtime::native::ProcessCall> SweepCall(runtime::native::ProcessHandle,FileControllerHandle,std::int32_t) const;
    std::optional<runtime::native::ProcessCall> DeleteCall(runtime::native::ProcessHandle,FileControllerHandle,FileObjectHandle) const;
    std::optional<runtime::native::ProcessCall> DelayedReleaseCall(runtime::native::ProcessHandle,FileObjectHandle) const;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    struct State;struct Continuation;
    explicit NativeFileController(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
