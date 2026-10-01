#include "fates/map/native_field_sources.hpp"
#include "fates/runtime/native_archive.hpp"
#include <bit>
#include <limits>
#include <algorithm>

namespace fates::map::native {
namespace {
void Publish(std::vector<FieldSourceEvent>& events,FieldSourceEvent event,FieldSourceObserver* observer) {
    events.push_back(std::move(event));
    if(observer)observer->OnSourceEvent(events.back());
}
}
bool DecodeFieldSourceArchive(std::span<const std::uint8_t> b,FieldSourceArchive& output) {
    namespace rn=fates::runtime::native;
    const auto version=rn::ReadArchiveLabeledWord(b,"MapHeader");
    if(!version||*version!=0x20140623u)return false;
    const auto word=[&](std::size_t at){return std::uint32_t(b[at])|(std::uint32_t(b[at+1])<<8)|(std::uint32_t(b[at+2])<<16)|(std::uint32_t(b[at+3])<<24);};
    const auto data_end=std::size_t(word(4)&~3u)+0x20;
    const auto fits=[&](std::size_t at,std::size_t size){return at>=0x20&&at<=data_end&&size<=data_end-at;};
    const auto string=[&](std::size_t at,std::string& out){
        const auto p=word(at);const auto offset=std::size_t(p)+0x20;
        if(!p||offset>=b.size())return false;
        auto end=offset;while(end<b.size()&&b[end])++end;if(end==b.size())return false;
        out.assign(reinterpret_cast<const char*>(b.data()+offset),end-offset);return true;
    };
    FieldSourceArchive next;
    const auto parts=rn::FindArchiveLabel(b,"PartsList");
    if(parts.status==rn::ArchiveLabelStatus::Invalid)return false;
    if(parts.status==rn::ArchiveLabelStatus::Found){
        FieldHeightArchive decoded;if(!DecodeFieldHeightArchive(b,decoded))return false;next.parts=std::move(decoded);
    }
    for(const auto label:{std::string_view("FileList"),std::string_view("ReferList")}){
        const auto found=rn::FindArchiveLabel(b,label);
        if(found.status==rn::ArchiveLabelStatus::Invalid)return false;
        if(found.status==rn::ArchiveLabelStatus::Missing)continue;
        const auto at=found.offset;if(!fits(at,8))return false;
        const auto count=std::size_t(word(at));const auto p=word(at+4);const auto base=std::size_t(p)+0x20;
        const auto stride=label=="FileList"?4u:44u;
        if(count>std::size_t(std::numeric_limits<std::int32_t>::max())||
           (count&&(!p||p%4||count>(data_end-0x20)/stride||!fits(base,count*stride))))return false;
        if(label=="FileList"){
            next.files.emplace();next.files->reserve(count);
            for(std::size_t i=0;i<count;++i){std::string name;if(!string(base+4*i,name))return false;next.files->push_back(std::move(name));}
        }else{
            next.references.emplace();next.references->reserve(count);
            for(std::size_t i=0;i<count;++i){
                const auto at=base+44*i;FieldReferData reference;
                if(!string(at,reference.name)||!string(at+4,reference.part_name))return false;
                auto offset=at+8;
                for(auto* group:{&reference.pose.scale,&reference.pose.rotation_degrees,&reference.pose.position})
                    for(auto& value:*group){value=std::bit_cast<float>(word(offset));offset+=4;}
                next.references->push_back(std::move(reference));
            }
        }
    }
    output=std::move(next);return true;
}

FieldSourcePart FieldSourceRegistry::FindPart(const char* name) const {
    const auto* entry=parts_.Find(name);return entry?entry->value:FieldSourcePart{};
}
bool FieldSourceRegistry::EntryData(std::string_view input,const FieldSourceProvider& provider,std::vector<FieldSourceEvent>& events,FieldSourceObserver* observer) {
    const std::string name(input.substr(0,input.find('\0')));
    if(const auto* entry=files_.Find(name.c_str());entry&&entry->value)return true;
    Publish(events,{FieldSourceEventKind::ReadResource,name},observer);
    auto archive=provider(name);
    // Retail assumes PartsList exists after TryRead. Refuse that unsafe native
    // input as a provider admission failure, before registering any source.
    if(!archive||!archive->parts){Publish(events,{FieldSourceEventKind::DestroyFailedResource,name},observer);return false;}
    auto resource=std::make_shared<const FieldSourceResource>(FieldSourceResource{name,std::move(archive)});
    resources_.push_back(resource);
    for(const auto& part:resource->archive->parts->parts){
        auto alias=std::shared_ptr<const FieldHeightPart>(resource->archive,&part);
        parts_.Append(part.name.c_str(),{resource,std::move(alias)});
    }
    files_.Append(name.c_str(),std::move(resource));return true;
}
std::shared_ptr<const FieldSourcePlacement> FieldSourceRegistry::CreateObject(const FieldReferData& reference,
    std::vector<FieldSourceEvent>& events,FieldSourceObserver* observer) {
    auto source=FindPart(reference.part_name.c_str());if(!source.part)return {};
    auto placement=std::make_shared<const FieldSourcePlacement>(FieldSourcePlacement{std::move(source),reference.name,reference.pose});
    Publish(events,{FieldSourceEventKind::ConstructAndLoadObject,{},placement->source.resource,placement},observer);
    placements_.push_back(placement);
    if(observer)observer->OnPlacementInserted(placement);
    return placement;
}
std::shared_ptr<const FieldSourcePlacement> FieldSourceRegistry::CreateObject(const char* name,
    const FieldSourceProvider& provider,std::vector<FieldSourceEvent>& events,FieldSourceObserver* observer) {
    const auto source=FindPart(name);
    if(!name||!source.part||!source.part->bch_container)return {};
    if(!EntryData(*source.part->bch_container,provider,events,observer))return {};
    return CreateObject(FieldReferData{source.part->name,source.part->name,FieldPose{}},events,observer);
}
bool FieldSourceRegistry::DeleteObject(const std::shared_ptr<const FieldSourcePlacement>& placement,
    std::vector<FieldSourceEvent>& events,FieldSourceObserver* observer) {
    const auto found=std::find(placements_.begin(),placements_.end(),placement);
    if(found==placements_.end())return false;
    auto removed=*found;placements_.erase(found);
    Publish(events,{FieldSourceEventKind::DestroyObject,{},removed->source.resource,std::move(removed)},observer);
    return true;
}
FieldSourceLoadResult FieldSourceRegistry::LoadField(std::string_view input,const FieldSourceProvider& provider,FieldSourceObserver* observer) {
    const std::string name(input.substr(0,input.find('\0')));FieldSourceLoadResult result;
    Publish(result.events,{FieldSourceEventKind::ReadField,name},observer);
    auto archive=provider(name);
    if(archive&&archive->files&&archive->references){
        for(const auto& file:*archive->files)EntryData(file,provider,result.events,observer);
        for(const auto& reference:*archive->references){
            auto placement=CreateObject(reference,result.events,observer);if(!placement)continue;
            Publish(result.events,{FieldSourceEventKind::SetDisposition,{},placement->source.resource,placement},observer);
        }
        result.loaded=true;
    }
    Publish(result.events,{FieldSourceEventKind::ReleaseField,name},observer);return result;
}
std::vector<FieldSourceEvent> FieldSourceRegistry::FreeField(FieldSourceObserver* observer) {
    std::vector<FieldSourceEvent> events;
    while(!placements_.empty()){
        const auto placement=placements_.back();DeleteObject(placement,events,observer);
    }
    while(!resources_.empty()) {
        auto resource=resources_.front();resources_.erase(resources_.begin());
        Publish(events,{FieldSourceEventKind::ReleaseResource,resource->name,resource},observer);
    }
    Publish(events,{FieldSourceEventKind::ClearFileRegistry},observer);files_.Clear();
    Publish(events,{FieldSourceEventKind::ClearPartRegistry},observer);parts_.Clear();return events;
}
}
