#include "fates/map/sequence_helper.hpp"
#include "fates/detail/map_battle_resolution_runtime.hpp"
namespace map::SequenceHelper {
void HpHeal(ProcInst* p,Unit* u,int amount,bool effect,bool wait){fates::decomp_detail::MapBattleResolutionCall("SequenceHelper.HpHeal",p,u,amount,effect,wait);}
void HpDamage(ProcInst* p,Unit* u,int amount,bool effect,bool wait){fates::decomp_detail::MapBattleResolutionCall("SequenceHelper.HpDamage",p,u,amount,effect,wait);}
namespace detail {
namespace {inline void H(const char* n){fates::decomp_detail::MapBattleResolutionCall(n);}}
void HpHealProcess::PlayEffect(){H("SequenceHelper.HpHeal.PlayEffect");}
void HpHealProcess::Construct(Unit* u,int amount){fates::decomp_detail::MapBattleResolutionCall("SequenceHelper.HpHeal.Construct",u,amount);}
void HpHealProcess::Execute(){H("SequenceHelper.HpHeal.Execute");}
void HpDamageProcess::PlayEffect(){H("SequenceHelper.HpDamage.PlayEffect");}
void HpDamageProcess::Execute(){H("SequenceHelper.HpDamage.Execute");}
void HpDamageProcess::Construct(Unit* u,int amount){fates::decomp_detail::MapBattleResolutionCall("SequenceHelper.HpDamage.Construct",u,amount);}
#define HPSTEP(Name) void HpEffectProcess::Name(){H("SequenceHelper.HpEffect." #Name);}
HPSTEP(Persistent) HPSTEP(ExecuteWait) HPSTEP(HpWindowOpen) HPSTEP(HpWindowWait)
HPSTEP(EffectEndWait) HPSTEP(HpWindowClose) HPSTEP(HpWindowCreate) HPSTEP(HpWindowDelete)
HPSTEP(ClearSkipStatus) HPSTEP(EffectBeginWait) HPSTEP(SkipInputEnable) HPSTEP(SkipInputDisable)
#undef HPSTEP
}
} // namespace map::SequenceHelper
