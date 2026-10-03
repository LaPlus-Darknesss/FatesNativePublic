#include "fates/runtime/native_message_files.hpp"
#include <algorithm>
#include <limits>
#include <map>

namespace fates::runtime::native {
namespace fn=io::native;
namespace {
constexpr std::uint32_t Initialize=0x3d15cc,Finalize=0x3d1f60,IsFileLoad=0x3d15f0,
    Load=0x3d1eac,Free=0x3d1e18,LoadRoute=0x3d1874,FreeRoute=0x3d179c,ExistsRoute=0x3d1984;
constexpr std::array Targets{Initialize,Finalize,IsFileLoad,Load,Free,LoadRoute,FreeRoute,ExistsRoute};
ProcessCall Service(ProcessHandle h,std::uint32_t target,std::optional<std::uint32_t> name={}) {
    ProcessCall c;c.process=std::move(h);c.kind=ProcessCallKind::Service;c.target=target;
    if(name) {c.arguments[0]=*name;c.argument_count=1;}return c;
}
}
struct NativeMessageFiles::State {
    std::shared_ptr<NativeRuntime> runtime;
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<fn::NativeFileBase> bases;
    std::shared_ptr<fn::NativeGlobalFiles> globals;
    std::function<fn::FileSourceStatus(std::string_view)> exists;
    std::unique_ptr<fn::NativeLanguagePaths> paths;
    fn::NativeLanguageState language;
    std::shared_ptr<NativeIdentifierHolder> holder;
    std::map<std::uint32_t,MessageArchiveName> names;
    std::uint32_t serial{};
    bool Live() const {auto s=scheduler.lock();return s && s->root(2);}
    MessageFileStatus Mutable(ProcessAccess* a) const {
        auto s=scheduler.lock();if(!s || !s->root(2))return MessageFileStatus::Retired;
        if(a)return a->BelongsTo(*s)?MessageFileStatus::Ready:MessageFileStatus::MismatchedDomain;
        return s->busy()?MessageFileStatus::Busy:MessageFileStatus::Ready;
    }
    MessageArchiveName Name(std::uint32_t id) const {auto at=names.find(id);return at==names.end()?nullptr:at->second;}
    bool NameValid(MessageArchiveName n) const {return n && Name(n->serial)==n;}
};
struct NativeMessageFiles::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;ProcessCall call;MessageArchiveName name;
    std::optional<std::string> route;std::string path;fn::FilePathHandle path_handle;
    unsigned stage{};bool route_call{},load{},query{},fallback{},common{};
    ProcessCallbackStep Step(ProcessAccess& a) override {
        auto scheduler=state->scheduler.lock();if(!scheduler || !a.BelongsTo(*scheduler))return ProcessCallbackStep::Blocked();
        if(call.target==Initialize) {
            // Replacement does not free any previously loaded GlobalFile data.
            // Unreachable holder record/name storage is ordinary host storage.
            state->holder=std::make_shared<NativeIdentifierHolder>();return ProcessCallbackStep::Return();
        }
        if(call.target==Finalize) {
            if(state->holder)state->holder->Retire();state->holder.reset();
            return ProcessCallbackStep::Return();
        }
        if(call.target==IsFileLoad) {
            if(!state->holder)return ProcessCallbackStep::Blocked();
            const auto result=state->holder->Find(name?std::optional<std::string_view>(name->text):std::nullopt);
            return ProcessCallbackStep::Return(result.status==IdentifierHolderStatus::Ready?1u:0u);
        }
        if(stage==0) {
            // Original reads GameUserData+2E and the route directory BEFORE
            // Bind/Unbind, including calls which do no further work.
            if(route_call) {
                const auto value=CurrentCarriedRoute(*state->runtime);
                if(!value)return ProcessCallbackStep::Blocked();
                route=std::string(1,static_cast<char>('A'+*value));
            }
            stage=1;
        }
        if(stage==1) {
            if(!query) {
                if(!state->holder)return ProcessCallbackStep::Blocked();
                const auto result=load?state->holder->Bind(name->text):state->holder->Unbind(name->text);
                if(result.status!=IdentifierHolderStatus::Ready)return ProcessCallbackStep::Blocked();
                if(!result.transition)return ProcessCallbackStep::Return();
            }
            stage=2;
        }
        if(stage==2) {
            const auto result=common?state->paths->CreateCommonMessageName(name->text):
                state->paths->CreateMessageName(route && !fallback?std::optional<std::string_view>(*route):std::nullopt,name->text,state->language);
            if(result.status!=fn::LanguagePathStatus::Ready)return ProcessCallbackStep::Blocked();
            path=result.path;path_handle.reset();stage=3;
        }
        if(stage==3) {
            if(!state->exists)return ProcessCallbackStep::Blocked();
            const auto available=state->exists(path);
            if(available==fn::FileSourceStatus::Unavailable)return ProcessCallbackStep::Blocked();
            if(available==fn::FileSourceStatus::Missing) {
                if(!common && !fallback && route_call) {fallback=true;stage=2;return ProcessCallbackStep::Continue();}
                return ProcessCallbackStep::Return(0);
            }
            if(query)return ProcessCallbackStep::Return(1);
            stage=4;
        }
        if(stage==4) {
            if(!path_handle && state->bases->RegisterPath(path,path_handle,&a)!=fn::FileBaseStatus::Ready)
                return ProcessCallbackStep::Blocked();
            const auto c=load?state->globals->LoadCall(call.process,fn::GlobalFileMode::Archive,path_handle):
                state->globals->FreeCall(call.process,fn::GlobalFileMode::Archive,path_handle);
            if(!c)return ProcessCallbackStep::Blocked();stage=5;return ProcessCallbackStep::Call(*c);
        }
        if(stage==5) {
            if(load) {
                // The original reloads the CURRENT holder global after load.
                if(!state->holder || state->holder->SetPointer(name->text,a.call_result())!=IdentifierHolderStatus::Ready)
                    return ProcessCallbackStep::Blocked();
            }
            if(!common && !route) {common=true;stage=2;return ProcessCallbackStep::Continue();}
            return ProcessCallbackStep::Return();
        }
        return ProcessCallbackStep::Blocked();
    }
};
NativeMessageFiles::NativeMessageFiles(std::shared_ptr<State> s):state_(std::move(s)){}
NativeMessageFiles::~NativeMessageFiles()=default;
MessageFileStatus NativeMessageFiles::Create(std::shared_ptr<NativeRuntime> runtime,std::shared_ptr<NativeProcessScheduler> scheduler,
    std::shared_ptr<ProcessCallbackRegistry> callbacks,std::shared_ptr<fn::NativeFileBase> bases,
    std::shared_ptr<fn::NativeGlobalFiles> globals,std::function<fn::FileSourceStatus(std::string_view)> exists,
    std::shared_ptr<NativeMessageFiles>& out) {
    using S=MessageFileStatus;if(!runtime || !scheduler || !scheduler->root(2))return S::NullOwner;
    if(!callbacks || !scheduler->UsesCallbacks(callbacks.get()) || !bases || !globals || !globals->UsesOwners(*scheduler,*bases))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->runtime=std::move(runtime);state->scheduler=scheduler;
    state->bases=std::move(bases);state->globals=std::move(globals);state->exists=std::move(exists);
    state->paths=std::make_unique<fn::NativeLanguagePaths>(state->exists);
    auto module=std::shared_ptr<NativeMessageFiles>(new NativeMessageFiles(state));
    if(!callbacks->Register(Targets,module))return S::DuplicateBinding;
    out=std::move(module);return S::Ready;
}
MessageFileStatus NativeMessageFiles::RegisterName(std::string_view text,MessageArchiveName& out,ProcessAccess* a) {
    using S=MessageFileStatus;if(auto status=state_->Mutable(a);status!=S::Ready)return status;
    if(text.size()>65535 || text.find('\0')!=std::string_view::npos)return S::InvalidName;
    if(state_->serial==std::numeric_limits<std::uint32_t>::max())return S::IdentityExhausted;
    auto name=std::make_shared<MessageArchiveNameIdentity>(++state_->serial,std::string(text));
    state_->names.emplace(name->serial,name);out=std::move(name);return S::Ready;
}
MessageFileStatus NativeMessageFiles::PublishLanguage(fn::NativeLanguageState language,ProcessAccess* a) {
    auto status=state_->Mutable(a);if(status==MessageFileStatus::Ready)state_->language=language;return status;
}
std::optional<MessageFileObservation> NativeMessageFiles::Observe() const {
    if(!state_->Live())return {};
    MessageFileObservation out;out.initialized=bool(state_->holder);if(state_->holder)out.bindings=state_->holder->Observe();
    const auto language=state_->paths->language_buffer(),message=state_->paths->message_buffer();
    std::copy(language.begin(),language.end(),out.language_buffer.begin());std::copy(message.begin(),message.end(),out.message_buffer.begin());return out;
}
ProcessCall NativeMessageFiles::InitializeCall(ProcessHandle p) {return Service(std::move(p),Initialize);}
ProcessCall NativeMessageFiles::FinalizeCall(ProcessHandle p) {return Service(std::move(p),Finalize);}
std::optional<ProcessCall> NativeMessageFiles::LoadCall(ProcessHandle p,MessageArchiveName n,bool route) const {
    if(!state_->Live() || !state_->NameValid(n))return {};return Service(std::move(p),route?LoadRoute:Load,n->serial);
}
std::optional<ProcessCall> NativeMessageFiles::FreeCall(ProcessHandle p,MessageArchiveName n,bool route) const {
    if(!state_->Live() || !state_->NameValid(n))return {};return Service(std::move(p),route?FreeRoute:Free,n->serial);
}
std::optional<ProcessCall> NativeMessageFiles::IsFileLoadCall(ProcessHandle p,MessageArchiveName n) const {
    if(!state_->Live() || (n && !state_->NameValid(n)))return {};return Service(std::move(p),IsFileLoad,n?n->serial:0);
}
std::optional<ProcessCall> NativeMessageFiles::IsFileExistRouteCall(ProcessHandle p,MessageArchiveName n) const {
    if(!state_->Live() || !state_->NameValid(n))return {};return Service(std::move(p),ExistsRoute,n->serial);
}
std::unique_ptr<ProcessContinuation> NativeMessageFiles::Begin(const ProcessCall& c) {
    if(!state_->Live() || c.kind!=ProcessCallKind::Service || c.has_self || c.this_adjustment || !c.process ||
        std::find(Targets.begin(),Targets.end(),c.target)==Targets.end())return {};
    const bool global=c.target==Initialize || c.target==Finalize;
    if(c.argument_count!=(global?0u:1u))return {};
    auto name=global?nullptr:state_->Name(c.arguments[0]);if(!global && !name && (c.target!=IsFileLoad || c.arguments[0]))return {};
    auto p=std::make_unique<Continuation>();p->state=state_;p->call=c;p->name=std::move(name);
    p->route_call=c.target==LoadRoute || c.target==FreeRoute || c.target==ExistsRoute;
    p->load=c.target==Load || c.target==LoadRoute;p->query=c.target==ExistsRoute;return p;
}
}
