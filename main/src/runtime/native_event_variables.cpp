#include "fates/runtime/native_event_variables.hpp"
#include "fates/event/backend/backend_spine.hpp"
#include <algorithm>
#include <limits>

namespace fates::runtime::native {
namespace eb=event::backend;
EventVariableStatus NativeEventVariableBank::Restore(const EventVariableSnapshot& state,std::shared_ptr<NativeEventVariableBank>& out) {
    if(state.names.size()>MaxCapacity || state.values.size()!=state.names.size())return EventVariableStatus::InvalidSnapshot;
    std::size_t bytes=0;
    for(const auto& name:state.names)if(name) {
        if(!IsValidEventName(*name))return EventVariableStatus::InvalidName;
        bytes+=name->size()+1;
        if(bytes>16*1024*1024)return EventVariableStatus::InvalidSnapshot;
    }
    auto next=std::shared_ptr<NativeEventVariableBank>(new NativeEventVariableBank);
    next->state_=state;out=std::move(next);return EventVariableStatus::Ok;
}
EventVariableStatus NativeEventVariableBank::Fresh(std::size_t capacity,std::shared_ptr<NativeEventVariableBank>& out) {
    if(capacity>MaxCapacity)return EventVariableStatus::InvalidSnapshot;
    EventVariableSnapshot state;state.names.resize(capacity);state.values.resize(capacity);
    return Restore(state,out);
}
EventVariableStatus NativeEventVariableBank::Snapshot(EventVariableSnapshot& out) const {
    if(retired_)return EventVariableStatus::Retired;
    out=state_;return EventVariableStatus::Ok;
}
std::optional<std::uintptr_t> EventVariableResult::address() const noexcept {
    if(!bank_ || !bank_->active())return {};
    return reinterpret_cast<std::uintptr_t>(bank_->state_.values.data());
}
EventVariableStatus NativeEventVariableBank::Prepare(EventVariableOperation op,std::string_view name,std::int32_t value,PreparedEventVariableOperation& out) const {
    if(retired_)return EventVariableStatus::Retired;
    if(op>EventVariableOperation::Reset)return EventVariableStatus::UnknownOperation;
    if(!IsValidEventName(name))return EventVariableStatus::InvalidName;
    PreparedEventVariableOperation plan;plan.bank_=shared_from_this();plan.revision_=revision_;
    if(op<=EventVariableOperation::Add)plan.result_.kind_=EventVariableResult::Kind::Integer;
    plan.write_=op!=EventVariableOperation::Get && op!=EventVariableOperation::GetIndex;
    if(plan.write_ && revision_==std::numeric_limits<std::uint64_t>::max())return EventVariableStatus::RevisionExhausted;
    if(plan.write_)plan.next_=state_;
    std::vector<const char*> pointers;pointers.reserve(capacity());
    for(const auto& slot:state_.names)pointers.push_back(slot?slot->c_str():nullptr);
    const eb::NamedVariableTableView lookup{pointers.data(),nullptr,capacity()};
    if(op==EventVariableOperation::Entry || op==EventVariableOperation::EntryGlobal) {
        const std::string owned(name);
        const eb::FlagNameTableView view{pointers.data(),capacity()};
        const auto index=op==EventVariableOperation::Entry?eb::FlagEntryForward(view,owned.c_str()):eb::FlagEntryReverse(view,owned.c_str());
        if(index>=0) {
            std::size_t bytes=owned.size()+1;
            for(const auto& slot:state_.names)if(slot)bytes+=slot->size()+1;
            if(bytes>16*1024*1024)return EventVariableStatus::InvalidSnapshot;
            plan.next_.names[static_cast<std::size_t>(index)]=owned;
        }
        plan.result_.integer_=index;
    } else if(op==EventVariableOperation::GetIndex)plan.result_.integer_=eb::VariableFind(lookup,name);
    else if(op==EventVariableOperation::Get) {
        const auto index=eb::VariableFind(lookup,name);
        plan.result_.integer_=index<0?0:state_.values[static_cast<std::size_t>(index)];
    } else if(op==EventVariableOperation::Set || op==EventVariableOperation::Add) {
        const eb::NamedVariableTableView view{pointers.data(),plan.next_.values.data(),capacity()};
        const bool found=op==EventVariableOperation::Set?eb::VariableSet(view,name,value):eb::VariableAdd(view,name,value);
        if(!found)plan.result_.integer_=static_cast<std::int32_t>(capacity());
        else {
            plan.result_.kind_=EventVariableResult::Kind::ValuesAddress;
            plan.result_.bank_=shared_from_this();
        }
    } else if(op==EventVariableOperation::ResetLocal) {
        for(std::size_t i=0;i<capacity() && plan.next_.names[i];++i) {
            plan.next_.names[i].reset();plan.next_.values[i]=0;
        }
    } else if(op==EventVariableOperation::ClearNotExist) {
        for(std::size_t i=0;i<capacity();++i)if(!plan.next_.names[i])plan.next_.values[i]=0;
    } else {
        for(auto& slot:plan.next_.names)slot.reset();
        std::fill(plan.next_.values.begin(),plan.next_.values.end(),std::int32_t{0});
    }
    out=std::move(plan);return EventVariableStatus::Ok;
}
EventVariableStatus NativeEventVariableBank::Commit(PreparedEventVariableOperation& plan,EventVariableResult& out) noexcept {
    if(retired_)return EventVariableStatus::Retired;
    if(plan.bank_.get()!=this)return EventVariableStatus::ForeignPlan;
    if(plan.consumed_)return EventVariableStatus::ConsumedPlan;
    if(plan.revision_!=revision_)return EventVariableStatus::StalePlan;
    if(plan.write_) {
        if(revision_==std::numeric_limits<std::uint64_t>::max())return EventVariableStatus::RevisionExhausted;
        state_.names.swap(plan.next_.names);
        // Retained VM words must keep the original allocation through writes.
        std::copy(plan.next_.values.begin(),plan.next_.values.end(),state_.values.begin());
        ++revision_;
    }
    out=std::move(plan.result_);plan.consumed_=true;return EventVariableStatus::Ok;
}
}
