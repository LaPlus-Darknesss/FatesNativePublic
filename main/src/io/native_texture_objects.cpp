#include "fates/io/native_texture_objects.hpp"
#include <algorithm>
#include <map>

namespace fates::io::native {
using namespace runtime::native;
using graphics::portable::TextureImage;
namespace {
using S=TextureObjectStatus;
constexpr std::uint32_t TexSetup=0x504b38,TexCleanup=0x504be4,TexDelete=0x504c1c,
    ResSetup=0x50336c,ResCleanup=0x503660,ResLink=0x503174,ResDestructor=0x178558,
    ResAllocator=0x549700,ResAlign=0x549800,ResDelay=0x5497f8;
constexpr std::array Targets{TexSetup,TexCleanup,TexDelete,ResSetup,ResCleanup,ResLink,
    ResDestructor,ResAllocator,ResAlign,ResDelay};
std::uint32_t Word(std::span<const std::uint8_t> bytes,std::size_t at) {
    return std::uint32_t(bytes[at])|(std::uint32_t(bytes[at+1])<<8)|
        (std::uint32_t(bytes[at+2])<<16)|(std::uint32_t(bytes[at+3])<<24);
}
ProcessCall Service(ProcessHandle process,std::uint32_t target,std::uint32_t object) {
    ProcessCall call;call.process=std::move(process);call.kind=ProcessCallKind::Service;
    call.target=target;call.argument_count=1;call.arguments[0]=object;return call;
}

}
bool ComputeNativeTextureImageSize(std::uint16_t width,std::uint16_t height,std::uint8_t format,std::uint8_t mips,std::uint32_t& output) noexcept {
    const auto product=std::uint64_t(width)*height;
    if(!width || !height || format>13 || product>0x7fffffffu)return false;
    const float pixels=static_cast<float>(static_cast<std::int32_t>(product));
    const float factor=format==0?4.0f:format==1?3.0f:format<=6?2.0f:(format>=10 && format<=12)?0.5f:1.0f;
    const float extent=pixels*factor;
    if(!(extent>=0.0f && extent<4294967296.0f))return false;
    auto level=static_cast<std::uint32_t>(extent);std::uint64_t total=level;
    for(unsigned mip=1;mip<mips;++mip){level>>=2;total+=level;}
    if(total>0xffffffffu)return false;
    output=static_cast<std::uint32_t>(total);return true;
}
struct TextureGeneration {
    std::shared_ptr<const NativeTextureResource> resource;
    std::vector<NativeTextureDescription> descriptions;
};
struct TextureStorage final:FileDataResource {
    std::weak_ptr<NativeFileController> files;FileObjectHandle object;
    std::uint32_t data{};std::uint64_t revision{};
    std::shared_ptr<const NativeFileImage> image;
    std::shared_ptr<const NativeTextureResource> resource;
    std::shared_ptr<const TextureGeneration> generation;
    std::array<std::unique_ptr<std::uint8_t[]>,2> scratch;
    std::size_t scratch_bytes{};
    
