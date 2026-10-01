#pragma once
#include "fates/runtime/native_event_state.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace fates::event::native {class NativeEventVariableCommands;}
namespace fates::runtime::native {
// GameUserData+20: a separate bank from the flags at +1C. Null names, empty
// names, duplicate names and values in unnamed slots are all distinct states.
struct EventVariableSnapshot {
    std::vector<std::optional<std::string>> names;
    std::vector<std::int32_t> values;
    bool operator==(const EventVariableSnapshot&) const=default;
};
using EventVariableStatus=EventStateStatus;
enum class EventVariableOperation : std::uint8_t {
    Entry, EntryGlobal, GetIndex, Get, Set, Add, ResetLocal, ClearNotExist, Reset
};
class NativeEventVariableBank;
class PreparedEventVariableOperation;
class EventVariableResult final {
public:
    enum class Kind : std::uint8_t {None,Integer,ValuesAddress};
    Kind kind() const noexcept {return kind_;}
    bool is_address() const noexcept {return static_cast<bool>(bank_);}
    std::optional<std::int32_t> integer() const noexcept {return kind_==Kind::Integer?std::optional(integer_):std::nullopt;}
    std::optional<std::uintptr_t> address() const noexcept;
    const std::shared_ptr<const NativeEventVariableBank>& bank() const noexcept {return bank_;}
private:
    friend class NativeEventVariableBank;
    friend class PreparedEventVariableOperation;
    friend class fates::event::native::NativeEventVariableCommands;
    Kind kind_{Kind::None};
    std::int32_t integer_{};
    std::shared_ptr<const NativeEventVariableBank> bank_;
};
class PreparedEventVariableOperation final {
public:
    PreparedEventVariableOperation()=default;
    PreparedEventVariableOperation(PreparedEventVariableOperation&&) noexcept=default;
    PreparedEventVariableOperation& operator=(PreparedEventVariableOperation&&) noexcept=default;
    PreparedEventVariableOperation(const PreparedEventVariableOperation&)=delete;
    PreparedEventVariableOperation& operator=(const PreparedEventVariableOperation&)=delete;
private:
    friend class NativeEventVariableBank;
    friend class fates::event::native::NativeEventVariableCommands;
    std::shared_ptr<const NativeEventVariableBank> bank_;
    std::uint64_t revision_{};
    bool consumed_{},write_{};
    EventVariableSnapshot next_;
    EventVariableResult result_;
};
class NativeEventVariableBank final : public std::enable_shared_from_this<NativeEventVariableBank> {
public:
    static constexpr std::size_t OriginalGameUserCapacity=48;
    static constexpr std::size_t MaxCapacity=65536; // host admission bound
    static EventVariableStatus Restore(const EventVariableSnapshot&,std::shared_ptr<NativeEventVariableBank>&);
    static EventVariableStatus Fresh(std::size_t explicit_capacity,std::shared_ptr<NativeEventVariableBank>&);
    NativeEventVariableBank(const NativeEventVariableBank&)=delete;
    NativeEventVariableBank& operator=(const NativeEventVariableBank&)=delete;
    // Entry operations are the underlying manager's name registration only.
    // They retain slot values and accept every prefix. ev::VariableEntry also
    // needs a live ChapterSequence owner before it can initialize the value.
    EventVariableStatus Prepare(EventVariableOperation,std::string_view,std::int32_t,PreparedEventVariableOperation&) const;
    EventVariableStatus Commit(PreparedEventVariableOperation&,EventVariableResult&) noexcept;
    EventVariableStatus Snapshot(EventVariableSnapshot&) const;
    void Retire() noexcept {retired_=true;}
    bool active() const noexcept {return !retired_;}
    std::size_t capacity() const noexcept {return state_.names.size();}
    std::uint64_t revision() const noexcept {return revision_;}
private:
    friend class EventVariableResult;
    NativeEventVariableBank()=default;
    EventVariableSnapshot state_;
    std::uint64_t revision_{1};
    bool retired_{};
};
}
