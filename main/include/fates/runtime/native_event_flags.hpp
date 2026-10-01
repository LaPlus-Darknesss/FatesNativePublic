#pragma once
#include "fates/runtime/native_event_state.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace fates::event::native {class NativeEventFlagCommands;}
namespace fates::runtime::native {
// GameUserData+1C, separate from the campaign GameUser flag word. A null name
// slot differs from an empty string; duplicates and unnamed set bits are valid.
struct EventFlagSnapshot {
    std::vector<std::optional<std::string>> names;
    std::vector<std::uint8_t> bits;
    bool operator==(const EventFlagSnapshot&) const=default;
};
using EventFlagStatus=EventStateStatus;
enum class EventFlagOperation : std::uint8_t {
    Entry, EntryGlobal, GetIndex, Get, Set, Clear, ResetLocal, ClearNotExist, Reset
};
class NativeEventFlagBank;
class PreparedEventFlagOperation;
class EventFlagResult final {
public:
    enum class Kind : std::uint8_t {None,Integer,BitsAddress};
    Kind kind() const noexcept {return kind_;}
    bool is_address() const noexcept {return static_cast<bool>(bank_);}
    std::optional<std::int32_t> integer() const noexcept {return kind_==Kind::Integer?std::optional(integer_):std::nullopt;}
    // An address is a retained allocation identity, never a narrowed PC integer.
    // Retirement invalidates it; ordinary mutations/reset keep its allocation.
    std::optional<std::uintptr_t> address() const noexcept;
    const std::shared_ptr<const NativeEventFlagBank>& bank() const noexcept {return bank_;}
private:
    friend class NativeEventFlagBank;
    friend class PreparedEventFlagOperation;
    friend class fates::event::native::NativeEventFlagCommands;
    Kind kind_{Kind::None};
    std::int32_t integer_{};
    std::shared_ptr<const NativeEventFlagBank> bank_;
};
class PreparedEventFlagOperation final {
public:
    PreparedEventFlagOperation()=default;
    PreparedEventFlagOperation(PreparedEventFlagOperation&&) noexcept=default;
    PreparedEventFlagOperation& operator=(PreparedEventFlagOperation&&) noexcept=default;
    PreparedEventFlagOperation(const PreparedEventFlagOperation&)=delete;
    PreparedEventFlagOperation& operator=(const PreparedEventFlagOperation&)=delete;
private:
    friend class NativeEventFlagBank;
    std::shared_ptr<const NativeEventFlagBank> bank_;
    std::uint64_t revision_{};
    bool consumed_{},write_{};
    EventFlagSnapshot next_;
    EventFlagResult result_;
};
class NativeEventFlagBank final : public std::enable_shared_from_this<NativeEventFlagBank> {
public:
    static constexpr std::size_t OriginalGameUserCapacity=128;
    static constexpr std::size_t MaxCapacity=65536; // host admission bound
    static EventFlagStatus Restore(const EventFlagSnapshot&,std::shared_ptr<NativeEventFlagBank>&);
    static EventFlagStatus Fresh(std::size_t explicit_capacity,std::shared_ptr<NativeEventFlagBank>&);
    NativeEventFlagBank(const NativeEventFlagBank&)=delete;
    NativeEventFlagBank& operator=(const NativeEventFlagBank&)=delete;
    EventFlagStatus Prepare(EventFlagOperation,std::string_view,PreparedEventFlagOperation&) const;
    EventFlagStatus Commit(PreparedEventFlagOperation&,EventFlagResult&) noexcept;
    EventFlagStatus Snapshot(EventFlagSnapshot&) const;
    void Retire() noexcept {retired_=true;}
    bool active() const noexcept {return !retired_;}
    std::size_t capacity() const noexcept {return state_.names.size();}
    std::uint64_t revision() const noexcept {return revision_;}
private:
    friend class EventFlagResult;
    NativeEventFlagBank()=default;
    EventFlagSnapshot state_;
    std::uint64_t revision_{1};
    bool retired_{};
};
bool IsValidEventFlagName(std::string_view) noexcept;
}