    bool Live() const {auto owner=files.lock();return owner && owner->OwnsLiveData(object,data,revision);}
};
struct NativeTextureObjects::State {
    struct Object {FileObjectHandle identity;std::weak_ptr<TextureStorage> storage;std::string unresolved;
        std::uint32_t resource_flags{},count{};};
    std::weak_ptr<NativeProcessScheduler> scheduler;std::shared_ptr<NativeFileController> files;
    std::shared_ptr<NativeFileEntry> entry;NativeTextureResourceBackend backend;std::map<std::uint32_t,std::shared_ptr<Object>> objects;
    std::optional<std::uint32_t> allocator;
    bool Live() const {auto owner=scheduler.lock();return owner && owner->root(2);}
    S Mutable(ProcessAccess* access) const {
        auto owner=scheduler.lock();if(!owner || !owner->root(2))return S::Retired;
        if(access)return access->BelongsTo(*owner)?S::Ready:S::MismatchedDomain;
        return owner->busy()?S::Busy:S::Ready;
    }
    std::shared_ptr<Object> Get(FileObjectHandle handle) const {
        if(!handle)return {};
        const auto found=objects.find(handle->serial);
        return found!=objects.end() && found->second->identity==handle?found->second:nullptr;
    }
    S Associate(FileObjectHandle handle,std::shared_ptr<const NativeFileImage> image,ProcessAccess* access) {
        if(auto status=Mutable(access);status!=S::Ready)return status;
        auto object=Get(handle);auto row=files->Observe(handle);
        if(!object || !row || row->life!=FileObjectLife::Live || !row->fields.data)return S::InvalidObject;
        if(!image || image->bytes().size()!=row->fields.size)return S::InvalidImage;
        if(auto current=object->storage.lock();current && current->Live())
            return current->image==image?S::Ready:S::ConflictingImage;
        if(object->resource_flags&0x4000u || object->count)return S::StaleValue;
        for(const auto& [serial,other]:objects) {
            (void)serial;auto storage=other->storage.lock();
            if(storage && storage->Live() && storage->data==row->fields.data && storage->object!=handle)return S::ConflictingImage;
        }
        auto storage=std::make_shared<TextureStorage>();storage->files=files;storage->object=handle;
        storage->data=row->fields.data;storage->revision=row->data_revision;storage->image=std::move(image);
        if(files->RetainDataResource(handle,storage->data,storage->revision,storage,access)!=FileControllerStatus::Ready)
            return S::InvalidObject;
        object->storage=storage;object->unresolved.clear();return S::Ready;
    }
};
struct NativeTextureObjects::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;std::shared_ptr<State::Object> object;ProcessCall call;unsigned stage{};
    ProcessCallbackStep Invoke(std::uint32_t target) {
        return ProcessCallbackStep::Call(Service(call.process,target,object->identity->serial));
    }
    ProcessCallbackStep Step(ProcessAccess& access) override {
        auto scheduler=state->scheduler.lock();
        if(!scheduler || !access.BelongsTo(*scheduler))return ProcessCallbackStep::Blocked();
        if(call.target==ResAllocator)return state->allocator?ProcessCallbackStep::Return(*state->allocator):ProcessCallbackStep::Blocked();
        if(call.target==ResAlign)return ProcessCallbackStep::Return(0x80);
        if(call.target==ResDelay)return ProcessCallbackStep::Return(1);
        if(call.target==TexDelete || call.target==ResDestructor) {
            // Retail destructors do NOT replace the explicit Cleanup operation.
            if(stage==0){stage=1;return Invoke(0x17855c);}
            if(stage==1 && call.target==TexDelete){stage=2;return Invoke(0x2fdce4);}
            return ProcessCallbackStep::Return(object->identity->serial);
        }
        auto row=state->files->Observe(object->identity);
        if(!row || row->life!=FileObjectLife::Live)return ProcessCallbackStep::Blocked();
        if((call.target==ResCleanup || call.target==ResLink) && !(object->resource_flags&0x4000u))
            return ProcessCallbackStep::Return();
        if(call.target==TexCleanup && !(object->resource_flags&0x4000u) && !object->count)
            return ProcessCallbackStep::Return();
        auto storage=object->storage.lock();
        if(!storage || !storage->Live()) {
            const auto read=state->entry->ReadResult(object->identity);
            if(!read || state->Associate(object->identity,read->image,&access)!=S::Ready) {
                object->unresolved="No current transport image for resource allocation";return ProcessCallbackStep::Blocked();
            }
            storage=object->storage.lock();
        }
        if(!storage || !storage->Live())return ProcessCallbackStep::Blocked();
        if(call.target==TexSetup) {
            if(stage==0){stage=1;return Invoke(ResSetup);}
            if(!(object->resource_flags&0x4000u))return ProcessCallbackStep::Return();
            if(!storage->resource)return ProcessCallbackStep::Blocked();
            auto generation=std::make_shared<TextureGeneration>();generation->resource=storage->resource;
            const auto content=storage->resource->contents();
            generation->descriptions.assign(content.begin(),content.end());
            for(auto& description:generation->descriptions) {
                description.filter=description.mip_count>1?std::uint8_t{1}:std::uint8_t{0};
                description.wrap=0;description.mode=0;description.target=0;
                const float sx=1.0f/static_cast<float>(description.width);
                const float sy=-1.0f/static_cast<float>(description.height);
                description.matrix={sx,0,0,0,0,sy,0,1,0,0,1,0};
            }
            object->count=static_cast<std::uint32_t>(generation->descriptions.size());
            if(object->count)storage->generation=std::move(generation);
            return ProcessCallbackStep::Return();
        }
        if(call.target==TexCleanup) {
            if(stage==0) {
                if(storage->generation){storage->generation.reset();object->count=0;}
                stage=1;return Invoke(ResCleanup);
            }
            return ProcessCallbackStep::Return();
        }
        if(call.target==ResCleanup) {
            if(!(object->resource_flags&0x4000u))return ProcessCallbackStep::Return();
            storage->resource.reset();storage->scratch={};storage->scratch_bytes=0;
            object->resource_flags&=~0xc000u;return ProcessCallbackStep::Return();
        }
        if(call.target==ResLink) {
            // The admitted portable resource has no Model/FragmentLight records.
            // Other resources refuse in Setup rather than receiving fake links.
            return storage->resource?ProcessCallbackStep::Return():ProcessCallbackStep::Blocked();
        }
        if(object->resource_flags&0x4000u)return ProcessCallbackStep::Return();
        const auto bytes=storage->image->bytes();
        if(bytes.size()<6){object->unresolved="Resource header extent unavailable";return ProcessCallbackStep::Blocked();}
        if(bytes[4]!=0x22 || bytes[5]!=0x23)return ProcessCallbackStep::Return();
        if((row->fields.flags&0x7fu)!=2u) {
            object->unresolved="Resource requires VRAM/command placement or delayed relocation policy";
            return ProcessCallbackStep::Blocked();
        }
        std::shared_ptr<const NativeTextureResource> resource;
        if(bytes.size()<0x44 || !state->backend.prepare ||
            !state->backend.prepare(bytes,resource,object->unresolved) || !resource)
            return ProcessCallbackStep::Blocked();
        const auto contents=resource->contents();
        if(contents.size()>65535)return ProcessCallbackStep::Blocked();
        for(const auto& content:contents)
            if(!content.width || !content.height || content.format>13)
                return ProcessCallbackStep::Blocked();
        // The two runtime scratch allocations are real native backing storage,
        // not invented original pointers. No contents are published as initialized.
        const auto first=Word(bytes,0x38),second=Word(bytes,0x3c);
        if(first>64u*1024u*1024u || second>64u*1024u*1024u) {
            object->unresolved="Resource scratch allocation outside admitted extent";return ProcessCallbackStep::Blocked();
        }
        std::array<std::unique_ptr<std::uint8_t[]>,2> scratch;
        if(first)scratch[0]=std::unique_ptr<std::uint8_t[]>(new std::uint8_t[first]);
        if(second)scratch[1]=std::unique_ptr<std::uint8_t[]>(new std::uint8_t[second]);
        storage->resource=std::move(resource);storage->scratch=std::move(scratch);
        storage->scratch_bytes=std::size_t(first)+second;object->resource_flags=(row->fields.flags&0xffffu)|0x4000u;
        object->unresolved.clear();return ProcessCallbackStep::Return();
    }
};
NativeTextureObjects::NativeTextureObjects(std::shared_ptr<State> state):state_(std::move(state)){}
NativeTextureObjects::~NativeTextureObjects()=default;
TextureObjectStatus NativeTextureObjects::Create(std::shared_ptr<NativeProcessScheduler> scheduler,
    std::shared_ptr<ProcessCallbackRegistry> registry,std::shared_ptr<NativeFileController> files,
    std::shared_ptr<NativeFileBase> bases,std::shared_ptr<NativeFileEntry> entry,NativeTextureResourceBackend backend,
    std::shared_ptr<NativeTextureObjects>& output) {
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !files || !files->UsesScheduler(*scheduler) ||
        !bases || !entry || !entry->UsesOwners(*files,*bases))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->files=std::move(files);state->entry=std::move(entry);state->backend=std::move(backend);
    auto module=std::shared_ptr<NativeTextureObjects>(new NativeTextureObjects(state));
    if(!registry->Register(Targets,module))return S::DuplicateBinding;
    output=std::move(module);return S::Ready;
}
FileObjectMethods NativeTextureObjects::Methods() noexcept {
    return {TexDelete,TexSetup,TexCleanup,ResAllocator,ResAlign,ResDelay};
}
TextureObjectStatus NativeTextureObjects::Construct(FileObjectHandle& output,ProcessAccess* access) {
    if(auto status=state_->Mutable(access);status!=S::Ready)return status;
    FileObjectHandle handle;
    if(state_->files->ConstructObject(Methods(),handle,access)!=FileControllerStatus::Ready)return S::InvalidObject;
    auto object=std::make_shared<State::Object>();object->identity=handle;state_->objects.emplace(handle->serial,std::move(object));
    output=std::move(handle);return S::Ready;
}
TextureObjectStatus NativeTextureObjects::PublishAllocator(std::uint32_t word,ProcessAccess* access) {
    if(auto status=state_->Mutable(access);status!=S::Ready)return status;
    state_->allocator=word;return S::Ready;
}
TextureObjectStatus NativeTextureObjects::Associate(FileObjectHandle handle,std::shared_ptr<const NativeFileImage> image,ProcessAccess* access) {
    return state_->Associate(std::move(handle),std::move(image),access);
}
std::optional<TextureObjectObservation> NativeTextureObjects::Observe(FileObjectHandle handle) const {
    auto object=state_->Get(handle);auto row=state_->files->Observe(handle);
    if(!state_->Live() || !object || !row || row->life!=FileObjectLife::Live)return {};
    auto storage=object->storage.lock();
    if(storage && !storage->Live())return {};
    return TextureObjectObservation{object->resource_flags,object->count,
        storage && bool(storage->resource),storage && bool(storage->generation),storage?storage->scratch_bytes:0u,object->unresolved};
}
TextureLookupResult NativeTextureObjects::GetTexture(FileObjectHandle handle,std::int32_t index) const {
    if(index==65535)return {S::Dummy,{}};
    if(!state_->Live())return {S::Retired,{}};
    auto object=state_->Get(handle);auto observed=Observe(handle);
    if(!object || !observed)return {S::InvalidObject,{}};
    if(!(observed->resource_flags&0x4000u))return {S::Dummy,{}};
    if(index<0)return {S::InvalidIndex,{}}; // original would form an out-of-array pointer
    if(static_cast<std::uint32_t>(index)>=observed->texture_count)return {S::Dummy,{}};
    auto storage=object->storage.lock();
    if(!storage || !storage->generation)return {S::StaleValue,{}};
    NativeTextureView view;view.storage_=storage;view.generation_=storage->generation;view.index_=static_cast<std::size_t>(index);
    return {S::Ready,std::move(view)};
}
TextureLookupResult NativeTextureObjects::GetTexture(FileObjectHandle handle,std::optional<std::string_view> name) const {
    if(!state_->Live())return {S::Retired,{}};
    auto object=state_->Get(handle);auto observed=Observe(handle);
    if(!object || !observed)return {S::InvalidObject,{}};
    if(!(observed->resource_flags&0x4000u))return {S::Dummy,{}};
    if(!name)return {S::InvalidName,{}}; // retail reaches strlen with a null pointer
    auto storage=object->storage.lock();if(!storage || !storage->generation)return {S::Dummy,{}};
    const auto prefix=name->substr(0,name->find('\0'));
    const auto& descriptions=storage->generation->descriptions;
    for(std::size_t index=0;index<descriptions.size();++index)
        if(descriptions[index].name==prefix)return GetTexture(handle,static_cast<std::int32_t>(index));
    return {S::Dummy,{}};
}
TextureObjectStatus NativeTextureObjects::Describe(const NativeTextureView& view,NativeTextureDescription& output) const {
    if(!state_->Live())return S::Retired;
    const auto storage=view.storage_.lock();
    const auto object=storage?state_->Get(storage->object):nullptr;
    if(!object || !storage || !storage->Live() || storage->files.lock()!=state_->files ||
        !(object->resource_flags&0x4000u) || !view.generation_ || storage->generation!=view.generation_ ||
        view.index_>=view.generation_->descriptions.size())return S::StaleValue;
    output=view.generation_->descriptions[view.index_];return S::Ready;
}
TextureObjectStatus NativeTextureObjects::DecodeBase(const NativeTextureView& view,TextureImage& output,std::string& error) const {
    NativeTextureDescription description;
    if(auto status=Describe(view,description);status!=S::Ready)return status;
    return view.generation_->resource->DecodeBase(view.index_,output,error)?S::Ready:S::UnsupportedResource;
}
bool NativeTextureView::SameTexture(const NativeTextureView& other) const noexcept {
    return generation_ && generation_==other.generation_ && index_==other.index_;
}
bool NativeTextureObjects::UsesScheduler(const NativeProcessScheduler& scheduler) const noexcept{return state_->scheduler.lock().get()==&scheduler;}
bool NativeTextureObjects::UsesOwners(const NativeFileController& files,const NativeFileEntry& entry) const noexcept {
    return state_->files.get()==&files && state_->entry.get()==&entry;
}
std::unique_ptr<ProcessContinuation> NativeTextureObjects::Begin(const ProcessCall& call) {
    if(!state_->Live() || call.kind!=ProcessCallKind::Service || call.has_self || call.this_adjustment || !call.process ||
        call.argument_count!=(call.target==ResLink?2u:1u) || std::find(Targets.begin(),Targets.end(),call.target)==Targets.end())return {};
    const auto object=state_->Get(state_->files->ResolveObject(call.arguments[0]));if(!object)return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->object=object;next->call=call;return next;
}
}
