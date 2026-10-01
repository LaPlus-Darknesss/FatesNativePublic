#include "fates/runtime/native_map_binder.hpp"
#include "fates/engine/bind_manager.hpp"
#include <limits>
#include <map>

namespace fates::runtime::native {
namespace {constexpr std::array<std::uint32_t,2> Targets{0x39b648,0x39b658};}
struct NativeMapBinder::State {
    struct Row {MapBinderHandle identity;std::array<BindManager,2> counters;std::uint8_t state{};};
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::map<std::uint64_t,Row> instances;
    std::optional<MapBinderHandle> current;
    std::uint64_t serial{};
    bool Live() const {auto s=scheduler.lock();return s && s->root(2);}
    Row* Find(MapBinderHandle h) {
        if(!h)return nullptr;auto it=instances.find(h->serial);
        return it!=instances.end() && it->second.identity==h?&it->second:nullptr;
    }
    MapBinderStatus Mutable(ProcessAccess* access) const {
        auto s=scheduler.lock();if(!s || !s->root(2))return MapBinderStatus::Retired;
        if(access)return access->BelongsTo(*s)?MapBinderStatus::Ready:MapBinderStatus::MismatchedDomain;
        return s->busy()?MapBinderStatus::Busy:MapBinderStatus::Ready;
    }
    MapBinderStatus Initialize(ProcessAccess* access,MapBinderHandle& out) {
        if(auto s=Mutable(access);s!=MapBinderStatus::Ready)return s;
        if(serial==std::numeric_limits<std::uint64_t>::max())return MapBinderStatus::IdentityExhausted;
        Row row;row.identity=std::make_shared<const MapBinderIdentity>(MapBinderIdentity{++serial});
        out=row.identity;instances.emplace(serial,std::move(row));current=out;return MapBinderStatus::Ready;
    }
    MapBinderStatus Finalize(ProcessAccess* access) {
        if(auto s=Mutable(access);s!=MapBinderStatus::Ready)return s;
        if(!current)return MapBinderStatus::UnknownCurrent;
        if(*current)instances.erase((*current)->serial);
        current=MapBinderHandle{};return MapBinderStatus::Ready;
    }
    MapBinderStatus Change(ProcessAccess* access,MapBinderHandle h,bool bind,bool& result) {
        if(auto s=Mutable(access);s!=MapBinderStatus::Ready)return s;
        auto* row=Find(std::move(h));if(!row)return MapBinderStatus::InvalidHandle;
        result=bind?row->counters[0].Bind():row->counters[0].Unbind();return MapBinderStatus::Ready;
    }
};
struct NativeMapBinder::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;bool bind{};
    ProcessCallbackStep Step(ProcessAccess& access) override {
        if(!state->current || !*state->current)return ProcessCallbackStep::Blocked();
        bool result{};
        if(state->Change(&access,*state->current,bind,result)!=MapBinderStatus::Ready)return ProcessCallbackStep::Blocked();
        return ProcessCallbackStep::Return(result?1u:0u);
    }
};
NativeMapBinder::NativeMapBinder(std::shared_ptr<State> s):state_(std::move(s)){}
NativeMapBinder::~NativeMapBinder()=default;
MapBinderStatus NativeMapBinder::Create(std::shared_ptr<NativeProcessScheduler> scheduler,
    std::shared_ptr<ProcessCallbackRegistry> registry,std::shared_ptr<NativeMapBinder>& out) {
    if(!scheduler || !scheduler->root(2))return MapBinderStatus::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()))return MapBinderStatus::MismatchedDomain;
    auto s=std::make_shared<State>();s->scheduler=scheduler;
    auto next=std::shared_ptr<NativeMapBinder>(new NativeMapBinder(std::move(s)));
    if(!registry->Register(Targets,next))return MapBinderStatus::DuplicateBinding;
    out=std::move(next);return MapBinderStatus::Ready;
}
MapBinderStatus NativeMapBinder::Initialize(MapBinderHandle& out) {return state_->Initialize(nullptr,out);}
MapBinderStatus NativeMapBinder::Initialize(ProcessAccess& a,MapBinderHandle& out) {return state_->Initialize(&a,out);}
MapBinderStatus NativeMapBinder::Finalize() {return state_->Finalize(nullptr);}
MapBinderStatus NativeMapBinder::Finalize(ProcessAccess& a) {return state_->Finalize(&a);}
MapBinderStatus NativeMapBinder::Bind(MapBinderHandle h,bool& result) {return state_->Change(nullptr,std::move(h),true,result);}
MapBinderStatus NativeMapBinder::Bind(ProcessAccess& a,MapBinderHandle h,bool& result) {return state_->Change(&a,std::move(h),true,result);}
MapBinderStatus NativeMapBinder::Unbind(MapBinderHandle h,bool& result) {return state_->Change(nullptr,std::move(h),false,result);}
MapBinderStatus NativeMapBinder::Unbind(ProcessAccess& a,MapBinderHandle h,bool& result) {return state_->Change(&a,std::move(h),false,result);}
bool NativeMapBinder::UsesScheduler(const NativeProcessScheduler& s) const noexcept {return state_->scheduler.lock().get()==&s;}
std::optional<MapBinderHandle> NativeMapBinder::Current() const {return state_->Live()?state_->current:std::optional<MapBinderHandle>{};}
std::optional<MapBinderSnapshot> NativeMapBinder::Observe(MapBinderHandle h) const {
    if(state_->Live())if(auto* row=state_->Find(std::move(h)))
        return MapBinderSnapshot{row->identity,{row->counters[0].GetCount(),row->counters[1].GetCount()},row->state};
    return {};
}
std::unique_ptr<ProcessContinuation> NativeMapBinder::Begin(const ProcessCall& c) {
    const bool service=c.kind==ProcessCallKind::Service && !c.has_self;
    const bool descriptor=c.kind==ProcessCallKind::Descriptor && c.command==8 && c.has_self;
    if(!state_->Live() || !c.process || c.this_adjustment || c.argument_count || (!service && !descriptor) ||
        (c.target!=Targets[0] && c.target!=Targets[1]))return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->bind=c.target==Targets[0];return next;
}
}
