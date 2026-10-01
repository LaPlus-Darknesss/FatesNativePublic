#include "fates/cmvm/native_script_session.hpp"
#include <algorithm>
#include <limits>

namespace fates::cmvm::native {
namespace en=event::native;
namespace {
bool ValidName(std::string_view name) {return name.size()<=1024 && name.find('\0')==std::string_view::npos;}
std::uint32_t U32(std::span<const std::uint8_t> b,std::size_t at) {
    return std::uint32_t(b[at])|(std::uint32_t(b[at+1])<<8u)|(std::uint32_t(b[at+2])<<16u)|(std::uint32_t(b[at+3])<<24u);
}
std::shared_ptr<const en::PhaseEventCatalog> Catalog(const std::vector<ScriptAttachment>& attachments) {
    std::vector<std::shared_ptr<const en::PhaseEventArchive>> archives;
    for(const auto& a:attachments)archives.push_back(a.table()->archive());
    std::shared_ptr<const en::PhaseEventCatalog> out;
    if(en::PhaseEventCatalog::Create(archives,out)!=en::PhaseSearchStatus::Ok)return {};
    return out;
}
}
bool ScriptAttachment::attached() const noexcept {return state_ && state_->attached;}
std::shared_ptr<const ScriptFunctionTable> ScriptAttachment::table() const noexcept {return state_?state_->table:nullptr;}
ScriptSessionStatus ScriptAttachmentSession::Create(std::uint32_t count,std::span<const std::string> names,
    std::shared_ptr<ScriptAttachmentSession>& out) {
    if(count==0 || count>65536)return ScriptSessionStatus::InvalidBucketCount;
    if(names.size()>65536)return ScriptSessionStatus::LimitExceeded;
    for(const auto& name:names)if(!ValidName(name))return ScriptSessionStatus::InvalidIdentifier;
    auto next=std::shared_ptr<ScriptAttachmentSession>(new ScriptAttachmentSession(count));
    for(const auto& name:names)next->registry_.Append(name.c_str(),{name,ScriptSymbolKind::Native,{}});
    next->catalog_=Catalog({});out=std::move(next);return ScriptSessionStatus::Ok;
}
ScriptAttachmentSession::~ScriptAttachmentSession() {Retire();}
bool ScriptAttachmentSession::IsAttached(const ScriptAttachment& a) const noexcept {
    return !retired_ && a.state_ && a.state_->session_identity==identity_ && a.state_->attached;
}
ScriptSessionStatus ScriptAttachmentSession::Attach(std::shared_ptr<const en::PhaseEventArchive> archive,ScriptAttachment& out,
    std::shared_ptr<const void> storage_identity) {
    using S=ScriptSessionStatus;
    if(retired_)return S::Retired;
    if(!archive)return S::NullArchive;
    if(!storage_identity)storage_identity=archive;
    for(const auto& a:attachments_)if(a.table()->archive()==archive || a.state_->storage_identity==storage_identity)return S::AlreadyAttached;
    const auto bytes=archive->bytes();
    if(U32(bytes,0x14) || U32(bytes,0x18))return S::UnsupportedStorage;
    if(bytes[0x26] || bytes[0x27])return S::DirectEntryRequired;
    const auto old=history_.find(storage_identity.get());
    // Retail Detach leaves this image's next link unchanged. Reappending a
    // formerly non-tail image would resurrect that link (often a cycle).
    if(old!=history_.end() && old->second.storage.lock()==storage_identity && old->second.detached_with_next)
        return S::DetachedLinkRequired;
    if(attachments_.size()>=1024)return S::LimitExceeded;
    if(revision_==std::numeric_limits<std::uint64_t>::max())return S::RevisionExhausted;
    std::shared_ptr<const ScriptFunctionTable> table;
    if(ScriptFunctionTable::Read(archive,table)!=ScriptFunctionStatus::Ok)return S::InvalidFunctions;
    std::size_t names=0;for(const auto& f:table->functions())if(f.name)++names;
    if(names>65536-registry_.Size())return S::LimitExceeded;
    auto next_registry=registry_;
    for(const auto& f:table->functions())if(f.name) {
        ScriptFunctionRef ref;table->Get(f.index,ref);
        next_registry.Append(f.name->c_str(),{*f.name,ScriptSymbolKind::Script,std::move(ref)});
    }
    ScriptAttachment next;next.state_=std::make_shared<ScriptAttachment::State>();
    next.state_->table=std::move(table);next.state_->session_identity=identity_;
    next.state_->storage_identity=storage_identity;
    auto next_attachments=attachments_;next_attachments.push_back(next);
    auto next_catalog=Catalog(next_attachments);if(!next_catalog)return S::LimitExceeded;
    auto next_history=history_;
    for(auto it=next_history.begin();it!=next_history.end();) {
        if(it->second.storage.expired())it=next_history.erase(it);else ++it;
    }
    next_history[storage_identity.get()]={storage_identity,false};
    registry_=std::move(next_registry);attachments_=std::move(next_attachments);
    catalog_=std::move(next_catalog);history_=std::move(next_history);++revision_;
    out=std::move(next);return S::Ok;
}
ScriptSessionStatus ScriptAttachmentSession::Detach(const ScriptAttachment& a) {
    using S=ScriptSessionStatus;
    if(retired_)return S::Retired;
    if(!a.state_ || a.state_->session_identity!=identity_)return S::ForeignAttachment;
    if(!a.attached())return S::DetachedAttachment;
    if(revision_==std::numeric_limits<std::uint64_t>::max())return S::RevisionExhausted;
    const auto it=std::find_if(attachments_.begin(),attachments_.end(),[&](const auto& v){return v.state_==a.state_;});
    if(it==attachments_.end())return S::ForeignAttachment;
    const auto index=static_cast<std::size_t>(it-attachments_.begin());
    auto next_registry=registry_;
    // Original Dispose deletes by name/hash, not by owning archive/function.
    for(const auto& f:a.table()->functions())if(f.name)next_registry.EraseFirst(f.name->c_str());
    auto next_attachments=attachments_;next_attachments.erase(next_attachments.begin()+index);
    auto next_catalog=Catalog(next_attachments);if(!next_catalog)return S::LimitExceeded;
    history_.at(a.state_->storage_identity.get()).detached_with_next=index+1<attachments_.size();
    a.state_->attached=false;registry_=std::move(next_registry);attachments_=std::move(next_attachments);
    catalog_=std::move(next_catalog);++revision_;return S::Ok;
}
void ScriptAttachmentSession::Retire() noexcept {
    if(retired_)return;
    retired_=true;for(auto& a:attachments_)a.state_->attached=false;
    registry_.Clear();attachments_.clear();catalog_.reset();history_.clear();
}
ScriptSessionStatus ScriptAttachmentSession::FindArchiveByName(std::string_view name,ScriptAttachment& out) const {
    using S=ScriptSessionStatus;
    if(retired_)return S::Retired;
    if(!ValidName(name))return S::InvalidIdentifier;
    for(const auto& a:attachments_) {
        const auto b=a.table()->archive()->bytes();const auto at=std::size_t(U32(b,0x0c));
        if(at<0x28 || at>=b.size())return S::InvalidArchiveName;
        auto end=at;while(end<b.size() && b[end] && end-at<=1024)++end;
        if(end==b.size() || end-at>1024)return S::InvalidArchiveName;
        if(std::string_view(reinterpret_cast<const char*>(b.data()+at),end-at)==name){out=a;return S::Found;}
    }
    out={};return S::NoMatchInSession;
}
LiveScriptLookup ScriptAttachmentSession::Find(std::string_view name) const {
    using L=LiveScriptLookupStatus;
    if(retired_)return {L::Retired,{},{},{}};
    if(!ValidName(name))return {L::InvalidIdentifier,{},{},{}};
    const std::string key(name);const auto* entry=registry_.Find(key.c_str());
    if(!entry)return {};
    if(entry->value.kind==ScriptSymbolKind::Native)return {L::NativeOwnerRequired,entry->identifier,{},{}};
    const auto archive=entry->value.function.table()->archive();
    for(const auto& a:attachments_)if(a.table()->archive()==archive)
        return {L::FoundScript,entry->identifier,entry->value.function,a};
    return {L::DetachedFunction,entry->identifier,entry->value.function,{}};
}
ScriptSessionStatus ScriptAttachmentSession::SelectFunction(const ScriptAttachment& attachment,std::size_t index,
    AttachedScriptFunction& out) const {
    using S=ScriptSessionStatus;
    if(retired_)return S::Retired;
    if(!attachment.state_ || attachment.state_->session_identity!=identity_)return S::ForeignAttachment;
    if(!attachment.attached())return S::DetachedAttachment;
    AttachedScriptFunction next;
    if(attachment.table()->Get(index,next.function_)!=ScriptFunctionStatus::Ok)return S::InvalidFunctions;
    next.attachment_=attachment;out=std::move(next);return S::Found;
}
ScriptSessionStatus ScriptAttachmentSession::FindNextTyped(std::uint8_t type,const AttachedScriptFunction* previous,
    AttachedScriptFunction& out) const {
    using S=ScriptSessionStatus;
    if(retired_)return S::Retired;
    std::size_t archive_index=0,function_index=0;
    if(previous) {
        if(!*previous || !IsAttached(previous->attachment_) || previous->function_.info()->type!=type)
            return S::ForeignSelection;
        const auto it=std::find_if(attachments_.begin(),attachments_.end(),[&](const auto& a){
            return a.state_==previous->attachment_.state_;});
        if(it==attachments_.end())return S::ForeignSelection;
        archive_index=static_cast<std::size_t>(it-attachments_.begin());
        function_index=std::size_t(previous->function_.info()->index)+1;
    }
    for(;archive_index<attachments_.size();++archive_index,function_index=0) {
        const auto& attachment=attachments_[archive_index];
        const auto functions=attachment.table()->functions();
        for(;function_index<functions.size();++function_index)if(functions[function_index].type==type)
            return SelectFunction(attachment,function_index,out);
    }
    out={};return S::NoMatchInSession;
}
ScriptSessionStatus ScriptAttachmentSession::FindNext(en::PhaseEventKind kind,std::uint16_t turn,std::uint8_t force,
    const AttachedPhaseSelection* previous,AttachedPhaseSelection& out) const {
    using S=ScriptSessionStatus;
    if(retired_)return S::Retired;
    if(static_cast<unsigned>(kind)<16 || static_cast<unsigned>(kind)>19)return S::InvalidKind;
    en::PhaseEventSelection anchor;
    if(previous) {
        if(!IsAttached(previous->attachment_) || !previous->selection_ ||
            previous->selection_.declaration()->kind!=kind)return S::ForeignSelection;
        for(std::size_t i=0;i<attachments_.size();++i)if(attachments_[i].state_==previous->attachment_.state_) {
            anchor=previous->selection_;anchor.catalog_=catalog_;anchor.archive_index_=i;break;
        }
        if(!anchor)return S::ForeignSelection;
    }
    en::PhaseEventSelection selected;
    const auto status=catalog_->FindNext(kind,turn,force,previous?&anchor:nullptr,selected);
    if(status==en::PhaseSearchStatus::NoMatchInSnapshot){out={};return S::NoMatchInSession;}
    if(status!=en::PhaseSearchStatus::Found)return S::ForeignSelection;
    AttachedPhaseSelection next;next.attachment_=attachments_[selected.archive_index()];
    next.selection_=std::move(selected);out=std::move(next);return S::Found;
}
}
