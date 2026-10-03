#include "fates/io/native_global_files.hpp"
#include "fates/runtime/native_identifier.hpp"
#include <algorithm>
#include <limits>
#include <list>
#include <map>

namespace fates::io::native {
using namespace runtime::native;
namespace {
constexpr std::uint32_t ArchiveLoad=0x17bed0,ArchiveFree=0x17bdfc,FileLoad=0x17c12c,FileFree=0x17c058;
constexpr std::array Targets{ArchiveLoad,ArchiveFree,FileLoad,FileFree};
bool Valid(GlobalFileMode mode) {return mode==GlobalFileMode::Plain || mode==GlobalFileMode::Archive;}
ProcessCall Service(ProcessHandle h,std::uint32_t target,std::initializer_list<std::uint32_t> args) {
    ProcessCall c;c.process=std::move(h);c.kind=ProcessCallKind::Service;c.target=target;
    c.argument_count=static_cast<std::uint8_t>(args.size());std::copy(args.begin(),args.end(),c.arguments.begin());return c;
}
FileObjectMethods Methods(GlobalFileMode mode) {
    const bool archive=mode==GlobalFileMode::Archive;
    return {archive?0x1c6558u:0x1caa90u,archive?0x165ee0u:0x1caa88u,
        archive?0x165e30u:0x1caa8cu,0x50a51c,0x50a534,0x50a52c};
}
}
struct NativeGlobalFiles::State {
    struct Registry {
        std::list<std::uint32_t> entries;
        IdentifierRegistry<std::uint32_t> index{127};
        std::uint32_t count{};
    };
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<NativeFileController> files;
    std::shared_ptr<NativeFileBase> bases;
    std::shared_ptr<NativeFileEntry> cold;
    std::shared_ptr<NativeArchiveObjects> archives;
    // Initialize constructs a third registry too; it has no reached operations.
    std::array<Registry,3> registries;
    std::map<std::uint32_t,std::shared_ptr<GlobalFileEntryObservation>> entries;
    std::uint32_t serial{};
    bool Live() const {auto s=scheduler.lock();return s && s->root(2);}
    GlobalFileStatus Mutable(ProcessAccess* a) const {
        auto s=scheduler.lock();if(!s || !s->root(2))return GlobalFileStatus::Retired;
        if(a)return a->BelongsTo(*s)?GlobalFileStatus::Ready:GlobalFileStatus::MismatchedDomain;
        return s->busy()?GlobalFileStatus::Busy:GlobalFileStatus::Ready;
    }
    Registry& Get(GlobalFileMode m) {return registries[static_cast<std::size_t>(m)];}
    const Registry& Get(GlobalFileMode m) const {return registries[static_cast<std::size_t>(m)];}
    std::shared_ptr<GlobalFileEntryObservation> Get(GlobalFileEntryHandle h) const {
        if(!h)return {};const auto at=entries.find(h->serial);
        return at!=entries.end() && at->second->identity==h?at->second:nullptr;
    }
    std::shared_ptr<GlobalFileEntryObservation> Find(GlobalFileMode m,std::string_view path) const {
        const auto node=Get(m).index.Find(std::string(path).c_str());
        return node?entries.at(node->value):nullptr;
    }
};
struct NativeGlobalFiles::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;ProcessCall call;GlobalFileMode mode{};bool load{};
    FilePathHandle path;std::shared_ptr<GlobalFileEntryObservation> entry;
    FileObjectHandle object;FileEntryHandle request;
    std::uint32_t type{};unsigned stage{};
    ProcessCallbackStep Invoke(std::uint32_t target,std::initializer_list<std::uint32_t> args) {
        return ProcessCallbackStep::Call(Service(call.process,target,args));
    }
    ProcessCallbackStep Step(ProcessAccess& a) override {
        auto scheduler=state->scheduler.lock();
        if(!scheduler || !a.BelongsTo(*scheduler))return ProcessCallbackStep::Blocked();
        auto& registry=state->Get(mode);
        if(stage==0) {
            entry=state->Find(mode,path->text);
            // A failed read or truncated lower name can leave the original
            // upper hash pointing at released storage. Keep that membership,
            // but refuse to read/update a dead entry rather than emulate UAF.
            if(entry && !entry->live)return ProcessCallbackStep::Blocked();
            if(load)stage=entry?8u:1u;
            else {
                if(!entry)return ProcessCallbackStep::Return();
                --entry->references;
                if(entry->references)return ProcessCallbackStep::Return();
                stage=10;return Invoke(0x545da0,{entry->base->serial});
            }
        }
        if(!load) {
            if(stage==10) {
                const auto actual=state->bases->ResolvePath(a.call_result());
                if(!actual)return ProcessCallbackStep::Blocked();
                registry.index.EraseFirst(actual->text.c_str());
                registry.entries.remove(entry->identity->serial);--registry.count;entry->linked=false;
                stage=11;return Invoke(0x11cf84,{entry->base->serial});
            }
            if(state->bases->RetireEmpty(entry->base,&a)!=FileBaseStatus::Ready)return ProcessCallbackStep::Blocked();
            entry->live=false;return ProcessCallbackStep::Return();
        }
        if(stage==1) {
            if(state->serial==std::numeric_limits<std::uint32_t>::max())return ProcessCallbackStep::Blocked();
            auto next=std::make_shared<GlobalFileEntryObservation>();
            if(state->bases->RestoreAttachment({},next->base,&a)!=FileBaseStatus::Ready)return ProcessCallbackStep::Blocked();
            next->identity=std::make_shared<GlobalFileEntryIdentity>(++state->serial);
            next->mode=mode;next->allocation_word=call.arguments[1];next->live=true;
            state->entries.emplace(next->identity->serial,next);entry=std::move(next);
            stage=2;return Invoke(0x112ca8,{entry->base->serial,path->serial,0});
        }
        if(stage==2) {type=a.call_result();stage=type?5u:3u;}
        if(stage==3) {
            if(state->files->ConstructObject(Methods(mode),object,&a)!=FileControllerStatus::Ready)
                return ProcessCallbackStep::Blocked();
            stage=4;
        }
        if(stage==4) {
            if(!request && state->cold->Prepare(entry->base,object,path,call.arguments[1],0,request,&a)!=FileEntryStatus::Ready)
                return ProcessCallbackStep::Blocked();
            const auto c=state->cold->EntryCall(call.process,request);if(!c)return ProcessCallbackStep::Blocked();
            stage=5;return ProcessCallbackStep::Call(*c);
        }
        if(stage==5) {
            const auto base=state->bases->Observe(entry->base);if(!base)return ProcessCallbackStep::Blocked();
            if(base->object) {
                const auto row=state->files->Observe(base->object);
                if(!row || row->life!=FileObjectLife::Live)return ProcessCallbackStep::Blocked();
                // Supply decoded-byte identity to the actual subtype. A raw
                // warm hit stays raw even for ArchiveLoad. Setup still happens
                // only through Close, with its original refcount/flag order.
                if(row->fields.setup==0x165ee0 && row->fields.data) {
                    const auto read=state->cold->ReadResult(base->object);
                    if(read && (!read->image || state->archives->Associate(base->object,read->image,&a)!=ArchiveObjectStatus::Ready))
                        return ProcessCallbackStep::Blocked();
                }
            }
            stage=6;return Invoke(0x112d28,{entry->base->serial,type,0});
        }
        if(stage==6) {
            // Original ignores Close's boolean, even when it freed failed data.
            registry.entries.push_back(entry->identity->serial);++registry.count;entry->linked=true;
            registry.index.Append(path->text.c_str(),entry->identity->serial);stage=8;
        }
        if(stage==8) {
            ++entry->references;stage=9;return Invoke(0x10987c,{entry->base->serial});
        }
        return ProcessCallbackStep::Return(a.call_result());
    }
};
NativeGlobalFiles::NativeGlobalFiles(std::shared_ptr<State> s):state_(std::move(s)){}
NativeGlobalFiles::~NativeGlobalFiles()=default;
GlobalFileStatus NativeGlobalFiles::Create(std::shared_ptr<NativeProcessScheduler> scheduler,
    std::shared_ptr<ProcessCallbackRegistry> callbacks,std::shared_ptr<NativeFileController> files,
    std::shared_ptr<NativeFileBase> bases,std::shared_ptr<NativeFileEntry> cold,std::shared_ptr<NativeArchiveObjects> archives,
    std::shared_ptr<NativeGlobalFiles>& out) {
    using S=GlobalFileStatus;if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!callbacks || !scheduler->UsesCallbacks(callbacks.get()) || !files || !files->UsesScheduler(*scheduler) ||
        !bases || !bases->UsesController(*files) || !cold || !cold->UsesOwners(*files,*bases) ||
        !archives || !archives->UsesController(*files))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->files=std::move(files);
    state->bases=std::move(bases);state->cold=std::move(cold);state->archives=std::move(archives);
    auto module=std::shared_ptr<NativeGlobalFiles>(new NativeGlobalFiles(state));
    if(!callbacks->Register(Targets,module))return S::DuplicateBinding;
    out=std::move(module);return S::Ready;
}
GlobalFileFindResult NativeGlobalFiles::Find(GlobalFileMode mode,std::string_view path) const {
    using S=GlobalFileStatus;if(!state_->Live())return {S::Retired,{}};
    if(!Valid(mode) || path.size()>65535 || path.find('\0')!=std::string_view::npos)return {S::InvalidPath,{}};
    const auto entry=state_->Find(mode,path);
    return entry?GlobalFileFindResult{entry->live?S::Ready:S::StaleValue,entry->identity}:GlobalFileFindResult{};
}
std::optional<GlobalFileEntryObservation> NativeGlobalFiles::Observe(GlobalFileEntryHandle h) const {
    const auto entry=state_->Get(h);return state_->Live() && entry?std::optional(*entry):std::nullopt;
}
std::optional<GlobalFileRegistryObservation> NativeGlobalFiles::Observe(GlobalFileMode mode) const {
    if(!state_->Live() || !Valid(mode))return {};
    const auto& r=state_->Get(mode);GlobalFileRegistryObservation out;
    out.count=r.count;out.index_size=r.index.Size();out.index_accounting_count=r.index.AccountingCount();
    for(auto id:r.entries)out.entries.push_back(state_->entries.at(id)->identity);return out;
}
GlobalFileStatus NativeGlobalFiles::WriteReferences(GlobalFileEntryHandle h,std::uint32_t refs,ProcessAccess* a) {
    using S=GlobalFileStatus;if(auto s=state_->Mutable(a);s!=S::Ready)return s;
    const auto entry=state_->Get(h);if(!entry || !entry->live)return S::InvalidHandle;
    entry->references=refs;return S::Ready;
}
std::optional<ProcessCall> NativeGlobalFiles::LoadCall(ProcessHandle process,GlobalFileMode mode,
    FilePathHandle path,std::uint32_t word) const {
    if(!state_->Live() || !Valid(mode) || !path || path->text.size()>65535 || state_->bases->ResolvePath(path->serial)!=path)return {};
    return Service(std::move(process),mode==GlobalFileMode::Archive?ArchiveLoad:FileLoad,{path->serial,word});
}
std::optional<ProcessCall> NativeGlobalFiles::FreeCall(ProcessHandle process,GlobalFileMode mode,FilePathHandle path) const {
    if(!state_->Live() || !Valid(mode) || !path || path->text.size()>65535 || state_->bases->ResolvePath(path->serial)!=path)return {};
    return Service(std::move(process),mode==GlobalFileMode::Archive?ArchiveFree:FileFree,{path->serial});
}
std::unique_ptr<ProcessContinuation> NativeGlobalFiles::Begin(const ProcessCall& c) {
    if(!state_->Live() || c.kind!=ProcessCallKind::Service || c.has_self || c.this_adjustment || !c.process ||
        std::find(Targets.begin(),Targets.end(),c.target)==Targets.end())return {};
    const bool load=c.target==ArchiveLoad || c.target==FileLoad;
    if(c.argument_count!=(load?2u:1u))return {};
    const auto path=state_->bases->ResolvePath(c.arguments[0]);if(!path || path->text.size()>65535)return {};
    auto p=std::make_unique<Continuation>();p->state=state_;p->call=c;p->path=path;p->load=load;
    p->mode=(c.target==ArchiveLoad || c.target==ArchiveFree)?GlobalFileMode::Archive:GlobalFileMode::Plain;return p;
}
bool NativeGlobalFiles::UsesOwners(const NativeProcessScheduler& scheduler,const NativeFileBase& bases) const noexcept {
    return state_->scheduler.lock().get()==&scheduler && state_->bases.get()==&bases;
}
}
