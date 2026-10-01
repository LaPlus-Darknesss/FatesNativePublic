#pragma once
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
namespace fates::decomp_detail {
template <class... Args> inline void EventRuntimeCall(const char*, Args&&... args){(static_cast<void>(args),...);} 
template <class R, class... Args> inline R EventRuntimeValue(const char*, Args&&... args){(static_cast<void>(args),...); if constexpr(std::is_pointer_v<R>) return nullptr; else return R{};}
}
