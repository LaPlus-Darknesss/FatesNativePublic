#include "fates/map/native_unit_deployment.hpp"
#include "fates/runtime/native_runtime.hpp"
#include "fates/runtime/native_current_item_eligibility.hpp"
namespace fates::map::native {
UnitDeploymentResult RunUnitDeploymentExact(int requested,std::uint32_t flags,std::uint32_t extra,UnitDeploymentServices& s) {
    using S=UnitDeploymentStatus;UnitDeploymentResult out{};out.power=requested;out.flags=flags|extra;
    auto fail=[&](S status){out.status=status;return out;};
    if(out.power<0){const auto p=s.CurrentMovementPower();if(!p)return fail(S::MissingPower);out.power=*p;}
    if(!(out.flags&0x2000)){const auto p=s.MovementProhibited();if(!p)return fail(S::MissingProhibition);if(*p)out.power=0;}
    const auto free=s.CostFree();if(!free)return fail(S::MissingCostFree);if(*free)out.flags|=0x100;
    const auto fallback=s.BaseCostFallback();if(!fallback)return fail(S::MissingFallback);if(*fallback)out.flags|=0x1000000;
    const auto pass=s.PassSkill();if(!pass)return fail(S::MissingPassSkill);if(*pass)out.flags=(out.flags&~2u)|0x4000;
    if(!s.Move(out.power,out.flags))return fail(S::MovementFailed);
    if(out.flags&0x27f0000){if(!s.Fill(out.flags))return fail(S::FillFailed);out.filled=true;}
    if(out.flags&0x1000){
        const auto active=s.SameAsActiveForce();if(!active)return fail(S::MissingPhase);
        if(!*active){const auto p=s.MovementProhibited();if(!p)return fail(S::MissingProhibition);
            if(*p){if(!s.EraseCurrentCell())return fail(S::InvalidCurrentCell);out.erased_current_cell=true;}}
    }
    return out;
}
CurrentUnitDeploymentResult BuildCurrentUnitDeployment(DeploymentFields& out,const fates::runtime::native::NativeRuntime& r,
    std::uint16_t slot,int x,int y,int power,std::uint32_t flags,std::uint32_t extra,
    const DeploymentRangeMasks* ranges,const DeploymentRouteSnapshot* route) {
    namespace rn=fates::runtime::native;CurrentUnitDeploymentResult result{};
    if(slot>=r.game.units.size()||!r.game.units[slot].occupied){result.operation.status=UnitDeploymentStatus::InvalidUnit;return result;}
    DeploymentFields candidate=out;
    struct Services:UnitDeploymentServices {
        const rn::NativeRuntime& r;const rn::UnitState& u;std::uint16_t slot;int x,y;DeploymentFields& candidate;
        CurrentUnitDeploymentResult& result;const DeploymentRangeMasks* ranges;const DeploymentRouteSnapshot* route;
        Services(const rn::NativeRuntime& r,std::uint16_t s,int x,int y,DeploymentFields& c,CurrentUnitDeploymentResult& o,
            const DeploymentRangeMasks* ranges,const DeploymentRouteSnapshot* route):r(r),u(r.game.units[s]),slot(s),x(x),y(y),candidate(c),result(o),ranges(ranges),route(route){}
        std::optional<int> CurrentMovementPower() override {const auto p=rn::ProjectCurrentMovementPower(r,u);result.power_status=p.status;return p.status==rn::MovementPowerStatus::Ok?std::optional<int>(p.value):std::nullopt;}
        std::optional<bool> MovementProhibited() override {return rn::ProjectCurrentMovementProhibition(r,u);}
        std::optional<bool> CostFree() override {return rn::ProjectCurrentMovementCostFree(r,u);}
        std::optional<bool> BaseCostFallback() override {return rn::ProjectCurrentMovementBaseCostFallback(r,u);}
        std::optional<bool> PassSkill() override {
            const auto* skill=r.definitions.FindSkill("SEID_\x82\xb7\x82\xe8\x94\xb2\x82\xaf");
            return skill?rn::ProjectCurrentEquippedSkill(r,u,std::int16_t(skill->id)):std::nullopt;
        }
        bool Move(int power,std::uint32_t flags) override {
            const auto* job=r.definitions.FindJob(u.job_id);if(!job){result.movement_status=DeploymentMovementStatus::MissingDefinition;return false;}
            const auto p=BuildCurrentDeploymentMovement(r,x,y,job->movement_cost_index,power,flags,u.force_type,route);
            result.movement_status=p.status;if(p.status!=DeploymentMovementStatus::Ok)return false;candidate.movement=*p.image;return true;
        }
        bool Fill(std::uint32_t flags) override {
            DeploymentRangeMasks current{};const auto* selected=ranges;
            if(!selected){const auto p=BuildCurrentDeploymentRanges(r,slot,flags,flags);
                result.range_status=p.status;result.item_range_status=p.item_range_status;
                if(p.status!=DeploymentRangeStatus::Ok){result.fill_status=DeploymentFillStatus::InvalidRange;return false;}
                current=p.masks;selected=&current;}
            const auto p=BuildCurrentDeploymentFill(r,slot,candidate.movement,*selected,flags);result.fill_status=p.status;
            if(p.status!=DeploymentFillStatus::Ok)return false;candidate.ranges=p.planes;return true;
        }
        std::optional<bool> SameAsActiveForce() override {
            const auto& p=r.game.phase;
            if(!r.game.map_active||p.stage==rn::PhaseAccessStage::Unbound||p.chapter_index!=r.game.campaign.current_chapter_index||p.situation.active_force>=3)return std::nullopt;
            return u.force_type==p.situation.active_force;
        }
        bool EraseCurrentCell() override {
            if(u.x<0||u.x>=32||u.y<0||u.y>=32)return false;candidate.movement[u.y*32+u.x]=-1;return true;
        }
    } svc(r,slot,x,y,candidate,result,ranges,route);
    result.operation=RunUnitDeploymentExact(power,flags,extra,svc);
    if(result.operation.status==UnitDeploymentStatus::Ok)out=candidate;return result;
}
}
