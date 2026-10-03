#pragma once
#include "fates/io/native_texture_objects.hpp"

namespace fates::io::native {
enum class TexFileRead:std::uint8_t {Immediate,Asynchronous,TryImmediate};
enum class TexFileStatus:std::uint8_t {Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,InvalidHandle};
// Embedded TexFiles are existing FileBase attachments with a concrete texture
// subtype factory. They do not create a separate file cache or own completion.
class NativeTexFiles final:public runtime::native::ProcessCallbacks {
public:
    static TexFileStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<NativeFileController>,
        std::shared_ptr<NativeFileBase>,std::shared_ptr<NativeFileEntry>,std::shared_ptr<NativeTextureObjects>,
        std::shared_ptr<NativeTexFiles>&);
    ~NativeTexFiles();
    NativeTexFiles(const NativeTexFiles&)=delete;
    NativeTexFiles& operator=(const NativeTexFiles&)=delete;
    TexFileStatus Construct(FileBaseHandle&,runtime::native::ProcessAccess* =nullptr);
    std::optional<runtime::native::ProcessCall> ReadCall(runtime::native::ProcessHandle,FileBaseHandle,
        FilePathHandle,std::uint32_t flags=2,TexFileRead=TexFileRead::Immediate) const;
    std::optional<runtime::native::ProcessCall> DestroyCall(runtime::native::ProcessHandle,FileBaseHandle,bool deleting=false) const;
    std::optional<std::uint32_t> TextureCount(FileBaseHandle) const;
    TextureLookupResult GetTexture(FileBaseHandle,std::int32_t) const;
    TextureLookupResult GetTexture(FileBaseHandle,std::optional<std::string_view>) const;
    bool UsesTextureObjects(const NativeTextureObjects&) const noexcept;
    bool UsesOwners(const NativeFileController&,const NativeFileBase&) const noexcept;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    struct State;struct Continuation;
    explicit NativeTexFiles(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
