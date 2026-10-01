#include "fates/runtime/native_rng.hpp"
namespace fates::runtime::native {
std::uint32_t NextRandomRaw(NativeRandomState& state) noexcept {
    std::uint32_t x=state.words[0];
    state.words[0]=state.words[1];
    x ^= x << 11;
    state.words[1]=state.words[2];
    state.words[2]=state.words[3];
    const std::uint32_t y=state.words[3];
    state.words[3]=x ^ (x >> 8) ^ y ^ (y >> 19);
    state.initialized=true;
    return state.words[3] & 0x7fffffffu;
}
std::uint32_t RandomValue(NativeRandomState& state,const std::uint32_t maximum) noexcept {
    if(maximum==0) return 0;
    return NextRandomRaw(state)%maximum;
}
std::uint32_t DrawGameRandom(NativeGameState& game,const std::uint32_t maximum) noexcept {
    ++game.rng.game;
    return RandomValue(game.rng.game_state,maximum);
}
}
