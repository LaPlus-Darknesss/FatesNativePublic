#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace fates::decomp_detail {

using Arm32Address = std::uint32_t;

inline Arm32Address ReadArm32Address(const void* base, std::size_t offset) {
    Arm32Address value{};
    const auto* bytes = static_cast<const std::byte*>(base);
    std::memcpy(&value, bytes + offset, sizeof(value));
    return value;
}

template <typename T = void>
const T* TargetPointer(Arm32Address address) {
    return reinterpret_cast<const T*>(static_cast<std::uintptr_t>(address));
}

template <typename T = void>
T* MutableTargetPointer(Arm32Address address) {
    return reinterpret_cast<T*>(static_cast<std::uintptr_t>(address));
}

} // namespace fates::decomp_detail
