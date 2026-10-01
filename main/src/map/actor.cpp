#include "fates/map/actor.hpp"
#include "fates/detail/unit_ownership_runtime.hpp"
#include "fates/game/terrain.hpp"
namespace map {
Actor::Actor(Unit* unit):unit_(unit){fates::decomp_detail::UnitOwnershipCall("Actor.Construct.UnitActorBoundary",this,unit);}
#define ACTOR_VOID(name,...) void Actor::name(__VA_ARGS__)
ACTOR_VOID(ActionBind){fates::decomp_detail::UnitOwnershipCall("Actor.ActionBind",this);}
ACTOR_VOID(ResetAlpha,bool immediate){fates::decomp_detail::UnitOwnershipCall("Actor.ResetAlpha",this,immediate);}
ACTOR_VOID(DrawBalloon,const ICamera* camera,float alpha){fates::decomp_detail::UnitOwnershipCall("Actor.DrawBalloon",this,camera,alpha);}
ACTOR_VOID(ResetMotion,bool keepFacing){fates::decomp_detail::UnitOwnershipCall("Actor.ResetMotion",this,keepFacing);}
ACTOR_VOID(UpdateMoved,unsigned int flags){fates::decomp_detail::UnitOwnershipCall("Actor.UpdateMoved",this,flags);}
ACTOR_VOID(ActionUnbind){fates::decomp_detail::UnitOwnershipCall("Actor.ActionUnbind",this);}
ACTOR_VOID(ChargeGoBind,ProcInst* proc,unsigned char side){fates::decomp_detail::UnitOwnershipCall("Actor.ChargeGoBind",this,proc,side);}
ACTOR_VOID(CutInPosBind,ProcInst* proc,int x,int y){fates::decomp_detail::UnitOwnershipCall("Actor.CutInPosBind",this,proc,x,y);}
nn::math::VEC3 Actor::DrawIconImpl(const ICamera* c,const nn::math::VEC3& p,float a,float b,float d,float e,float f,float g){return fates::decomp_detail::UnitOwnershipValue<nn::math::VEC3>("Actor.DrawIconImpl",this,c,p,a,b,d,e,f,g);}
ACTOR_VOID(MoveComplete){fates::decomp_detail::UnitOwnershipCall("Actor.MoveComplete",this);}
ACTOR_VOID(UpdateHeight){fates::decomp_detail::UnitOwnershipCall("Actor.UpdateHeight",this);}
ACTOR_VOID(ChangePosBind,ProcInst* p,int x,int y){fates::decomp_detail::UnitOwnershipCall("Actor.ChangePosBind",this,p,x,y);}
ACTOR_VOID(ChargeRtnBind,ProcInst* p){fates::decomp_detail::UnitOwnershipCall("Actor.ChargeRtnBind",this,p);}
ACTOR_VOID(MoveSlideBind,ProcInst* p,const nn::math::VEC3& v,float s){fates::decomp_detail::UnitOwnershipCall("Actor.MoveSlideBind",this,p,v,s);}
ACTOR_VOID(MoveStandBind,ProcInst* p,const nn::math::VEC3& v,float s){fates::decomp_detail::UnitOwnershipCall("Actor.MoveStandBind",this,p,v,s);}
ACTOR_VOID(SetUnitMotion,UnitAnim::Type m,bool loop){fates::decomp_detail::UnitOwnershipCall("Actor.SetUnitMotion",this,m,loop);}
ACTOR_VOID(ChargedPosBind,ProcInst* p,int x,int y){fates::decomp_detail::UnitOwnershipCall("Actor.ChargedPosBind",this,p,x,y);}
ACTOR_VOID(UpdatePosition,bool h){fates::decomp_detail::UnitOwnershipCall("Actor.UpdatePosition",this,h);}
ACTOR_VOID(UpdateUnitIcon){fates::decomp_detail::UnitOwnershipCall("Actor.UpdateUnitIcon",this);}
ACTOR_VOID(MoveDoubleOnOff,ProcInst* p,unsigned char* r,unsigned int n){fates::decomp_detail::UnitOwnershipCall("Actor.MoveDoubleOnOff",this,p,r,n);}
ACTOR_VOID(SetActiveMotion,bool a){fates::decomp_detail::UnitOwnershipCall("Actor.SetActiveMotion",this,a);}
ACTOR_VOID(MoveRouteInstant,unsigned char* r,unsigned int n){fates::decomp_detail::UnitOwnershipCall("Actor.MoveRouteInstant",this,r,n);}
ACTOR_VOID(TransposePosBind,ProcInst* p,int x,int y){fates::decomp_detail::UnitOwnershipCall("Actor.TransposePosBind",this,p,x,y);}
ACTOR_VOID(UpdateFixedColor){fates::decomp_detail::UnitOwnershipCall("Actor.UpdateFixedColor",this);}
nn::math::VEC3 Actor::GetRenderScreenPos(const nn::math::VEC3& p,const ICamera* c){return fates::decomp_detail::UnitOwnershipValue<nn::math::VEC3>("Actor.GetRenderScreenPos",this,p,c);}
ACTOR_VOID(ChangeDoubleOnChildPosBind,ProcInst* p,int x,int y){fates::decomp_detail::UnitOwnershipCall("Actor.ChangeDoubleOnChildPosBind",this,p,x,y);}
ACTOR_VOID(ChangeDoubleOffChildPosBind,ProcInst* p,int x,int y){fates::decomp_detail::UnitOwnershipCall("Actor.ChangeDoubleOffChildPosBind",this,p,x,y);}
ACTOR_VOID(ChangeDoubleOnParentPosBind,ProcInst* p){fates::decomp_detail::UnitOwnershipCall("Actor.ChangeDoubleOnParentPosBind",this,p);}
ACTOR_VOID(ChangeDoubleOffParentPosBind,ProcInst* p){fates::decomp_detail::UnitOwnershipCall("Actor.ChangeDoubleOffParentPosBind",this,p);}
ACTOR_VOID(Tick){fates::decomp_detail::UnitOwnershipCall("Actor.Tick",this);}
ACTOR_VOID(Blink,ProcInst* p){fates::decomp_detail::UnitOwnershipCall("Actor.Blink",this,p);}
ACTOR_VOID(Shake,ProcInst* p,float s){fates::decomp_detail::UnitOwnershipCall("Actor.Shake",this,p,s);}
ACTOR_VOID(Shine,ProcInst* p){fates::decomp_detail::UnitOwnershipCall("Actor.Shine",this,p);}
ACTOR_VOID(WarpIn,ProcInst* p,unsigned int f){fates::decomp_detail::UnitOwnershipCall("Actor.WarpIn",this,p,f);}
ACTOR_VOID(WarpOut,ProcInst* p,unsigned int f){fates::decomp_detail::UnitOwnershipCall("Actor.WarpOut",this,p,f);}
ACTOR_VOID(ActiveOn,bool i){fates::decomp_detail::UnitOwnershipCall("Actor.ActiveOn",this,i);}
ACTOR_VOID(FadeDead,ProcInst* p){fates::decomp_detail::UnitOwnershipCall("Actor.FadeDead",this,p);}
ACTOR_VOID(MoveJump,ProcInst* p,const nn::math::VEC3& v,int f){fates::decomp_detail::UnitOwnershipCall("Actor.MoveJump",this,p,v,f);}
ACTOR_VOID(ActiveOff,bool i){fates::decomp_detail::UnitOwnershipCall("Actor.ActiveOff",this,i);}
ACTOR_VOID(MoveAlpha,ProcInst* p,int a,int b,int f,int e){fates::decomp_detail::UnitOwnershipCall("Actor.MoveAlpha",this,p,a,b,f,e);}
ACTOR_VOID(MoveRoute,ProcInst* p,unsigned char* r,unsigned int n){fates::decomp_detail::UnitOwnershipCall("Actor.MoveRoute",this,p,r,n);}
ACTOR_VOID(MoveSlide,ProcInst* p,const nn::math::VEC3& v,float s){fates::decomp_detail::UnitOwnershipCall("Actor.MoveSlide",this,p,v,s);}
ACTOR_VOID(MoveStand,ProcInst* p,const nn::math::VEC3& v,float s){fates::decomp_detail::UnitOwnershipCall("Actor.MoveStand",this,p,v,s);}
ACTOR_VOID(SetPickUpMotion,bool e){fates::decomp_detail::UnitOwnershipCall("Actor.SetPickUpMotion",this,e);}
#undef ACTOR_VOID
nn::math::VEC3 Actor::GetBasePos() const{return fates::decomp_detail::UnitOwnershipValue<nn::math::VEC3>("Actor.GetBasePos.heightmap",this);}
nn::math::VEC3 Actor::GetCellPos() const{return fates::decomp_detail::UnitOwnershipValue<nn::math::VEC3>("Actor.GetCellPos.heightmap",this);}
const Terrain* Actor::GetTerrain() const{return fates::decomp_detail::UnitOwnershipValue<const Terrain*>("Actor.GetTerrain.cell_lookup",this);}
nn::math::VEC3 Actor::GetCenterPos() const{return fates::decomp_detail::UnitOwnershipValue<nn::math::VEC3>("Actor.GetCenterPos",this);}
nn::math::VEC3 Actor::GetCellOffset() const{return fates::decomp_detail::UnitOwnershipValue<nn::math::VEC3>("Actor.GetCellOffset",this);}
nn::math::VEC3 Actor::GetOverheadPos() const{return fates::decomp_detail::UnitOwnershipValue<nn::math::VEC3>("Actor.GetOverheadPos",this);}
}
