#include "fates/map/native_deployment_workspace.hpp"
#include "fates/runtime/native_runtime.hpp"
#include <limits>
namespace fates::map::native {
namespace rn=fates::runtime::native;
struct DeploymentWorkspaceAccess {
    static auto& State(rn::NativeRuntime& r){return r.game.deployment_workspace;}
    static const auto& State(const rn::NativeRuntime& r){return r.game.deployment_workspace;}
    static bool Valid(const rn::NativeRuntime& r) {
        const auto& g=r.game;const auto& p=g.phase;
        return g.map_active && p.stage!=rn::PhaseAccessStage::Unbound && p.stage!=rn::PhaseAccessStage::Terminal &&
            p.chapter_index==g.campaign.current_chapter_index && p.situation.active_force<3 && p.situation.human_force<3;
    }
    static bool Matches(const rn::TacticalPhaseContext& a,const rn::TacticalPhaseContext& b) {
        const auto& x=a.situation;const auto& y=b.situation;
        return a.revision==b.revision && a.chapter_index==b.chapter_index && x.active_force==y.active_force &&
            x.human_force==y.human_force && x.turn==y.turn && x.turn_limit==y.turn_limit && x.control==y.control;
    }
    static DeploymentWorkspaceStatus Reset(rn::NativeRuntime& r,std::uint64_t revision) {
        using S=DeploymentWorkspaceStatus;auto& s=State(r);
        if(!Valid(r) || r.game.phase.stage!=rn::PhaseAccessStage::AwaitingEntryServices)return S::InvalidPhase;
        if(revision!=r.game.phase.revision)return S::StaleRevision;
        if(s.selection_revision_==std::numeric_limits<std::uint64_t>::max())return S::RevisionExhausted;
        s.selected_=r.game.phase;++s.selection_revision_;return S::Ok;
    }
    static DeploymentWorkspaceStatus Restore(rn::NativeRuntime& r,const DeploymentFields& fields) {
        auto& s=State(r);if(s.movement_ || s.ranges_)return DeploymentWorkspaceStatus::AlreadyCarried;
        s.movement_=fields.movement;s.ranges_=fields.ranges;return DeploymentWorkspaceStatus::Ok;
    }
    static DeploymentWorkspaceView Read(const rn::NativeRuntime& r) {
        using S=DeploymentWorkspaceStatus;DeploymentWorkspaceView out;const auto& s=State(r);
        if(!Valid(r)){out.status=S::InvalidPhase;return out;}
        if(!s.selected_)return out;
        if(!Matches(*s.selected_,r.game.phase)){out.status=S::ChangedContext;return out;}
        out.status=S::Ok;out.force=s.selected_->situation.active_force;out.selection_revision=s.selection_revision_;
        out.movement=s.movement_?&*s.movement_:nullptr;out.ranges=s.ranges_?&*s.ranges_:nullptr;
        out.terrain=ReadCurrentTerrainImage(r);return out;
    }
    static WorkspaceUnitDeploymentResult Build(rn::NativeRuntime& r,std::uint16_t slot,int x,int y,int power,
        std::uint32_t flags,std::uint32_t extra,const DeploymentRangeMasks* ranges,const DeploymentRouteSnapshot* route) {
        WorkspaceUnitDeploymentResult out;const auto current=Read(r);out.status=current.status;
        if(out.status!=DeploymentWorkspaceStatus::Ok)return out;
        DeploymentFields candidate;
        if(current.movement)candidate.movement=*current.movement;
        if(current.ranges)candidate.ranges=*current.ranges;
        out.deployment=BuildCurrentUnitDeployment(candidate,r,slot,x,y,power,flags,extra,ranges,route);
        if(out.deployment.operation.status!=UnitDeploymentStatus::Ok){out.status=DeploymentWorkspaceStatus::BuildFailed;return out;}
        auto& s=State(r);s.movement_=candidate.movement;
        if(out.deployment.operation.filled)s.ranges_=candidate.ranges;
        return out;
    }
};
DeploymentWorkspaceStatus ResetCurrentDeploymentWorkspace(rn::NativeRuntime& r,std::uint64_t revision){return DeploymentWorkspaceAccess::Reset(r,revision);}
DeploymentWorkspaceStatus RestoreCurrentDeploymentWorkspaceContents(rn::NativeRuntime& r,const DeploymentFields& fields){return DeploymentWorkspaceAccess::Restore(r,fields);}
DeploymentWorkspaceView ReadCurrentDeploymentWorkspace(const rn::NativeRuntime& r){return DeploymentWorkspaceAccess::Read(r);}
WorkspaceUnitDeploymentResult BuildCurrentUnitDeploymentInWorkspace(rn::NativeRuntime& r,std::uint16_t slot,int x,int y,int power,
    std::uint32_t flags,std::uint32_t extra,const DeploymentRangeMasks* ranges,const DeploymentRouteSnapshot* route) {
    return DeploymentWorkspaceAccess::Build(r,slot,x,y,power,flags,extra,ranges,route);
}
}
