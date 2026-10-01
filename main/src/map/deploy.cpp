#include "fates/map/deploy.hpp"
#include "fates/detail/tactical_map_runtime.hpp"
namespace map {
void Deploy::Initialize(){ fates::decomp_detail::TacticalMapCall("Deploy.Initialize"); }
void Deploy::Finalize(){ fates::decomp_detail::TacticalMapCall("Deploy.Finalize"); }
void Deploy::DisposMove(int a,int b,int c,int d,unsigned int f){
    // PORTABLE_EXACT: retail is a direct wrapper around Deploy::Move.
    Move(a,b,c,d,f);
}
void Deploy::DoubleFill(const Unit* u,unsigned long long a,unsigned long long b,unsigned long long c,unsigned long long d,int x,int y){ fates::decomp_detail::TacticalMapCall("Deploy.DoubleFill",this,u,a,b,c,d,x,y); }
void Deploy::FillAttack(unsigned long long m,int r){ fates::decomp_detail::TacticalMapCall("Deploy.FillAttack",this,m,r); }
void Deploy::UnitMoveXY(const Unit* u,int x,int y,int m,unsigned int f,unsigned int l){ fates::decomp_detail::TacticalMapCall("Deploy.UnitMoveXY",this,u,x,y,m,f,l); }
void Deploy::RangeAround(int r){ fates::decomp_detail::TacticalMapCall("Deploy.RangeAround",this,r); }
void Deploy::UnitAIMoveXY(const Unit* u,int x,int y,int m,unsigned int f,unsigned int l){ fates::decomp_detail::TacticalMapCall("Deploy.UnitAIMoveXY",this,u,x,y,m,f,l); }
void Deploy::DoubleFillRod(const Unit* u,unsigned long long a,unsigned long long b,int r){ fates::decomp_detail::TacticalMapCall("Deploy.DoubleFillRod",this,u,a,b,r); }
void Deploy::RangeImmobile(int x,int y,int mn,int mx){ fates::decomp_detail::TacticalMapCall("Deploy.RangeImmobile",this,x,y,mn,mx); }
void Deploy::UnitAIMoveLimit(const Unit* u,unsigned int l){ fates::decomp_detail::TacticalMapCall("Deploy.UnitAIMoveLimit",this,u,l); }
void Deploy::DoubleFillAttack(const Unit* u,unsigned long long a,unsigned long long b,int r){ fates::decomp_detail::TacticalMapCall("Deploy.DoubleFillAttack",this,u,a,b,r); }
void Deploy::UnitAIMove(const Unit* u,int m,unsigned int f,unsigned int l){ fates::decomp_detail::TacticalMapCall("Deploy.UnitAIMove",this,u,m,f,l); }
void Deploy::UnitAIAttackLimit(const Unit* u,unsigned int l){ fates::decomp_detail::TacticalMapCall("Deploy.UnitAIAttackLimit",this,u,l); }
void Deploy::Move(int sx,int sy,int mn,int mx,unsigned int f){ fates::decomp_detail::TacticalMapCall("Deploy.Move",this,sx,sy,mn,mx,f); }
bool Deploy::Cannon(int x,int y,int mn,int mx,int radius){
    // PROVEN: retail returns a boolean-like success and writes two cannon-range
    // bit images while clipping Manhattan distance to map bounds.
    return fates::decomp_detail::TacticalMapValue<bool>("Deploy.Cannon",this,x,y,mn,mx,radius);
}
void Deploy::FillRod(unsigned long long m,int r){ fates::decomp_detail::TacticalMapCall("Deploy.FillRod",this,m,r); }
void Deploy::UnitFill(const Unit* u,unsigned int f){ fates::decomp_detail::TacticalMapCall("Deploy.UnitFill",this,u,f); }
void Deploy::UnitMove(const Unit* u,int m,unsigned int f,unsigned int l){ fates::decomp_detail::TacticalMapCall("Deploy.UnitMove",this,u,m,f,l); }
void Deploy::EventMove(int sx,int sy,int gx,int gy,unsigned int f){
    // PORTABLE_EXACT: retail is a direct wrapper around Deploy::Move.
    Move(sx,sy,gx,gy,f);
}
void Deploy::SearchDir(Dir::Type d){ fates::decomp_detail::TacticalMapCall("Deploy.SearchDir",this,d); }
void Deploy::TurnReset(Force::Type f){ fates::decomp_detail::TacticalMapCall("Deploy.TurnReset",this,f); }
void Deploy::GetRangeBit(const Unit* u,unsigned long long* a,unsigned long long* b,unsigned long long* c,unsigned long long* d,int* mn,int* mx,unsigned int f) const {
    fates::decomp_detail::TacticalMapCall("Deploy.GetRangeBit",this,u,a,b,c,d,mn,mx,f);
}
bool Deploy::IsFill(int x,int y) const { return fates::decomp_detail::TacticalMapValue<bool>("Deploy.IsFill",this,x,y); }
int Deploy::MoveImage::Get(int x,int y) const { return fates::decomp_detail::TacticalMapValue<int>("Deploy.MoveImage.Get",this,x,y); }
} // namespace map
