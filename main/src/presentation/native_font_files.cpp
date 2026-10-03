#include "fates/presentation/native_font_files.hpp"
#include <algorithm>

namespace fates::presentation::native {
using namespace runtime::native;
using namespace io::native;
namespace {
constexpr std::uint32_t Load=0x3cfbac,Free=0x3cfb50,Finish=0x3cfa44,IsAsync=0x3cfa90;
constexpr std::array Targets{Load,Free,Finish,IsAsync};
ProcessCall Service(ProcessHandle process,std::uint32_t target,std::initializer_list<std::uint32_t> args) {
    ProcessCall call;call.process=std::move(process);call.kind=ProcessCallKind::Service;call.target=target;
    call.argument_count=static_cast<std::uint8_t>(args.size());std::copy(args.begin(),args.end(),call.arguments.begin());return call;
}
}
struct NativeFontFiles::State {
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<NativeFileBase> bases;std::shared_ptr<NativeFileEntry> entry;
    std::shared_ptr<NativeFontMetrics> metrics;std::shared_ptr<NativeFontObjects> objects;
    std::optional<std::array<FileBaseHandle,4>> slots;
    bool Live() const {const auto owner=scheduler.lock();return owner && owner->root(2);}
    FontFileStatus Mutable(ProcessAccess* access) const {
        const auto owner=scheduler.lock();if(!owner || !owner->root(2))return FontFileStatus::Retired;
        if(access)return access->BelongsTo(*owner)?FontFileStatus::Ready:FontFileStatus::MismatchedDomain;
        return owner->busy()?FontFileStatus::Busy:FontFileStatus::Ready;
    }
};
struct NativeFontFiles::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;ProcessCall call;FileBaseHandle base;FilePathHandle input,path;
    FileObjectHandle object;FileEntryHandle request;std::uint32_t file_type{},read_type{};unsigned stage{};
    ProcessCallbackStep Block() const {return ProcessCallbackStep::Blocked();}
    ProcessCallbackStep Step(ProcessAccess& access) override {
        const auto owner=state->scheduler.lock();if(!owner || !access.BelongsTo(*owner))return Block();
        const auto slot=call.arguments[0];
        if(stage==0) {
            if(call.target==Load) {
                if(!call.arguments[1])return ProcessCallbackStep::Return();
                // The original formats before checking the complete slot index.
                // Stable NUL-free capability represents the admitted char* input.
                input=state->bases->ResolvePath(call.arguments[1]);if(!input)return Block();
                std::string formatted="font/";formatted.append(input->text,0,74);
                if(state->bases->RegisterPath(formatted,path,&access)!=FileBaseStatus::Ready)return Block();
                read_type=call.arguments[2]?1u:0u;
            }
            stage=1;
        }
        if(stage==1) {
            if(slot>=4) {
                if(call.target==Finish || call.target==IsAsync)return ProcessCallbackStep::Return();
                stage=7;
            } else {
                if(!state->slots)return Block();
                base=(*state->slots)[slot];if(!state->bases->Observe(base))return Block();
                std::optional<ProcessCall> child;
                if(call.target==Free)child=state->bases->FreeCall(call.process,base);
                else if(call.target==Finish)child=state->bases->FinishCall(call.process,base);
                else if(call.target==IsAsync)child=Service(call.process,0x545dd0,{base->serial});
                else child=state->bases->OpenCall(call.process,base,path,read_type);
                if(!child)return Block();
                stage=2;return ProcessCallbackStep::Call(*child);
            }
        }
        if(stage==2) {
            if(call.target==Finish || call.target==IsAsync)return ProcessCallbackStep::Return(access.call_result());
            if(call.target==Free)stage=7;
            else {file_type=access.call_result();stage=file_type?5u:3u;}
        }
        if(stage==3) {
            if(state->objects->Construct(object,&access)!=FontObjectStatus::Ready)return Block();
            stage=4;
        }
        if(stage==4) {
            // Font::Load passes flags0 (not TexFile's common flags2).
            if(!request && state->entry->Prepare(base,object,path,0,read_type,request,&access)!=FileEntryStatus::Ready)return Block();
            const auto child=state->entry->EntryCall(call.process,request);if(!child)return Block();
            stage=5;return ProcessCallbackStep::Call(*child);
        }
        if(stage==5) {
            const auto child=state->bases->CloseCall(call.process,base,file_type,read_type);if(!child)return Block();
            stage=6;return ProcessCallbackStep::Call(*child);
        }
        if(stage==6)stage=7;
        if(stage==7) {
            // Refresh ONLY when the current low-byte selector equals the full
            // requested slot. Reread after all file work, not before Open/Free.
            const auto selection=state->metrics->Selection();if(!selection || !selection->current_type)return Block();
            if(slot!=*selection->current_type)return ProcessCallbackStep::Return();
            stage=8;return ProcessCallbackStep::Call(NativeFontMetrics::SetCurrentCall(call.process,slot));
        }
        return ProcessCallbackStep::Return();
    }
};
NativeFontFiles::NativeFontFiles(std::shared_ptr<State> value):state_(std::move(value)){}
NativeFontFiles::~NativeFontFiles()=default;
FontFileStatus NativeFontFiles::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<NativeFileController> files,std::shared_ptr<NativeFileBase> bases,std::shared_ptr<NativeFileEntry> entry,
    std::shared_ptr<NativeFontMetrics> metrics,std::shared_ptr<NativeFontObjects> objects,std::shared_ptr<NativeFontFiles>& output) {
    if(!scheduler || !scheduler->root(2))return FontFileStatus::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !files || !files->UsesScheduler(*scheduler) || !bases ||
       !entry || !entry->UsesOwners(*files,*bases) || !metrics || !metrics->UsesOwners(*files,*entry) ||
       !objects || !objects->UsesOwners(*files,*entry,*metrics))return FontFileStatus::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->bases=std::move(bases);state->entry=std::move(entry);
    state->metrics=std::move(metrics);state->objects=std::move(objects);
    auto module=std::shared_ptr<NativeFontFiles>(new NativeFontFiles(state));
    if(!registry->Register(Targets,module))return FontFileStatus::DuplicateBinding;
    output=std::move(module);return FontFileStatus::Ready;
}
FontFileStatus NativeFontFiles::BindSlots(const std::array<FileBaseHandle,4>& slots,ProcessAccess* access) {
    if(const auto status=state_->Mutable(access);status!=FontFileStatus::Ready)return status;
    if(state_->metrics->BindFileSlots(slots,access)!=FontMetricStatus::Ready)return FontFileStatus::InvalidSlot;
    state_->slots=slots;return FontFileStatus::Ready;
}
std::optional<FileBaseObservation> NativeFontFiles::ObserveSlot(std::uint32_t slot) const {
    if(!state_->Live() || slot>=4 || !state_->slots)return {};
    return state_->bases->Observe((*state_->slots)[slot]);
}
std::optional<ProcessCall> NativeFontFiles::LoadCall(ProcessHandle process,std::uint32_t slot,FilePathHandle name,bool async) const {
    if(!state_->Live() || (name && state_->bases->ResolvePath(name->serial)!=name))return {};
    return Service(std::move(process),Load,{slot,name?name->serial:0u,async?1u:0u});
}
ProcessCall NativeFontFiles::FreeCall(ProcessHandle process,std::uint32_t slot){return Service(std::move(process),Free,{slot});}
ProcessCall NativeFontFiles::FinishAsyncCall(ProcessHandle process,std::uint32_t slot){return Service(std::move(process),Finish,{slot});}
ProcessCall NativeFontFiles::IsAsyncLoadingCall(ProcessHandle process,std::uint32_t slot){return Service(std::move(process),IsAsync,{slot});}
bool NativeFontFiles::UsesOwners(const NativeFileBase& bases,const NativeFontMetrics& metrics,const NativeFontObjects& objects) const noexcept {
    return state_->bases.get()==&bases && state_->metrics.get()==&metrics && state_->objects.get()==&objects;
}
std::unique_ptr<ProcessContinuation> NativeFontFiles::Begin(const ProcessCall& call) {
    if(!state_->Live() || !call.process || call.kind!=ProcessCallKind::Service || call.has_self || call.this_adjustment ||
       std::find(Targets.begin(),Targets.end(),call.target)==Targets.end() || call.argument_count!=(call.target==Load?3u:1u))return {};
    if(call.target==Load && call.arguments[1] && !state_->bases->ResolvePath(call.arguments[1]))return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->call=call;return next;
}
}
