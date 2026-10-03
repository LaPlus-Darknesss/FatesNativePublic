#include "fates/runtime/native_identifier_holder.hpp"
#include <limits>

namespace fates::runtime::native {
IdentifierHolderStatus NativeIdentifierHolder::Validate(std::optional<std::string_view> name) const {
    if(retired_)return IdentifierHolderStatus::Retired;
    if(name && (name->size()>65535 || name->find('\0')!=std::string_view::npos))return IdentifierHolderStatus::InvalidIdentifier;
    return IdentifierHolderStatus::Ready;
}
std::shared_ptr<NativeIdentifierHolder::Entry> NativeIdentifierHolder::Get(std::optional<std::string_view> name) const {
    if(!name)return {};
    const auto entry=index_.Find(std::string(*name).c_str());return entry?entry->value:nullptr;
}
std::shared_ptr<NativeIdentifierHolder::Entry> NativeIdentifierHolder::Get(IdentifierBindingHandle h) const {
    if(!h)return {};const auto at=identities_.find(h->serial);
    return at!=identities_.end() && at->second->row.identity==h?at->second:nullptr;
}
IdentifierBindingResult NativeIdentifierHolder::Bind(std::optional<std::string_view> name) {
    using S=IdentifierHolderStatus;if(const auto status=Validate(name);status!=S::Ready)return {status};
    auto entry=Get(name);
    if(!entry) {
        if(serial_==std::numeric_limits<std::uint32_t>::max())return {S::IdentityExhausted};
        auto next=std::make_shared<Entry>();next->row.identity=std::make_shared<IdentifierBindingIdentity>(serial_+1);
        if(name)next->row.identifier=std::string(*name);
        entries_.push_back(next);++count_;identities_.emplace(++serial_,next);
        index_.Append(next->row.identifier?next->row.identifier->c_str():nullptr,next);entry=std::move(next);
    }
    const auto previous=entry->row.count++;
    return {S::Ready,previous==0,entry->row.identity};
}
IdentifierBindingResult NativeIdentifierHolder::Unbind(std::optional<std::string_view> name) {
    using S=IdentifierHolderStatus;if(const auto status=Validate(name);status!=S::Ready)return {status};
    auto entry=Get(name);if(!entry)return {S::Ready};
    const auto previous=entry->row.count;
    const auto decremented=previous-1u;
    // BindManager compares the wrapped result as SIGNED, then clamps to zero.
    entry->row.count=(decremented&0x80000000u)?0u:decremented;
    const auto identity=entry->row.identity;
    if(previous==1) {
        index_.EraseFirst(entry->row.identifier?entry->row.identifier->c_str():nullptr);
        entries_.remove(entry);--count_;identities_.erase(identity->serial);
    }
    return {S::Ready,previous==1,identity};
}
IdentifierHolderStatus NativeIdentifierHolder::SetPointer(std::optional<std::string_view> name,std::uint32_t value) {
    using S=IdentifierHolderStatus;if(const auto status=Validate(name);status!=S::Ready)return status;
    if(const auto entry=Get(name))entry->row.pointer_word=value;
    return S::Ready;
}
IdentifierHolderStatus NativeIdentifierHolder::RestoreCount(IdentifierBindingHandle identity,std::uint32_t count) {
    using S=IdentifierHolderStatus;if(retired_)return S::Retired;
    const auto entry=Get(identity);if(!entry)return S::InvalidHandle;
    entry->row.count=count;return S::Ready;
}
IdentifierBindingResult NativeIdentifierHolder::Find(std::optional<std::string_view> name) const {
    using S=IdentifierHolderStatus;if(const auto status=Validate(name);status!=S::Ready)return {status};
    const auto entry=Get(name);return entry?IdentifierBindingResult{S::Ready,false,entry->row.identity}:IdentifierBindingResult{S::Missing};
}
std::optional<IdentifierBindingObservation> NativeIdentifierHolder::Observe(IdentifierBindingHandle identity) const {
    const auto entry=Get(identity);return !retired_ && entry?std::optional(entry->row):std::nullopt;
}
std::optional<IdentifierHolderObservation> NativeIdentifierHolder::Observe() const {
    if(retired_)return {};
    IdentifierHolderObservation result;result.count=count_;result.index_size=index_.Size();result.index_accounting_count=index_.AccountingCount();
    for(const auto& entry:entries_)result.entries.push_back(entry->row);return result;
}
void NativeIdentifierHolder::Retire() noexcept {
    if(retired_)return;
    index_.Clear();entries_.clear();identities_.clear();count_=0;retired_=true;
}
}
