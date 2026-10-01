#include "fates/runtime/native_process.hpp"
#include "fates/runtime/native_identifier.hpp"
#include <algorithm>
#include <bit>
#include <limits>
#include <map>
#include <utility>

namespace fates::runtime::native {
namespace {
std::uint8_t Command(const ProcessDescriptor& d) {return static_cast<std::uint8_t>(d.command);}
std::int16_t LowShort(std::uint32_t v) {return std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(v));}
std::uint32_t MillisecondFrames(std::int32_t value) {
    // VCVT.F32.S32, VMUL.F32 with the executable's 0x3d75c28f, then
    // VCVT.S32.F32 (toward zero). The product fits signed32 for every input.
    const float source=static_cast<float>(value);
    const float frames=source*std::bit_cast<float>(std::uint32_t{0x3d75c28f});
    return static_cast<std::uint32_t>(static_cast<std::int32_t>(frames));
}
bool BaseNoop(std::uint32_t target) {
    return target==0x4ee0b8 || target==0x4edef8 || target==0x4ee170 || target==0x4ee318;
}
}
ProcessProgram::ProcessProgram(std::vector<ProcessDescriptor> d):descriptors_(std::move(d)){}
std::shared_ptr<const ProcessProgram> ProcessProgram::Create(std::vector<ProcessDescriptor> d) {
    if(d.empty() || std::none_of(d.begin(),d.end(),[](const auto& v){return Command(v)==0;}))return {};
    return std::shared_ptr<const ProcessProgram>(new ProcessProgram(std::move(d)));
}
std::shared_ptr<const ProcessProgram> ProcessProgram::Default() {
    // __sti___8_Proc_cpp005BD6E0: repeated virtual Tick, followed by End.
    static const auto value=Create({{13,0,0,8,1},{}});return value;
}
ProcessType ProcessType::Base() {
    return {{{0,4,0x4ee310},{0,8,0x4ee0b8},{0,12,0x4edef8},{0,16,0x4ee170}}};
}
bool ProcessCallbackRegistry::Register(std::span<const std::uint32_t> targets,const std::shared_ptr<ProcessCallbacks>& owner) {
    if(!owner || targets.empty())return false;
    for(std::size_t i=0;i<targets.size();++i) {
        if(!targets[i] || std::find(targets.begin(),targets.begin()+i,targets[i])!=targets.begin()+i)return false;
        if(std::any_of(entries_.begin(),entries_.end(),[&](const auto& e){return e.first==targets[i];}))return false;
    }
    entries_.reserve(entries_.size()+targets.size());
    for(auto target:targets)entries_.emplace_back(target,owner);
    return true;
}
std::unique_ptr<ProcessContinuation> ProcessCallbackRegistry::Begin(const ProcessCall& call) {
    for(const auto& e:entries_)if(e.first==call.target) {
        if(auto owner=e.second.lock())return owner->Begin(call);return {};
    }
    return {};
}
struct NativeProcessScheduler::Impl {
    struct Node {ProcessObservation view;ProcessType type;};
    enum class Kind {Exec,Tick,Persistent,Mark,Remove,Sweep,Callback};
    struct Frame {
        Kind kind;ProcessHandle id;unsigned stage{};
        bool only_this{true};std::uint8_t command{};
        ProcessCall call;std::unique_ptr<ProcessContinuation> continuation;
        ProcessHandle pending_deletion;bool deletion_requested{};
        std::optional<ProcessCall> pending_call;
        std::uint32_t raw_target{},raw_adjustment{};bool member{},resolved{};
        Frame(Kind k,ProcessHandle h={}):kind(k),id(std::move(h)){}
    };
    std::map<std::uint64_t,std::unique_ptr<Node>> nodes;
    std::array<ProcessHandle,3> roots;
    std::shared_ptr<ProcessCallbacks> callbacks;
    std::vector<Frame> stack;
    std::uint64_t serial{},transitions{};
    std::uint32_t delta{},return_value{};
    bool retired{},in_run{},in_callback{};
    Node* Get(const ProcessHandle& h) const {
        if(!h)return nullptr;
        auto it=nodes.find(h->serial);
        return it!=nodes.end() && it->second->view.identity==h?it->second.get():nullptr;
    }
    ProcessStatus Mutable() const {
        if(retired)return ProcessStatus::Retired;
        return !stack.empty() && !in_callback?ProcessStatus::Busy:ProcessStatus::Ready;
    }
    ProcessStatus Create(ProcessHandle parent,std::shared_ptr<const ProcessProgram> program,
        std::optional<std::string> name,bool blocking,const ProcessType& type,ProcessHandle& result,bool root=false) {
        if(auto s=Mutable();s!=ProcessStatus::Ready)return s;
        auto* p=Get(parent);
        if(!root && (!p || !p->view.linked || (p->view.flags&1)))return ProcessStatus::InvalidHandle;
        if(!program)return ProcessStatus::InvalidProgram;
        if(!name || name->find('\0')!=std::string::npos)return ProcessStatus::InvalidName;
        if(serial==std::numeric_limits<std::uint64_t>::max())return ProcessStatus::IdentityExhausted;
        auto node=std::make_unique<Node>();auto& v=node->view;
        v.identity=std::make_shared<const ProcessIdentity>(ProcessIdentity{++serial});
        v.parent=parent;v.program=std::move(program);v.name=std::move(name);v.linked=true;v.root=root;
        if(v.name && !v.name->empty()) {v.name_hash=HashIdentifierExact(*v.name).nameHash;if(!v.name_hash)v.name_hash=1;}
        v.persistent_target=12;v.persistent_adjustment=1;node->type=type;
        if(p) {
            v.older=p->view.child;
            if(auto* old=Get(v.older))old->view.newer=v.identity;
            p->view.child=v.identity;
            if(blocking) {v.flags|=2;++p->view.blocking_children;}
        }
        result=v.identity;nodes.emplace(serial,std::move(node));return ProcessStatus::Ready;
    }
    ProcessStatus Next(const ProcessHandle& h,bool immediate) {
        if(auto s=Mutable();s!=ProcessStatus::Ready)return s;
        auto* n=Get(h);if(!n)return ProcessStatus::InvalidHandle;
        const auto& d=n->view.program->descriptors();
        if(n->view.pc>=d.size())return ProcessStatus::InvalidProgram;
        if(Command(d[n->view.pc])!=0)++n->view.pc;
        if(immediate)n->view.flags|=8;return ProcessStatus::Ready;
    }
    ProcessStatus Jump(const ProcessHandle& h,std::uint32_t label,bool immediate) {
        if(auto s=Mutable();s!=ProcessStatus::Ready)return s;
        auto* n=Get(h);if(!n)return ProcessStatus::InvalidHandle;
        const auto& d=n->view.program->descriptors();
        for(std::size_t i=0;i<d.size() && Command(d[i])!=0;++i)
            if(Command(d[i])==4 && d[i].argument==label) {n->view.pc=i;break;}
        if(immediate)n->view.flags|=8;return ProcessStatus::Ready;
    }
    ProcessStatus Wait(const ProcessHandle& h,std::uint32_t frames) {
        if(auto s=Mutable();s!=ProcessStatus::Ready)return s;
        auto* n=Get(h);if(!n)return ProcessStatus::InvalidHandle;
        n->view.wait_frames=LowShort(frames);return ProcessStatus::Ready;
    }
    void NextInternal(Node& n) {
        const auto& d=n.view.program->descriptors();
        if(n.view.pc<d.size() && Command(d[n.view.pc])!=0)++n.view.pc;
    }
    void JumpInternal(Node& n,std::uint32_t label) {
        const auto& d=n.view.program->descriptors();
        for(std::size_t i=0;i<d.size() && Command(d[i])!=0;++i)
            if(Command(d[i])==4 && d[i].argument==label) {n.view.pc=i;return;}
    }
    void Push(Kind kind,ProcessHandle id,bool only_this=true) {
        if(!id)return;
        stack.emplace_back(kind,std::move(id));stack.back().only_this=only_this;
    }
    void Callback(ProcessHandle id,ProcessCallKind kind,std::uint32_t target,std::uint32_t adjustment,
                  bool member,std::uint8_t command=0,std::uint32_t a=0,std::uint32_t b=0) {
        stack.emplace_back(Kind::Callback,id);auto& f=stack.back();
        f.call.process=id;f.call.kind=kind;f.call.command=command;
        f.raw_target=target;f.raw_adjustment=adjustment;f.member=member;
        f.call.has_self=kind!=ProcessCallKind::Descriptor || (command!=17 && command!=18);
        if(command>=15 && command<=20) {
            f.call.arguments={a,command==18?b:0};f.call.argument_count=command==18?2:1;
        }
    }
    ProcessStatus AdmitService(const ProcessCall& call) const {
        if(call.kind!=ProcessCallKind::Service || call.argument_count>call.arguments.size())return ProcessStatus::InvalidCall;
        auto* n=Get(call.process);
        if(!n || !n->view.linked || (n->view.flags&1))return ProcessStatus::InvalidHandle;
        return ProcessStatus::Ready;
    }
    void Service(ProcessCall call) {
        stack.emplace_back(Kind::Callback,call.process);auto& f=stack.back();
        f.call=std::move(call);f.resolved=true;
    }
    ProcessStatus Step(NativeProcessScheduler& owner) {
        auto& f=stack.back();auto* n=Get(f.id);
        if(f.kind!=Kind::Sweep && !n)return ProcessStatus::InvalidHandle;
        switch(f.kind) {
        case Kind::Exec:
            if(f.stage==0) {f.stage=1;auto head=n->view.child;f.id=head;if(!head){stack.pop_back();break;}Push(Kind::Tick,head);}
            else if(f.stage==1) {f.stage=2;Push(Kind::Persistent,f.id);}
            else stack.pop_back();
            break;
        case Kind::Tick:
            if(f.stage==0) {f.stage=1;Push(Kind::Tick,n->view.older);break;}
            if(f.stage==1) {n->view.flags|=4;f.stage=2;break;}
            if(f.stage==4) {stack.pop_back();break;}
            if(f.stage==3) {
                if(n->view.flags&1) {stack.pop_back();break;}
                f.stage=4;Push(Kind::Tick,n->view.child);break;
            }
            if(f.stage==5) {
                const auto command=f.command;f.stage=2;
                if(command==9 || command==12) {if(return_value){f.stage=3;break;}NextInternal(*n);}
                else if(command==10 || command==13) {if(!(n->view.flags&8))f.stage=3;}
                else if(command>=21) {
                    if(n->view.pc>=n->view.program->descriptors().size())return ProcessStatus::InvalidProgram;
                    if((command==21 || command==23)?return_value!=0:return_value==0)
                        JumpInternal(*n,n->view.program->descriptors()[n->view.pc].argument);
                    else NextInternal(*n);
                } else NextInternal(*n);
                break;
            }
            if(n->view.flags&1) {stack.pop_back();break;}
            if(n->view.blocking_children) {f.stage=3;break;}
            if(n->view.wait_frames>0) {
                auto value=LowShort(static_cast<std::uint16_t>(n->view.wait_frames)-delta);
                n->view.wait_frames=std::max<std::int16_t>(value,0);f.stage=3;break;
            }
            if(n->view.pc>=n->view.program->descriptors().size())return ProcessStatus::InvalidProgram;
            n->view.flags&=static_cast<std::uint8_t>(~8u);
            {
                const auto d=n->view.program->descriptors()[n->view.pc];const auto c=Command(d);
                if(c==0) {f.stage=3;Push(Kind::Mark,f.id);}
                else if(c==3)JumpInternal(*n,d.argument);
                else if(c==4)NextInternal(*n);
                else if(c==5 || c==6 || c==7) {
                    if(c!=7)n->view.wait_frames=LowShort(c==5?MillisecondFrames(std::bit_cast<std::int32_t>(d.argument)):d.argument);
                    NextInternal(*n);f.stage=3;
                } else if(c==14) {
                    n->view.persistent_target=d.target;n->view.persistent_adjustment=d.adjustment;NextInternal(*n);
                } else if(c>=8 && c<=24) {
                    f.command=c;f.stage=5;
                    Callback(f.id,ProcessCallKind::Descriptor,d.target,d.adjustment,
                        (c>=11 && c<=13) || c==19 || c==20 || c==23 || c==24,c,d.argument,d.argument2);
                } else f.stage=3; // 1,2 and unknown command bytes stall, then visit children.
            }
            break;
        case Kind::Persistent:
            if(f.stage==0) {f.stage=1;Push(Kind::Persistent,n->view.older);}
            else if(f.stage==1) {
                if(n->view.flags&1) {stack.pop_back();break;}
                f.stage=2;
                if((n->view.flags&4) && (n->view.persistent_target || (n->view.persistent_adjustment&1)))
                    Callback(f.id,ProcessCallKind::Persistent,n->view.persistent_target,n->view.persistent_adjustment,true);
            } else if(f.stage==2) {f.stage=3;Push(Kind::Persistent,n->view.child);}
            else stack.pop_back();
            break;
        case Kind::Mark:
            if(f.stage==0) {f.stage=1;if(!f.only_this)Push(Kind::Mark,n->view.older,false);}
            else if(f.stage==1) {f.stage=2;Push(Kind::Mark,n->view.child,false);}
            else if(f.stage==2) {
                if(n->view.flags&2) {
                    auto* parent=Get(n->view.parent);if(!parent)return ProcessStatus::InvalidHandle;
                    --parent->view.blocking_children;n->view.flags&=static_cast<std::uint8_t>(~2u);
                }
                if(n->view.flags&1) {stack.pop_back();break;}
                n->view.flags|=1;f.stage=3;Callback(f.id,ProcessCallKind::Dispose,16,1,true);
            } else stack.pop_back();
            break;
        case Kind::Sweep:
            if(f.stage>=3)stack.pop_back();else {auto h=roots[f.stage++];Push(Kind::Remove,Get(h)->view.child);}
            break;
        case Kind::Remove:
            if(f.stage==0) {f.stage=1;Push(Kind::Remove,n->view.older);}
            else if(f.stage==1) {f.stage=2;Push(Kind::Remove,n->view.child);}
            else if(f.stage==2) {
                if(!(n->view.flags&1)) {stack.pop_back();break;}
                auto& v=n->view;
                if(auto* parent=Get(v.parent);parent && parent->view.child==v.identity)parent->view.child=v.older;
                if(auto* next=Get(v.newer))next->view.older=v.older;
                if(auto* prev=Get(v.older))prev->view.newer=v.newer;
                v.linked=false;f.stage=3;Callback(f.id,ProcessCallKind::Destroy,4,1,true);
            } else {auto serial=f.id->serial;stack.pop_back();nodes.erase(serial);}
            break;
        case Kind::Callback:
            if(f.pending_call) {
                if(auto s=AdmitService(*f.pending_call);s!=ProcessStatus::Ready)return s;
                auto call=std::move(*f.pending_call);f.pending_call.reset();Service(std::move(call));break;
            }
            if(f.deletion_requested) {
                auto* victim=Get(f.pending_deletion);
                if(!victim || victim->view.root)return ProcessStatus::InvalidHandle;
                if(!(victim->view.flags&1) && !victim->view.linked)return ProcessStatus::InvalidHandle;
                f.deletion_requested=false;
                if(!(victim->view.flags&1))Push(Kind::Mark,f.pending_deletion);
                break;
            }
            if(!f.resolved) {
                f.call.target=f.raw_target;
                if(f.member) {
                    const auto a=std::bit_cast<std::int32_t>(f.raw_adjustment);
                    // Arithmetic shift without relying on signed shift semantics.
                    f.call.this_adjustment=a>=0?a/2:-1-((-1-a)/2);
                    if(f.raw_adjustment&1) {
                        const auto& methods=n->type.methods;
                        auto it=std::find_if(methods.begin(),methods.end(),[&](const auto& m) {
                            return m.this_adjustment==f.call.this_adjustment && m.offset==f.raw_target;
                        });
                        if(it==methods.end())return ProcessStatus::MissingCallback;
                        f.call.target=it->target;
                    }
                }
                f.resolved=true;
            }
            // Base deletion4EE310 only releases allocation storage; erasure below
            // owns that host storage. No derived destructor is inferred from it.
            const bool condition=f.call.kind==ProcessCallKind::Descriptor &&
                (f.call.command==9 || f.call.command==12 || f.call.command>=21);
            if((BaseNoop(f.call.target) && !condition && f.call.kind!=ProcessCallKind::Service) || (f.call.kind==ProcessCallKind::Destroy && f.call.target==0x4ee310)) {
                return_value=0;stack.pop_back();break;
            }
            if(!f.continuation) {
                if(!callbacks)return ProcessStatus::MissingCallback;
                f.continuation=callbacks->Begin(f.call);
                if(!f.continuation)return ProcessStatus::MissingCallback;
            }
            ProcessAccess access(owner);in_callback=true;
            ProcessCallbackStep result;
            try {result=f.continuation->Step(access);} catch(...) {in_callback=false;throw;}
            in_callback=false;
            if(result.kind==ProcessCallbackStep::Kind::Blocked)return ProcessStatus::CallbackBlocked;
            if(result.kind==ProcessCallbackStep::Kind::Continue)break;
            if(result.kind==ProcessCallbackStep::Kind::Return) {return_value=result.value;stack.pop_back();break;}
            if(result.kind==ProcessCallbackStep::Kind::Call) {
                if(!result.call)return ProcessStatus::InvalidCall;
                f.pending_call=std::move(result.call);break;
            }
            f.pending_deletion=std::move(result.deletion);f.deletion_requested=true;
            break;
        }
        return ProcessStatus::Ready;
    }
};
NativeProcessScheduler::NativeProcessScheduler(std::shared_ptr<ProcessCallbacks> callbacks):impl_(std::make_unique<Impl>()) {
    impl_->callbacks=std::move(callbacks);
    const char* names[]={"HI","DEF","LOW"};
    for(unsigned i=0;i<3;++i)impl_->Create({},ProcessProgram::Default(),std::string(names[i]),false,ProcessType::Base(),impl_->roots[i],true);
}
NativeProcessScheduler::~NativeProcessScheduler()=default;
ProcessHandle NativeProcessScheduler::root(std::uint32_t thread) const {return !impl_->retired && thread<3?impl_->roots[thread]:ProcessHandle{};}
std::optional<ProcessObservation> NativeProcessScheduler::Observe(ProcessHandle h) const {
    if(auto* n=impl_->Get(h))return n->view;return {};
}
ProcessStatus NativeProcessScheduler::Create(ProcessHandle p,std::shared_ptr<const ProcessProgram> d,
    std::optional<std::string> n,bool b,const ProcessType& t,ProcessHandle& h) {
    return impl_->in_run?ProcessStatus::Busy:impl_->Create(p,std::move(d),std::move(n),b,t,h);
}
ProcessStatus NativeProcessScheduler::Next(ProcessHandle h,bool immediate) {return impl_->in_run?ProcessStatus::Busy:impl_->Next(h,immediate);}
ProcessStatus NativeProcessScheduler::Jump(ProcessHandle h,std::uint32_t label,bool immediate) {return impl_->in_run?ProcessStatus::Busy:impl_->Jump(h,label,immediate);}
ProcessStatus NativeProcessScheduler::WaitFrame(ProcessHandle h,std::uint32_t frames) {return impl_->in_run?ProcessStatus::Busy:impl_->Wait(h,frames);}
ProcessStatus NativeProcessScheduler::WaitMilliseconds(ProcessHandle h,std::int32_t value) {return WaitFrame(h,MillisecondFrames(value));}
ProcessHandle NativeProcessScheduler::FindNext(ProcessHandle h,bool children) const {
    auto* n=impl_->Get(h);if(!n)return {};
    for(;;) {
        ProcessHandle next=children && n->view.child?n->view.child:n->view.older;
        if(next) {
            n=impl_->Get(next);if(!n)return {};
            if(!(n->view.flags&1))return next;
            children=true;continue;
        }
        n=impl_->Get(n->view.parent);if(!n)return {};children=false;
    }
}
ProcessHandle NativeProcessScheduler::FindByProgram(std::shared_ptr<const ProcessProgram> p) const {
    if(impl_->retired || !p)return {};
    for(const auto& root:impl_->roots)
        for(auto h=root;h;h=FindNext(h)) {auto* n=impl_->Get(h);if(n->view.program==p && !(n->view.flags&1))return h;}
    return {};
}
ProcessHandle NativeProcessScheduler::FindByName(std::string_view name) const {
    if(impl_->retired)return {};
    auto hash=HashIdentifierExact(name).nameHash;
    if(!name.empty() && name.front()!='\0' && !hash)hash=1;
    // FindByName reads each root before FindNext filters marked descendants.
    // Do not add a string comparison or skip the original root observation.
    for(const auto& root:impl_->roots)
        for(auto h=root;h;h=FindNext(h)) {auto* n=impl_->Get(h);if(n && n->view.name_hash==hash)return h;}
    return {};
}
ProcessStatus NativeProcessScheduler::BeginExec(std::uint32_t thread,std::uint32_t delta) {
    if(impl_->retired)return ProcessStatus::Retired;if(busy())return ProcessStatus::Busy;
    if(thread>=3)return ProcessStatus::InvalidThread;
    impl_->delta=delta;impl_->transitions=0;impl_->Push(Impl::Kind::Exec,impl_->roots[thread]);return ProcessStatus::Ready;
}
ProcessStatus NativeProcessScheduler::BeginSweep() {
    if(impl_->retired)return ProcessStatus::Retired;if(busy())return ProcessStatus::Busy;
    impl_->transitions=0;impl_->stack.emplace_back(Impl::Kind::Sweep);return ProcessStatus::Ready;
}
ProcessStatus NativeProcessScheduler::BeginDelete(ProcessHandle h) {
    if(impl_->retired)return ProcessStatus::Retired;if(busy())return ProcessStatus::Busy;
    auto* n=impl_->Get(h);if(!n || n->view.root || !n->view.linked)return ProcessStatus::InvalidHandle;
    impl_->transitions=0;if(!(n->view.flags&1))impl_->Push(Impl::Kind::Mark,h);return ProcessStatus::Ready;
}
ProcessStatus NativeProcessScheduler::BeginServiceCall(ProcessCall call) {
    if(impl_->retired)return ProcessStatus::Retired;if(busy())return ProcessStatus::Busy;
    if(auto s=impl_->AdmitService(call);s!=ProcessStatus::Ready)return s;
    impl_->transitions=0;impl_->Service(std::move(call));return ProcessStatus::Ready;
}
bool NativeProcessScheduler::UsesCallbacks(const ProcessCallbacks* callbacks) const noexcept {return impl_->callbacks.get()==callbacks;}
bool NativeProcessScheduler::HasType(ProcessHandle h,const ProcessType& type) const noexcept {
    if(impl_->retired)return false;const auto* node=impl_->Get(h);if(!node)return false;
    const auto& actual=node->type.methods;
    if(actual.size()!=type.methods.size())return false;
    for(std::size_t i=0;i<actual.size();++i)if(actual[i].offset!=type.methods[i].offset ||
        actual[i].target!=type.methods[i].target || actual[i].this_adjustment!=type.methods[i].this_adjustment)return false;
    return true;
}
ProcessRunObservation NativeProcessScheduler::Run(std::size_t budget) {
    if(impl_->retired)return {ProcessStatus::Retired,impl_->transitions,{}};
    if(impl_->in_run)return {ProcessStatus::Busy,impl_->transitions,{}};
    impl_->in_run=true;ProcessStatus status=ProcessStatus::Ready;
    try {
        while(budget && !impl_->stack.empty()) {
            --budget;++impl_->transitions;status=impl_->Step(*this);
            if(status!=ProcessStatus::Ready)break;
        }
    } catch(...) {impl_->in_run=false;throw;}
    impl_->in_run=false;
    if(status==ProcessStatus::Ready)status=impl_->stack.empty()?ProcessStatus::Complete:ProcessStatus::WorkBudget;
    std::optional<ProcessCall> callback;
    if(!impl_->stack.empty() && impl_->stack.back().kind==Impl::Kind::Callback)callback=impl_->stack.back().call;
    return {status,impl_->transitions,std::move(callback)};
}
bool NativeProcessScheduler::busy() const noexcept {return impl_->in_run || !impl_->stack.empty();}
void NativeProcessScheduler::Retire() noexcept {
    if(impl_->in_run)return;impl_->retired=true;impl_->stack.clear();impl_->nodes.clear();impl_->roots={};
}
std::optional<ProcessObservation> ProcessAccess::Observe(ProcessHandle h) const {return scheduler_.Observe(h);}
bool ProcessAccess::BelongsTo(const NativeProcessScheduler& s) const noexcept {return &scheduler_==&s;}
std::uint32_t ProcessAccess::frame_delta() const noexcept {return scheduler_.impl_->delta;}
std::uint32_t ProcessAccess::call_result() const noexcept {return scheduler_.impl_->return_value;}
ProcessStatus ProcessAccess::Next(ProcessHandle h,bool i) {return scheduler_.impl_->Next(h,i);}
ProcessStatus ProcessAccess::Jump(ProcessHandle h,std::uint32_t l,bool i) {return scheduler_.impl_->Jump(h,l,i);}
ProcessStatus ProcessAccess::WaitFrame(ProcessHandle h,std::uint32_t f) {return scheduler_.impl_->Wait(h,f);}
ProcessStatus ProcessAccess::Create(ProcessHandle p,std::shared_ptr<const ProcessProgram> d,std::optional<std::string> n,
    bool b,const ProcessType& t,ProcessHandle& h) {return scheduler_.impl_->Create(p,std::move(d),std::move(n),b,t,h);}
}
