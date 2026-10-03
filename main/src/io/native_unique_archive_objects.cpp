#include "fates/io/native_unique_archive_objects.hpp"
#include "fates/runtime/native_identifier.hpp"
#include <map>

namespace fates::io::native {
using namespace runtime::native;
namespace {
constexpr std::uint32_t Setup=0x216db8,Cleanup=0x216de8,Destructor=0x216e20;
constexpr std::array Targets{Setup,Cleanup,Destructor};
using S=UniqueArchiveStatus;
std::uint32_t Word(std::span<const std::uint8_t> b,std::size_t p) {
    return std::uint32_t(b[p])|(std::uint32_t(b[p+1])<<8)|
        (std::uint32_t(b[p+2])<<16)|(std::uint32_t(b[p+3])<<24);
}
ProcessCall Service(ProcessHandle h,std::uint32_t target,std::uint32_t object) {
    ProcessCall c;c.process=std::move(h);c.kind=ProcessCallKind::Service;c.target=target;
    c.arguments[0]=object;c.argument_count=1;return c;
}
}
struct UniqueArchiveStorage final:FileDataResource {
    std::shared_ptr<const std::uint8_t> domain;
    std::weak_ptr<NativeFileController> files;
    FileObjectHandle object;
    std::uint32_t data{};
    std::uint64_t revision{};
    std::shared_ptr<const NativeFileImage> image;
    ArchiveLabelTable labels;
    std::vector<std::size_t> relocations;
    std::unique_ptr<IdentifierRegistry<std::size_t>> index;
    bool constructed{};
    bool Live() const {
        auto f=files.lock();return f && f->OwnsLiveData(object,data,revision);
    }
    bool Overlaps(std::size_t at,std::size_t size) const {
        for(auto p:relocations)if(at<p+4 && p<at+size)return true;
        return false;
    }
};
struct NativeUniqueArchiveObjects::State {
    struct History {FileObjectHandle object;std::weak_ptr<UniqueArchiveStorage> storage;};
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<NativeFileController> files;
    std::shared_ptr<const std::uint8_t> domain=std::make_shared<const std::uint8_t>(std::uint8_t{0});
    std::map<std::uint32_t,History> associations;
    bool Live() const {auto s=scheduler.lock();return s && s->root(2);}
    S Mutable(ProcessAccess* a) const {
        auto s=scheduler.lock();if(!s || !s->root(2))return S::Retired;
        if(a)return a->BelongsTo(*s)?S::Ready:S::MismatchedDomain;
        return s->busy()?S::Busy:S::Ready;
    }
    std::shared_ptr<UniqueArchiveStorage> Get(FileObjectHandle o) const {
        if(!o)return {};
        auto i=associations.find(o->serial);
        return i!=associations.end() && i->second.object==o?i->second.storage.lock():nullptr;
    }
};
struct NativeUniqueArchiveObjects::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;
    ProcessCall call;
    FileObjectHandle object;
    unsigned stage{};
    ProcessCallbackStep Step(ProcessAccess& a) override {
        auto s=state->scheduler.lock();if(!s || !a.BelongsTo(*s))return ProcessCallbackStep::Blocked();
        if(call.target==Destructor) {
            if(stage==0){stage=1;return ProcessCallbackStep::Call(Service(call.process,0x17855c,object->serial));}
            if(stage==1){stage=2;return ProcessCallbackStep::Call(Service(call.process,0x2fdce4,object->serial));}
            return ProcessCallbackStep::Return();
        }
        auto p=state->Get(object);if(!p || !p->Live())return ProcessCallbackStep::Blocked();
        if(call.target==Setup) {
            // Setup allocates a NEW index even for the same already-constructed
            // image. ArchiveConstruct then skips that image: the new index is EMPTY.
            // Do not borrow shared-registry Construct's idempotent early return.
            auto index=std::make_unique<IdentifierRegistry<std::size_t>>(127);
            if(!p->constructed) {
                for(const auto& label:p->labels.entries)index->Append(label.identifier.c_str(),label.payload_offset);
                p->constructed=true;
            }
            p->index=std::move(index);
        } else {
            // Original ArchiveDestruct's image flag is independent of index lifetime.
            // A constructed image with no admitted index is an invalid original state.
            if(p->constructed) {
                if(!p->index)return ProcessCallbackStep::Blocked();
                for(auto i=p->labels.entries.rbegin();i!=p->labels.entries.rend();++i)
                    p->index->EraseFirst(i->identifier.c_str());
                p->constructed=false;
            }
            p->index.reset();
        }
        return ProcessCallbackStep::Return();
    }
};
NativeUniqueArchiveObjects::NativeUniqueArchiveObjects(std::shared_ptr<State> s):state_(std::move(s)){}
NativeUniqueArchiveObjects::~NativeUniqueArchiveObjects()=default;
UniqueArchiveStatus NativeUniqueArchiveObjects::Create(std::shared_ptr<NativeProcessScheduler> s,
    std::shared_ptr<ProcessCallbackRegistry> registry,std::shared_ptr<NativeFileController> files,
    std::shared_ptr<NativeUniqueArchiveObjects>& out) {
    if(!s || !s->root(2))return S::NullScheduler;
    if(!registry || !s->UsesCallbacks(registry.get()) || !files || !files->UsesScheduler(*s))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=s;state->files=std::move(files);
    auto module=std::shared_ptr<NativeUniqueArchiveObjects>(new NativeUniqueArchiveObjects(state));
    if(!registry->Register(Targets,module))return S::DuplicateBinding;
    out=std::move(module);return S::Ready;
}
UniqueArchiveStatus NativeUniqueArchiveObjects::Associate(FileObjectHandle object,
    std::shared_ptr<const NativeFileImage> image,ProcessAccess* a) {
    if(auto s=state_->Mutable(a);s!=S::Ready)return s;
    auto row=state_->files->Observe(object);
    if(!row || row->life!=FileObjectLife::Live || !row->fields.data || row->fields.setup!=Setup ||
        row->fields.cleanup!=Cleanup || row->fields.deleting_destructor!=Destructor)return S::InvalidObject;
    if(!image || image->bytes().size()!=row->fields.size)return S::InvalidImage;
    if(auto old=state_->Get(object);old && old->Live())
        return old->image==image?S::Ready:S::ConflictingAssociation;
    // Distinct lower objects aliasing the same live allocation need a shared
    // physical-image flag owner first. Do not infer separate flags from two handles.
    for(const auto& [serial,h]:state_->associations) {
        (void)serial;auto p=h.storage.lock();
        if(p && p->Live() && p->data==row->fields.data && p->object!=object)return S::ConflictingAssociation;
    }
    auto p=std::make_shared<UniqueArchiveStorage>();p->domain=state_->domain;p->files=state_->files;
    p->object=object;p->data=row->fields.data;p->revision=row->data_revision;p->image=std::move(image);
    if(ReadArchiveLabelTable(p->image->bytes(),p->labels)!=ArchiveLabelTableStatus::Ready)return S::InvalidImage;
    for(std::size_t i=0;i<p->labels.relocation_count;++i)
        p->relocations.push_back(Word(p->image->bytes(),32+p->labels.data_bytes+4*i)&~3u);
    if(state_->files->RetainDataResource(object,p->data,p->revision,p,a)!=FileControllerStatus::Ready)return S::InvalidObject;
    std::erase_if(state_->associations,[](const auto& h){return h.second.storage.expired();});
    state_->associations.insert_or_assign(object->serial,State::History{object,p});return S::Ready;
}
UniqueArchiveRecordResult NativeUniqueArchiveObjects::Find(FileObjectHandle object,
    std::optional<std::string_view> name) const {
    if(!state_->Live())return {S::Retired,{}};
    if(!object || !state_->files->Observe(object))return {S::InvalidObject,{}};
    // IdentHash::Get tests a null identifier before dereferencing its index.
    if(!name)return {S::Missing,{}};
    if(name->size()>65535 || name->find('\0')!=std::string_view::npos)return {S::InvalidIdentifier,{}};
    auto p=state_->Get(object);if(!p || !p->Live())return {S::StaleValue,{}};
    if(!p->index)return {S::NotSetup,{}};
    const auto* entry=p->index->Find(std::string(*name).c_str());if(!entry)return {S::Missing,{}};
    if(!p->constructed)return {S::StaleValue,{}};
    UniqueArchiveRecord value;value.storage_=p;value.offset_=entry->value;return {S::Ready,std::move(value)};
}
std::optional<UniqueArchiveObservation> NativeUniqueArchiveObjects::Observe(FileObjectHandle object) const {
    if(!state_->Live())return {};
    auto p=state_->Get(object);if(!p || !p->Live())return {};
    return UniqueArchiveObservation{bool(p->index),p->constructed,p->index?p->index->Size():0,
        p->index?p->index->AccountingCount():0};
}
UniqueArchiveStatus NativeUniqueArchiveObjects::Validate(const UniqueArchiveRecord& record) const {
    if(!state_->Live())return S::Retired;
    if(!record.storage_ || record.storage_->domain!=state_->domain)return S::ForeignValue;
    if(!record.storage_->Live() || !record.storage_->constructed)return S::StaleValue;
    return S::Ready;
}
UniqueArchiveRecordResult NativeUniqueArchiveObjects::At(const UniqueArchiveRecord& r,
    std::size_t displacement,std::size_t extent) const {
    if(auto s=Validate(r);s!=S::Ready)return {s,{}};
    const auto size=r.storage_->image->bytes().size()-32;
    if(r.offset_>size || displacement>size-r.offset_ || extent>size-r.offset_-displacement)return {S::InvalidRecord,{}};
    auto next=r;next.offset_+=displacement;return {S::Ready,std::move(next)};
}
UniqueArchiveWordResult NativeUniqueArchiveObjects::Scalar(const UniqueArchiveRecord& r,
    std::size_t field,std::size_t width) const {
    if(auto s=Validate(r);s!=S::Ready)return {s,0};
    const auto& p=*r.storage_;const auto size=p.labels.data_bytes;
    if(r.offset_>size || field>size-r.offset_ || width>size-r.offset_-field)return {S::InvalidRecord,0};
    const auto at=r.offset_+field;if(p.Overlaps(at,width))return {S::InvalidPointer,0};
    auto b=p.image->bytes();std::uint32_t value{};
    for(std::size_t i=0;i<width;++i)value|=std::uint32_t(b[32+at+i])<<unsigned(8*i);
    return {S::Ready,value};
}
UniqueArchiveWordResult NativeUniqueArchiveObjects::ReadU16(const UniqueArchiveRecord& r,std::size_t field) const {
    return Scalar(r,field,2);
}
UniqueArchiveWordResult NativeUniqueArchiveObjects::ReadU32(const UniqueArchiveRecord& r,std::size_t field) const {
    return Scalar(r,field,4);
}
UniqueArchiveRecordResult NativeUniqueArchiveObjects::Pointer(const UniqueArchiveRecord& r,std::size_t field) const {
    if(auto s=Validate(r);s!=S::Ready)return {s,{}};
    const auto& p=*r.storage_;const auto size=p.labels.data_bytes;
    if(r.offset_>size || field>size-r.offset_ || size-r.offset_-field<4)return {S::InvalidRecord,{}};
    const auto at=r.offset_+field;std::size_t relocations{};
    for(auto v:p.relocations) {
        if(v==at)++relocations;
        else if(at<v+4 && v<at+4)return {S::InvalidPointer,{}};
    }
    const auto target=Word(p.image->bytes(),32+at);
    if(!relocations && !target)return {S::Ready,{}};
    if(relocations!=1 || target>=p.image->bytes().size()-32)return {S::InvalidPointer,{}};
    auto result=r;result.offset_=target;return {S::Ready,std::move(result)};
}
UniqueArchiveStringResult NativeUniqueArchiveObjects::ReadString(const UniqueArchiveRecord& r) const {
    if(!r)return {state_->Live()?S::Ready:S::Retired,std::nullopt};
    if(auto s=Validate(r);s!=S::Ready)return {s,std::nullopt};
    const auto& p=*r.storage_;auto bytes=p.image->bytes();
    if(r.offset_>=bytes.size()-32)return {S::InvalidPointer,std::nullopt};
    auto end=r.offset_;
    while(end<bytes.size()-32 && bytes[32+end])++end;
    if(end==bytes.size()-32 || p.Overlaps(r.offset_,end-r.offset_+1))return {S::InvalidPointer,std::nullopt};
    return {S::Ready,std::string(reinterpret_cast<const char*>(bytes.data()+32+r.offset_),end-r.offset_)};
}
bool NativeUniqueArchiveObjects::UsesController(const NativeFileController& f) const noexcept {return state_->files.get()==&f;}
std::unique_ptr<ProcessContinuation> NativeUniqueArchiveObjects::Begin(const ProcessCall& c) {
    if(!state_->Live() || c.kind!=ProcessCallKind::Service || c.has_self || c.this_adjustment ||
        !c.process || c.argument_count!=1 || (c.target!=Setup && c.target!=Cleanup && c.target!=Destructor))return {};
    auto object=state_->files->ResolveObject(c.arguments[0]);auto row=state_->files->Observe(object);
    if(!row || row->life!=(c.target==Destructor?FileObjectLife::Destroying:FileObjectLife::Live))return {};
    if((c.target==Setup && row->fields.setup!=Setup) || (c.target==Cleanup && row->fields.cleanup!=Cleanup) ||
        (c.target==Destructor && row->fields.deleting_destructor!=Destructor))return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->call=c;next->object=std::move(object);return next;
}
}
