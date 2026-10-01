#include "fates/map/native_deployment_fill.hpp"
#include "fates/map/native_deployment_semantics.hpp"
#include "fates/runtime/native_movement_rules.hpp"
#include "fates/runtime/native_runtime.hpp"
#include "fates/runtime/native_force_order.hpp"
#include <algorithm>
#include <cstdlib>
namespace fates::map::native {
namespace {
using S=DeploymentFillStatus;
bool Range(int v) noexcept {return v>=0&&v<=255;}
void Mark(DeploymentBits& bits,int x,int y) noexcept {bits[(y*32+x)/8]|=std::uint8_t(1u<<(x&7));}
bool Bit(std::uint64_t mask,int distance,bool low_only) noexcept {
    return distance<(low_only?32:64)&&(mask&(std::uint64_t{1}<<distance))!=0;
}
S Basic(DeploymentBits& out,MovementBounds b,std::uint64_t mask,int maximum,DeploymentFillServices& svc) {
    if(maximum==255){out.fill(255);return S::Ok;}
    for(int y=b.min_y;y<b.max_y;++y)for(int x=b.min_x;x<b.max_x;++x){
        const auto fill=svc.IsFill(x,y);if(!fill)return S::MissingFillInput;if(!*fill)continue;
        for(int ty=std::max(b.min_y,y-maximum);ty<std::min(b.max_y,y+maximum+1);++ty)
            for(int tx=std::max(b.min_x,x-maximum);tx<std::min(b.max_x,x+maximum+1);++tx){
                const int distance=ManhattanDistance(x,y,tx,ty);
                if(distance<=maximum&&Bit(mask,distance,false))Mark(out,tx,ty);
            }
    }return S::Ok;
}
S Double(DeploymentRangePlanes& out,MovementBounds b,const DeploymentRangeMasks& r,
    bool attack,bool rod,bool partner,DeploymentFillServices& svc) {
    if((attack&&r.attack_max==255)||(rod&&r.rod_max==255)){
        if(attack){const auto status=Basic(out.attack,b,r.attack,r.attack_max,svc);if(status!=S::Ok)return status;}
        return rod?Basic(out.rod,b,r.rod,r.rod_max,svc):S::Ok;
    }
    const int maximum=attack&&rod?std::max(r.attack_max,r.rod_max):(attack?r.attack_max:r.rod_max);
    for(int y=b.min_y;y<b.max_y;++y)for(int x=b.min_x;x<b.max_x;++x){
        const auto fill=svc.IsFill(x,y);if(!fill)return S::MissingFillInput;if(!*fill)continue;
        bool stand=true;if(partner){const auto cost=svc.PartnerCanStand(x,y);if(!cost)return S::MissingPartnerCost;stand=*cost;}
        const auto attack_mask=stand?r.attack:r.without_partner_attack;
        const auto rod_mask=stand?r.rod:r.without_partner_rod;
        // Original combined loop uses the larger maximum for BOTH masks.
        for(int ty=std::max(b.min_y,y-maximum);ty<std::min(b.max_y,y+maximum+1);++ty)
            for(int tx=std::max(b.min_x,x-maximum);tx<std::min(b.max_x,x+maximum+1);++tx){
                const int distance=ManhattanDistance(x,y,tx,ty);if(distance>maximum)continue;
                if(attack&&Bit(attack_mask,distance,true))Mark(out.attack,tx,ty);
                if(rod&&Bit(rod_mask,distance,true))Mark(out.rod,tx,ty);
            }
    }return S::Ok;
}
}
std::optional<bool> IsDeploymentFillExact(bool blocked,std::int8_t movement,bool occupied,
    std::uint32_t flags,DeploymentFillOccupantServices& svc) {
    if(blocked||movement<0)return false;
    if(movement==0||!occupied)return true;
    if(flags&kDeployFlagBlockOccupied)return false;
    if(flags&kDeployFlagRespectMovementBlock){
        const auto prohibited=svc.MovementProhibited();if(!prohibited)return std::nullopt;if(*prohibited)return false;
        const auto state=svc.State100();if(!state)return std::nullopt;if(*state)return false;
    }
    if(flags&kDeployFlagRequireAlliedOccupant)return svc.Allied();
    return true;
}
DeploymentFillStatus FillDeploymentRangesExact(DeploymentRangePlanes& output,MovementBounds bounds,
    DeploymentFillMode mode,const DeploymentRangeMasks& ranges,bool partner,DeploymentFillServices& svc) {
    if(!ValidMovementBounds(bounds))return S::InvalidBounds;
    if(mode>DeploymentFillMode::Unit)return S::InvalidMode;
    if((mode!=DeploymentFillMode::Rod&&mode!=DeploymentFillMode::DoubleRod&&!Range(ranges.attack_max))||
       (mode!=DeploymentFillMode::Attack&&mode!=DeploymentFillMode::DoubleAttack&&!Range(ranges.rod_max)))return S::InvalidRange;
    auto next=output;S status=S::Ok;
    switch(mode){
    case DeploymentFillMode::Attack:status=Basic(next.attack,bounds,ranges.attack,ranges.attack_max,svc);break;
    case DeploymentFillMode::Rod:status=Basic(next.rod,bounds,ranges.rod,ranges.rod_max,svc);break;
    case DeploymentFillMode::DoubleAttack:status=Double(next,bounds,ranges,true,false,partner,svc);break;
    case DeploymentFillMode::DoubleRod:status=Double(next,bounds,ranges,false,true,partner,svc);break;
    case DeploymentFillMode::Double:status=Double(next,bounds,ranges,true,true,partner,svc);break;
    case DeploymentFillMode::Unit:
        next={};
        switch(ResolveUnitFillRoute(ranges.attack_max,ranges.rod_max)){
        case UnitFillRoute::None:break;
        case UnitFillRoute::AttackOnly:status=Double(next,bounds,ranges,true,false,partner,svc);break;
        case UnitFillRoute::RodOnly:status=Double(next,bounds,ranges,false,true,partner,svc);break;
        case UnitFillRoute::AttackAndRod:status=Double(next,bounds,ranges,true,true,partner,svc);break;
        }break;
    }
    if(status==S::Ok)output=next;return status;
}
CurrentDeploymentFillResult BuildCurrentDeploymentFill(const fates::runtime::native::NativeRuntime& runtime,
    std::uint16_t slot,const std::array<std::int8_t,1024>& movement,const DeploymentRangeMasks& ranges,std::uint32_t flags) {
    namespace rn=fates::runtime::native;CurrentDeploymentFillResult result{};
    if(slot>=runtime.game.units.size()||!runtime.game.units[slot].occupied)return result;
    const auto terrain=ReadCurrentTerrainImage(runtime);result.terrain_status=terrain.status;
    if(terrain.status!=TerrainImageStatus::Ok){result.status=S::TerrainImageUnavailable;return result;}
    const auto units=ReadCurrentUnitImage(runtime.game);result.unit_status=units.status;
    if(units.status!=UnitImageStatus::Ok){result.status=S::UnitImageUnavailable;return result;}
    const auto g=terrain.geometry;MovementBounds bounds{g.min_x,g.min_y,g.max_x,g.max_y};
    if(g.max_x>g.width||g.max_y>g.height){result.status=S::InvalidBounds;return result;}
    const auto& unit=runtime.game.units[slot];
    struct Services:DeploymentFillServices {
        const rn::NativeRuntime& r;const rn::UnitState& unit;const TerrainImagePlanes& terrain;
        const UnitImageGrid& units;const std::array<std::int8_t,1024>& movement;std::uint32_t flags;S error{S::Ok};
        Services(const rn::NativeRuntime& r,const rn::UnitState& u,const TerrainImagePlanes& t,
            const UnitImageGrid& i,const std::array<std::int8_t,1024>& m,std::uint32_t f):r(r),unit(u),terrain(t),units(i),movement(m),flags(f){}
        std::optional<bool> IsFill(int x,int y) override {
            const auto cell=y*32+x;const auto& tiles=r.definitions.terrain_map()->terrain_types;
            const auto index=terrain.terrain[cell];if(index>=tiles.size()){error=S::MissingCurrentDefinition;return std::nullopt;}
            struct Occupant:DeploymentFillOccupantServices {
                Services& owner;std::uint8_t key;
                Occupant(Services& o,std::uint8_t k):owner(o),key(k){}
                const rn::UnitState* Get() {
                    if(key==0||key>owner.r.game.units.size()||!owner.r.game.units[key-1].occupied){owner.error=S::InvalidUnit;return nullptr;}
                    return &owner.r.game.units[key-1];
                }
                std::optional<bool> MovementProhibited() override {
                    const auto* u=Get();const auto value=u?rn::ProjectCurrentMovementProhibition(owner.r,*u):std::nullopt;
                    if(!value&&owner.error==S::Ok)owner.error=S::MissingCurrentDefinition;return value;
                }
                std::optional<bool> State100() override {
                    const auto* u=Get();if(!u)return std::nullopt;
                    // This is Unit+54, the existing AI policy owner, not public flags.
                    if(!u->ai.runtime_tuning_bound){owner.error=S::MissingCurrentState;return std::nullopt;}
                    return (u->ai.policy_flags&0x100)!=0;
                }
                std::optional<bool> Allied() override {
                    const auto* u=Get();if(!u)return std::nullopt;
                    const auto a=owner.unit.force_type,b=u->force_type;
                    if(a>=9||b>=9){owner.error=S::MissingCurrentState;return std::nullopt;}
                    return rn::ForcesAlliedExact(a,b);
                }
            } occupant(*this,units.cells[cell]);
            return IsDeploymentFillExact((tiles[index].flags_0x18&8)!=0,movement[cell],units.cells[cell]!=0,flags,occupant);
        }
        std::optional<bool> PartnerCanStand(int x,int y) override {
            const auto slot=unit.pair.partner_slot;
            if(slot>=r.game.units.size()){error=S::MalformedPair;return std::nullopt;}
            const auto& other=r.game.units[slot];
            if(!unit.pair.bound||!other.occupied||!other.pair.bound||other.pair.partner_slot>=r.game.units.size()||
               &r.game.units[other.pair.partner_slot]!=&unit||&other==&unit){error=S::MalformedPair;return std::nullopt;}
            const auto cost=rn::ProjectCurrentTerrainCost(r,other,terrain.cost_indices[y*32+x]);
            if(!cost){error=S::MissingCurrentDefinition;return std::nullopt;}return *cost>=0;
        }
    } services(runtime,unit,*terrain.planes,*units.image,movement,flags);
    result.status=FillDeploymentRangesExact(result.planes,bounds,DeploymentFillMode::Unit,ranges,
        unit.pair.partner_slot!=0xffffu,services);
    if(result.status!=S::Ok&&services.error!=S::Ok)result.status=services.error;
    return result;
}
}
