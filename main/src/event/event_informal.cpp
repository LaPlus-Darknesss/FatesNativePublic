#include "fates/event/event_informal.hpp"
#include "fates/detail/event_runtime.hpp"
namespace event::Informal {
int GetRandomFood(Random* r){return fates::decomp_detail::EventRuntimeValue<int>("event.Informal.GetRandomFood",r);} int GetRandomItem(Random* r,ItemTable::Type t,ItemTable::Route q){return fates::decomp_detail::EventRuntimeValue<int>("event.Informal.GetRandomItem",r,t,q);} int GetRandomGemstone(Random* r){return fates::decomp_detail::EventRuntimeValue<int>("event.Informal.GetRandomGemstone",r);}
Type CaclulateTypeForMap(Unit* u,Random* r){
    // PROVEN: Random.GetValue(100) buckets are <20 -> 4, <50 -> 6,
    // <80 -> 3, otherwise 2. Numeric Type names stay conservative.
    return fates::decomp_detail::EventRuntimeValue<Type>("event.Informal.CaclulateTypeForMap",u,r);
}
Type CaclulateTypeForCastle(Random* r){
    // PROVEN: the same 20/30/30/20 split maps to numeric 1,5,3,7.
    return fates::decomp_detail::EventRuntimeValue<Type>("event.Informal.CaclulateTypeForCastle",r);
}
int GetRank(){return fates::decomp_detail::EventRuntimeValue<int>("event.Informal.GetRank");} void Create(ProcInst* p,Unit* a,Unit* b,Type t,unsigned int v){fates::decomp_detail::EventRuntimeCall("event.Informal.Create",p,a,b,t,v);}
namespace detail {
#define S(N) void N(){fates::decomp_detail::EventRuntimeCall("event.Informal." #N);} S(Enhance) S(GainGemstone) S(ShowMessageNone) S(Wait) S(Escape) S(Rollback) S(Reliance) S(GainWeaponExp) S(ShowWeaponLevelUp) S(GainFood) S(GainItem) S(GainMoney)
#undef S
bool GainItemIsExclusion(const void* p){return fates::decomp_detail::EventRuntimeValue<bool>("event.Informal.GainItemIsExclusion",p);}
}
}
