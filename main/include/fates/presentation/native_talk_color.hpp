#pragma once
#include "fates/runtime/native_process.hpp"
#include "fates/runtime/native_object_registry.hpp"
#include <array>
#include <optional>
namespace fates::presentation::native {
using TalkColorBytes=std::array<std::uint8_t,4>;
// Access to an EXISTING registered color. No independent registry or copied color
// cache. A false write must leave that channel untouched; other earlier channels
// may already have committed and are not repeated on retry.
class TalkColorOwner {
public:
    virtual ~TalkColorOwner()=default;
    virtual bool UsesScheduler(const runtime::native::NativeProcessScheduler&) const noexcept=0;
    virtual bool UsesObjectRegistry(const runtime::native::ObjectHandleRegistry&) const noexcept=0;
    virtual std::optional<TalkColorBytes> ReadColor(runtime::native::ObjectIdentity) const=0;
    virtual bool WriteColorChannel(runtime::native::ObjectIdentity,std::uint8_t channel,std::uint8_t value,runtime::native::ProcessAccess&)=0;
};
}
