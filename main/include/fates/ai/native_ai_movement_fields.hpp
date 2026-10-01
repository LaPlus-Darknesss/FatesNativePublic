#pragma once
#include "fates/ai/native_ai_position_movement.hpp"
#include "fates/runtime/native_player_action.hpp"
#include <array>
namespace fates::ai::native::detail {
struct OrdinaryAiField {
    std::array<int,1024> cost{};std::array<bool,1024> stop{};
    OrdinaryAiField() {cost.fill(-1);}
    int get(int x,int y) const noexcept {return x<0 || y<0 || x>=32 || y>=32?-1:cost[std::size_t(y*32+x)];}
};
OrdinaryAiField BuildOrdinaryAiField(const fates::runtime::native::NativeRuntime&,std::uint16_t,int,int,int,
                                    fates::runtime::native::MovementFieldOptions);
int OrdinaryAdjacentAllyCount(const fates::runtime::native::NativeRuntime&,std::uint16_t,int,int);
// Internal shared MoveTo slice. Entry points MUST first prove the ordinary
// locomotion/terrain/profile contract; this is not a universal movement API.
AiPositionMoveResult ExecuteOrdinaryMoveToResolved(fates::runtime::native::NativeRuntime&,std::uint16_t,int,int,bool retain_no_progress_rng=false);
}
