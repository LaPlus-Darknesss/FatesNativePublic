#include "fates/engine/bind_manager.hpp"
#include <bit>
#include <cstdint>

static_assert(sizeof(int)==sizeof(std::uint32_t));

BindManager::BindManager() = default;

bool BindManager::Bind() {
    const int previous = count_;
    count_ = std::bit_cast<int>(static_cast<std::uint32_t>(count_) + 1u);
    return previous == 0;
}

bool BindManager::Unbind() {
    const int previous = count_;
    count_ = std::bit_cast<int>(static_cast<std::uint32_t>(count_) - 1u);
    if (count_ < 0) {
        count_ = 0;
    }
    return previous == 1;
}
