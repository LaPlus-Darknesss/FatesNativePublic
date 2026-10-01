#include "fates/runtime/native_resource_delay.hpp"
#include "fates/io/delay_queue.hpp"
#include <limits>
#include <map>

namespace fates::runtime::native {
namespace {
constexpr std::uint32_t InitializeTarget=0x104b20,TickTarget=0x104c20,EntryTarget=0x11f960,
    DumpTarget=0x1adab8,ActiveTarget=0x1adae4,BindTarget=0x3d2940,WaitTick=0x1eeea4,WaitDestroy=0x1eef14;
constexpr std::array Targets{InitializeTarget,TickTarget,EntryTarget,DumpTarget,ActiveTarget,BindTarget,WaitTick,WaitDestroy};
constexpr std::array<std::uint32_t,3> Cleanup{0x1e7d98,0x1b2288,0x3cf924};
ProcessCall Service(ProcessHandle context,std::uint32_t target) {
    ProcessCall c;c.process=std::move(context);c.kind=ProcessCallKind::Service;c.target=target;return c;
}
using Queue=fates::io::DelayQueue<DelayedResourceCall>;
}
struct NativeResourceDelay::State {
    struct Storage {std::uint64_t identity{};Queue queue;bool ticking{};};
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::vector<std::shared_ptr<Storage>> allocations;
    std::optional<std::shared_ptr<Storage>> current;
    std::map<std::uint64_t,ResourceWaitSnapshot> waits;
    std::uint64_t serial{};
    bool Live() const {auto s=scheduler.lock();return s && s->root(2);}
    ResourceDelayStatus Mutable(ProcessAccess* a) const {
        auto s=scheduler.lock();if(!s || !s->root(2))return ResourceDelayStatus::Retired;
        if(a)return a->BelongsTo(*s)?ResourceDelayStatus::Ready:ResourceDelayStatus::MismatchedDomain;
        return s->busy()?ResourceDelayStatus::Busy:ResourceDelayStatus::Ready;
    }
    ResourceDelayStatus Initialize(ProcessAccess* a) {
        if(auto s=Mutable(a);s!=ResourceDelayStatus::Ready)return s;
        if(serial==std::numeric_limits<std::uint64_t>::max())return ResourceDelayStatus::IdentityExhausted;
        auto next=std::make_shared<Storage>();next->identity=++serial;allocations.push_back(next);current=std::move(next);
        return ResourceDelayStatus::Ready;
    }
    ResourceDelayStatus Absent(ProcessAccess* a) {
        if(auto s=Mutable(a);s!=ResourceDelayStatus::Ready)return s;
        current=std::shared_ptr<Storage>{};return ResourceDelayStatus::Ready;
    }
    std::optional<bool> Active() const {
        if(!current)return {};return *current && (*current)->queue.size()>0;
    }
};
struct NativeResourceDelay::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;ProcessCall call;
    std::shared_ptr<State::Storage> storage;
    Queue::Index node{Queue::None},next{Queue::None};
    unsigned stage{};std::size_t cleanup_index{};bool ticking{};
    ~Continuation() override {if(ticking)storage->ticking=false;}
    void Advance() {node=next;if(node!=Queue::None)next=storage->queue.Next(node);}
    ProcessCallbackStep Step(ProcessAccess& access) override {
        auto scheduler=state->scheduler.lock();if(!scheduler || !access.BelongsTo(*scheduler))return ProcessCallbackStep::Blocked();
        if(call.target==InitializeTarget)return state->Initialize(&access)==ResourceDelayStatus::Ready?ProcessCallbackStep::Return():ProcessCallbackStep::Blocked();
        if(call.target==TickTarget) {
            if(stage==0) {
                if(!state->current)return ProcessCallbackStep::Blocked();storage=*state->current;
                if(!storage)return ProcessCallbackStep::Return();
                // Same-queue recursive Tick is not admitted. Replacing the
                // current queue during a callback retains the old traversal.
                if(storage->ticking)return ProcessCallbackStep::Blocked();
                ticking=storage->ticking=true;node=storage->queue.Head();
                if(node!=Queue::None)next=storage->queue.Next(node);stage=1;
            }
            if(stage==2) {storage->queue.Recycle(node);Advance();stage=1;}
            while(node!=Queue::None) {
                if(storage->queue.Ticks(node)>0) {storage->queue.WaitOne(node);Advance();continue;}
                const auto& value=storage->queue.Get(node);auto nested=Service(call.process,value.target);
                nested.argument_count=value.argument_count;
                for(std::size_t i=0;i<value.argument_count;++i)nested.arguments[i]=value.arguments[i];
                stage=2;return ProcessCallbackStep::Call(std::move(nested));
            }
            ticking=storage->ticking=false;return ProcessCallbackStep::Return();
        }
        if(call.target==EntryTarget) {
            if(stage)return ProcessCallbackStep::Return();
            if(!state->current)return ProcessCallbackStep::Blocked();
            DelayedResourceCall value{call.arguments[0],{call.arguments[2],call.arguments[3]},static_cast<std::uint8_t>(call.arguments[1])};
            if(*state->current && (*state->current)->queue.Push(value))return ProcessCallbackStep::Return();
            auto nested=Service(call.process,value.target);nested.argument_count=value.argument_count;
            for(std::size_t i=0;i<value.argument_count;++i)nested.arguments[i]=value.arguments[i];
            stage=1;return ProcessCallbackStep::Call(std::move(nested));
        }
        if(call.target==ActiveTarget || call.target==DumpTarget) {
            auto active=state->Active();if(!active)return ProcessCallbackStep::Blocked();
            if(call.target==DumpTarget) {
                if(*state->current)for(auto i=(*state->current)->queue.Head();i!=Queue::None;i=(*state->current)->queue.Next(i)){}
                return ProcessCallbackStep::Return();
            }
            return ProcessCallbackStep::Return(*active?1u:0u);
        }
        if(call.target==WaitTick) {
            auto it=state->waits.find(call.process->serial);if(it==state->waits.end())return ProcessCallbackStep::Blocked();
            if(stage==0) {
                auto active=state->Active();if(!active)return ProcessCallbackStep::Blocked();
                if(*active) {
                    ++it->second.ticks;if(it->second.ticks!=60)return ProcessCallbackStep::Return();
                    stage=1;return ProcessCallbackStep::Call(NativeResourceDelay::DumpCall(call.process));
                }
            }
            if(access.Next(call.process,true)!=ProcessStatus::Ready)return ProcessCallbackStep::Blocked();
            return ProcessCallbackStep::Return();
        }
        if(call.target==BindTarget && stage==0) {
            auto active=state->Active();if(!active)return ProcessCallbackStep::Blocked();
            if(*active) {
                auto type=ProcessType::Base();type.methods[0].target=WaitDestroy;type.methods[1].target=WaitTick;
                ProcessHandle h;
                if(access.Create(call.process,ProcessProgram::Default(),std::string("ProcResDelayBind"),true,type,h)!=ProcessStatus::Ready)return ProcessCallbackStep::Blocked();
                state->waits.emplace(h->serial,ResourceWaitSnapshot{h,0,static_cast<std::uint8_t>(call.arguments[0])});
                return ProcessCallbackStep::Return();
            }
            if(!call.arguments[0])return ProcessCallbackStep::Return();stage=1;
        }
        if(call.target==WaitDestroy) {
            auto it=state->waits.find(call.process->serial);if(it==state->waits.end())return ProcessCallbackStep::Blocked();
            if(!it->second.cleanup)cleanup_index=Cleanup.size();
        }
        if(cleanup_index<Cleanup.size()) {
            // Global services have no receiver. Root2 stays live while the
            // swept child is unlinked but retained in real destruction.
            return ProcessCallbackStep::Call(Service(scheduler->root(2),Cleanup[cleanup_index++]));
        }
        if(call.target==WaitDestroy)state->waits.erase(call.process->serial);
        return ProcessCallbackStep::Return();
    }
};
NativeResourceDelay::NativeResourceDelay(std::shared_ptr<State> s):state_(std::move(s)){}
NativeResourceDelay::~NativeResourceDelay()=default;
ResourceDelayStatus NativeResourceDelay::Create(std::shared_ptr<NativeProcessScheduler> scheduler,
    std::shared_ptr<ProcessCallbackRegistry> registry,std::shared_ptr<NativeResourceDelay>& out) {
    if(!scheduler || !scheduler->root(2))return ResourceDelayStatus::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()))return ResourceDelayStatus::MismatchedDomain;
    auto s=std::make_shared<State>();s->scheduler=scheduler;
    auto next=std::shared_ptr<NativeResourceDelay>(new NativeResourceDelay(s));
    if(!registry->Register(Targets,next))return ResourceDelayStatus::DuplicateBinding;
    out=std::move(next);return ResourceDelayStatus::Ready;
}
ResourceDelayStatus NativeResourceDelay::Initialize() {return state_->Initialize(nullptr);}
ResourceDelayStatus NativeResourceDelay::Initialize(ProcessAccess& a) {return state_->Initialize(&a);}
ResourceDelayStatus NativeResourceDelay::PublishAbsent() {return state_->Absent(nullptr);}
ResourceDelayStatus NativeResourceDelay::PublishAbsent(ProcessAccess& a) {return state_->Absent(&a);}
bool NativeResourceDelay::UsesScheduler(const NativeProcessScheduler& s) const noexcept {return state_->scheduler.lock().get()==&s;}
std::optional<ResourceDelaySnapshot> NativeResourceDelay::Observe() const {
    if(!state_->Live() || !state_->current)return {};ResourceDelaySnapshot out;
    if(*state_->current) {
        const auto& s=**state_->current;out.present=true;out.identity=s.identity;out.free_slots=s.queue.free_size();
        for(auto i=s.queue.Head();i!=Queue::None;i=s.queue.Next(i))out.entries.push_back({i,s.queue.Ticks(i),s.queue.Get(i)});
    }
    return out;
}
std::vector<ResourceWaitSnapshot> NativeResourceDelay::Waits() const {
    std::vector<ResourceWaitSnapshot> out;if(state_->Live())for(const auto& [id,row]:state_->waits)out.push_back(row);return out;
}
std::optional<ProcessCall> NativeResourceDelay::EntryCall(ProcessHandle h,const DelayedResourceCall& value) {
    if(!value.target || value.argument_count>2)return {};auto c=Service(std::move(h),EntryTarget);
    c.arguments={value.target,value.argument_count,value.arguments[0],value.arguments[1]};c.argument_count=4;return c;
}
ProcessCall NativeResourceDelay::InitializeCall(ProcessHandle h) {return Service(std::move(h),InitializeTarget);}
ProcessCall NativeResourceDelay::TickCall(ProcessHandle h) {return Service(std::move(h),TickTarget);}
ProcessCall NativeResourceDelay::IsActiveCall(ProcessHandle h) {return Service(std::move(h),ActiveTarget);}
ProcessCall NativeResourceDelay::DumpCall(ProcessHandle h) {return Service(std::move(h),DumpTarget);}
ProcessCall NativeResourceDelay::BindCall(ProcessHandle h,std::uint32_t cleanup) {auto c=Service(std::move(h),BindTarget);c.arguments[0]=cleanup;c.argument_count=1;return c;}
std::unique_ptr<ProcessContinuation> NativeResourceDelay::Begin(const ProcessCall& c) {
    if(!state_->Live() || !c.process || c.this_adjustment)return {};
    if(c.target==WaitTick || c.target==WaitDestroy) {
        auto it=state_->waits.find(c.process->serial);
        if(it==state_->waits.end() || it->second.process!=c.process || !c.has_self || c.argument_count)return {};
        if(c.target==WaitTick && (c.kind!=ProcessCallKind::Descriptor || c.command!=13))return {};
        if(c.target==WaitDestroy && c.kind!=ProcessCallKind::Destroy)return {};
    } else if(c.target==BindTarget) {
        if(c.argument_count!=1 || (c.kind!=ProcessCallKind::Service && (c.kind!=ProcessCallKind::Descriptor || c.command!=15 || !c.has_self)))return {};
        if(c.kind==ProcessCallKind::Service && c.has_self)return {};
    } else {
        if(c.kind!=ProcessCallKind::Service || c.has_self)return {};
        if(c.target==EntryTarget) {if(c.argument_count!=4 || !c.arguments[0] || c.arguments[1]>2)return {};}
        else if(c.argument_count || (c.target!=InitializeTarget && c.target!=TickTarget && c.target!=ActiveTarget && c.target!=DumpTarget))return {};
    }
    auto next=std::make_unique<Continuation>();next->state=state_;next->call=c;return next;
}
}
