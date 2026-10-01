#include "fates/presentation/native_frame_manager.hpp"
#include <algorithm>
#include <limits>
#include <map>
namespace fates::presentation::native {
using namespace runtime::native;
namespace {
constexpr std::uint32_t GlobalDelete=0x1b2288,DeleteTitleItems=0x1b2304,DeleteItems=0x1b22b0,
    LayoutDestructor=0x1b2ef8,ManagerDestructor=0x1b3008,Finalize=0x1b2fe4,
    IconTextDestructor=0x1b2218,ItemDestructor=0x1b2044;
constexpr std::array Targets{GlobalDelete,DeleteTitleItems,DeleteItems,LayoutDestructor,ManagerDestructor,Finalize,IconTextDestructor,ItemDestructor};
ProcessCall Service(ProcessHandle h,std::uint32_t target,std::initializer_list<std::uint32_t> args={}) {
    ProcessCall c;c.process=std::move(h);c.kind=ProcessCallKind::Service;c.target=target;
    c.argument_count=static_cast<std::uint8_t>(args.size());std::copy(args.begin(),args.end(),c.arguments.begin());return c;
}
bool Valid(const FrameLayoutState& l) {
    if(l.title && !l.title->object_word)return false;
    for(const auto& item:l.items)if(item && !item->object_word)return false;
    return true;
}
}
FrameLayoutState ConstructFrameLayoutState(std::uint32_t side,std::uint32_t dirty) noexcept {
    FrameLayoutState s;s.height=32;s.word_28=1;s.word_2c=1;
    s.byte_24=static_cast<std::uint8_t>(side);s.byte_25=static_cast<std::uint8_t>(dirty);
    s.byte_26=s.byte_24?1u:0u;return s;
}
struct NativeFrameManager::State {
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::optional<FrameManagerHandle> current;
    std::optional<ProcessHandle> process;
    std::map<std::uint32_t,std::shared_ptr<FrameManagerObservation>> managers;
    std::map<std::uint32_t,std::shared_ptr<FrameIconTextObservation>> texts;
    std::uint32_t serial{};
    bool Live() const {auto s=scheduler.lock();return s && s->root(2);}
    FrameManagerStatus Mutable(ProcessAccess* a) const {
        auto s=scheduler.lock();if(!s || !s->root(2))return FrameManagerStatus::Retired;
        if(a)return a->BelongsTo(*s)?FrameManagerStatus::Ready:FrameManagerStatus::MismatchedDomain;
        return s->busy()?FrameManagerStatus::Busy:FrameManagerStatus::Ready;
    }
    std::shared_ptr<FrameManagerObservation> Get(std::uint32_t id) const {auto it=managers.find(id);return it==managers.end()?nullptr:it->second;}
    std::shared_ptr<FrameManagerObservation> Get(FrameManagerHandle h) const {auto m=h?Get(h->serial):nullptr;return m && m->identity==h?m:nullptr;}
    std::shared_ptr<FrameManagerObservation> Current() const {return current && *current?Get(*current):nullptr;}
};
struct NativeFrameManager::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;ProcessCall call;
    std::shared_ptr<FrameManagerObservation> manager;
    std::shared_ptr<FrameIconTextObservation> text;
    unsigned stage{},layout{},slot{};
    std::optional<io::native::ResourceDeletionReference> item;
    ProcessCallbackStep Nested(std::uint32_t target,std::initializer_list<std::uint32_t> args) {
        return ProcessCallbackStep::Call(Service(call.process,target,args));
    }
    ProcessCallbackStep Step(ProcessAccess& access) override {
        auto s=state->scheduler.lock();if(!s || !access.BelongsTo(*s))return ProcessCallbackStep::Blocked();
        if(call.target==ItemDestructor) {
            if(!stage++){return Nested(0x2fdce4,{call.arguments[0]});}
            return ProcessCallbackStep::Return();
        }
        if(call.target==IconTextDestructor) {
            if(stage==0) {
                text->destructor_entered=true;
                if(text->text_storage_word) {stage=1;return Nested(0x2fe014,{text->text_storage_word});}
                stage=2;
            }
            if(stage==1) {text->text_storage_word=0;stage=2;}
            if(stage==2) {stage=3;return Nested(0x2fdce4,{text->object_word});}
            text->destroyed=true;return ProcessCallbackStep::Return();
        }
        if(call.target==Finalize) {
            if(stage==0) {
                if(!state->current)return ProcessCallbackStep::Blocked();
                manager=state->Current();if(!manager)return ProcessCallbackStep::Return();
                stage=1;return Nested(ManagerDestructor,{manager->identity->serial});
            }
            return ProcessCallbackStep::Return();
        }
        if(call.target==ManagerDestructor) {
            if(stage==0) {
                manager->life=FrameManagerLife::Destroying;
                if(!state->process)return ProcessCallbackStep::Blocked();
                if(*state->process) {
                    if(!access.Observe(*state->process))return ProcessCallbackStep::Blocked();
                    stage=1;return ProcessCallbackStep::Delete(*state->process);
                }
                stage=1;
            }
            if(stage==1) {
                // Even Dispose publishing a replacement does not change this
                // store: it clears CURRENT, then destroys the captured manager.
                state->current=FrameManagerHandle{};stage=2;
                return Nested(LayoutDestructor,{manager->identity->serial,1});
            }
            if(stage==2) {stage=3;return Nested(LayoutDestructor,{manager->identity->serial,0});}
            if(stage==3) {stage=4;return Nested(0x2fdce4,{manager->fields.storage_word});}
            manager->life=FrameManagerLife::Destroyed;return ProcessCallbackStep::Return();
        }
        if(call.target==GlobalDelete) {
            if(stage<2) {
                // Original re-reads the singleton before each layout. Null is
                // an invalid reached receiver, unlike Finalize's null guard.
                manager=state->Current();if(!manager || manager->life==FrameManagerLife::Destroyed)return ProcessCallbackStep::Blocked();
                const auto which=stage++;return Nested(DeleteTitleItems,{manager->identity->serial,which});
            }
            return ProcessCallbackStep::Return();
        }
        if(manager->life==FrameManagerLife::Destroyed)return ProcessCallbackStep::Blocked();
        auto& l=manager->fields.layouts[layout];
        if(call.target==LayoutDestructor) {
            if(stage==0) {l.destructor_entered=true;stage=1;return Nested(DeleteTitleItems,{manager->identity->serial,layout});}
            return ProcessCallbackStep::Return();
        }
        if(stage==0) {
            if(call.target==DeleteTitleItems && l.title) {item=l.title;stage=1;}
            else stage=3;
        }
        if(stage==1) {
            if(!item->deleting_destructor)return ProcessCallbackStep::Blocked();
            stage=2;return Nested(item->deleting_destructor,{item->object_word});
        }
        if(stage==2) {l.title.reset();stage=3;}
        if(stage==5) {
            // Unconditional stores and a fresh count load after the destructor.
            // A callback's replacement slot is cleared too, as in the original.
            l.items[slot].reset();--l.count;++slot;stage=3;
        }
        while(slot<5) {
            if(stage==3) {
                item=l.items[slot];if(!item){++slot;continue;}stage=4;
            }
            if(!item->deleting_destructor)return ProcessCallbackStep::Blocked();
            stage=5;return Nested(item->deleting_destructor,{item->object_word});
        }
        l.byte_25=1;return ProcessCallbackStep::Return();
    }
};
NativeFrameManager::NativeFrameManager(std::shared_ptr<State> s):state_(std::move(s)){}
NativeFrameManager::~NativeFrameManager()=default;
FrameManagerStatus NativeFrameManager::Create(std::shared_ptr<NativeProcessScheduler> s,
    std::shared_ptr<ProcessCallbackRegistry> registry,std::shared_ptr<NativeFrameManager>& out) {
    using S=FrameManagerStatus;if(!s || !s->root(2))return S::NullScheduler;
    if(!registry || !s->UsesCallbacks(registry.get()))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=s;
    auto module=std::shared_ptr<NativeFrameManager>(new NativeFrameManager(state));
    if(!registry->Register(Targets,module))return S::DuplicateBinding;out=std::move(module);return S::Ready;
}
FrameManagerStatus NativeFrameManager::Restore(const CarriedFrameManager& in,std::optional<ProcessHandle> process,
    FrameManagerHandle& out,ProcessAccess* access) {
    using S=FrameManagerStatus;if(auto s=state_->Mutable(access);s!=S::Ready)return s;
    if(!in.storage_word || !Valid(in.layouts[0]) || !Valid(in.layouts[1]))return S::InvalidState;
    for(const auto& [id,m]:state_->managers)if(m->fields.storage_word==in.storage_word && m->life!=FrameManagerLife::Destroyed)return S::InvalidState;
    if(process && *process && !state_->scheduler.lock()->Observe(*process))return S::InvalidHandle;
    if(state_->serial==std::numeric_limits<std::uint32_t>::max())return S::IdentityExhausted;
    auto m=std::make_shared<FrameManagerObservation>();m->identity=std::make_shared<FrameManagerIdentity>(++state_->serial);m->fields=in;
    state_->managers.emplace(m->identity->serial,m);state_->current=m->identity;state_->process=std::move(process);out=m->identity;return S::Ready;
}
FrameManagerStatus NativeFrameManager::PublishAbsent(ProcessAccess* a) {
    auto s=state_->Mutable(a);if(s==FrameManagerStatus::Ready)state_->current=FrameManagerHandle{};return s;
}
FrameManagerStatus NativeFrameManager::WriteLayout(FrameManagerHandle h,std::uint32_t index,const FrameLayoutState& l,ProcessAccess* a) {
    using S=FrameManagerStatus;if(auto s=state_->Mutable(a);s!=S::Ready)return s;
    auto m=state_->Get(h);if(!m || index>=2 || m->life==FrameManagerLife::Destroyed)return S::InvalidHandle;
    if(!Valid(l))return S::InvalidState;m->fields.layouts[index]=l;return S::Ready;
}
FrameManagerStatus NativeFrameManager::RestoreIconText(std::uint32_t word,std::uint32_t buffer,ProcessAccess* a) {
    using S=FrameManagerStatus;if(auto s=state_->Mutable(a);s!=S::Ready)return s;
    if(!word)return S::InvalidState;
    auto it=state_->texts.find(word);if(it!=state_->texts.end() && !it->second->destroyed)return S::InvalidState;
    state_->texts[word]=std::make_shared<FrameIconTextObservation>(FrameIconTextObservation{word,buffer,false,false});return S::Ready;
}
std::optional<FrameManagerHandle> NativeFrameManager::Current() const {return state_->Live()?state_->current:std::nullopt;}
std::optional<ProcessHandle> NativeFrameManager::CurrentProcess() const {return state_->Live()?state_->process:std::nullopt;}
std::optional<FrameManagerObservation> NativeFrameManager::Observe(FrameManagerHandle h) const {
    auto m=state_->Get(h);return state_->Live() && m?std::optional(*m):std::nullopt;
}
std::optional<FrameIconTextObservation> NativeFrameManager::ObserveIconText(std::uint32_t word) const {
    auto it=state_->texts.find(word);if(!state_->Live() || it==state_->texts.end())return {};return *it->second;
}
ProcessCall NativeFrameManager::DeleteCall(ProcessHandle h) {return Service(std::move(h),GlobalDelete);}
ProcessCall NativeFrameManager::FinalizeCall(ProcessHandle h) {return Service(std::move(h),Finalize);}
std::optional<ProcessCall> NativeFrameManager::DeleteLayoutCall(ProcessHandle h,FrameManagerHandle m,std::uint32_t index,bool title) const {
    if(!state_->Live() || !state_->Get(m) || index>=2)return {};
    return Service(std::move(h),title?DeleteTitleItems:DeleteItems,{m->serial,index});
}
std::optional<ProcessCall> NativeFrameManager::DestroyCall(ProcessHandle h,FrameManagerHandle m) const {
    if(!state_->Live() || !state_->Get(m))return {};return Service(std::move(h),ManagerDestructor,{m->serial});
}
std::unique_ptr<ProcessContinuation> NativeFrameManager::Begin(const ProcessCall& c) {
    if(!state_->Live() || c.kind!=ProcessCallKind::Service || !c.process || c.has_self || c.this_adjustment)return {};
    auto p=std::make_unique<Continuation>();p->state=state_;p->call=c;
    if(c.target==GlobalDelete || c.target==Finalize) {if(c.argument_count)return {};}
    else if(c.target==ManagerDestructor || c.target==LayoutDestructor || c.target==DeleteTitleItems || c.target==DeleteItems) {
        const auto n=c.target==ManagerDestructor?1u:2u;
        if(c.argument_count!=n || !(p->manager=state_->Get(c.arguments[0])) || p->manager->life==FrameManagerLife::Destroyed)return {};
        if(n==2) {if(c.arguments[1]>=2)return {};p->layout=c.arguments[1];}
    } else if(c.target==ItemDestructor || c.target==IconTextDestructor) {
        if(c.argument_count!=1 || !c.arguments[0])return {};
        if(c.target==IconTextDestructor) {
            auto it=state_->texts.find(c.arguments[0]);if(it==state_->texts.end() || it->second->destroyed || it->second->destructor_entered)return {};
            p->text=it->second;
        }
    } else return {};return p;
}
}
