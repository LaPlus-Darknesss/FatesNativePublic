#pragma once
#include "fates/runtime/native_game_state.hpp"
#include <cstdint>
namespace fates::runtime::native {
std::uint32_t NextRandomRaw(NativeRandomState& state) noexcept;
std::uint32_t RandomValue(NativeRandomState& state, std::uint32_t maximum) noexcept;
std::uint32_t DrawGameRandom(NativeGameState& game, std::uint32_t maximum) noexcept;
}
