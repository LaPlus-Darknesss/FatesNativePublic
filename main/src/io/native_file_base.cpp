#include "fates/io/native_file_base.hpp"
#include "fates/io/file_cache_policies.hpp"
#include "fates/runtime/native_resource_delay.hpp"
#include <algorithm>
#include <limits>
#include <map>

namespace fates::io::native {
using namespace runtime::native;
namespace {
constexpr std::uint32_t Open=0x112ca8,OpenBinary=0x4e656c,Close=0x112d28,CloseBinary=0x112d20,
    Free=0x11cf84,Replace=0x4e6804,Finish=0x4e65e4,TryFinish=0x4e68ac,
    GetData=0x10987c,GetPath=0x545da0,GetSize=0x545db8,IsAsync=0x545dd0,IsDone=0x545df4,
    Touch=0x115a18,ControllerFinish=0x10c014,Delete=0x11fa44,SetPriority=0x4e67f4;
constexpr std::array Targets{Open,OpenBinary,Close,CloseBinary,Free,Replace,Finish,TryFinish,
    GetData,GetPath,GetSize,IsAsync,IsDone,SetPriority};
ProcessCall Service(ProcessHandle h,std::uint32_t target,std::initializer_list<std::uint32_t> args) {
    ProcessCall c;c.process=std::move(h);c.kind=ProcessCallKind::Service;c.target=target;
    c.argument_count=static_cast<std::uint8_t>(args.size());std::copy(args.begin(),args.end(),c.arguments.begin());return c;
}
}
struct NativeFileBase::State {
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<NativeFileController> files;
    std::map<std::uint32_t,std::shared_ptr<FileBaseObservation>> bases;
    std::map<std::uint32_t,FilePathHandle> paths;
    std::map<std::string,FilePathHandle,std::less<>> returned_paths;
    std::uint32_t base_serial{},path_serial{};
    bool Live() const {auto s=scheduler.lock();return s && s->root(2);}
    FileBaseStatus Mutable(ProcessAccess* a) const {
        auto s=scheduler.lock();if(!s || !s->root(2))return FileBaseStatus::Retired;
        if(a)return a->BelongsTo(*s)?FileBaseStatus::Ready:FileBaseStatus::MismatchedDomain;
        return s->busy()?FileBaseStatus::Busy:FileBaseStatus::Ready;
    }
    std::shared_ptr<FileBaseObservation> Get(std::uint32_t id) const {
        auto it=bases.find(id);return it==bases.end()?nullptr:it->second;
    }
    std::shared_ptr<FileBaseObservation> Get(FileBaseHandle h) const {
        auto b=h?Get(h->serial):nullptr;return b && b->identity==h?b:nullptr;
    }
    FilePathHandle Path(std::uint32_t id) const {auto it=paths.find(id);return it==paths.end()?nullptr:it->second;}
    bool ObjectValid(FileObjectHandle o) const {
        if(!o)return true;
        auto row=files->Observe(o);return row && row->life==FileObjectLife::Live;
    }
};
struct NativeFileBase::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;
    ProcessCall call;
    std::shared_ptr<FileBaseObservation> base,source;
    FilePathHandle path;
    FileObjectHandle object,found;
    FileControllerHandle controller;
    unsigned stage{},ready_stage{};
    ProcessCallbackStep Invoke(std::uint32_t target,std::initializer_list<std::uint32_t> args) {
        return ProcessCallbackStep::Call(Service(call.process,target,args));
    }
    std::optional<FileObjectObservation> Object() const {
        auto row=state->files->Observe(object);
        return row && row->life==FileObjectLife::Live?row:std::nullopt;
    }
    bool Write(const CarriedFileObject& fields,ProcessAccess& a) {
        return state->files->WriteFields(object,fields,&a)==FileControllerStatus::Ready;
    }
    // Captured FileObject survives callbacks; its fields are always read again.
    // The caller separately reloads its FileBase attachment for the final result.
    std::optional<ProcessCallbackStep> Ready(ProcessAccess& a) {
        for(;;) {
            auto row=Object();if(!row)return ProcessCallbackStep::Blocked();
            auto f=row->fields;
            if(ready_stage==0) {
                if(f.flags&0x04000000u)return {};
                if(f.flags&0x60000000u) {
                    auto current=state->files->Observe();
                    if(!current || !current->present)return ProcessCallbackStep::Blocked();
                    ready_stage=1;return Invoke(ControllerFinish,{current->identity->serial,object->serial});
                }
                ready_stage=1;
            }
            if(ready_stage==1) {
                f.flags&=~0xe0000000u;if(!Write(f,a))return ProcessCallbackStep::Blocked();
                ready_stage=2;
            }
            if(ready_stage==2) {
                if(f.data) {
                    if(!f.setup)return ProcessCallbackStep::Blocked();
                    ready_stage=3;return Invoke(f.setup,{object->serial});
                }
                ready_stage=3;
            }
            if(ready_stage==3) {
                f.flags|=0x04000000u;if(!Write(f,a))return ProcessCallbackStep::Blocked();
                ready_stage=4;
            }
            return {};
        }
    }
    ProcessCallbackStep Release(ProcessAccess& a) {
        if(stage==0) {
            object=base->object;if(!object)return ProcessCallbackStep::Return();
            auto row=Object();if(!row)return ProcessCallbackStep::Blocked();
            auto f=row->fields;f.references=FileReleaseReference(f.references);
            if(!Write(f,a))return ProcessCallbackStep::Blocked();
            if(f.references) {base->object.reset();return ProcessCallbackStep::Return();}
            stage=1;
        }
        if(stage==1) {
            auto row=Object();if(!row || !row->fields.is_delay)return ProcessCallbackStep::Blocked();
            stage=2;return Invoke(row->fields.is_delay,{object->serial});
        }
        if(stage==2) {
            if(a.call_result())stage=3;
            else {
                // Original reloads the base after virtual IsDelay returns false.
                object=base->object;stage=6;
            }
        }
        if(stage==3) {
            auto row=Object();if(!row)return ProcessCallbackStep::Blocked();
            auto f=row->fields;f.references=static_cast<std::uint16_t>(f.references+1u);
            if(!Write(f,a))return ProcessCallbackStep::Blocked();
            stage=4;return Invoke(Touch,{object->serial});
        }
        if(stage==4) {
            auto c=NativeResourceDelay::EntryCall(call.process,{0x11fb34,{object->serial,0},1});
            if(!c)return ProcessCallbackStep::Blocked();
            stage=5;return ProcessCallbackStep::Call(*c);
        }
        if(stage==5 || stage==10) {base->object.reset();return ProcessCallbackStep::Return();}
        if(stage==6) {
            if(!Object())return ProcessCallbackStep::Blocked();
            auto current=state->files->Observe();if(!current)return ProcessCallbackStep::Blocked();
            controller=current->identity;stage=7;
        }
        if(stage==7) {
            auto row=Object();if(!row)return ProcessCallbackStep::Blocked();
            if((row->fields.flags&0x04000000u) && row->fields.data) {
                if(!row->fields.cleanup)return ProcessCallbackStep::Blocked();
                stage=8;return Invoke(row->fields.cleanup,{object->serial});
            }
            stage=8;
        }
        if(stage==8) {
            auto row=Object();if(!row)return ProcessCallbackStep::Blocked();
            auto f=row->fields;f.flags&=~0x04000000u;
            if(!Write(f,a))return ProcessCallbackStep::Blocked();stage=9;
        }
        auto row=Object();if(!row)return ProcessCallbackStep::Blocked();
        const auto& f=row->fields;
        if(FileKeepAfterRelease(f.data!=0,f.cache_level,f.flags)) {
            base->object.reset();return ProcessCallbackStep::Return();
        }
        if(!controller)return ProcessCallbackStep::Blocked();
        stage=10;return Invoke(Delete,{controller->serial,object->serial});
    }
    ProcessCallbackStep Step(ProcessAccess& a) override {
        auto s=state->scheduler.lock();if(!s || !a.BelongsTo(*s))return ProcessCallbackStep::Blocked();
        if(call.target==Free)return Release(a);
        if(call.target==Open || call.target==OpenBinary) {
            if(stage==0) {
                auto current=state->files->Observe();
                if(!current || !current->present)return ProcessCallbackStep::Blocked();
                found=path?state->files->Find(current->identity,path->text):nullptr;
                stage=1;if(found)return Invoke(Touch,{found->serial});
            }
            if(stage==1) {
                if(found && found==base->object)return ProcessCallbackStep::Return(2);
                stage=2;return Invoke(Free,{base->identity->serial});
            }
            base->object=found;return ProcessCallbackStep::Return(found?1u:0u);
        }
        if(call.target==Replace) {
            if(stage==0) {
                if(base->object==source->object)return ProcessCallbackStep::Return();
                stage=1;return Invoke(Free,{base->identity->serial});
            }
            if(stage==1) {
                object=source->object;if(!object)return ProcessCallbackStep::Return();
                auto row=Object();if(!row)return ProcessCallbackStep::Blocked();
                auto f=row->fields;f.references=static_cast<std::uint16_t>(f.references+1u);
                if(!Write(f,a))return ProcessCallbackStep::Blocked();
                base->object=object;stage=2;return Invoke(Touch,{object->serial});
            }
            if(stage==2) {object=base->object;stage=3;}
            if(auto step=Ready(a))return *step;
            return ProcessCallbackStep::Return();
        }
        if(call.target==SetPriority) {
            object=base->object;
            if(!object)return ProcessCallbackStep::Return(0);
            auto row=Object();if(!row)return ProcessCallbackStep::Blocked();
            auto fields=row->fields;fields.priority=static_cast<std::uint8_t>(call.arguments[1]);
            if(!Write(fields,a))return ProcessCallbackStep::Blocked();
            return ProcessCallbackStep::Return(object->serial);
        }
        if(call.target==GetPath) {
            // An empty attachment returns the actual empty-string value, not
            // a null path. Otherwise use the lower object's stored name.
            std::string text;
            object=base->object;
            if(object) {
                auto row=Object();if(!row)return ProcessCallbackStep::Blocked();
                text=row->fields.path;
            }
            const auto found_=state->returned_paths.find(text);
            if(found_!=state->returned_paths.end())return ProcessCallbackStep::Return(found_->second->serial);
            if(state->path_serial==std::numeric_limits<std::uint32_t>::max())return ProcessCallbackStep::Blocked();
            auto result=std::make_shared<FilePathIdentity>(++state->path_serial,text);
            state->paths.emplace(result->serial,result);state->returned_paths.emplace(std::move(text),result);
            return ProcessCallbackStep::Return(result->serial);
        }
        if(call.target==GetData || call.target==GetSize || call.target==IsAsync || call.target==IsDone) {
            object=base->object;if(!object)return ProcessCallbackStep::Return();
            auto row=Object();if(!row)return ProcessCallbackStep::Blocked();
            const auto& f=row->fields;
            return ProcessCallbackStep::Return(call.target==GetData?f.data:call.target==GetSize?f.size:
                call.target==IsAsync?std::uint32_t((f.flags&0x60000000u)!=0):std::uint32_t((f.flags&0x04000000u)!=0));
        }
        const bool close=call.target==Close || call.target==CloseBinary;
        if(stage==0) {
            if(close && call.arguments[1]!=2) {
                object=base->object;auto row=Object();if(!row)return ProcessCallbackStep::Blocked();
                auto f=row->fields;f.references=static_cast<std::uint16_t>(f.references+1u);
                if(!Write(f,a))return ProcessCallbackStep::Blocked();
                stage=1;return Invoke(Touch,{object->serial});
            }
            stage=1;
        }
        if(stage==1) {
            if(call.target==Close && call.arguments[2]==1)return ProcessCallbackStep::Return(1);
            object=base->object;
            if(!object) {
                if(call.target!=Finish)return ProcessCallbackStep::Return(0);
                stage=5;return Invoke(Free,{base->identity->serial});
            }
            stage=2;
        }
        if(stage==2) {
            if(auto step=Ready(a))return *step;
            // This reread is distinct from the captured readiness object.
            auto row=state->files->Observe(base->object);
            if(!row || row->life!=FileObjectLife::Live)return ProcessCallbackStep::Blocked();
            if(row->fields.data)return ProcessCallbackStep::Return(call.target==Finish?0u:1u);
            stage=3;return Invoke(Free,{base->identity->serial});
        }
        if(stage==3 && call.target==Finish) {
            // Retail FinishAsync really calls Free twice on failed data.
            stage=5;return Invoke(Free,{base->identity->serial});
        }
        return ProcessCallbackStep::Return(0);
    }
};
NativeFileBase::NativeFileBase(std::shared_ptr<State> s):state_(std::move(s)){}
NativeFileBase::~NativeFileBase()=default;
FileBaseStatus NativeFileBase::Create(std::shared_ptr<NativeProcessScheduler> s,
    std::shared_ptr<ProcessCallbackRegistry> registry,std::shared_ptr<NativeFileController> files,
    std::shared_ptr<NativeFileBase>& out) {
    using S=FileBaseStatus;if(!s || !s->root(2))return S::NullScheduler;
    if(!registry || !s->UsesCallbacks(registry.get()) || !files || !files->UsesScheduler(*s))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=s;state->files=std::move(files);
    auto module=std::shared_ptr<NativeFileBase>(new NativeFileBase(state));
    if(!registry->Register(Targets,module))return S::DuplicateBinding;
    out=std::move(module);return S::Ready;
}
FileBaseStatus NativeFileBase::RestoreAttachment(FileObjectHandle object,FileBaseHandle& out,ProcessAccess* a) {
    using S=FileBaseStatus;if(auto s=state_->Mutable(a);s!=S::Ready)return s;
    if(!state_->ObjectValid(object))return S::InvalidHandle;
    if(state_->base_serial==std::numeric_limits<std::uint32_t>::max())return S::IdentityExhausted;
    auto b=std::make_shared<FileBaseObservation>();
    b->identity=std::make_shared<FileBaseIdentity>(++state_->base_serial);b->object=std::move(object);
    state_->bases.emplace(b->identity->serial,b);out=b->identity;return S::Ready;
}
FileBaseStatus NativeFileBase::WriteAttachment(FileBaseHandle h,FileObjectHandle object,ProcessAccess* a) {
    using S=FileBaseStatus;if(auto s=state_->Mutable(a);s!=S::Ready)return s;
    auto b=state_->Get(h);if(!b || !state_->ObjectValid(object))return S::InvalidHandle;
    b->object=std::move(object);return S::Ready;
}
FileBaseStatus NativeFileBase::RegisterPath(std::string_view text,FilePathHandle& out,ProcessAccess* a) {
    using S=FileBaseStatus;if(auto s=state_->Mutable(a);s!=S::Ready)return s;
    if(text.find('\0')!=std::string_view::npos)return S::InvalidPath;
    if(state_->path_serial==std::numeric_limits<std::uint32_t>::max())return S::IdentityExhausted;
    auto p=std::make_shared<FilePathIdentity>(++state_->path_serial,std::string(text));
    state_->paths.emplace(p->serial,p);out=std::move(p);return S::Ready;
}
FileBaseStatus NativeFileBase::RetireEmpty(FileBaseHandle h,ProcessAccess* a) {
    using S=FileBaseStatus;if(auto s=state_->Mutable(a);s!=S::Ready)return s;
    auto b=state_->Get(h);if(!b || b->object)return S::InvalidHandle;
    state_->bases.erase(h->serial);return S::Ready;
}
std::optional<FileBaseObservation> NativeFileBase::Observe(FileBaseHandle h) const {
    auto b=state_->Get(h);return state_->Live() && b?std::optional(*b):std::nullopt;
}
FileBaseHandle NativeFileBase::ResolveBase(std::uint32_t id) const {
    auto b=state_->Get(id);return state_->Live() && b?b->identity:nullptr;
}
FilePathHandle NativeFileBase::ResolvePath(std::uint32_t id) const {
    return state_->Live()?state_->Path(id):nullptr;
}
bool NativeFileBase::UsesController(const NativeFileController& files) const noexcept {
    return state_->files.get()==&files;
}
std::optional<ProcessCall> NativeFileBase::OpenCall(ProcessHandle p,FileBaseHandle h,FilePathHandle path,
    std::uint32_t read,bool binary) const {
    if(!Observe(h) || (path && state_->Path(path->serial)!=path))return {};
    if(binary)return Service(std::move(p),OpenBinary,{h->serial,path?path->serial:0});
    return Service(std::move(p),Open,{h->serial,path?path->serial:0,read});
}
std::optional<ProcessCall> NativeFileBase::CloseCall(ProcessHandle p,FileBaseHandle h,
    std::uint32_t type,std::uint32_t read,bool binary) const {
    if(!Observe(h))return {};
    if(binary)return Service(std::move(p),CloseBinary,{h->serial,type});
    return Service(std::move(p),Close,{h->serial,type,read});
}
std::optional<ProcessCall> NativeFileBase::FreeCall(ProcessHandle p,FileBaseHandle h) const {
    if(!Observe(h))return {};return Service(std::move(p),Free,{h->serial});
}
std::optional<ProcessCall> NativeFileBase::PriorityCall(ProcessHandle process,FileBaseHandle handle,
    std::uint32_t priority) const {
    if(!Observe(handle))return {};
    return Service(std::move(process),SetPriority,{handle->serial,priority});
}
std::optional<ProcessCall> NativeFileBase::PathCall(ProcessHandle p,FileBaseHandle h) const {
    if(!Observe(h))return {};return Service(std::move(p),GetPath,{h->serial});
}
std::optional<ProcessCall> NativeFileBase::ReplaceCall(ProcessHandle p,FileBaseHandle h,FileBaseHandle source) const {
    if(!Observe(h) || !Observe(source))return {};return Service(std::move(p),Replace,{h->serial,source->serial});
}
std::optional<ProcessCall> NativeFileBase::FinishCall(ProcessHandle p,FileBaseHandle h,bool try_finish) const {
    if(!Observe(h))return {};return Service(std::move(p),try_finish?TryFinish:Finish,{h->serial});
}
std::unique_ptr<ProcessContinuation> NativeFileBase::Begin(const ProcessCall& c) {
    if(!state_->Live() || c.kind!=ProcessCallKind::Service || c.has_self || c.this_adjustment ||
        !c.process || std::find(Targets.begin(),Targets.end(),c.target)==Targets.end())return {};
    const unsigned count=(c.target==Open || c.target==Close)?3u:
        (c.target==OpenBinary || c.target==CloseBinary || c.target==Replace || c.target==SetPriority)?2u:1u;
    if(c.argument_count!=count)return {};
    auto b=state_->Get(c.arguments[0]);if(!b)return {};
    auto p=std::make_unique<Continuation>();p->state=state_;p->call=c;p->base=std::move(b);
    if(c.target==Replace && !(p->source=state_->Get(c.arguments[1])))return {};
    if((c.target==Open || c.target==OpenBinary) && c.arguments[1] && !(p->path=state_->Path(c.arguments[1])))return {};
    return p;
}
}
