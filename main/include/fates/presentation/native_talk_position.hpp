#pragma once
#include "fates/runtime/native_object_registry.hpp"
#include "fates/runtime/native_process.hpp"
namespace fates::presentation::native {
using TalkVectorBits=std::array<std::uint32_t,3>;
// Object owners expose their existing registered position. This is not another
// registry or pointer map: lookup and generation remain ObjectHandleRegistry's.
class TalkPositionOwner {
public:
    virtual ~TalkPositionOwner()=default;
    virtual bool UsesObjectRegistry(const runtime::native::ObjectHandleRegistry&) const noexcept=0;
    virtual bool UsesScheduler(const runtime::native::NativeProcessScheduler&) const noexcept=0;
    virtual std::optional<TalkVectorBits> ReadPosition(runtime::native::ObjectIdentity) const=0;
    virtual bool WritePosition(runtime::native::ObjectIdentity,TalkVectorBits,runtime::native::ProcessAccess&)=0;
};
}
