#include "fates/event/native_instant_call.hpp"
#include <algorithm>
#include <limits>

namespace fates::event::native {
InstantCallStatus NativeTypedInstantCall::Validate() const noexcept {
    if(!session_)return InstantCallStatus::NullSession;
    if(session_->retired())return InstantCallStatus::StaleSession;
    if(queries_ && queries_->Validate()!=PhaseQueryStatus::Ok)return InstantCallStatus::StaleNativeContext;
    if(flags_ && flags_->Validate()!=runtime::native::EventFlagStatus::Ok)return InstantCallStatus::StaleNativeContext;
    if(queries_ && flags_ && queries_->runtime()!=flags_->runtime())return InstantCallStatus::MismatchedNativeContext;
    return InstantCallStatus::Ready;
}
InstantCallStatus NativeTypedInstantCall::Create(std::uint32_t type,
    std::shared_ptr<const cmvm::native::ScriptAttachmentSession> session,
    std::shared_ptr<const NativePhaseEventQueries> queries,
    std::shared_ptr<const NativeEventFlagCommands> flags,std::unique_ptr<NativeTypedInstantCall>& out) {
    auto next=std::unique_ptr<NativeTypedInstantCall>(new NativeTypedInstantCall);
    next->session_=std::move(session);next->queries_=std::move(queries);next->flags_=std::move(flags);
    const auto status=next->Validate();if(status!=InstantCallStatus::Ready)return status;
    next->observation_.type=type;out=std::move(next);return InstantCallStatus::Ready;
}
NativeTypedInstantCall::~NativeTypedInstantCall()=default;
void NativeTypedInstantCall::Retire() noexcept {
    if(vm_) {vm_.reset();++observation_.contexts_destroyed;}
    observation_.context_alive=false;observation_.status=InstantCallStatus::Retired;
}
InstantCallObservation NativeTypedInstantCall::Run(std::size_t budget) {
    auto& o=observation_;
    if(o.status!=InstantCallStatus::Ready && o.status!=InstantCallStatus::InstructionBudget &&
       o.status!=InstantCallStatus::VmBarrier)return o;
    if(const auto status=Validate();status!=InstantCallStatus::Ready) {o.status=status;return o;}
    budget=std::min<std::size_t>(budget,1000000);
    const auto room=std::numeric_limits<std::uint64_t>::max()-o.instructions;
    budget=static_cast<std::size_t>(std::min<std::uint64_t>(budget,room));
    while(budget) {
        if(!vm_) {
            // The original compares the full requested word with the stored
            // type byte. Out-of-range values never wrap to a valid event kind.
            if(o.type>255) {o.status=InstantCallStatus::Complete;return o;}
            cmvm::native::AttachedScriptFunction next;
            const auto found=session_->FindNextTyped(static_cast<std::uint8_t>(o.type),o.function?&o.function:nullptr,next);
            if(found==cmvm::native::ScriptSessionStatus::NoMatchInSession) {o.status=InstantCallStatus::Complete;return o;}
            if(found!=cmvm::native::ScriptSessionStatus::Found) {o.status=InstantCallStatus::StaleSession;return o;}
            // Publish the traversal anchor only after the next context exists.
            // Refusal cannot skip a function on a later retry.
            const auto created=PhaseEventVm::CreateAttachedFunction(next,{},ContextWords,session_,queries_,flags_,vm_);
            if(created!=PhaseVmStatus::Ready) {
                PhaseVmObservation barrier;barrier.status=created;barrier.current_function=next.function();
                o.vm=std::move(barrier);o.status=InstantCallStatus::VmBarrier;return o;
            }
            o.function=std::move(next);++o.contexts_started;o.context_alive=true;
        }
        const auto before=vm_->observation();
        const auto after=vm_->Run(budget);
        const auto used=after.instructions-before.instructions;
        budget-=static_cast<std::size_t>(used);o.instructions+=used;
        o.native_flags+=after.native_flags-before.native_flags;
        o.native_queries+=after.native_queries-before.native_queries;
        o.vm=after;
        if(after.status==PhaseVmStatus::Returned || after.status==PhaseVmStatus::Yielded) {
            if(after.status==PhaseVmStatus::Returned)++o.returned;else ++o.yielded;
            vm_.reset();++o.contexts_destroyed;o.context_alive=false;
            // FindNextTyped on the following iteration observes the current
            // attached list. No list of matches is cached before script effects.
        } else if(after.status==PhaseVmStatus::InstructionBudget) {
            o.status=InstantCallStatus::InstructionBudget;return o;
        } else {
            o.status=InstantCallStatus::VmBarrier;return o;
        }
    }
    o.status=InstantCallStatus::InstructionBudget;return o;
}
}
