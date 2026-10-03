#include "fates/io/native_file_entry.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <map>

namespace fates::io::native {
using namespace runtime::native;
namespace {
constexpr std::uint32_t Entry=0x112de8,ObjectSweep=0x11591c,RawAllocator=0x50a51c,RawAlign=0x50a534;
constexpr std::array Targets{Entry,ObjectSweep,RawAllocator,RawAlign};
ProcessCall Service(ProcessHandle h,std::uint32_t target,std::initializer_list<std::uint32_t> args) {
    ProcessCall c;c.process=std::move(h);c.kind=ProcessCallKind::Service;c.target=target;
    c.argument_count=static_cast<std::uint8_t>(args.size());std::copy(args.begin(),args.end(),c.arguments.begin());return c;
}
float F32(std::uint32_t v) {volatile float result=static_cast<float>(v);return result;}
std::uint32_t UnsignedTruncate(float value) {
    if(std::isnan(value) || value<=0)return 0;
    if(value>=4294967296.0f)return 0xffffffffu;
    return static_cast<std::uint32_t>(value);
}
}
struct NativeFileEntry::State {
    struct Request {
        FileEntryHandle identity;FileBaseHandle base;FileObjectHandle object;FilePathHandle path;
        std::uint32_t flags{},read_type{};bool started{};
    };
    struct Read final:FileDataResource {FileObjectHandle object;std::uint64_t revision{};FileReadResult result;};
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<NativeFileController> files;
    std::shared_ptr<NativeFileBase> bases;
    FileEntryHostServices host;
    std::map<std::uint32_t,std::uint32_t> allocators;
    std::optional<std::uint32_t> raw_allocator;
    std::map<std::uint32_t,std::shared_ptr<Request>> requests;
    std::map<std::uint32_t,std::weak_ptr<const Read>> reads;
    std::uint32_t serial{};
    bool Live() const {auto s=scheduler.lock();return s && s->root(2);}
    FileEntryStatus Mutable(ProcessAccess* a) const {
        auto s=scheduler.lock();if(!s || !s->root(2))return FileEntryStatus::Retired;
        if(a)return a->BelongsTo(*s)?FileEntryStatus::Ready:FileEntryStatus::MismatchedDomain;
        return s->busy()?FileEntryStatus::Busy:FileEntryStatus::Ready;
    }
    std::shared_ptr<Request> Get(std::uint32_t id) const {
        auto it=requests.find(id);return it==requests.end()?nullptr:it->second;
    }
};
struct NativeFileEntry::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;ProcessCall call;
    std::shared_ptr<State::Request> request;
    FileObjectHandle object;FileControllerHandle controller;
    FileReadResult read;
    std::uint32_t allocator{},alignment{},heap[6]{};
    unsigned stage{},queries{};
    ProcessCallbackStep Invoke(std::uint32_t target,std::initializer_list<std::uint32_t> args) {
        return ProcessCallbackStep::Call(Service(call.process,target,args));
    }
    std::optional<FileObjectObservation> Object() const {
        auto row=state->files->Observe(object);
        return row && row->life==FileObjectLife::Live?row:std::nullopt;
    }
    bool Write(const CarriedFileObject& f,ProcessAccess& a) {
        return state->files->WriteFields(object,f,&a)==FileControllerStatus::Ready;
    }
    ProcessCallbackStep Sweep(ProcessAccess& a) {
        if(stage==0) {
            // Captured BEFORE virtual GetAllocator and all heap queries.
            auto current=state->files->Observe();auto row=Object();
            if(!current || !row || !row->fields.get_allocator)return ProcessCallbackStep::Blocked();
            controller=current->identity;stage=1;return Invoke(row->fields.get_allocator,{object->serial});
        }
        if(stage==1) {allocator=a.call_result();stage=2;}
        if(stage==2) {
            if(!allocator || !state->allocators.contains(allocator) || !state->host.heap)return ProcessCallbackStep::Blocked();
            const auto query=(queries==1 || queries==5)?FileHeapQuery::Free:FileHeapQuery::Total;
            const auto value=state->host.heap(allocator,query,a);
            if(!value)return ProcessCallbackStep::Blocked();
            heap[queries++]=*value;
            if(queries==3) {
                const auto used=heap[0]-heap[1];
                const float numerator=F32(used),denominator=F32(heap[2]);
                // Unsigned operands admit only positive zero/Inf or canonical
                // ARM default NaN for0/0. Retail compares SIGNED FLOAT BITS.
                volatile float ratio=denominator==0?
                    (numerator==0?std::bit_cast<float>(0x7fc00000u):std::numeric_limits<float>::infinity()):
                    numerator/denominator;
                const float value_=ratio;
                if(std::bit_cast<std::int32_t>(value_)<=std::bit_cast<std::int32_t>(0x3f4ccccdu))
                    return ProcessCallbackStep::Return();
            }
            if(queries<6)return ProcessCallbackStep::Continue();
            stage=3;
        }
        if(stage==3) {
            volatile float product=F32(heap[3])*std::bit_cast<float>(0x3f19999au);
            volatile float amount=F32(heap[4]-heap[5])-product;
            const auto budget=UnsignedTruncate(amount);
            if(!controller)return ProcessCallbackStep::Blocked();
            stage=4;return Invoke(0x11fbcc,{controller->serial,budget});
        }
        return ProcessCallbackStep::Return();
    }
    ProcessCallbackStep Step(ProcessAccess& a) override {
        auto s=state->scheduler.lock();if(!s || !a.BelongsTo(*s))return ProcessCallbackStep::Blocked();
        if(call.target==ObjectSweep)return Sweep(a);
        if(call.target==RawAlign)return ProcessCallbackStep::Return(0x80);
        if(call.target==RawAllocator)
            return state->raw_allocator?ProcessCallbackStep::Return(*state->raw_allocator):ProcessCallbackStep::Blocked();
        if(stage==0) {
            auto row=Object();
            if(!row || row->cache_linked || row->async_linked)return ProcessCallbackStep::Blocked();
            auto f=row->fields;f.flags|=request->flags;
            if(state->bases->WriteAttachment(request->base,object,&a)!=FileBaseStatus::Ready || !Write(f,a))
                return ProcessCallbackStep::Blocked();
            request->started=true;stage=1;return Invoke(ObjectSweep,{object->serial});
        }
        if(stage==1) {
            if(request->read_type>2)return ProcessCallbackStep::Return();
            const auto base=state->bases->Observe(request->base);
            if(!base || !base->object || !request->path)return ProcessCallbackStep::Blocked();
            object=base->object;
            auto row=Object();if(!row || row->cache_linked || row->async_linked)return ProcessCallbackStep::Blocked();
            stage=request->read_type==1?5u:2u;
        }
        if(stage==2) {
            auto row=Object();if(!row || !row->fields.get_align)return ProcessCallbackStep::Blocked();
            stage=3;return Invoke(row->fields.get_align,{object->serial});
        }
        if(stage==3) {alignment=a.call_result();stage=4;}
        if(stage==4) {
            auto row=Object();if(!row || !row->fields.get_allocator)return ProcessCallbackStep::Blocked();
            stage=10;return Invoke(row->fields.get_allocator,{object->serial});
        }
        if(stage==10) {allocator=a.call_result();stage=11;}
        if(stage==11) {
            if(!state->host.read)return ProcessCallbackStep::Blocked();
            const auto value=state->host.read({request->path->text,allocator,alignment,request->read_type==2},a);
            if(!value)return ProcessCallbackStep::Blocked();
            read=*value;stage=5;
        }
        if(stage==5) {
            if(state->files->WriteUnlinkedPath(object,request->path->text.substr(0,79),&a)!=FileControllerStatus::Ready)
                return ProcessCallbackStep::Blocked();
            stage=6;
        }
        if(stage==6) {
            auto row=Object();if(!row || !row->fields.get_allocator)return ProcessCallbackStep::Blocked();
            stage=7;return Invoke(row->fields.get_allocator,{object->serial});
        }
        if(stage==7) {allocator=a.call_result();stage=8;}
        if(stage==8) {
            const auto release=state->allocators.find(allocator);
            if(allocator && release==state->allocators.end())return ProcessCallbackStep::Blocked();
            auto row=Object();if(!row)return ProcessCallbackStep::Blocked();
            auto f=row->fields;f.allocator=allocator;f.allocator_free=allocator?release->second:0;
            f.data=read.data;f.size=read.size;if(request->read_type==1)f.flags|=0x20000000u;
            if(!Write(f,a))return ProcessCallbackStep::Blocked();
            auto retained=std::make_shared<State::Read>();retained->object=object;retained->result=read;
            retained->revision=state->files->Observe(object)->data_revision;
            if(state->files->RetainDataResource(object,read.data,retained->revision,retained,&a)!=FileControllerStatus::Ready)
                return ProcessCallbackStep::Blocked();
            std::erase_if(state->reads,[](const auto& entry){return entry.second.expired();});
            state->reads.insert_or_assign(object->serial,retained);
            stage=9;
        }
        if(stage==9) {
            auto current=state->files->Observe();
            if(!current || !current->present)return ProcessCallbackStep::Blocked();
            if(state->files->RegisterUnlinked(current->identity,object,&a)!=FileControllerStatus::Ready)
                return ProcessCallbackStep::Blocked();
            if(request->read_type!=1)return ProcessCallbackStep::Return();
            stage=12;
        }
        if(stage==12) {
            auto current=state->files->Observe();
            if(!current || !current->present)return ProcessCallbackStep::Blocked();
            controller=current->identity;
            if(state->files->QueueUnlinked(controller,object,&a)!=FileControllerStatus::Ready)return ProcessCallbackStep::Blocked();
            stage=13;
        }
        if(!state->host.signal_async || !state->host.signal_async(controller,a))return ProcessCallbackStep::Blocked();
        return ProcessCallbackStep::Return();
    }
};
NativeFileEntry::NativeFileEntry(std::shared_ptr<State> s):state_(std::move(s)){}
NativeFileEntry::~NativeFileEntry()=default;
FileEntryStatus NativeFileEntry::Create(std::shared_ptr<NativeProcessScheduler> s,
    std::shared_ptr<ProcessCallbackRegistry> registry,std::shared_ptr<NativeFileController> files,
    std::shared_ptr<NativeFileBase> bases,FileEntryHostServices host,std::shared_ptr<NativeFileEntry>& out) {
    using S=FileEntryStatus;if(!s || !s->root(2))return S::NullScheduler;
    if(!registry || !s->UsesCallbacks(registry.get()) || !files || !files->UsesScheduler(*s) ||
        !bases || !bases->UsesController(*files))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=s;state->files=std::move(files);
    state->bases=std::move(bases);state->host=std::move(host);
    auto module=std::shared_ptr<NativeFileEntry>(new NativeFileEntry(state));
    if(!registry->Register(Targets,module))return S::DuplicateBinding;
    out=std::move(module);return S::Ready;
}
FileEntryStatus NativeFileEntry::RegisterAllocator(std::uint32_t word,std::uint32_t release,ProcessAccess* a) {
    using S=FileEntryStatus;if(auto s=state_->Mutable(a);s!=S::Ready)return s;
    if(!word || !release)return S::InvalidAllocator;
    const auto at=state_->allocators.find(word);
    if(at!=state_->allocators.end())return at->second==release?S::Ready:S::ConflictingAllocator;
    state_->allocators.emplace(word,release);return S::Ready;
}
FileEntryStatus NativeFileEntry::PublishRawAllocator(std::uint32_t word,ProcessAccess* a) {
    using S=FileEntryStatus;if(auto s=state_->Mutable(a);s!=S::Ready)return s;
    if(word && !state_->allocators.contains(word))return S::InvalidAllocator;
    state_->raw_allocator=word;return S::Ready;
}
FileEntryStatus NativeFileEntry::Prepare(FileBaseHandle base,FileObjectHandle object,FilePathHandle path,
    std::uint32_t flags,std::uint32_t read_type,FileEntryHandle& out,ProcessAccess* a) {
    using S=FileEntryStatus;if(auto s=state_->Mutable(a);s!=S::Ready)return s;
    const auto scheduler=state_->scheduler.lock();const auto row=state_->files->Observe(object);
    if(!scheduler || !state_->bases->OpenCall(scheduler->root(2),base,path) || !row ||
        row->life!=FileObjectLife::Live || row->cache_linked || row->async_linked)return S::InvalidHandle;
    if(state_->serial==std::numeric_limits<std::uint32_t>::max())return S::IdentityExhausted;
    auto r=std::make_shared<State::Request>();r->identity=std::make_shared<FileEntryIdentity>(++state_->serial);
    r->base=std::move(base);r->object=std::move(object);r->path=std::move(path);r->flags=flags;r->read_type=read_type;
    state_->requests.emplace(r->identity->serial,r);out=r->identity;return S::Ready;
}
std::optional<ProcessCall> NativeFileEntry::EntryCall(ProcessHandle process,FileEntryHandle h) const {
    const auto r=h?state_->Get(h->serial):nullptr;
    if(!state_->Live() || !r || r->identity!=h || r->started)return {};
    // The five original arguments are carried by one typed request capability.
    return Service(std::move(process),Entry,{h->serial});
}
FileEntryStatus NativeFileEntry::PublishAsyncResult(FileObjectHandle object,
    const FileReadResult& result,ProcessAccess& access) {
    using S=FileEntryStatus;
    if(auto status=state_->Mutable(&access);status!=S::Ready)return status;
    auto row=state_->files->Observe(object);
    if(!row || row->life!=FileObjectLife::Live || !row->async_linked ||
        !(row->fields.flags&0x40000000u))return S::InvalidHandle;
    if(result.data && (!result.image || result.image->bytes().size()!=result.size))return S::InvalidHandle;
    if(!result.data && result.image)return S::InvalidHandle;
    auto fields=row->fields;fields.data=result.data;fields.size=result.size;
    if(state_->files->WriteFields(object,fields,&access)!=FileControllerStatus::Ready)return S::InvalidHandle;
    auto retained=std::make_shared<State::Read>();retained->object=object;retained->result=result;
    retained->revision=state_->files->Observe(object)->data_revision;
    if(state_->files->RetainDataResource(object,result.data,retained->revision,retained,&access)!=FileControllerStatus::Ready)
        return S::InvalidHandle;
    std::erase_if(state_->reads,[](const auto& entry){return entry.second.expired();});
    state_->reads.insert_or_assign(object->serial,retained);
    return S::Ready;
}
std::optional<FileReadResult> NativeFileEntry::ReadResult(FileObjectHandle object) const {
    const auto at=object?state_->reads.find(object->serial):state_->reads.end();
    if(!state_->Live() || at==state_->reads.end())return {};
    const auto r=at->second.lock();if(!r || r->object!=object)return {};
    const auto row=state_->files->Observe(object);
    if(!row || row->life!=FileObjectLife::Live || row->data_revision!=r->revision ||
        row->fields.data!=r->result.data || row->fields.size!=r->result.size)return {};
    return r->result;
}
std::unique_ptr<ProcessContinuation> NativeFileEntry::Begin(const ProcessCall& c) {
    if(!state_->Live() || c.kind!=ProcessCallKind::Service || c.has_self || c.this_adjustment ||
        !c.process || c.argument_count!=1 || std::find(Targets.begin(),Targets.end(),c.target)==Targets.end())return {};
    auto p=std::make_unique<Continuation>();p->state=state_;p->call=c;
    if(c.target==Entry) {
        p->request=state_->Get(c.arguments[0]);if(!p->request || p->request->started)return {};
        p->object=p->request->object;
        // The continuation now owns its request. Completed calls must not leave
        // every base/path/object retained in a diagnostic request history.
        state_->requests.erase(c.arguments[0]);
    } else {
        p->object=state_->files->ResolveObject(c.arguments[0]);auto row=state_->files->Observe(p->object);
        if(!row || row->life!=FileObjectLife::Live ||
            (c.target==RawAllocator && row->fields.get_allocator!=RawAllocator) ||
            (c.target==RawAlign && row->fields.get_align!=RawAlign))return {};
    }
    return p;
}
bool NativeFileEntry::UsesOwners(const NativeFileController& files,const NativeFileBase& bases) const noexcept {
    return state_->files.get()==&files && state_->bases.get()==&bases;
}
}
