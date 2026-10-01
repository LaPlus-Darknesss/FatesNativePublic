#pragma once
#include <array>
#include <functional>
#include <optional>
namespace fates::map::native {
struct MovementBounds { int min_x{},min_y{},max_x{},max_y{}; };
using MovementCostField=std::array<int,1024>;
// A negative cost blocks entry. Nullopt means required data is unavailable and
// invalidates the entire query. Coordinates use the map's 32-cell backing stride.
using MovementCostReader=std::function<std::optional<int>(int,int)>;
bool ValidMovementBounds(MovementBounds) noexcept;
// Shared nonnegative-cost relaxation. The origin may be outside the search
// rectangle, but must be inside the backing grid. Cost-free still requires a
// passable destination. A zero budget can traverse zero-cost cells.
std::optional<MovementCostField> ComputeMovementCostField(MovementBounds,
    int origin_x,int origin_y,int budget,const MovementCostReader&,bool cost_free=false);
}
