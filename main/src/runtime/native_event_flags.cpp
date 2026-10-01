#include "fates/runtime/native_event_flags.hpp"
#include "fates/event/backend/backend_spine.hpp"
#include <algorithm>
#include <limits>

namespace fates::runtime::native {
namespace eb=event::backend;
bool IsValidEventFlagName(std::string_view name) noexcept {
    return IsValidEventName(name);
}
EventFlagStatus NativeEventFlagBank::Restore(const EventFlagSnapshot& state,std::shared_ptr<NativeEventFlagBank>& out) {
    if(state.names.size()>MaxCapacity || state.bits.size()!=(state.names.size()+7)/8)return EventFlagStatus::InvalidSnapshot;
    std::size_t bytes=0;
    for(const auto& name:state.names)if(name) {
        if(!IsValidEventFlagName(*name))return EventFlagStatus::InvalidName;
        bytes+=name->size()+1;
        if(bytes>16*1024*1024)return EventFlagStatus::InvalidSnapshot;
    }
    auto next=std::shared_ptr<NativeEventFlagBank>(new NativeEventFlagBank);
    next->state_=state;out=std::move(next);return EventFlagStatus::Ok;
}
EventFlagStatus NativeEventFlagBank::Fresh(std::size_t capacity,std::shared_ptr<NativeEventFlagBank>& out) {
    if(capacity>MaxCapacity)return EventFlagStatus::InvalidSnapshot;
    EventFlagSnapshot state;state.names.resize(capacity);state.bits.resize((capacity+7)/8);
    return Restore(state,out);
}
EventFlagStatus NativeEventFlagBank::Snapshot(EventFlagSnapshot& out) const {
    if(retired_)return EventFlagStatus::Retired;
    out=state_;return EventFlagStatus::Ok;
}
std::optional<std::uintptr_t> EventFlagResult::address() const noexcept {
    if(!bank_ || !bank_->active())return {};
    return reinterpret_cast<std::uintptr_t>(bank_->state_.bits.data());
}
EventFlagStatus NativeEventFlagBank::Prepare(EventFlagOperation op,std::string_view name,PreparedEventFlagOperation& out) const {
    if(retired_)return EventFlagStatus::Retired;
    if(op>EventFlagOperation::Reset)return EventFlagStatus::UnknownOperation;
    if(!IsValidEventFlagName(name))return EventFlagStatus::InvalidName;
    PreparedEventFlagOperation plan;plan.bank_=shared_from_this();plan.revision_=revision_;
    if(op<=EventFlagOperation::Clear)plan.result_.kind_=EventFlagResult::Kind::Integer;
    plan.write_=op!=EventFlagOperation::Get && op!=EventFlagOperation::GetIndex;
    if(plan.write_ && revision_==std::numeric_limits<std::uint64_t>::max())return EventFlagStatus::RevisionExhausted;
    if(plan.write_)plan.next_=state_;
    const auto find=[&]() -> std::int32_t {
        for(std::size_t i=0;i<state_.names.size();++i)
            if(state_.names[i] && *state_.names[i]==name)return static_cast<std::int32_t>(i);
        return -1;
    };
    if(op==EventFlagOperation::Entry || op==EventFlagOperation::EntryGlobal) {
        std::vector<const char*> pointers; pointers.reserve(state_.names.size());
        for(const auto& slot:state_.names)pointers.push_back(slot?slot->c_str():nullptr);
        const std::string owned(name);
        const eb::FlagNameTableView view{pointers.data(),pointers.size()};
        const auto index=op==EventFlagOperation::Entry?eb::FlagEntryForward(view,owned.c_str()):eb::FlagEntryReverse(view,owned.c_str());
        if(index>=0) {
            std::size_t bytes=owned.size()+1;
            for(const auto& slot:state_.names)if(slot)bytes+=slot->size()+1;
            if(bytes>16*1024*1024)return EventFlagStatus::InvalidSnapshot;
            plan.next_.names[static_cast<std::size_t>(index)]=owned;
        }
        plan.result_.integer_=index; // registration leaves bits untouched
    } else if(op==EventFlagOperation::GetIndex)plan.result_.integer_=find();
    else if(op==EventFlagOperation::Get)plan.result_.integer_=eb::FlagTest(state_.bits.data(),capacity(),find())?1:0;
    else if(op==EventFlagOperation::Set || op==EventFlagOperation::Clear) {
        const auto index=find();
        if(index<0)plan.result_.integer_=static_cast<std::int32_t>(capacity());
        else {
            if(op==EventFlagOperation::Set)eb::FlagSet(plan.next_.bits.data(),capacity(),index);
            else eb::FlagClear(plan.next_.bits.data(),capacity(),index);
            plan.result_.bank_=shared_from_this();
            plan.result_.kind_=EventFlagResult::Kind::BitsAddress;
        }
    } else if(op==EventFlagOperation::ResetLocal) {
        for(std::size_t i=0;i<capacity() && plan.next_.names[i];++i) {
            plan.next_.names[i].reset();eb::FlagClear(plan.next_.bits.data(),capacity(),static_cast<std::int32_t>(i));
        }
    } else if(op==EventFlagOperation::ClearNotExist) {
        for(std::size_t i=0;i<capacity();++i)if(!plan.next_.names[i])
            eb::FlagClear(plan.next_.bits.data(),capacity(),static_cast<std::int32_t>(i));
    } else {
        for(auto& slot:plan.next_.names)slot.reset();
        std::fill(plan.next_.bits.begin(),plan.next_.bits.end(),std::uint8_t{0});
    }
    out=std::move(plan);return EventFlagStatus::Ok;
}
EventFlagStatus NativeEventFlagBank::Commit(PreparedEventFlagOperation& plan,EventFlagResult& out) noexcept {
    if(retired_)return EventFlagStatus::Retired;
    if(plan.bank_.get()!=this)return EventFlagStatus::ForeignPlan;
    if(plan.consumed_)return EventFlagStatus::ConsumedPlan;
    if(plan.revision_!=revision_)return EventFlagStatus::StalePlan;
    if(plan.write_) {
        if(revision_==std::numeric_limits<std::uint64_t>::max())return EventFlagStatus::RevisionExhausted;
        state_.names.swap(plan.next_.names);
        // Keep the original bit-storage allocation alive for retained VM words.
        std::copy(plan.next_.bits.begin(),plan.next_.bits.end(),state_.bits.begin());
        ++revision_;
    }
    out=std::move(plan.result_);plan.consumed_=true;return EventFlagStatus::Ok;
}
}
