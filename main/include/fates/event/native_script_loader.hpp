#pragma once
#include "fates/cmvm/native_script_session.hpp"
#include "fates/io/native_file_store.hpp"
#include "fates/runtime/native_runtime.hpp"

namespace fates::event::native {
enum class ScriptLoadStatus : std::uint8_t {
    Ready, Loaded, Freed, NoArchive, Missing, UnknownRoute, InvalidName,
    PathTooLong, FileBoundary, InvalidArchive, SessionBoundary, NullOwner, Retired
};
struct ScriptLoadObservation {
    ScriptLoadStatus status{ScriptLoadStatus::Ready};
    std::string path;
    cmvm::native::ScriptAttachment attachment;
    io::native::FileStoreStatus file_status{io::native::FileStoreStatus::Ready};
    cmvm::native::ScriptSessionStatus session_status{cmvm::native::ScriptSessionStatus::Ok};
    PhaseArchiveStatus archive_status{PhaseArchiveStatus::Ok};
};
// Composes event::ScriptLoad/Free[Route], the retained file registry and the
// existing attachment session. No chapter-specific names and no implicit
// ScriptLoad event dispatch: ChapterSequence is the caller of InstantCall(6).
// This owns bounded synchronous calls, not ChapterSequence or ProcEvent.
class NativeScriptLoader final {
public:
    static ScriptLoadStatus Create(std::shared_ptr<runtime::native::NativeRuntime>,
        std::shared_ptr<cmvm::native::ScriptAttachmentSession>,
        std::shared_ptr<io::native::NativeFileStore>,std::unique_ptr<NativeScriptLoader>&);
    NativeScriptLoader(const NativeScriptLoader&)=delete;
    NativeScriptLoader& operator=(const NativeScriptLoader&)=delete;
    ScriptLoadObservation Load(std::string_view name,bool route);
    ScriptLoadObservation Free(std::string_view name,bool route);
    // Retires this coordinator only. The host owns shared session/store teardown.
    void Retire() noexcept {retired_=true;}
private:
    NativeScriptLoader()=default;
    ScriptLoadStatus Paths(std::string_view,bool,std::string&,std::string&) const;
    struct ParsedImage {
        std::weak_ptr<const io::native::NativeFileImage> file;
        std::shared_ptr<const PhaseEventArchive> archive;
    };
    std::shared_ptr<runtime::native::NativeRuntime> runtime_;
    std::shared_ptr<cmvm::native::ScriptAttachmentSession> session_;
    std::shared_ptr<io::native::NativeFileStore> files_;
    std::vector<ParsedImage> parsed_;
    bool retired_{};
};
}
