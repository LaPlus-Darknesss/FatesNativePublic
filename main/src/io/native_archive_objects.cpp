#include "fates/io/native_archive_objects.hpp"
#include <map>

namespace fates::io::native {
using namespace runtime::native;
namespace {
constexpr std::uint32_t Setup=0x165ee0,Cleanup=0x165e30,Destructor=0x1c6558;
constexpr std::array Targets{Setup,Cleanup,Destructor};
struct FileImageLifetime final:ArchiveImageLifetime {
    std::weak_ptr<NativeFileController> files;
    FileObjectHandle object;
    std::uint32_t data{};
    std::uint64_t revision{};
    bool live() const noexcept override {
        auto f=files.lock();return f && f->OwnsLiveData(object,data,revision);
    }
};
ProcessCall Service(ProcessHandle h,std::uint32_t target,std::uint32_t object) {
    ProcessCall c;c.process=std::move(h);c.kind=ProcessCallKind::Service;c.target=target;
    c.arguments[0]=object;c.argument_count=1;return c;
}
}
struct NativeArchiveObjects::State {
    struct Association final:FileDataResource {
        FileObjectHandle object;
        std::shared_ptr<const NativeFileImage> image;
        std::shared_ptr<FileImageLifetime> lifetime;
        std::shared_ptr<const ArchiveRegistration> registration;
    };
    struct History {
        FileObjectHandle object;
        std::weak_ptr<Association> association;
        std::weak_ptr<const ArchiveRegistration> registration;
    };
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<NativeFileController> files;
    std::shared_ptr<NativeArchiveIdentifiers> identifiers;
    std::map<std::uint32_t,History> associations;
    bool Live() const {auto s=scheduler.lock();return s && s->root(2);}
    ArchiveObjectStatus Mutable(ProcessAccess* a) const {
        auto s=scheduler.lock();if(!s || !s->root(2))return ArchiveObjectStatus::Retired;
        if(a)return a->BelongsTo(*s)?ArchiveObjectStatus::Ready:ArchiveObjectStatus::MismatchedDomain;
        return s->busy()?ArchiveObjectStatus::Busy:ArchiveObjectStatus::Ready;
    }
    std::shared_ptr<Association> Get(FileObjectHandle o) const {
        if(!o)return {};
        auto at=associations.find(o->serial);
        return at!=associations.end() && at->second.object==o?at->second.association.lock():nullptr;
    }
};
struct NativeArchiveObjects::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;
    ProcessCall call;
    FileObjectHandle object;
    unsigned stage{};
    ProcessCallbackStep Step(ProcessAccess& a) override {
        auto s=state->scheduler.lock();if(!s || !a.BelongsTo(*s))return ProcessCallbackStep::Blocked();
        if(call.target==Destructor) {
            if(stage==0) {stage=1;return ProcessCallbackStep::Call(Service(call.process,0x17855c,object->serial));}
            if(stage==1) {stage=2;return ProcessCallbackStep::Call(Service(call.process,0x2fdce4,object->serial));}
            return ProcessCallbackStep::Return();
        }
        auto entry=state->Get(object);
        if(!entry || !entry->lifetime->live())return ProcessCallbackStep::Blocked();
        if(call.target==Setup) {
            if(state->identifiers->Construct(entry->image,entry->registration,entry->lifetime)!=ArchiveIdentifierStatus::Ready)
                return ProcessCallbackStep::Blocked();
            state->associations.at(object->serial).registration=entry->registration;
        } else {
            // An associated but never-constructed image is an explicit no-op;
            // an unknown image association above is never treated as that case.
            if(state->identifiers->DestructImage(entry->image,entry->lifetime)!=ArchiveIdentifierStatus::Ready)
                return ProcessCallbackStep::Blocked();
        }
        return ProcessCallbackStep::Return();
    }
};
NativeArchiveObjects::NativeArchiveObjects(std::shared_ptr<State> s):state_(std::move(s)){}
NativeArchiveObjects::~NativeArchiveObjects()=default;
ArchiveObjectStatus NativeArchiveObjects::Create(std::shared_ptr<NativeProcessScheduler> s,
    std::shared_ptr<ProcessCallbackRegistry> registry,std::shared_ptr<NativeFileController> files,
    std::shared_ptr<NativeArchiveIdentifiers> identifiers,std::shared_ptr<NativeArchiveObjects>& out) {
    using S=ArchiveObjectStatus;if(!s || !s->root(2))return S::NullScheduler;
    if(!registry || !s->UsesCallbacks(registry.get()) || !files || !files->UsesScheduler(*s) || !identifiers)
        return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=s;state->files=std::move(files);state->identifiers=std::move(identifiers);
    auto module=std::shared_ptr<NativeArchiveObjects>(new NativeArchiveObjects(state));
    if(!registry->Register(Targets,module))return S::DuplicateBinding;
    out=std::move(module);return S::Ready;
}
ArchiveObjectStatus NativeArchiveObjects::Associate(FileObjectHandle o,std::shared_ptr<const NativeFileImage> image,ProcessAccess* a) {
    using S=ArchiveObjectStatus;if(auto s=state_->Mutable(a);s!=S::Ready)return s;
    const auto row=state_->files->Observe(o);
    if(!row || row->life!=FileObjectLife::Live || row->fields.setup!=Setup || row->fields.cleanup!=Cleanup ||
        row->fields.deleting_destructor!=Destructor || !row->fields.data)return S::InvalidObject;
    if(!image || image->bytes().size()!=row->fields.size)return S::InvalidImage;
    ArchiveLabelTable labels;
    if(ReadArchiveLabelTable(image->bytes(),labels)!=ArchiveLabelTableStatus::Ready)return S::InvalidImage;
    if(const auto old=state_->Get(o))
        return old->image==image && old->lifetime->live()?S::Ready:S::ConflictingAssociation;
    auto entry=std::make_shared<State::Association>();entry->object=std::move(o);entry->image=std::move(image);
    entry->lifetime=std::make_shared<FileImageLifetime>();entry->lifetime->files=state_->files;
    entry->lifetime->object=entry->object;entry->lifetime->data=row->fields.data;entry->lifetime->revision=row->data_revision;
    if(state_->files->RetainDataResource(entry->object,row->fields.data,row->data_revision,entry,a)!=FileControllerStatus::Ready)
        return S::InvalidObject;
    std::erase_if(state_->associations,[](const auto& old){
        return old.second.association.expired() && old.second.registration.expired();
    });
    state_->associations.insert_or_assign(entry->object->serial,State::History{entry->object,entry,{}});return S::Ready;
}
std::shared_ptr<const ArchiveRegistration> NativeArchiveObjects::Registration(FileObjectHandle o) const {
    if(!state_->Live() || !o)return {};
    const auto at=state_->associations.find(o->serial);
    return at!=state_->associations.end() && at->second.object==o?at->second.registration.lock():nullptr;
}
std::unique_ptr<ProcessContinuation> NativeArchiveObjects::Begin(const ProcessCall& c) {
    if(!state_->Live() || c.kind!=ProcessCallKind::Service || c.has_self || c.this_adjustment ||
        !c.process || c.argument_count!=1 || (c.target!=Setup && c.target!=Cleanup && c.target!=Destructor))return {};
    auto o=state_->files->ResolveObject(c.arguments[0]);auto row=state_->files->Observe(o);
    if(!row || row->life!=(c.target==Destructor?FileObjectLife::Destroying:FileObjectLife::Live))return {};
    const auto& f=row->fields;
    if((c.target==Setup && f.setup!=Setup) || (c.target==Cleanup && f.cleanup!=Cleanup) ||
        (c.target==Destructor && f.deleting_destructor!=Destructor))return {};
    auto p=std::make_unique<Continuation>();p->state=state_;p->call=c;p->object=std::move(o);return p;
}
bool NativeArchiveObjects::UsesController(const NativeFileController& files) const noexcept {
    return state_->files.get()==&files;
}
}
