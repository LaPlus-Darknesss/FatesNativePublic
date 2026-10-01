#include "fates/map/route.hpp"
#include "fates/detail/tactical_map_runtime.hpp"

namespace map {
void Route::Initialize(){ fates::decomp_detail::TacticalMapCall("Route.Initialize"); }
Route* Route::Get(){ return fates::decomp_detail::TacticalMapValue<Route*>("Route.Get"); }
void Route::Finalize(){ fates::decomp_detail::TacticalMapCall("Route.Finalize"); }

void Route::SetForEvent(int sx,int sy,int gx,int gy){
    fates::decomp_detail::TacticalMapCall("Route.SetForEvent",this,sx,sy,gx,gy);
}
void Route::SetOneRoute(unsigned char* route,unsigned char step){
    if (!route) return;
    route[0]=step; route[1]=0x80;
}
void Route::SetOneRoute(unsigned char* route,int sx,int sy,int gx,int gy){
    if (!route) return;
    unsigned char step=0;
    if (sx<gx) step|=0x02;
    if (gx<sx) step|=0x01;
    if (sy<gy) step|=0x04;
    if (gy<sy) step|=0x08;
    if (step==0) step=0x80;
    route[0]=step; route[1]=0x80;
}
int Route::GetRouteCost(Unit* unit,int sx,int sy,const unsigned char* route){
    // PROVEN control semantics: retail walks until 0x80, updates X/Y from
    // direction bits, accumulates terrain cost, clamps negative costs to 0,
    // and collapses costs >=2 to 1 for the retail "cost free" Unit rule.
    return fates::decomp_detail::TacticalMapValue<int>("Route.GetRouteCost",unit,sx,sy,route);
}
bool Route::TickMindHere(int x,int y,unsigned int flags){
    return fates::decomp_detail::TacticalMapValue<bool>("Route.TickMindHere",this,x,y,flags);
}
bool Route::TickMindSeek(int x,int y,unsigned int flags){
    // PROVEN: retail evaluates reachable candidates, action/item usability,
    // route-cross state and terrain priority; exact equal-score selection
    // consumes RNG, so this remains adapter-owned until differential tests.
    return fates::decomp_detail::TacticalMapValue<bool>("Route.TickMindSeek",this,x,y,flags);
}
void Route::Back(int amount){ fates::decomp_detail::TacticalMapCall("Route.Back",this,amount); }
void Route::Seek(int x,int y,bool alternate){ fates::decomp_detail::TacticalMapCall("Route.Seek",this,x,y,alternate); }
void Route::Tick(int x,int y){ fates::decomp_detail::TacticalMapCall("Route.Tick",this,x,y); }
void Route::Reset(int x,int y){ fates::decomp_detail::TacticalMapCall("Route.ResetXY",this,x,y); }
void Route::Reset(){ fates::decomp_detail::TacticalMapCall("Route.Reset",this); }
int Route::GetGoalX(int index,const unsigned char* route) const {
    return fates::decomp_detail::TacticalMapValue<int>("Route.GetGoalX",this,index,route);
}
int Route::GetGoalY(int index,const unsigned char* route) const {
    return fates::decomp_detail::TacticalMapValue<int>("Route.GetGoalY",this,index,route);
}
void Route::SetForAI(int sx,int sy,int gx,int gy){ fates::decomp_detail::TacticalMapCall("Route.SetForAI",this,sx,sy,gx,gy); }
bool Route::CanMindSeek(int x,int y,unsigned int flags){ return fates::decomp_detail::TacticalMapValue<bool>("Route.CanMindSeek",this,x,y,flags); }
void Route::TickMind(int x,int y,unsigned int flags){ fates::decomp_detail::TacticalMapCall("Route.TickMind",this,x,y,flags); }
void Route::TickMove(int x,int y){ fates::decomp_detail::TacticalMapCall("Route.TickMove",this,x,y); }
int Route::GetCross(int x,int y) const { return fates::decomp_detail::TacticalMapValue<int>("Route.GetCross",this,x,y); }
} // namespace map
