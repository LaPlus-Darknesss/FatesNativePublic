#include "fates/event/native_script_loader.hpp"
#include <algorithm>

namespace fates::event::native {
namespace sn=cmvm::native;
namespace fn=io::native;
ScriptLoadStatus NativeScriptLoader::Create(std::shared_ptr<runtime::native::NativeRuntime> runtime,
    std::shared_ptr<sn::ScriptAttachmentSession> session,std::shared_ptr<fn::NativeFileStore> files,
    std::unique_ptr<NativeScriptLoader>& out) {
    if(!runtime || !session || !files)return ScriptLoadStatus::NullOwner;
    if(session->retired() || files->retired())return ScriptLoadStatus::Retired;
    auto next=std::unique_ptr<NativeScriptLoader>(new NativeScriptLoader);
    next->runtime_=std::move(runtime);next->session_=std::move(session);next->files_=std::move(files);
    out=std::move(next);return ScriptLoadStatus::Ready;
}
ScriptLoadStatus NativeScriptLoader::Paths(std::string_view name,bool route,std::string& primary,std::string& fallback) const {
    if(retired_ || session_->retired() || files_->retired())return ScriptLoadStatus::Retired;
    if(name.find('\0')!=std::string_view::npos)return ScriptLoadStatus::InvalidName;
    // The original uses64-byte buffers. Refuse truncation/aliasing until that
    // caller domain is owned; do not fabricate a differently named resource.
    if(name.size()>51)return ScriptLoadStatus::PathTooLong;
    fallback="Scripts/"+std::string(name)+".cmb";primary=fallback;
    if(route) {
        const auto value=runtime::native::CurrentCarriedRoute(*runtime_);
        if(!value)return ScriptLoadStatus::UnknownRoute;
        if(name.size()>49)return ScriptLoadStatus::PathTooLong;
        primary="Scripts/"+std::string(1,static_cast<char>('A'+*value))+"/"+std::string(name)+".cmb";
    }
    return ScriptLoadStatus::Ready;
}
ScriptLoadObservation NativeScriptLoader::Load(std::string_view name,bool route) {
    ScriptLoadObservation o;std::string fallback;
    o.status=Paths(name,route,o.path,fallback);if(o.status!=ScriptLoadStatus::Ready)return o;
    o.file_status=files_->Exists(o.path);
    if(route && o.file_status==fn::FileStoreStatus::Missing){o.path=fallback;o.file_status=files_->Exists(o.path);}
    if(o.file_status!=fn::FileStoreStatus::Ready) {
        o.status=o.file_status==fn::FileStoreStatus::Missing?ScriptLoadStatus::Missing:ScriptLoadStatus::FileBoundary;return o;
    }
    const auto acquired=files_->Acquire(o.path);o.file_status=acquired.status;
    if(o.file_status!=fn::FileStoreStatus::Ready){o.status=ScriptLoadStatus::FileBoundary;return o;}
    std::erase_if(parsed_,[](const auto& p){return p.file.expired();});
    std::shared_ptr<const PhaseEventArchive> archive;
    for(const auto& p:parsed_)if(p.file.lock()==acquired.image){archive=p.archive;break;}
    if(!archive) {
        o.archive_status=PhaseEventArchive::Read(acquired.image->bytes(),archive);
        if(o.archive_status!=PhaseArchiveStatus::Ok){files_->Release(o.path);o.status=ScriptLoadStatus::InvalidArchive;return o;}
        const auto b=archive->bytes();
        const auto at=std::size_t(std::uint32_t(b[12])|(std::uint32_t(b[13])<<8u)|(std::uint32_t(b[14])<<16u)|(std::uint32_t(b[15])<<24u));
        auto end=at;if(at>=0x28 && at<b.size())while(end<b.size() && b[end] && end-at<=1024)++end;
        if(at<0x28 || at>=b.size() || end==b.size() || end-at>1024) {
            files_->Release(o.path);o.status=ScriptLoadStatus::InvalidArchive;o.session_status=sn::ScriptSessionStatus::InvalidArchiveName;return o;
        }
        parsed_.push_back({acquired.image,archive});
    }
    o.session_status=session_->Attach(archive,o.attachment,acquired.image);
    if(o.session_status!=sn::ScriptSessionStatus::Ok) {
        files_->Release(o.path);o.status=ScriptLoadStatus::SessionBoundary;return o;
    }
    o.status=ScriptLoadStatus::Loaded;return o;
}
ScriptLoadObservation NativeScriptLoader::Free(std::string_view name,bool route) {
    ScriptLoadObservation o;std::string fallback;
    o.status=Paths(name,route,o.path,fallback);if(o.status!=ScriptLoadStatus::Ready)return o;
    const auto slash=name.find_last_of('/');const std::string embedded(name.substr(slash==std::string_view::npos?0:slash+1));
    o.session_status=session_->FindArchiveByName(embedded+".cmb",o.attachment);
    if(o.session_status==sn::ScriptSessionStatus::NoMatchInSession){o.status=ScriptLoadStatus::NoArchive;return o;}
    if(o.session_status!=sn::ScriptSessionStatus::Found){o.status=ScriptLoadStatus::SessionBoundary;return o;}
    o.session_status=session_->Detach(o.attachment);
    if(o.session_status!=sn::ScriptSessionStatus::Ok){o.status=ScriptLoadStatus::SessionBoundary;return o;}
    // Current route and loaded registry membership decide release, even if a
    // different path originally supplied the first same-basename attachment.
    if(route && files_->Loaded(o.path).status==fn::FileStoreStatus::Missing)o.path=fallback;
    o.file_status=files_->Release(o.path);o.status=ScriptLoadStatus::Freed;return o;
}
}
