#include "fates/map/dispos.hpp"
#include "fates/detail/tactical_map_runtime.hpp"
namespace map {
void Dispos::Initialize(){ fates::decomp_detail::TacticalMapCall("Dispos.Initialize"); }
void Dispos::Deserialize(Stream* s){ fates::decomp_detail::TacticalMapCall("Dispos.Deserialize",this,s); }
void Dispos::Serialize(Stream* s) const { fates::decomp_detail::TacticalMapCall("Dispos.Serialize",this,s); }
void Dispos::CreateProcess(ProcInst* p,const char* label,unsigned int f){ fates::decomp_detail::TacticalMapCall("Dispos.CreateProcess",this,p,label,f); }
bool Dispos::IsWaitProcess(){ return fates::decomp_detail::TacticalMapValue<bool>("Dispos.IsWaitProcess"); }
Dispos* Dispos::Get(){ return fates::decomp_detail::TacticalMapValue<Dispos*>("Dispos.Get"); }
void Dispos::Free(){ fates::decomp_detail::TacticalMapCall("Dispos.Free",this); }
bool Dispos::Load(const char* name){
    // PROVEN policy: retail clears an existing archive, prefers a route-aware
    // path, falls back to the common path, and reports false if neither exists.
    return fates::decomp_detail::TacticalMapValue<bool>("Dispos.Load",this,name);
}
void Dispos::Finalize(){ fates::decomp_detail::TacticalMapCall("Dispos.Finalize"); }
bool Dispos::Calculate(Header* h,unsigned int f){ return fates::decomp_detail::TacticalMapValue<bool>("Dispos.CalculateHeader",this,h,f); }
void Dispos::Data::CreateSortie(){ fates::decomp_detail::TacticalMapCall("Dispos.Data.CreateSortie",this); }
bool Dispos::Data::CalculateImpl(unsigned int i,Unit* u,const Assign* a){ return fates::decomp_detail::TacticalMapValue<bool>("Dispos.Data.CalculateImpl",this,i,u,a); }
Unit* Dispos::Data::GetUnit(unsigned int i){ return fates::decomp_detail::TacticalMapValue<Unit*>("Dispos.Data.GetUnit",this,i); }
void Dispos::Data::UnitMove(Unit* u,int x,int y,int d){ fates::decomp_detail::TacticalMapCall("Dispos.Data.UnitMove",this,u,x,y,d); }
int Dispos::Data::Calculate(unsigned int i,const Data* s){ return fates::decomp_detail::TacticalMapValue<int>("Dispos.Data.Calculate",this,i,s); }
bool Dispos::Data::IsEnable(unsigned int i) const { return fates::decomp_detail::TacticalMapValue<bool>("Dispos.Data.IsEnable",this,i); }
} // namespace map
