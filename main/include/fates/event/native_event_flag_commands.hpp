#pragma once
#include "fates/runtime/native_player_state_provider.hpp"

namespace fates::event::native {
class NativeEventFlagCommands;
class PreparedEventFlagCommand final {
public:
    PreparedEventFlagCommand()=default;
    PreparedEventFlagCommand(PreparedEventFlagCommand&&) noexcept=default;
    PreparedEventFlagCommand& operator=(PreparedEventFlagCommand&&) noexcept=default;
    PreparedEventFlagCommand(const PreparedEventFlagCommand&)=delete;
    PreparedEventFlagCommand& operator=(const PreparedEventFlagCommand&)=delete;
private:
    friend class NativeEventFlagCommands;
    std::shared_ptr<const NativeEventFlagCommands> owner_;
    std::shared_ptr<runtime::native::NativeEventFlagBank> bank_;
    runtime::native::PreparedEventFlagOperation operation_;
    std::int32_t immediate_{};
    bool consumed_{};
};
// Concrete carried flag callbacks, usable before a tactical phase exists.
// Prepare has no effects. Commit validates the exact retained player identity
// and applies its opaque operation once. These are not VM acknowledgment hooks.
class NativeEventFlagCommands final : public std::enable_shared_from_this<NativeEventFlagCommands> {
public:
    NativeEventFlagCommands(const NativeEventFlagCommands&)=delete;
    NativeEventFlagCommands& operator=(const NativeEventFlagCommands&)=delete;
    static runtime::native::EventFlagStatus Bind(std::shared_ptr<runtime::native::NativeRuntime>,
        std::shared_ptr<NativeEventFlagCommands>&);
    static bool Recognizes(std::string_view) noexcept;
    runtime::native::EventFlagStatus Validate() const noexcept;
    runtime::native::EventFlagStatus Prepare(std::string_view registered_name,std::string_view flag_name,
        PreparedEventFlagCommand&) const;
    runtime::native::EventFlagStatus Commit(PreparedEventFlagCommand&,runtime::native::EventFlagResult&) const noexcept;
    void Retire() noexcept {retired_=true;}
    const std::shared_ptr<runtime::native::NativeRuntime>& runtime() const noexcept {return runtime_;}
private:
    NativeEventFlagCommands()=default;
    std::shared_ptr<runtime::native::NativeRuntime> runtime_;
    std::shared_ptr<const runtime::native::NativePlayerEventState> player_;
    bool retired_{};
};
}
