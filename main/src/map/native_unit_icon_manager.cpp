#include "fates/map/native_unit_icon_manager.hpp"
#include "fates/runtime/native_identifier.hpp"
#include <algorithm>
#include <limits>
#include <list>
#include <map>
#include <set>
namespace fates::map::native {
using namespace runtime::native;
namespace {
constexpr std::uint32_t GlobalSweep=0x1e7d98,InstanceSweep=0x1f2310;
ProcessCall Service(ProcessHandle h,std::uint32_t target) {
    ProcessCall c;c.process=std::move(h);c.kind=ProcessCallKind::Service;c.target=target;return c;
}
bool Valid(const CarriedUnitIconCacheEntry& e) {return e.resource.object_word && e.name.find('\0')==std::string::npos;}
}
struct NativeUnitIconManager::State {
    struct Allocation {
        UnitIconManagerHandle identity;
        std::list<std::uint32_t> order;
        IdentifierRegistry<std::uint32_t> index{127};
    };
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::optional<std::shared_ptr<Allocation>> current;
    std::map<std::uint32_t,std::shared_ptr<Allocation>> allocations;
    std::map<std::uint32_t,std::shared_ptr<UnitIconCacheObservation>> entries;
    std::uint32_t manager_serial{},entry_serial{};
    bool Live() const {auto s=scheduler.lock();return s && s->root(2);}
    UnitIconManagerStatus Mutable(ProcessAccess* a) const {
        auto s=scheduler.lock();if(!s || !s->root(2))return UnitIconManagerStatus::Retired;
        if(a)return a->BelongsTo(*s)?UnitIconManagerStatus::Ready:UnitIconManagerStatus::MismatchedDomain;
        return s->busy()?UnitIconManagerStatus::Busy:UnitIconManagerStatus::Ready;
    }
    std::shared_ptr<Allocation> Get(std::uint32_t id) const {auto it=allocations.find(id);return it==allocations.end()?nullptr:it->second;}
    std::shared_ptr<Allocation> Get(UnitIconManagerHandle h) const {auto a=h?Get(h->serial):nullptr;return a && a->identity==h?a:nullptr;}
    std::shared_ptr<UnitIconCacheObservation> Entry(std::uint32_t id) const {auto it=entries.find(id);return it==entries.end()?nullptr:it->second;}
    std::shared_ptr<UnitIconCacheObservation> Get(UnitIconCacheHandle h) const {auto e=h?Entry(h->serial):nullptr;return e && e->identity==h?e:nullptr;}
    bool UsedWord(std::uint32_t word) const {
        for(const auto& [id,e]:entries)if(e->entry.resource.object_word==word && e->life!=UnitIconCacheLife::Destroyed)return true;
        return false;
    }
    std::shared_ptr<UnitIconCacheObservation> Append(Allocation& a,const CarriedUnitIconCacheEntry& in,bool index=true) {
        auto e=std::make_shared<UnitIconCacheObservation>();e->identity=std::make_shared<UnitIconCacheIdentity>(++entry_serial);
        e->allocation=a.identity;e->entry=in;entries.emplace(e->identity->serial,e);a.order.push_back(e->identity->serial);
        if(index)a.index.Append(in.name.c_str(),e->identity->serial);return e;
    }
};
struct NativeUnitIconManager::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;ProcessCall call;
    std::shared_ptr<State::Allocation> allocation;
    std::shared_ptr<UnitIconCacheObservation> entry;
    unsigned stage{};std::uint32_t cursor{},next{};
    ProcessCallbackStep Step(ProcessAccess& access) override {
        auto s=state->scheduler.lock();if(!s || !access.BelongsTo(*s))return ProcessCallbackStep::Blocked();
        if(stage==0) {
            if(call.target==GlobalSweep) {
                if(!state->current)return ProcessCallbackStep::Blocked();allocation=*state->current;
                if(!allocation)return ProcessCallbackStep::Return();
            }
            cursor=allocation->order.empty()?0:allocation->order.front();stage=1;
        }
        if(stage==3) {entry->life=UnitIconCacheLife::Destroyed;cursor=next;stage=1;}
        for(;;) {
            if(stage==2) {
                if(!entry->entry.resource.deleting_destructor)return ProcessCallbackStep::Blocked();
                auto nested=Service(call.process,entry->entry.resource.deleting_destructor);
                nested.argument_count=1;nested.arguments[0]=entry->entry.resource.object_word;
                stage=3;return ProcessCallbackStep::Call(std::move(nested));
            }
            if(!cursor)return ProcessCallbackStep::Return();
            entry=state->Entry(cursor);
            if(!entry || entry->life!=UnitIconCacheLife::Linked || entry->allocation!=allocation->identity)return ProcessCallbackStep::Blocked();
            auto it=std::find(allocation->order.begin(),allocation->order.end(),cursor);
            if(it==allocation->order.end())return ProcessCallbackStep::Blocked();
            ++it;next=it==allocation->order.end()?0:*it;
            if(entry->entry.references) {cursor=next;continue;}
            allocation->index.EraseFirst(entry->entry.name.c_str());allocation->order.remove(cursor);
            entry->life=UnitIconCacheLife::Destroying;stage=2;
        }
    }
};
NativeUnitIconManager::NativeUnitIconManager(std::shared_ptr<State> s):state_(std::move(s)){}
NativeUnitIconManager::~NativeUnitIconManager()=default;
UnitIconManagerStatus NativeUnitIconManager::Create(std::shared_ptr<NativeProcessScheduler> s,
    std::shared_ptr<ProcessCallbackRegistry> registry,std::shared_ptr<NativeUnitIconManager>& out) {
    using S=UnitIconManagerStatus;if(!s || !s->root(2))return S::NullScheduler;
    if(!registry || !s->UsesCallbacks(registry.get()))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=s;
    auto module=std::shared_ptr<NativeUnitIconManager>(new NativeUnitIconManager(state));
    constexpr std::array targets{GlobalSweep,InstanceSweep};if(!registry->Register(targets,module))return S::DuplicateBinding;
    out=std::move(module);return S::Ready;
}
UnitIconManagerStatus NativeUnitIconManager::Restore(const CarriedUnitIconCache& in,UnitIconManagerHandle& out,
    std::vector<UnitIconCacheHandle>& handles,ProcessAccess* access) {
    using S=UnitIconManagerStatus;if(auto s=state_->Mutable(access);s!=S::Ready)return s;
    if(in.order.size()!=in.index_order.size())return S::InvalidState;
    std::set<std::size_t> index;std::set<std::uint32_t> words;
    for(auto i:in.index_order)if(i>=in.order.size() || !index.insert(i).second)return S::InvalidState;
    for(const auto& e:in.order)if(!Valid(e) || !words.insert(e.resource.object_word).second || state_->UsedWord(e.resource.object_word))return S::InvalidState;
    if(state_->manager_serial==std::numeric_limits<std::uint32_t>::max() ||
        in.order.size()>std::numeric_limits<std::uint32_t>::max()-state_->entry_serial)return S::IdentityExhausted;
    auto a=std::make_shared<State::Allocation>();a->identity=std::make_shared<UnitIconManagerIdentity>(++state_->manager_serial);
    std::vector<UnitIconCacheHandle> result;for(const auto& e:in.order)result.push_back(state_->Append(*a,e,false)->identity);
    for(auto i:in.index_order)a->index.Append(in.order[i].name.c_str(),result[i]->serial);
    state_->allocations.emplace(a->identity->serial,a);state_->current=a;out=a->identity;handles=std::move(result);return S::Ready;
}
UnitIconManagerStatus NativeUnitIconManager::PublishAbsent(ProcessAccess* a) {
    auto s=state_->Mutable(a);if(s==UnitIconManagerStatus::Ready)state_->current=std::shared_ptr<State::Allocation>{};return s;
}
UnitIconManagerStatus NativeUnitIconManager::AppendCarried(UnitIconManagerHandle h,const CarriedUnitIconCacheEntry& in,
    UnitIconCacheHandle& out,ProcessAccess* access) {
    using S=UnitIconManagerStatus;if(auto s=state_->Mutable(access);s!=S::Ready)return s;
    auto a=state_->Get(h);if(!a)return S::InvalidHandle;
    if(!Valid(in) || state_->UsedWord(in.resource.object_word))return S::InvalidState;
    if(state_->entry_serial==std::numeric_limits<std::uint32_t>::max())return S::IdentityExhausted;
    out=state_->Append(*a,in)->identity;return S::Ready;
}
UnitIconManagerStatus NativeUnitIconManager::SetReferences(UnitIconCacheHandle h,std::uint16_t value,ProcessAccess* access) {
    using S=UnitIconManagerStatus;if(auto s=state_->Mutable(access);s!=S::Ready)return s;
    auto e=state_->Get(h);if(!e || e->life!=UnitIconCacheLife::Linked)return S::InvalidHandle;
    e->entry.references=value;return S::Ready;
}
std::optional<UnitIconManagerObservation> NativeUnitIconManager::Observe(UnitIconManagerHandle h) const {
    auto a=state_->Get(h);if(!state_->Live() || !a)return {};
    UnitIconManagerObservation out;out.present=true;out.identity=a->identity;
    for(auto id:a->order)out.order.push_back(state_->Entry(id)->identity);
    out.index_accounting_count=a->index.AccountingCount();out.index_size=a->index.Size();return out;
}
std::optional<UnitIconManagerObservation> NativeUnitIconManager::Observe() const {
    if(!state_->Live() || !state_->current)return {};
    return *state_->current?Observe((*state_->current)->identity):UnitIconManagerObservation{};
}
std::optional<UnitIconCacheObservation> NativeUnitIconManager::Observe(UnitIconCacheHandle h) const {
    auto e=state_->Get(h);return state_->Live() && e?std::optional(*e):std::nullopt;
}
UnitIconCacheHandle NativeUnitIconManager::Find(UnitIconManagerHandle h,std::string_view name) const {
    auto a=state_->Get(h);if(!state_->Live() || !a || name.find('\0')!=std::string_view::npos)return {};
    auto e=a->index.Find(std::string(name).c_str());return e?state_->Entry(e->value)->identity:nullptr;
}
ProcessCall NativeUnitIconManager::SweepCall(ProcessHandle h) {return Service(std::move(h),GlobalSweep);}
std::optional<ProcessCall> NativeUnitIconManager::SweepCall(ProcessHandle h,UnitIconManagerHandle a) const {
    if(!state_->Live() || !state_->Get(a))return {};auto c=Service(std::move(h),InstanceSweep);
    c.argument_count=1;c.arguments[0]=a->serial;return c;
}
std::unique_ptr<ProcessContinuation> NativeUnitIconManager::Begin(const ProcessCall& c) {
    if(!state_->Live() || c.kind!=ProcessCallKind::Service || !c.process || c.has_self || c.this_adjustment)return {};
    auto p=std::make_unique<Continuation>();p->state=state_;p->call=c;
    if(c.target==GlobalSweep) {if(c.argument_count)return {};}
    else if(c.target==InstanceSweep) {if(c.argument_count!=1 || !(p->allocation=state_->Get(c.arguments[0])))return {};}
    else return {};return p;
}
}
