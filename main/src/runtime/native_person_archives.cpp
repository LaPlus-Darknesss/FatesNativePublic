#include "fates/runtime/native_person_archives.hpp"
#include <algorithm>
namespace fates::runtime::native {
namespace {
bool Contains(const PersonArchiveDescriptor& a,std::uint16_t id) noexcept {
    return id>=a.first_id&&std::uint32_t(id)<std::uint32_t(a.first_id)+a.count;
}
}
std::optional<std::size_t> FindPersonArchiveExact(std::span<const PersonArchiveDescriptor> a,std::uint16_t id) noexcept {
    for(std::size_t i=0;i<a.size();++i)if(Contains(a[i],id))return i;return std::nullopt;
}
std::optional<PersonArchiveLocation> FindPersonRecordExact(std::span<const PersonArchiveDescriptor> a,std::uint16_t id,bool fallback) noexcept {
    if(const auto i=FindPersonArchiveExact(a,id))return PersonArchiveLocation{a[*i].token,std::uint32_t(id-a[*i].first_id)};
    if(fallback&&!a.empty()&&a.front().count)return PersonArchiveLocation{a.front().token,0};
    return std::nullopt;
}
bool PersonIsResidentExact(std::span<const PersonArchiveDescriptor> a,std::uint16_t id) noexcept {
    const auto i=FindPersonArchiveExact(a,id);return i&&a[*i].resident!=0;
}
std::optional<bool> PersonIsResidentFirstExact(std::span<const PersonArchiveDescriptor> a,std::uint16_t id) noexcept {
    if(a.empty())return std::nullopt;return Contains(a.front(),id)&&a.front().resident!=0;
}
bool PersonIsDownloadExact(std::span<const PersonArchiveDescriptor> a,std::uint16_t id,std::uint64_t flags) noexcept {
    if(flags&(1ull<<62))return true;
    if(!PersonIsResidentExact(a,id))return false;
    return !(Contains(a.front(),id)&&a.front().resident!=0);
}
PersonArchivePlan PlanPersonArchivesExact(std::span<const PersonArchiveDescriptor> before,
    PersonArchiveOperation op,const PersonArchiveDescriptor& incoming,std::string_view name) {
    PersonArchivePlan out{};out.archives.assign(before.begin(),before.end());
    auto fail=[&](PersonArchiveStatus status){return PersonArchivePlan{status,{before.begin(),before.end()},{}};};
    if(static_cast<unsigned>(op)>static_cast<unsigned>(PersonArchiveOperation::Finalize))return fail(PersonArchiveStatus::InvalidState);
    for(std::size_t i=0;i<before.size();++i) {
        if(!before[i].token)return fail(PersonArchiveStatus::InvalidToken);
        for(std::size_t j=0;j<i;++j)if(before[j].token==before[i].token)return fail(PersonArchiveStatus::InvalidToken);
    }
    auto event=[&](PersonArchiveEventKind kind,std::uint32_t token=0) {
        PersonArchiveEvent e{kind,token,{}};for(const auto& a:out.archives)e.visible_archives.push_back(a.token);out.events.push_back(std::move(e));
    };
    auto remove=[&](std::string_view target,bool destroy)->PersonArchiveStatus {
        for(std::size_t i=1;i<out.archives.size();++i) {
            if(!out.archives[i].name)return PersonArchiveStatus::UnresolvedName;
            if(*out.archives[i].name!=target)continue;
            const auto node=out.archives[i];out.archives.erase(out.archives.begin()+i);
            if(node.has_support)event(PersonArchiveEventKind::FreeSupport,node.token);
            event(PersonArchiveEventKind::ReleaseNode,node.token);
            if(destroy&&node.has_owner)event(PersonArchiveEventKind::DestroyOwner,node.token);
            break;
        }return PersonArchiveStatus::Ok;
    };
    if(op==PersonArchiveOperation::Initialize||op==PersonArchiveOperation::Append) {
        if(!incoming.token)return fail(PersonArchiveStatus::InvalidToken);
        if(op==PersonArchiveOperation::Initialize) {
            auto base=incoming;base.has_owner=false;out.archives={base};event(PersonArchiveEventKind::InitializeSupport);
            if(base.has_support)event(PersonArchiveEventKind::LoadSupport,base.token);
        } else {
            if(before.empty()||!incoming.has_owner)return fail(PersonArchiveStatus::InvalidState);
            if(std::any_of(before.begin(),before.end(),[&](const auto& a){return a.token==incoming.token;}))return fail(PersonArchiveStatus::InvalidToken);
            if(incoming.has_support)event(PersonArchiveEventKind::LoadSupport,incoming.token);
            out.archives.push_back(incoming);
        }
    } else if(op==PersonArchiveOperation::Finalize) {
        if(out.archives.empty())return out;
        while(out.archives.size()>1) {
            const auto target=out.archives[1].name;if(!target)return fail(PersonArchiveStatus::UnresolvedName);
            const auto status=remove(*target,true);if(status!=PersonArchiveStatus::Ok)return fail(status);
        }
        const auto base=out.archives.front();if(base.has_support)event(PersonArchiveEventKind::FreeSupport,base.token);
        event(PersonArchiveEventKind::FinalizeSupport);event(PersonArchiveEventKind::ReleaseNode,base.token);out.archives.clear();
    } else {
        if(out.archives.empty())return fail(PersonArchiveStatus::InvalidState);
        if(op==PersonArchiveOperation::Prune) {
            std::uint32_t previous=out.archives.front().token;
            for(;;) {
                const auto it=std::find_if(out.archives.begin(),out.archives.end(),[&](const auto& a){return a.token==previous;});
                if(it==out.archives.end())return fail(PersonArchiveStatus::InvalidCursor);
                const auto next=it+1;if(next==out.archives.end())break;
                if(next->resident){previous=next->token;continue;}
                // Original pruning calls the first-name removal path, then
                // resumes from the old previous node. Preserve duplicate names.
                const auto target=next->name;if(!target)return fail(PersonArchiveStatus::UnresolvedName);
                const auto status=remove(*target,true);if(status!=PersonArchiveStatus::Ok)return fail(status);
            }
        } else {
            const auto status=remove(name,op==PersonArchiveOperation::Free);if(status!=PersonArchiveStatus::Ok)return fail(status);
        }
    }
    return out;
}
}
