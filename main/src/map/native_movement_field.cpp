#include "fates/map/native_movement_field.hpp"
#include <queue>
#include <vector>
#include <limits>
namespace fates::map::native {
bool ValidMovementBounds(MovementBounds b) noexcept {
    return b.min_x>=0&&b.min_y>=0&&b.max_x<=32&&b.max_y<=32&&b.min_x<=b.max_x&&b.min_y<=b.max_y;
}
std::optional<MovementCostField> ComputeMovementCostField(MovementBounds b,int sx,int sy,
    int budget,const MovementCostReader& cost,bool cost_free) {
    if(!ValidMovementBounds(b)||sx<0||sy<0||sx>=32||sy>=32||budget<0||!cost)return std::nullopt;
    MovementCostField field;field.fill(-1);field[sy*32+sx]=0;
    using Entry=std::pair<int,int>;std::priority_queue<Entry,std::vector<Entry>,std::greater<Entry>> pending;
    pending.push({0,sy*32+sx});
    constexpr std::array<int,4> dx{1,-1,0,0},dy{0,0,1,-1};
    while(!pending.empty()) {
        const auto [distance,index]=pending.top();pending.pop();if(field[index]!=distance)continue;
        const int x=index%32,y=index/32;
        for(unsigned direction=0;direction<4;++direction) {
            const int nx=x+dx[direction],ny=y+dy[direction];
            if(nx<b.min_x||nx>=b.max_x||ny<b.min_y||ny>=b.max_y)continue;
            const auto raw=cost(nx,ny);if(!raw)return std::nullopt;if(*raw<0)continue;
            const int edge=cost_free?1:*raw;if(edge>budget-distance)continue;
            const int next=distance+edge,cell=ny*32+nx;
            if(field[cell]<0||next<field[cell]){field[cell]=next;pending.push({next,cell});}
        }
    }return field;
}
}
