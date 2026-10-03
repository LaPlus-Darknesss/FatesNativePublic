#pragma once
#include "fates/io/native_texture_coordinate.hpp"
#include "fates/io/native_tex_files.hpp"
#include "fates/io/native_language_paths.hpp"

namespace fates::io::native {
struct CoordinateSceneIdentity final {const std::uint32_t serial;};
using CoordinateSceneHandle=std::shared_ptr<const CoordinateSceneIdentity>;
enum class CoordinateResourceStatus:std::uint8_t {
    Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,
    Unavailable,InvalidScene,InvalidRecord,IdentityExhausted
};
struct CoordinateSlotObservation {
    // Scene's constructor does not initialize its sixteen pointer pairs.
    bool path_known{},file_known{};
    UniqueArchiveRecord path;
    FileBaseHandle file;
};
struct CoordinateSceneObservation {
    CoordinateSceneHandle identity;
    UniqueArchiveRecord header;
    std::uint32_t loaded_count{};
    std::array<CoordinateSlotObservation,16> slots;
};
struct CoordinateTextureResult {
    CoordinateResourceStatus status{CoordinateResourceStatus::Unavailable};
    // false is original null, distinct from TexFile's nonnull dummy texture.
    bool has_texture_result{};
    TextureLookupResult texture;
};
// One process-wide TextureCoordinate StructFile, plus individually owned scenes.
// Uses the existing lower cache, private index and TexFiles. No scene load is
// implied by construction. All resource mutation runs under the same scheduler.
class NativeCoordinateResources final:public runtime::native::ProcessCallbacks {
public:
    static CoordinateResourceStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<NativeFileController>,
        std::shared_ptr<NativeFileBase>,std::shared_ptr<NativeFileEntry>,std::shared_ptr<NativeUniqueArchiveObjects>,
        std::shared_ptr<NativeTexFiles>,std::shared_ptr<NativeLanguagePaths>,
        std::function<FileSourceStatus(std::string_view)>,std::shared_ptr<NativeCoordinateResources>&);
    ~NativeCoordinateResources();
    NativeCoordinateResources(const NativeCoordinateResources&)=delete;
    NativeCoordinateResources& operator=(const NativeCoordinateResources&)=delete;
    CoordinateResourceStatus PublishLanguage(NativeLanguageState,runtime::native::ProcessAccess* =nullptr);
    static runtime::native::ProcessCall InitializeCall(runtime::native::ProcessHandle);
    static runtime::native::ProcessCall FinalizeCall(runtime::native::ProcessHandle);
    CoordinateResourceStatus CreateScene(std::optional<std::string_view>,CoordinateSceneHandle&,
        runtime::native::ProcessAccess* =nullptr);
    std::optional<runtime::native::ProcessCall> LoadCall(runtime::native::ProcessHandle,CoordinateSceneHandle) const;
    std::optional<runtime::native::ProcessCall> FreeCall(runtime::native::ProcessHandle,CoordinateSceneHandle) const;
    std::optional<runtime::native::ProcessCall> DestroyCall(runtime::native::ProcessHandle,CoordinateSceneHandle) const;
    std::optional<CoordinateSceneObservation> Observe(CoordinateSceneHandle) const;
    std::optional<FileBaseObservation> ObserveArchive() const;
    UniqueArchiveRecordResult Find(std::optional<std::string_view>) const;
    CoordinateTextureResult GetTexture(CoordinateSceneHandle,const UniqueArchiveRecord& file) const;
    UniqueArchiveWordResult ReadU16(const UniqueArchiveRecord&,std::size_t field=0) const;
    UniqueArchiveRecordResult Pointer(const UniqueArchiveRecord&,std::size_t field) const;
    bool UsesScheduler(const runtime::native::NativeProcessScheduler&) const noexcept;
    bool UsesOwners(const NativeFileController&,const NativeFileBase&,const NativeTexFiles&) const noexcept;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    struct State;struct Continuation;
    explicit NativeCoordinateResources(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
