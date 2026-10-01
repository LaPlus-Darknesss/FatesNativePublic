#pragma once
#include "fates/graphics/basic_types.hpp"
class ICamera; class ProcInst; class Terrain; class Unit; class UnitActor;
namespace UnitAnim { enum class Type : int; }
namespace map {
class Actor {
public:
    explicit Actor(Unit* unit);
    void ActionBind();
    void ResetAlpha(bool immediate);
    void DrawBalloon(const ICamera* camera,float alpha);
    void ResetMotion(bool keepFacing);
    void UpdateMoved(unsigned int flags);
    void ActionUnbind();
    void ChargeGoBind(ProcInst* proc,unsigned char side);
    void CutInPosBind(ProcInst* proc,int x,int y);
    nn::math::VEC3 DrawIconImpl(const ICamera* camera,const nn::math::VEC3& worldPos,float a,float b,float c,float d,float e,float f);
    void MoveComplete();
    void UpdateHeight();
    void ChangePosBind(ProcInst* proc,int x,int y);
    void ChargeRtnBind(ProcInst* proc);
    void MoveSlideBind(ProcInst* proc,const nn::math::VEC3& target,float speed);
    void MoveStandBind(ProcInst* proc,const nn::math::VEC3& target,float speed);
    void SetUnitMotion(UnitAnim::Type motion,bool loop);
    void ChargedPosBind(ProcInst* proc,int x,int y);
    void UpdatePosition(bool updateHeight);
    void UpdateUnitIcon();
    void MoveDoubleOnOff(ProcInst* proc,unsigned char* route,unsigned int routeLength);
    void SetActiveMotion(bool active);
    void MoveRouteInstant(unsigned char* route,unsigned int routeLength);
    void TransposePosBind(ProcInst* proc,int x,int y);
    void UpdateFixedColor();
    nn::math::VEC3 GetRenderScreenPos(const nn::math::VEC3& worldPos,const ICamera* camera);
    void ChangeDoubleOnChildPosBind(ProcInst* proc,int x,int y);
    void ChangeDoubleOffChildPosBind(ProcInst* proc,int x,int y);
    void ChangeDoubleOnParentPosBind(ProcInst* proc);
    void ChangeDoubleOffParentPosBind(ProcInst* proc);
    void Tick();
    void Blink(ProcInst* proc);
    void Shake(ProcInst* proc,float strength);
    void Shine(ProcInst* proc);
    void WarpIn(ProcInst* proc,unsigned int frames);
    void WarpOut(ProcInst* proc,unsigned int frames);
    void ActiveOn(bool immediate);
    void FadeDead(ProcInst* proc);
    void MoveJump(ProcInst* proc,const nn::math::VEC3& target,int frames);
    void ActiveOff(bool immediate);
    void MoveAlpha(ProcInst* proc,int fromAlpha,int toAlpha,int frames,int easing);
    void MoveRoute(ProcInst* proc,unsigned char* route,unsigned int routeLength);
    void MoveSlide(ProcInst* proc,const nn::math::VEC3& target,float speed);
    void MoveStand(ProcInst* proc,const nn::math::VEC3& target,float speed);
    void SetPickUpMotion(bool enabled);

    nn::math::VEC3 GetBasePos() const;
    nn::math::VEC3 GetCellPos() const;
    const Terrain* GetTerrain() const;
    nn::math::VEC3 GetCenterPos() const;
    nn::math::VEC3 GetCellOffset() const;
    nn::math::VEC3 GetOverheadPos() const;

private:
    // OPAQUE: constructor/destructor evidence strongly indicates a UnitActor
    // base, but UnitActor is not source-owned yet; inheritance is intentionally
    // not published until that base family is recovered.
    Unit* unit_{};
};
}
