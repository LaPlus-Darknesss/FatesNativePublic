#include "fates/io/native_async_files.hpp"
#include <algorithm>

namespace fates::io::native {
using namespace runtime::native;
namespace {
constexpr std::uint32_t Pump=0xf1610200u,Finish=0x10c014,Remove=0x12a168;
constexpr std::array Targets{Pump,Finish};
ProcessCall Service(ProcessHandle process,std::uint32_t target,std::initializer_list<std::uint32_t> arguments) {
    ProcessCall call;call.process=std::move(process);call.kind=ProcessCallKind::Service;call.target=target;
    call.argument_count=static_cast<std::uint8_t>(arguments.size());
    std::copy(arguments.begin(),arguments.end(),call.arguments.begin());return call;
}
}
struct NativeAsyncFiles::State {
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<NativeFileController> files;
    std::shared_ptr<NativeFileEntry> entry;
    FileEntryHostServices host;
    bool Live() const {auto owner=scheduler.lock();return owner && owner->root(2);}
};
struct NativeAsyncFiles::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;ProcessCall call;FileControllerHandle controller;FileObjectHandle object;
    unsigned stage{};std::uint32_t alignment{},allocator{},original_data{};std::uint64_t original_revision{};
    ProcessCallbackStep Invoke(std::uint32_t target,std::initializer_list<std::uint32_t> arguments) {
        return ProcessCallbackStep::Call(Service(call.process,target,arguments));
    }
    std::optional<FileObjectObservation> Object() const {
        auto row=state->files->Observe(object);
        return row && row->life==FileObjectLife::Live?row:std::nullopt;
    }
    ProcessCallbackStep FinishRead(ProcessAccess& access) {
        if(stage==0) {
            if(state->files->PrioritizeAsync(controller,object,&access)!=FileControllerStatus::Ready)
                return ProcessCallbackStep::Blocked();
            stage=1;
        }
        if(stage==1) {
            if(!state->host.signal_async || !state->host.signal_async(controller,access))return ProcessCallbackStep::Blocked();
            stage=2;
        }
        if(stage==3) {
            // No queue progress and a still-set mask is an unowned carried
            // worker, not a successful read or a reason to clear those bits.
            auto row=Object();if(!row)return ProcessCallbackStep::Blocked();
            if((row->fields.flags&0x60000000u) && !access.call_result())return ProcessCallbackStep::Blocked();
            stage=2;
        }
        if(stage==2) {
            auto row=Object();if(!row)return ProcessCallbackStep::Blocked();
            if(row->fields.flags&0x60000000u) {
                stage=3;return ProcessCallbackStep::Call(NativeAsyncFiles::PumpCall(call.process));
            }
            stage=4;
        }
        // CTR priority/sleep/wake is replaced by explicit cooperative service
        // scheduling. Signal ordering remains visible at the existing host seam.
        if(!state->host.signal_async || !state->host.signal_async(controller,access))return ProcessCallbackStep::Blocked();
        return ProcessCallbackStep::Return();
    }
    ProcessCallbackStep Step(ProcessAccess& access) override {
        auto scheduler=state->scheduler.lock();
        if(!scheduler || !access.BelongsTo(*scheduler))return ProcessCallbackStep::Blocked();
        if(call.target==Finish)return FinishRead(access);
        if(stage==0) {
            const auto current=state->files->Observe();
            if(!current || !current->present)return ProcessCallbackStep::Blocked();
            controller=current->identity;
            int best=-1;
            for(const auto& handle:current->async) {
                auto row=state->files->Observe(handle);
                if(!row || row->life!=FileObjectLife::Live || !row->async_linked)return ProcessCallbackStep::Blocked();
                // An abandoned or externally running read needs its own
                // completion; a second native iteration cannot restart it.
                if(row->fields.flags&0x40000000u)return ProcessCallbackStep::Blocked();
                if(static_cast<int>(row->fields.priority)>best) {
                    best=row->fields.priority;object=handle;
                }
            }
            if(!object)return ProcessCallbackStep::Return(0);
            auto row=Object();if(!row)return ProcessCallbackStep::Blocked();
            original_data=row->fields.data;original_revision=row->data_revision;
            auto fields=row->fields;fields.flags|=0x40000000u;
            if(state->files->WriteFields(object,fields,&access)!=FileControllerStatus::Ready)return ProcessCallbackStep::Blocked();
            stage=1;
        }
        if(stage==1) {
            auto row=Object();if(!row || !row->fields.get_align)return ProcessCallbackStep::Blocked();
            stage=2;return Invoke(row->fields.get_align,{object->serial});
        }
        if(stage==2) {alignment=access.call_result();stage=3;}
        if(stage==3) {
            auto row=Object();if(!row || !row->fields.get_allocator)return ProcessCallbackStep::Blocked();
            stage=4;return Invoke(row->fields.get_allocator,{object->serial});
        }
        if(stage==4) {allocator=access.call_result();stage=5;}
        if(stage==5) {
            auto row=Object();if(!row || !row->async_linked || row->controller!=controller ||
                row->data_revision!=original_revision || row->fields.data!=original_data ||
                !(row->fields.flags&0x40000000u) || !state->host.read)return ProcessCallbackStep::Blocked();
            const auto result=state->host.read({row->fields.path,allocator,alignment,true},access);
            if(!result)return ProcessCallbackStep::Blocked();
            if(state->entry->PublishAsyncResult(object,*result,access)!=FileEntryStatus::Ready)
                return ProcessCallbackStep::Blocked();
            stage=6;
        }
        if(stage==6) {stage=7;return Invoke(Remove,{controller->serial,object->serial});}
        auto row=Object();if(!row || row->async_linked)return ProcessCallbackStep::Blocked();
        auto fields=row->fields;fields.flags=(fields.flags&~0x60000000u)|0x80000000u;
        if(state->files->WriteFields(object,fields,&access)!=FileControllerStatus::Ready)return ProcessCallbackStep::Blocked();
        return ProcessCallbackStep::Return(1);
    }
};
NativeAsyncFiles::NativeAsyncFiles(std::shared_ptr<State> state):state_(std::move(state)){}
NativeAsyncFiles::~NativeAsyncFiles()=default;
AsyncFileStatus NativeAsyncFiles::Create(std::shared_ptr<NativeProcessScheduler> scheduler,
    std::shared_ptr<ProcessCallbackRegistry> registry,std::shared_ptr<NativeFileController> files,
    std::shared_ptr<NativeFileBase> bases,std::shared_ptr<NativeFileEntry> entry,FileEntryHostServices host,
    std::shared_ptr<NativeAsyncFiles>& output) {
    using S=AsyncFileStatus;
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !files || !files->UsesScheduler(*scheduler) ||
        !bases || !entry || !entry->UsesOwners(*files,*bases))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->files=std::move(files);
    state->entry=std::move(entry);state->host=std::move(host);
    auto module=std::shared_ptr<NativeAsyncFiles>(new NativeAsyncFiles(state));
    if(!registry->Register(Targets,module))return S::DuplicateBinding;
    output=std::move(module);return S::Ready;
}
ProcessCall NativeAsyncFiles::PumpCall(ProcessHandle process) {return Service(std::move(process),Pump,{});}
bool NativeAsyncFiles::UsesOwners(const NativeFileController& files,const NativeFileEntry& entry) const noexcept {
    return state_->files.get()==&files && state_->entry.get()==&entry;
}
std::unique_ptr<ProcessContinuation> NativeAsyncFiles::Begin(const ProcessCall& call) {
    if(!state_->Live() || call.kind!=ProcessCallKind::Service || call.has_self || call.this_adjustment || !call.process ||
        std::find(Targets.begin(),Targets.end(),call.target)==Targets.end() ||
        call.argument_count!=(call.target==Pump?0u:2u))return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->call=call;
    if(call.target==Finish) {
        const auto current=state_->files->Observe();
        if(!current || !current->present || current->identity->serial!=call.arguments[0])return {};
        next->controller=current->identity;next->object=state_->files->ResolveObject(call.arguments[1]);
        if(!next->Object())return {};
    }
    return next;
}
}
