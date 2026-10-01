#include "fates/io/native_file_controller.hpp"
#include "fates/io/file_cache_policies.hpp"
#include "fates/runtime/native_identifier.hpp"
#include <algorithm>
#include <bit>
#include <limits>
#include <list>
#include <map>
#include <set>

namespace fates::io::native {
using namespace runtime::native;
namespace {
constexpr std::uint32_t Sweep=0x3cf924,ControllerSweep=0x11fbcc,Delete=0x11fa44,
    Delayed=0x11fb34,RemoveAsync=0x12a168,BaseDestructor=0x17855c,
    DeletingDestructor=0x178508,RawDestructor=0x1caa90,RawCleanup=0x1caa8c,RawSetup=0x1caa88;
constexpr std::array Targets{Sweep,ControllerSweep,Delete,Delayed,RemoveAsync,
    BaseDestructor,DeletingDestructor,RawDestructor,RawCleanup,RawSetup};
ProcessCall Service(ProcessHandle h,std::uint32_t target,std::initializer_list<std::uint32_t> args={}) {
    ProcessCall c;c.process=std::move(h);c.kind=ProcessCallKind::Service;c.target=target;
    c.argument_count=static_cast<std::uint8_t>(args.size());std::copy(args.begin(),args.end(),c.arguments.begin());return c;
}
bool ValidFields(const CarriedFileObject& f) {return f.path.size()<0x50 && f.path.find('\0')==std::string::npos;}
}
struct NativeFileController::State {
    struct Controller {
        FileControllerHandle identity;
        std::list<std::uint32_t> cache,async;
        IdentifierRegistry<std::uint32_t> index{127};
    };
    struct Object {
        FileObjectObservation row;
    };
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::optional<std::shared_ptr<Controller>> current;
    std::map<std::uint32_t,std::shared_ptr<Controller>> controllers;
    std::map<std::uint32_t,std::shared_ptr<Object>> objects;
    std::uint32_t controller_serial{},object_serial{};
    bool Live() const {auto s=scheduler.lock();return s && s->root(2);}
    FileControllerStatus Mutable(ProcessAccess* a) const {
        auto s=scheduler.lock();if(!s || !s->root(2))return FileControllerStatus::Retired;
        if(a)return a->BelongsTo(*s)?FileControllerStatus::Ready:FileControllerStatus::MismatchedDomain;
        return s->busy()?FileControllerStatus::Busy:FileControllerStatus::Ready;
    }
    std::shared_ptr<Controller> Get(std::uint32_t id) const {auto it=controllers.find(id);return it==controllers.end()?nullptr:it->second;}
    std::shared_ptr<Object> ObjectAt(std::uint32_t id) const {auto it=objects.find(id);return it==objects.end()?nullptr:it->second;}
    std::shared_ptr<Controller> Get(FileControllerHandle h) const {auto c=h?Get(h->serial):nullptr;return c && c->identity==h?c:nullptr;}
    std::shared_ptr<Object> Get(FileObjectHandle h) const {auto o=h?ObjectAt(h->serial):nullptr;return o && o->row.identity==h?o:nullptr;}
    bool Member(const std::shared_ptr<Controller>& c,const std::shared_ptr<Object>& o) const {
        return c && o && o->row.controller==c->identity && o->row.cache_linked && o->row.life==FileObjectLife::Live;
    }
    void RemoveQueued(Controller& c,Object& o) {
        if(o.row.controller!=c.identity || !o.row.async_linked)return;
        c.async.remove(o.row.identity->serial);o.row.async_linked=false;
        // Actual RemoveAsync only unlinks. The queued flag remains unchanged.
    }
    void Unlink(Controller& c,Object& o) {
        c.index.EraseFirst(o.row.fields.path.c_str());c.cache.remove(o.row.identity->serial);
        o.row.cache_linked=false;o.row.life=FileObjectLife::Destroying;
    }
};
struct NativeFileController::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;
    ProcessCall call;
    std::shared_ptr<State::Controller> controller;
    std::shared_ptr<State::Object> object;
    unsigned stage{},level{};
    std::uint32_t cursor{},next{};
    std::int32_t budget{};
    ProcessCallbackStep Invoke(std::uint32_t target,std::initializer_list<std::uint32_t> args) {
        return ProcessCallbackStep::Call(Service(call.process,target,args));
    }
    ProcessCallbackStep Step(ProcessAccess& access) override {
        auto s=state->scheduler.lock();if(!s || !access.BelongsTo(*s))return ProcessCallbackStep::Blocked();
        if(call.target==RawCleanup || call.target==RawSetup)return ProcessCallbackStep::Return();
        if(call.target==BaseDestructor || call.target==DeletingDestructor || call.target==RawDestructor) {
            if(stage==0) {
                // Changing to the FileObject vtable occurs before allocator Free.
                object->row.base_destructor_entered=true;
                auto& f=object->row.fields;
                if(f.allocator) {
                    if(!f.allocator_free)return ProcessCallbackStep::Blocked();
                    stage=1;return Invoke(f.allocator_free,{f.allocator,f.data});
                }
                stage=1;
            }
            if(stage==1) {
                auto& f=object->row.fields;f.allocator=0;f.data=0;f.size=0;
                if(call.target==BaseDestructor)return ProcessCallbackStep::Return(object->row.identity->serial);
                stage=2;return Invoke(0x2fdce4,{object->row.identity->serial});
            }
            return ProcessCallbackStep::Return();
        }
        if(call.target==Delayed) {
            if(stage==0) {
                object->row.fields.references=FileReleaseReference(object->row.fields.references);
                if(object->row.fields.references)return ProcessCallbackStep::Return();stage=1;
            }
            if(stage==1) {
                if(!state->current)return ProcessCallbackStep::Blocked();controller=*state->current;stage=2;
            }
            if(stage==2) {
                auto& f=object->row.fields;
                if(f.flags&0x04000000u) {
                    if(f.data) {
                        if(!f.cleanup)return ProcessCallbackStep::Blocked();
                        stage=3;return Invoke(f.cleanup,{object->row.identity->serial});
                    }
                    f.flags&=~0x04000000u;
                }
                stage=4;
            }
            if(stage==3) {
                if(object->row.life!=FileObjectLife::Live)return ProcessCallbackStep::Blocked();
                object->row.fields.flags&=~0x04000000u;stage=4;
            }
            if(stage==4) {
                const auto& f=object->row.fields;
                if(FileKeepAfterRelease(f.data!=0,f.cache_level,f.flags))return ProcessCallbackStep::Return();
                if(!controller)return ProcessCallbackStep::Blocked();
                stage=5;return Invoke(Delete,{controller->identity->serial,object->row.identity->serial});
            }
            return ProcessCallbackStep::Return();
        }
        if(call.target==RemoveAsync) {state->RemoveQueued(*controller,*object);return ProcessCallbackStep::Return();}
        if(call.target==Delete) {
            if(stage==0) {
                if(!state->Member(controller,object))return ProcessCallbackStep::Blocked();
                auto& f=object->row.fields;
                if(f.flags&0x40000000u)return ProcessCallbackStep::Return(0);
                if(f.flags&0x20000000u)state->RemoveQueued(*controller,*object);
                state->Unlink(*controller,*object);stage=1;
            }
            if(stage==1) {
                if(!object->row.fields.deleting_destructor)return ProcessCallbackStep::Blocked();
                stage=2;return Invoke(object->row.fields.deleting_destructor,{object->row.identity->serial});
            }
            object->row.life=FileObjectLife::Destroyed;return ProcessCallbackStep::Return(1);
        }
        if(stage==0) {
            if(call.target==Sweep) {
                if(!state->current)return ProcessCallbackStep::Blocked();controller=*state->current;
                if(!controller)return ProcessCallbackStep::Return();budget=0x40000000;
            } else budget=std::bit_cast<std::int32_t>(call.arguments[1]);
            cursor=controller->cache.empty()?0:controller->cache.front();stage=1;
        }
        if(stage==3) {object->row.life=FileObjectLife::Destroyed;cursor=next;stage=1;}
        for(;;) {
            if(stage==2) {
                if(!object->row.fields.deleting_destructor)return ProcessCallbackStep::Blocked();
                stage=3;return Invoke(object->row.fields.deleting_destructor,{object->row.identity->serial});
            }
            if(budget<=0 || level>3)return ProcessCallbackStep::Return();
            if(!cursor) {
                ++level;cursor=controller->cache.empty()?0:controller->cache.front();continue;
            }
            object=state->ObjectAt(cursor);
            if(!state->Member(controller,object))return ProcessCallbackStep::Blocked();
            auto it=std::find(controller->cache.begin(),controller->cache.end(),cursor);
            if(it==controller->cache.end())return ProcessCallbackStep::Blocked();
            ++it;next=it==controller->cache.end()?0:*it;
            const auto& f=object->row.fields;
            if(FileSweepEligible(f.cache_level,f.references,f.flags,level)) {
                budget=FileSweepSubtract(budget,f.size);state->Unlink(*controller,*object);stage=2;
            } else cursor=next;
        }
    }
};
NativeFileController::NativeFileController(std::shared_ptr<State> s):state_(std::move(s)){}
NativeFileController::~NativeFileController()=default;
FileControllerStatus NativeFileController::Create(std::shared_ptr<NativeProcessScheduler> s,
    std::shared_ptr<ProcessCallbackRegistry> registry,std::shared_ptr<NativeFileController>& out) {
    using S=FileControllerStatus;
    if(!s || !s->root(2))return S::NullScheduler;
    if(!registry || !s->UsesCallbacks(registry.get()))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=s;
    auto module=std::shared_ptr<NativeFileController>(new NativeFileController(state));
    if(!registry->Register(Targets,module))return S::DuplicateBinding;
    out=std::move(module);return S::Ready;
}
FileControllerStatus NativeFileController::Restore(const CarriedFileController& in,FileControllerHandle& out,
    std::vector<FileObjectHandle>& objects,ProcessAccess* a) {
    using S=FileControllerStatus;if(auto s=state_->Mutable(a);s!=S::Ready)return s;
    const auto count=in.cache.size();
    if(in.index_order.size()!=count || count>std::numeric_limits<std::uint32_t>::max())return S::InvalidSnapshot;
    for(const auto& f:in.cache)if(!ValidFields(f))return S::InvalidSnapshot;
    std::set<std::size_t> indices,async;
    for(auto i:in.index_order)if(i>=count || !indices.insert(i).second)return S::InvalidSnapshot;
    for(auto i:in.async_order)if(i>=count || !async.insert(i).second)return S::InvalidSnapshot;
    if(state_->controller_serial==std::numeric_limits<std::uint32_t>::max() ||
        count>std::numeric_limits<std::uint32_t>::max()-state_->object_serial)return S::IdentityExhausted;
    auto c=std::make_shared<State::Controller>();c->identity=std::make_shared<FileControllerIdentity>(++state_->controller_serial);
    std::vector<FileObjectHandle> handles;
    for(const auto& f:in.cache) {
        auto o=std::make_shared<State::Object>();o->row.identity=std::make_shared<FileObjectIdentity>(++state_->object_serial);
        o->row.controller=c->identity;o->row.fields=f;o->row.cache_linked=true;
        state_->objects.emplace(o->row.identity->serial,o);handles.push_back(o->row.identity);c->cache.push_back(o->row.identity->serial);
    }
    for(auto i:in.index_order)c->index.Append(in.cache[i].path.c_str(),handles[i]->serial);
    for(auto i:in.async_order) {c->async.push_back(handles[i]->serial);state_->Get(handles[i])->row.async_linked=true;}
    state_->controllers.emplace(c->identity->serial,c);state_->current=c;out=c->identity;objects=std::move(handles);return S::Ready;
}
FileControllerStatus NativeFileController::PublishAbsent(ProcessAccess* a) {
    auto s=state_->Mutable(a);if(s==FileControllerStatus::Ready)state_->current=std::shared_ptr<State::Controller>{};return s;
}
FileControllerStatus NativeFileController::WriteFields(FileObjectHandle h,const CarriedFileObject& f,ProcessAccess* a) {
    using S=FileControllerStatus;if(auto s=state_->Mutable(a);s!=S::Ready)return s;
    auto o=state_->Get(h);if(!o || o->row.life!=FileObjectLife::Live)return S::InvalidHandle;
    const auto& old=o->row.fields;
    if(!ValidFields(f) || f.path!=old.path || f.deleting_destructor!=old.deleting_destructor || f.cleanup!=old.cleanup)return S::InvalidSnapshot;
    o->row.fields=f;return S::Ready;
}
std::optional<FileControllerObservation> NativeFileController::Observe(FileControllerHandle h) const {
    auto c=state_->Get(h);if(!state_->Live() || !c)return {};
    FileControllerObservation out;out.present=true;out.identity=c->identity;
    for(auto id:c->cache)out.cache.push_back(state_->ObjectAt(id)->row.identity);
    for(auto id:c->async)out.async.push_back(state_->ObjectAt(id)->row.identity);
    out.index_accounting_count=c->index.AccountingCount();out.index_size=c->index.Size();return out;
}
std::optional<FileControllerObservation> NativeFileController::Observe() const {
    if(!state_->Live() || !state_->current)return {};
    return *state_->current?Observe((*state_->current)->identity):FileControllerObservation{};
}
std::optional<FileObjectObservation> NativeFileController::Observe(FileObjectHandle h) const {
    auto o=state_->Get(h);if(!state_->Live() || !o)return {};return o->row;
}
FileObjectHandle NativeFileController::Find(FileControllerHandle h,std::string_view path) const {
    auto c=state_->Get(h);if(!state_->Live() || !c || path.find('\0')!=std::string_view::npos)return {};
    auto row=c->index.Find(std::string(path).c_str());return row?state_->ObjectAt(row->value)->row.identity:nullptr;
}
FileObjectHandle NativeFileController::ResolveObject(std::uint32_t id) const {
    auto o=state_->ObjectAt(id);return state_->Live() && o?o->row.identity:nullptr;
}
ProcessCall NativeFileController::SweepCall(ProcessHandle h) {return Service(std::move(h),Sweep);}
std::optional<ProcessCall> NativeFileController::SweepCall(ProcessHandle h,FileControllerHandle c,std::int32_t budget) const {
    if(!state_->Live() || !state_->Get(c))return {};
    return Service(std::move(h),ControllerSweep,{c->serial,std::bit_cast<std::uint32_t>(budget)});
}
std::optional<ProcessCall> NativeFileController::DeleteCall(ProcessHandle h,FileControllerHandle c,FileObjectHandle o) const {
    if(!state_->Live() || !state_->Get(c) || !state_->Get(o))return {};
    return Service(std::move(h),Delete,{c->serial,o->serial});
}
std::optional<ProcessCall> NativeFileController::DelayedReleaseCall(ProcessHandle h,FileObjectHandle o) const {
    if(!state_->Live() || !state_->Get(o))return {};
    return Service(std::move(h),Delayed,{o->serial});
}
std::unique_ptr<ProcessContinuation> NativeFileController::Begin(const ProcessCall& c) {
    if(!state_->Live() || c.kind!=ProcessCallKind::Service || c.has_self || c.this_adjustment || !c.process)return {};
    auto p=std::make_unique<Continuation>();p->state=state_;p->call=c;
    if(c.target==Sweep) {if(c.argument_count)return {};return p;}
    if(c.target==ControllerSweep || c.target==Delete || c.target==RemoveAsync) {
        if(c.argument_count!=2 || !(p->controller=state_->Get(c.arguments[0])))return {};
        if(c.target!=ControllerSweep && !(p->object=state_->ObjectAt(c.arguments[1])))return {};
    } else {
        if(std::find(Targets.begin(),Targets.end(),c.target)==Targets.end() || c.argument_count!=1 ||
            !(p->object=state_->ObjectAt(c.arguments[0])) || p->object->row.life==FileObjectLife::Destroyed)return {};
        if(c.target==Delayed && p->object->row.life!=FileObjectLife::Live)return {};
        if((c.target==BaseDestructor || c.target==DeletingDestructor || c.target==RawDestructor) &&
            p->object->row.life!=FileObjectLife::Destroying)return {};
    }
    return p;
}
}
