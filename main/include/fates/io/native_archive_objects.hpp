#pragma once
#include "fates/io/native_file_controller.hpp"
#include "fates/runtime/native_archive_identifiers.hpp"

namespace fates::io::native {
enum class ArchiveObjectStatus:std::uint8_t {
    Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,
    InvalidObject,InvalidImage,ConflictingAssociation
};
// Concrete ArchiveObject virtuals over the shared FileObject/cache domain and
// shared Ident registry. Association supplies the actual decoded data image; it
// neither loads a file nor performs Setup. FileBase owns the setup-complete bit.
class NativeArchiveObjects final:public runtime::native::ProcessCallbacks {
public:
    static ArchiveObjectStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<NativeFileController>,
        std::shared_ptr<runtime::native::NativeArchiveIdentifiers>,std::shared_ptr<NativeArchiveObjects>&);
    ~NativeArchiveObjects();
    NativeArchiveObjects(const NativeArchiveObjects&)=delete;
    NativeArchiveObjects& operator=(const NativeArchiveObjects&)=delete;
    ArchiveObjectStatus Associate(FileObjectHandle,std::shared_ptr<const NativeFileImage>,
        runtime::native::ProcessAccess* =nullptr);
    std::shared_ptr<const runtime::native::ArchiveRegistration> Registration(FileObjectHandle) const;
    bool UsesController(const NativeFileController&) const noexcept;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    struct State;struct Continuation;
    explicit NativeArchiveObjects(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
