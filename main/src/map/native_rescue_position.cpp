#include "fates/map/native_rescue_position.hpp"
#include "fates/map/native_deployment_semantics.hpp"
#include "fates/map/native_deployment_movement.hpp"
#include "fates/runtime/native_movement_rules.hpp"
#include "fates/runtime/native_runtime.hpp"
#include <cstdlib>
namespace fates::map::native {
namespace {
bool OriginValid(int x,int y) {return x>=0&&y>=0&&x<32&&y<32;}
bool Inside(MovementBounds b,int x,int y){return x>=b.min_x&&x<b.max_x&&y>=b.min_y&&y<b.max_y;}
}
std::optional<RescueMovementFields> BuildRescueMovementFields(MovementBounds bounds,int x,int y,
    const RescueTerrainCosts& costs,bool prohibited,bool free) {
    if(!ValidMovementBounds(bounds))return std::nullopt;
    // GetRescuePosition asks UnitMoveXY for power100, flags0 then100. Its
    // already-projected signed terrain costs need no occupancy/route queries.
    struct Services:DeploymentMovementServices {
        const RescueTerrainCosts& costs;int cell{};
        explicit Services(const RescueTerrainCosts& c):costs(c){}
        std::optional<std::uint8_t> CostIndexAt(int x,int y,std::uint32_t) override {cell=y*32+x;return 0;}
        std::optional<std::int8_t> TerrainCost(std::uint8_t,bool) override {return costs[cell];}
        std::optional<bool> OccupantAllows(int,int,std::uint32_t) override {return std::nullopt;}
        std::optional<int> RouteCross(int,int) override {return std::nullopt;}
    } svc(costs);
    const TerrainImageGeometry geometry{32,32,std::uint8_t(bounds.min_x),std::uint8_t(bounds.min_y),std::uint8_t(bounds.max_x),std::uint8_t(bounds.max_y)};
    RescueMovementFields result{};
    if(BuildDeploymentMovementExact(result.primary,geometry,x,y,prohibited?0:100,free?0x100:0,svc)!=DeploymentMovementStatus::Ok)return std::nullopt;
    if(BuildDeploymentMovementExact(result.secondary,geometry,x,y,prohibited?0:100,0x100,svc)!=DeploymentMovementStatus::Ok)return std::nullopt;
    return result;
}
RescuePositionResult SelectRescuePositionExact(const RescueMapImage& image,const RescueTerrainCosts& costs,
    const RescueMovementFields& fields,const RescueRequest& request) {
    RescuePositionResult result{};
    if(!ValidMovementBounds(image.active)||!OriginValid(request.origin_x,request.origin_y)||!request.unit_key)return result;
    const auto bounds=image.active;const int ox=request.origin_x,oy=request.origin_y;
    const bool allow=request.allow_origin&&costs[oy*32+ox]>=0;
    const bool prefer=request.prefer_unit_origin&&Inside(bounds,request.unit_x,request.unit_y);
    const auto value=[&](const RescueField& field,int x,int y){const auto i=y*32+x;return ResolveMoveImageCell(image.terrain_blocks_move_image[i],field[i]);};
    int best=100;
    for(int y=bounds.min_y;y<bounds.max_y;++y)for(int x=bounds.min_x;x<bounds.max_x;++x) {
        const int primary=value(fields.primary,x,y);if(primary<0)continue;
        const int score=prefer?value(fields.secondary,x,y):primary;if(score>best)continue;
        if(score==best&&result.x>=0) {
            if(!prefer){if(value(fields.secondary,x,y)>=value(fields.secondary,result.x,result.y))continue;}
            else {
                const int dx=std::abs(int(request.unit_x)-x),dy=std::abs(int(request.unit_y)-y);
                const int old_dx=std::abs(int(request.unit_x)-result.x),old_dy=std::abs(int(request.unit_y)-result.y);
                if(dx+dy>old_dx+old_dy)continue;
                if(dx+dy==old_dx+old_dy&&std::abs(dx-dy)>=std::abs(old_dx-old_dy))continue;
            }
        }
        const auto occupant=image.occupants[y*32+x];if(occupant&&occupant!=request.unit_key)continue;
        if(!allow&&x==ox&&y==oy)continue;
        result.x=x;result.y=y;best=score;
    }
    result.status=RescuePositionStatus::Ok;
    if(result.x>=0){result.route=prefer?RescuePositionRoute::PreferredField:RescuePositionRoute::NormalField;return result;}
    for(int y=bounds.min_y;y<bounds.max_y;++y)for(int x=bounds.min_x;x<bounds.max_x;++x) {
        if(image.occupants[y*32+x]||(!allow&&x==ox&&y==oy)||costs[y*32+x]<0)continue;
        const int distance=std::abs(x-ox)+std::abs(y-oy);
        if(distance<best){result.x=x;result.y=y;best=distance;}
    }
    if(result.x>=0){result.route=RescuePositionRoute::TerrainFallback;return result;}
    result.x=ox;result.y=oy;result.route=RescuePositionRoute::UncheckedOrigin;return result;
}
RescuePositionResult QueryRescuePositionForCurrentUnit(const fates::runtime::native::NativeRuntime& r,
    std::uint16_t slot,const RescueMapImage& image,int x,int y,bool allow,bool prefer) {
    using namespace fates::runtime::native;
    if(slot>=r.game.units.size()||!OriginValid(x,y)||!ValidMovementBounds(image.active))return {};
    const auto& unit=r.game.units[slot];
    if(unit.x<-128||unit.x>255||unit.y<-128||unit.y>255)return {};
    const auto rules=ProjectCurrentMovementRules(r,unit);
    if(!rules)return {RescuePositionStatus::MissingCurrentRules};
    RescueTerrainCosts costs{};std::array<std::optional<int>,256> resolved{};
    for(unsigned i=0;i<1024;++i) {
        const auto index=image.terrain_cost_indices[i];
        if(!resolved[index])resolved[index]=ProjectCurrentTerrainCost(r,unit,index);
        if(!resolved[index])return {RescuePositionStatus::MissingTerrainCost};
        costs[i]=std::int8_t(*resolved[index]);
    }
    const auto fields=BuildRescueMovementFields(image.active,x,y,costs,rules->movement_prohibited,rules->cost_free);
    if(!fields)return {};
    return SelectRescuePositionExact(image,costs,*fields,{x,y,std::int8_t(unit.x),std::int8_t(unit.y),std::uint16_t(slot+1),allow,prefer});
}
}
