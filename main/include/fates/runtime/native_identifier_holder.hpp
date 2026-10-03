#pragma once
#include "fates/runtime/native_identifier.hpp"
#include <list>
#include <map>
#include <memory>
#include <optional>

namespace fates::runtime::native {
struct IdentifierBindingIdentity final {const std::uint32_t serial;};
using IdentifierBindingHandle=std::shared_ptr<const IdentifierBindingIdentity>;
struct IdentifierBindingObservation {
    IdentifierBindingHandle identity;
    std::optional<std::string> identifier;
    std::uint32_t count{},pointer_word{};
};
enum class IdentifierHolderStatus:std::uint8_t {Ready,Missing,Retired,InvalidIdentifier,InvalidHandle,IdentityExhausted};
struct IdentifierBindingResult {
    IdentifierHolderStatus status{IdentifierHolderStatus::Ready};
    bool transition{};
    IdentifierBindingHandle binding;
};
struct IdentifierHolderObservation {
    std::vector<IdentifierBindingObservation> entries;
    std::uint32_t count{},index_accounting_count{};
    std::size_t index_size{};
};
// Original IdentHolder/BindManager decisions over native name/record storage.
// The pointer word is opaque and non-owning: destruction never frees its data.
// Null Bind creates an unindexed entry; empty and missing names stay distinct.
// The caller serializes mutation, as for NativeArchiveIdentifiers. Process
// coordinators must keep their scheduler's mutation capability at this boundary.
class NativeIdentifierHolder final {
public:
    NativeIdentifierHolder()=default;
    NativeIdentifierHolder(const NativeIdentifierHolder&)=delete;
    NativeIdentifierHolder& operator=(const NativeIdentifierHolder&)=delete;
    IdentifierBindingResult Bind(std::optional<std::string_view>);
    IdentifierBindingResult Unbind(std::optional<std::string_view>);
    IdentifierHolderStatus SetPointer(std::optional<std::string_view>,std::uint32_t);
    IdentifierHolderStatus RestoreCount(IdentifierBindingHandle,std::uint32_t);
    IdentifierBindingResult Find(std::optional<std::string_view>) const;
    std::optional<IdentifierBindingObservation> Observe(IdentifierBindingHandle) const;
    std::optional<IdentifierHolderObservation> Observe() const;
    bool retired() const noexcept {return retired_;}
    void Retire() noexcept;
private:
    struct Entry {IdentifierBindingObservation row;};
    IdentifierHolderStatus Validate(std::optional<std::string_view>) const;
    std::shared_ptr<Entry> Get(std::optional<std::string_view>) const;
    std::shared_ptr<Entry> Get(IdentifierBindingHandle) const;
    IdentifierRegistry<std::shared_ptr<Entry>> index_{127};
    std::list<std::shared_ptr<Entry>> entries_;
    std::map<std::uint32_t,std::shared_ptr<Entry>> identities_;
    std::uint32_t serial_{},count_{};
    bool retired_{};
};
}
