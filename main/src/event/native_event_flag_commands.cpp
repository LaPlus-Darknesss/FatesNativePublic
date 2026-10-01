#include "fates/event/native_event_flag_commands.hpp"
#include "fates/runtime/native_runtime.hpp"

namespace fates::event::native {
namespace rn=runtime::native;
namespace {
std::int32_t ComparePrefix(std::string_view name,std::string_view prefix) noexcept {
    for(std::size_t i=0;i<2;++i) {
        const auto a=i<name.size()?static_cast<unsigned char>(name[i]):0u;
        const auto b=static_cast<unsigned char>(prefix[i]);
        // sut::strncmp first rejects unequal leading signed bytes with -1,
        // then enters the library unsigned-byte comparison for equal starts.
        if(a!=b)return i==0?-1:static_cast<std::int32_t>(a)-static_cast<std::int32_t>(b);
        if(a==0)return 0;
    }
    return 0;
}
}
rn::EventFlagStatus NativeEventFlagCommands::Bind(std::shared_ptr<rn::NativeRuntime> runtime,
    std::shared_ptr<NativeEventFlagCommands>& out) {
    if(!runtime)return rn::EventFlagStatus::NullRuntime;
    if(!runtime->PlayerEvents() || !runtime->PlayerEvents()->active())return rn::EventFlagStatus::StalePlayerState;
    auto next=std::shared_ptr<NativeEventFlagCommands>(new NativeEventFlagCommands);
    next->player_=runtime->PlayerEvents();next->runtime_=std::move(runtime);
    out=std::move(next);return rn::EventFlagStatus::Ok;
}
bool NativeEventFlagCommands::Recognizes(std::string_view name) noexcept {
    return name=="ev::FlagEntry" || name=="ev::FlagEntryGlobal" || name=="ev::FlagGet" ||
        name=="ev::FlagSet" || name=="ev::FlagClr";
}
rn::EventFlagStatus NativeEventFlagCommands::Validate() const noexcept {
    if(retired_)return rn::EventFlagStatus::Retired;
    if(runtime_->PlayerEvents()!=player_ || !player_ || !player_->active())return rn::EventFlagStatus::StalePlayerState;
    if(player_ && player_->flags() && !player_->flags()->active())return rn::EventFlagStatus::Retired;
    return rn::EventFlagStatus::Ok;
}
rn::EventFlagStatus NativeEventFlagCommands::Prepare(std::string_view command,std::string_view name,
    PreparedEventFlagCommand& out) const {
    const auto valid=Validate();if(valid!=rn::EventFlagStatus::Ok)return valid;
    if(!Recognizes(command))return rn::EventFlagStatus::UnknownCommand;
    if(!rn::IsValidEventFlagName(name))return rn::EventFlagStatus::InvalidName;
    PreparedEventFlagCommand next;next.owner_=shared_from_this();
    rn::EventFlagOperation operation;
    bool skipped=false;
    if(command=="ev::FlagEntry") {
        skipped=ComparePrefix(name,"G_")==0 || ComparePrefix(name,"S_")==0;
        operation=rn::EventFlagOperation::Entry;
    } else if(command=="ev::FlagEntryGlobal") {
        next.immediate_=ComparePrefix(name,"G_");skipped=next.immediate_!=0;
        operation=rn::EventFlagOperation::EntryGlobal;
    } else if(command=="ev::FlagGet")operation=rn::EventFlagOperation::Get;
    else {
        skipped=ComparePrefix(name,"S_")==0;
        operation=command=="ev::FlagSet"?rn::EventFlagOperation::Set:rn::EventFlagOperation::Clear;
    }
    if(!skipped) {
        if(!player_ || !player_->flags())return rn::EventFlagStatus::MissingBank;
        next.bank_=player_->flags();
        const auto prepared=next.bank_->Prepare(operation,name,next.operation_);
        if(prepared!=rn::EventFlagStatus::Ok)return prepared;
    }
    out=std::move(next);return rn::EventFlagStatus::Ok;
}
rn::EventFlagStatus NativeEventFlagCommands::Commit(PreparedEventFlagCommand& plan,rn::EventFlagResult& out) const noexcept {
    const auto valid=Validate();if(valid!=rn::EventFlagStatus::Ok)return valid;
    if(plan.owner_.get()!=this)return rn::EventFlagStatus::ForeignPlan;
    if(plan.consumed_)return rn::EventFlagStatus::ConsumedPlan;
    if(plan.bank_) {
        const auto committed=plan.bank_->Commit(plan.operation_,out);
        if(committed!=rn::EventFlagStatus::Ok)return committed;
    } else {
        rn::EventFlagResult result;result.kind_=rn::EventFlagResult::Kind::Integer;
        result.integer_=plan.immediate_;out=std::move(result);
    }
    plan.consumed_=true;return rn::EventFlagStatus::Ok;
}
}
