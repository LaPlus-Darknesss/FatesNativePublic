#pragma once
#include "fates/io/native_file_controller.hpp"
#include "fates/io/native_file_store.hpp"
#include "fates/runtime/native_archive.hpp"

namespace fates::io::native {
struct UniqueArchiveStorage;
enum class UniqueArchiveStatus:std::uint8_t {
    Ready,Missing,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,
    InvalidObject,InvalidImage,NotSetup,StaleValue,ForeignValue,InvalidIdentifier,
    InvalidPointer,InvalidRecord,CapacityExceeded,ConflictingAssociation
};
// An original data-pointer identity, not a host pointer or a spelling lookup.
// Holding this token retains bytes but never keeps a freed FileObject data life alive.
class UniqueArchiveRecord final {
public:
    explicit operator bool() const noexcept {return bool(storage_);}
    std::size_t offset() const noexcept {return offset_;}
    bool SamePointer(const UniqueArchiveRecord& other) const noexcept {
        return storage_==other.storage_ && offset_==other.offset_;
    }
private:
    friend class NativeUniqueArchiveObjects;
    std::shared_ptr<const UniqueArchiveStorage> storage_;
    std::size_t offset_{};
};
struct UniqueArchiveRecordResult {
    UniqueArchiveStatus status{UniqueArchiveStatus::Missing};
    UniqueArchiveRecord value; // Ready + false is the actual null pointer.
};
struct UniqueArchiveWordResult {
    UniqueArchiveStatus status{UniqueArchiveStatus::Missing};
    std::uint32_t value{};
};
struct UniqueArchiveStringResult {
    UniqueArchiveStatus status{UniqueArchiveStatus::Missing};
    std::optional<std::string> value; // Ready + nullopt is null, not empty.
};
struct UniqueArchiveObservation {
    bool index_present{},constructed{};
    std::size_t entries{};
    std::uint32_t accounting_count{};
};
// UniqueArchiveObject has its own 127-bucket IdentHash. It must never publish its
// labels into the process-wide 2039-bucket archive/message registry. These actual
// Setup/Cleanup/deleting-destructor callbacks reuse the single lower FileController
// and FileBase domain. Association admits final decoded data, not a loaded file.
// Host index allocation is explicit; original allocator exhaustion is not emulated.
class NativeUniqueArchiveObjects final:public runtime::native::ProcessCallbacks {
public:
    static UniqueArchiveStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<NativeFileController>,
        std::shared_ptr<NativeUniqueArchiveObjects>&);
    ~NativeUniqueArchiveObjects();
    NativeUniqueArchiveObjects(const NativeUniqueArchiveObjects&)=delete;
    NativeUniqueArchiveObjects& operator=(const NativeUniqueArchiveObjects&)=delete;
    UniqueArchiveStatus Associate(FileObjectHandle,std::shared_ptr<const NativeFileImage>,
        runtime::native::ProcessAccess* =nullptr);
    UniqueArchiveRecordResult Find(FileObjectHandle,std::optional<std::string_view>) const;
    std::optional<UniqueArchiveObservation> Observe(FileObjectHandle) const;
    UniqueArchiveStatus Validate(const UniqueArchiveRecord&) const;
    UniqueArchiveRecordResult At(const UniqueArchiveRecord&,std::size_t displacement,std::size_t extent) const;
    UniqueArchiveWordResult ReadU16(const UniqueArchiveRecord&,std::size_t field=0) const;
    UniqueArchiveWordResult ReadU32(const UniqueArchiveRecord&,std::size_t field=0) const;
    UniqueArchiveRecordResult Pointer(const UniqueArchiveRecord&,std::size_t field) const;
    UniqueArchiveStringResult ReadString(const UniqueArchiveRecord&) const;
    bool UsesController(const NativeFileController&) const noexcept;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    struct State;struct Continuation;
    explicit NativeUniqueArchiveObjects(std::shared_ptr<State>);
    UniqueArchiveWordResult Scalar(const UniqueArchiveRecord&,std::size_t,std::size_t) const;
    std::shared_ptr<State> state_;
};
}
