#include "fates/map/trick.hpp"
#include "fates/detail/tactical_map_runtime.hpp"
#include <cstdint>
namespace map::trick {
namespace {
unsigned int TypeCode(const Data* d){ return fates::decomp_detail::TacticalMapValue<unsigned int>("Trick.Data.TypeCode",d); }
unsigned int Flags(const Data* d){ return fates::decomp_detail::TacticalMapValue<unsigned int>("Trick.Data.Flags",d); }
}

Enumerator::Enumerator(Search s,int x,int y,Type t,unsigned char f){ fates::decomp_detail::TacticalMapCall("Trick.Enumerator.Ctor",this,s,x,y,t,f); }
Enumerator::~Enumerator(){
    // PROVEN: the non-deleting destructor owns/releases its enumeration buffer.
    fates::decomp_detail::TacticalMapCall("Trick.Enumerator.Dtor",this);
}
bool Enumerator::IsExclusion(const Data*){ return false; } // PROVEN base default.
void Enumerator::Enumerate(){ fates::decomp_detail::TacticalMapCall("Trick.Enumerator.Enumerate",this); }
bool EnumeratorInfo::IsExclusion(const Data* d){ return fates::decomp_detail::TacticalMapValue<bool>("Trick.EnumeratorInfo.IsExclusion",this,d); }
bool EnumeratorVisit::IsExclusion(const Data* d){ const auto t=TypeCode(d); return t!=0x01u && t!=0x02u; }
bool EnumeratorCannon::IsExclusion(const Data* d){ return fates::decomp_detail::TacticalMapValue<bool>("Trick.EnumeratorCannon.IsExclusion",this,d); }
bool EnumeratorDeploy::IsExclusion(const Data* d){ return fates::decomp_detail::TacticalMapValue<bool>("Trick.EnumeratorDeploy.IsExclusion",this,d); }
bool EnumeratorEnhance::IsExclusion(const Data* d){ return (TypeCode(d)-0x0fu)>5u; }
bool EnumeratorInformal::IsExclusion(const Data* d){ const auto t=TypeCode(d); return t!=0x0du && t!=0x0eu; }
bool EnumeratorMaterial::IsExclusion(const Data* d){ const auto t=TypeCode(d); return t!=0x17u && t!=0x18u; }
bool EnumeratorBreakable::IsExclusion(const Data* d){ return fates::decomp_detail::TacticalMapValue<bool>("Trick.EnumeratorBreakable.IsExclusion",this,d); }
bool EnumeratorCastleDestroyed::IsExclusion(const Data* d){ return fates::decomp_detail::TacticalMapValue<bool>("Trick.EnumeratorCastleDestroyed.IsExclusion",this,d); }
FocusCalculator::FocusCalculator(const char* label){ fates::decomp_detail::TacticalMapCall("Trick.FocusCalculator.Ctor",this,label); }
PositionEnumerator::PositionEnumerator(const char* label){ fates::decomp_detail::TacticalMapCall("Trick.PositionEnumerator.Ctor",this,label); }
PositionEnumerator::~PositionEnumerator(){
    // PROVEN: the non-deleting destructor releases its dynamically-owned position buffer.
    fates::decomp_detail::TacticalMapCall("Trick.PositionEnumerator.Dtor",this);
}

Data::Data(Data* next,int x,int y,int a3,int a4,Type type,int a6,int a7,int a8,int a9,int a10,int a11,int a12,int a13,const char* label,unsigned char flags){
    fates::decomp_detail::TacticalMapCall("Trick.Data.Ctor",this,next,x,y,a3,a4,type,a6,a7,a8,a9,a10,a11,a12,a13,label,flags);
}
void Data::EnableDone(ProcInst* p){ fates::decomp_detail::TacticalMapCall("Trick.Data.EnableDone",this,p); }
void Data::MutationEffect(ProcInst* p,const char* n,int x,int y){ fates::decomp_detail::TacticalMapCall("Trick.Data.MutationEffect",this,p,n,x,y); }
void Data::Done(ProcInst* p){ fates::decomp_detail::TacticalMapCall("Trick.Data.Done",this,p); }
void Data::Ruin(ProcInst* p){ fates::decomp_detail::TacticalMapCall("Trick.Data.Ruin",this,p); }
void Data::Enable(ProcInst* p){ fates::decomp_detail::TacticalMapCall("Trick.Data.Enable",this,p); }
void Data::Cleanup(){ fates::decomp_detail::TacticalMapCall("Trick.Data.Cleanup",this); }
void Data::Setup(){ fates::decomp_detail::TacticalMapCall("Trick.Data.Setup",this); }
void Data::Mutation(ProcInst* p){ fates::decomp_detail::TacticalMapCall("Trick.Data.Mutation",this,p); }
bool Data::IsBreakable() const { return fates::decomp_detail::TacticalMapValue<bool>("Trick.Data.IsBreakable",this); }
bool Data::IsCannonUse(const ::Unit* u) const { return fates::decomp_detail::TacticalMapValue<bool>("Trick.Data.IsCannonUse",this,u); }
bool Data::IsShowDeploy() const {
    // PROVEN numeric policy, without inventing labels for Type: types 0x0B,
    // 0x19..0x1C are candidates and retail flag bit 0 must be set.
    const auto t=TypeCode(this); const bool typeOk=(t==0x0bu)||((t-0x19u)<4u);
    return typeOk && ((Flags(this)&1u)!=0u);
}
void Data::GetFocusPoint(int* x,int* y) const { fates::decomp_detail::TacticalMapCall("Trick.Data.GetFocusPoint",this,x,y); }
void Data::GetAccessForCannon(int* x,int* y) const { fates::decomp_detail::TacticalMapCall("Trick.Data.GetAccessForCannon",this,x,y); }
void Data::GetUnitDirectionForCannon(int* x,int* y) const { fates::decomp_detail::TacticalMapCall("Trick.Data.GetUnitDirectionForCannon",this,x,y); }
bool Data::IsDestroyTargetCastleOffense() const { return (TypeCode(this)-0x0fu)<8u; }
void Data::GetHelp() const { fates::decomp_detail::TacticalMapCall("Trick.Data.GetHelp",this); }
bool Data::IsAccess(int x,int y) const { return fates::decomp_detail::TacticalMapValue<bool>("Trick.Data.IsAccess",this,x,y); }
bool Data::IsCannon() const { return (TypeCode(this)-0x19u)<3u; }
void Data::SetDeploy() const { fates::decomp_detail::TacticalMapCall("Trick.Data.SetDeploy",this); }

namespace anonymous_namespace {
class ProcMutation {
public:
    void FadeInUnit(){ fates::decomp_detail::TacticalMapCall("Trick.ProcMutation.FadeInUnit",this); }
    void PlayEffect(){ fates::decomp_detail::TacticalMapCall("Trick.ProcMutation.PlayEffect",this); }
    void WaitEffect(){ fates::decomp_detail::TacticalMapCall("Trick.ProcMutation.WaitEffect",this); }
    void FadeOutUnit(){ fates::decomp_detail::TacticalMapCall("Trick.ProcMutation.FadeOutUnit",this); }
    void EndAnim(){ fates::decomp_detail::TacticalMapCall("Trick.ProcMutation.EndAnim",this); }
    void WaitAnim(){ fates::decomp_detail::TacticalMapCall("Trick.ProcMutation.WaitAnim",this); }
    void WaitUnit(){ fates::decomp_detail::TacticalMapCall("Trick.ProcMutation.WaitUnit",this); }
    void BeginAnim(){ fates::decomp_detail::TacticalMapCall("Trick.ProcMutation.BeginAnim",this); }
    void EntryUnit(){ fates::decomp_detail::TacticalMapCall("Trick.ProcMutation.EntryUnit",this); }
};
} // namespace anonymous_namespace
} // namespace map::trick
