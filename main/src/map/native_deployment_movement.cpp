#include "fates/map/native_deployment_movement.hpp"
#include "fates/runtime/native_runtime.hpp"
#include "fates/runtime/native_force_order.hpp"
#include <vector>
namespace fates::map::native {
namespace {using S=DeploymentMovementStatus;}
std::optional<std::uint8_t> ResolveDeploymentMoveCostIndexExact(
    std::uint8_t id,std::uint32_t flags,const DeploymentTileLookup& lookup) {
    if(!lookup)return std::nullopt;auto tile=lookup(id);if(!tile)return std::nullopt;
    if((flags&0x400)&&(tile->flags&1)&&tile->change_id_2){tile=lookup(tile->change_id_2);if(!tile)return std::nullopt;}
    if((flags&0x200)&&(tile->flags&2)&&tile->change_id_1){tile=lookup(tile->change_id_1);if(!tile)return std::nullopt;}
    return tile->cost_index;
}
std::optional<bool> DeploymentMoveOccupantAllowedExact(std::uint32_t flags,DeploymentMoveOccupantServices& svc) {
    if(!(flags&0xc2))return true;
    const auto allied=svc.AlliedToActingForce();if(!allied)return std::nullopt;if(*allied)return true;
    if(flags&2)return false;
    if(flags&0x4000)return true;
    const auto active=svc.SameAsActiveForce();if(!active)return std::nullopt;if(!*active)return false;
    if(flags&0x40){const auto value=svc.PublicFlag1();if(!value)return std::nullopt;if(*value)return false;}
    if(flags&0x80){const auto value=svc.PolicyFlag400000();if(!value)return std::nullopt;if(*value)return false;}
    return true;
}
std::optional<int> DeploymentRouteCrossExact(const DeploymentRouteSnapshot& route,int x,int y) noexcept {
    if(route.count>route.steps.size())return std::nullopt;
    int cx=route.start_x,cy=route.start_y;
    for(unsigned i=0;i<route.count;++i){
        if(x==cx&&y==cy)return int(i);
        const auto d=route.steps[i];cx+=(d&1)?-1:((d&2)?1:0);cy+=(d&8)?-1:((d&4)?1:0);
    }return -1;
}
DeploymentMovementStatus BuildDeploymentMovementExact(DeploymentMovementPlane& out,TerrainImageGeometry g,
    int sx,int sy,int power,std::uint32_t flags,DeploymentMovementServices& svc) {
    const MovementBounds bounds=(flags&1)?MovementBounds{0,0,g.width,g.height}:MovementBounds{g.min_x,g.min_y,g.max_x,g.max_y};
    if(g.width>32||g.height>32||!ValidMovementBounds(bounds))return S::InvalidGeometry;
    if(sx<0||sx>=32||sy<0||sy>=32)return S::InvalidOrigin;
    const auto budget=std::uint8_t(power);DeploymentMovementPlane image;image.fill(-1);image[sy*32+sx]=0;
    struct Node {int x,y;std::uint8_t direction,cost;};
    std::vector<Node> current{{sx,sy,0x20,0}},next;current.reserve(255);next.reserve(255);
    constexpr std::array<std::uint8_t,4> dirs{8,4,1,2},reverse{4,8,2,1};
    constexpr std::array<int,4> dx{0,0,-1,1},dy{-1,1,0,0};
    while(!current.empty()){
        next.clear();
        for(const auto& node:current){
            if(int(node.cost)>int(image[node.y*32+node.x]))continue;
            for(unsigned d=0;d<4;++d){
                if(node.direction==reverse[d])continue;
                const int x=node.x+dx[d],y=node.y+dy[d];
                if(x<bounds.min_x||x>=bounds.max_x||y<bounds.min_y||y>=bounds.max_y)continue;
                const auto index=svc.CostIndexAt(x,y,flags);if(!index)return S::MissingCostIndex;
                auto cost=svc.TerrainCost(*index,false);if(!cost)return S::MissingTerrainCost;
                if(*cost<0&&(flags&0x1000000)){cost=svc.TerrainCost(*index,true);if(!cost)return S::MissingTerrainCost;}
                if(*cost<0)continue;
                const int total=int(node.cost)+((flags&0x100)?1:int(*cost));const int cell=y*32+x;
                if(total>=std::uint8_t(image[cell])||total>budget)continue;
                if(flags&0xc2){const auto allowed=svc.OccupantAllows(x,y,flags);if(!allowed)return S::MissingOccupancy;if(!*allowed)continue;}
                if(flags&0x20){const auto cross=svc.RouteCross(x,y);if(!cross)return S::MissingRoute;if(*cross>=0)continue;}
                if(next.size()==255)return S::FrontierOverflow;
                image[cell]=std::int8_t(total);next.push_back({x,y,dirs[d],std::uint8_t(total)});
            }
        }
        current.swap(next);
    }
    out=image;return S::Ok;
}
CurrentDeploymentMovementResult BuildCurrentDeploymentMovement(const fates::runtime::native::NativeRuntime& r,
    int x,int y,int cost_row,int power,std::uint32_t flags,std::uint8_t force,const DeploymentRouteSnapshot* route) {
    namespace rn=fates::runtime::native;CurrentDeploymentMovementResult out{};
    const auto terrain=ReadCurrentTerrainImage(r);out.terrain_status=terrain.status;
    if(terrain.status!=TerrainImageStatus::Ok){out.status=S::TerrainImageUnavailable;return out;}
    const auto units=ReadCurrentUnitImage(r.game);out.unit_status=units.status;
    if(units.status!=UnitImageStatus::Ok){out.status=S::UnitImageUnavailable;return out;}
    const auto& rows=r.definitions.movement_costs();
    if(cost_row<0||std::size_t(cost_row)>=rows.size()||((flags&0x1000000)&&rows.empty())){out.status=S::MissingDefinition;return out;}
    const auto g=terrain.geometry;
    if(!(flags&1)&&(g.max_x>g.width||g.max_y>g.height)){out.status=S::InvalidGeometry;return out;}
    struct Services:DeploymentMovementServices {
        const rn::NativeRuntime& r;const TerrainImagePlanes& terrain;const UnitImageGrid& units;
        int row;std::uint8_t force;const DeploymentRouteSnapshot* route;S error{S::Ok};
        Services(const rn::NativeRuntime& r,const TerrainImagePlanes& t,const UnitImageGrid& u,int row,
            std::uint8_t force,const DeploymentRouteSnapshot* route):r(r),terrain(t),units(u),row(row),force(force),route(route){}
        std::optional<std::uint8_t> CostIndexAt(int x,int y,std::uint32_t flags) override {
            const int cell=y*32+x;if(!(flags&0x600))return terrain.cost_indices[cell];
            return ResolveDeploymentMoveCostIndexExact(terrain.terrain[cell],flags,[&](std::uint8_t id)->std::optional<DeploymentMoveTile>{
                const auto& tiles=r.definitions.terrain_map()->terrain_types;
                if(id>=tiles.size()){error=S::MissingDefinition;return std::nullopt;}
                const auto& t=tiles[id];return DeploymentMoveTile{t.change_id_1,t.change_id_2,t.movement_cost_index,t.flags_0x18};
            });
        }
        std::optional<std::int8_t> TerrainCost(std::uint8_t i,bool fallback) override {
            const auto& values=r.definitions.movement_costs()[fallback?0:row];
            if(i>=values.size()){error=S::MissingDefinition;return std::nullopt;}return values[i];
        }
        std::optional<bool> OccupantAllows(int x,int y,std::uint32_t flags) override {
            const auto key=units.cells[y*32+x];if(!key)return true;
            if(key>r.game.units.size()||!r.game.units[key-1].occupied){error=S::InvalidUnit;return std::nullopt;}
            struct Occupant:DeploymentMoveOccupantServices {
                Services& owner;const rn::UnitState& u;Occupant(Services& s,const rn::UnitState& u):owner(s),u(u){}
                std::optional<bool> AlliedToActingForce() override {
                    if(owner.force>=9||u.force_type>=9){owner.error=S::MissingUnitState;return std::nullopt;}
                    return rn::ForcesAlliedExact(owner.force,u.force_type);
                }
                std::optional<bool> SameAsActiveForce() override {
                    const auto& p=owner.r.game.phase;
                    if(!owner.r.game.map_active||p.stage==rn::PhaseAccessStage::Unbound||p.chapter_index!=owner.r.game.campaign.current_chapter_index||p.situation.active_force>=3){owner.error=S::MissingPhase;return std::nullopt;}
                    return u.force_type==p.situation.active_force;
                }
                std::optional<bool> PublicFlag1() override {return (u.flags&1)!=0;}
                std::optional<bool> PolicyFlag400000() override {
                    if(!u.ai.runtime_tuning_bound){owner.error=S::MissingUnitState;return std::nullopt;}return (u.ai.policy_flags&0x400000)!=0;
                }
            } occupant(*this,r.game.units[key-1]);
            return DeploymentMoveOccupantAllowedExact(flags,occupant);
        }
        std::optional<int> RouteCross(int x,int y) override {return route?DeploymentRouteCrossExact(*route,x,y):std::nullopt;}
    } svc(r,*terrain.planes,*units.image,cost_row,force,route);
    DeploymentMovementPlane image;out.status=BuildDeploymentMovementExact(image,g,x,y,power,flags,svc);
    if(out.status==S::Ok)out.image=image;
    else if(svc.error!=S::Ok)out.status=svc.error;
    return out;
}
CurrentDeploymentFieldsResult BuildCurrentDeploymentFields(const fates::runtime::native::NativeRuntime& r,
    std::uint16_t slot,int x,int y,int power,std::uint32_t flags,const DeploymentRangeMasks& ranges,
    const DeploymentRouteSnapshot* route) {
    CurrentDeploymentFieldsResult out{};
    if(slot>=r.game.units.size()||!r.game.units[slot].occupied)return out;
    const auto& u=r.game.units[slot];const auto* job=r.definitions.FindJob(u.job_id);
    if(!job){out.movement_status=S::MissingDefinition;return out;}
    const auto movement=BuildCurrentDeploymentMovement(r,x,y,job->movement_cost_index,power,flags,u.force_type,route);
    out.movement_status=movement.status;if(movement.status!=S::Ok)return out;
    const auto fill=BuildCurrentDeploymentFill(r,slot,*movement.image,ranges,flags);
    out.fill_status=fill.status;if(fill.status==DeploymentFillStatus::Ok)out.fields=DeploymentFields{*movement.image,fill.planes};
    return out;
}
}
