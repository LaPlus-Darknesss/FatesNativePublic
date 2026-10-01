#include "fates/map/sequence_helper.hpp"
#include "fates/detail/map_battle_resolution_runtime.hpp"
namespace map::SequenceHelper {
void MaterialGain(ProcInst* p,Material::Type t,int amount,int sub){fates::decomp_detail::MapBattleResolutionCall("SequenceHelper.MaterialGain",p,t,amount,sub);}
void MaterialGain(ProcInst* p,map::trick::Data* d){fates::decomp_detail::MapBattleResolutionCall("SequenceHelper.MaterialGainFromTrick",p,d);}
void GoldGainSilent(int forceType,int amount){
    // PROVEN: only forces allied with numeric force 0 mutate player gold; retail
    // clamps the result to zero on underflow and to the retail maximum on gain.
    fates::decomp_detail::MapBattleResolutionCall("SequenceHelper.GoldGainSilent",forceType,amount);
}
void ItemGainSilent(Unit* u,const unit::Item* item){
    // PROVEN: acquisition chooses Unit inventory, transporter, or the item's
    // gold-conversion path from item flags, recipient state and capacity. Clone
    // inventory state is synchronized after Unit insertion.
    fates::decomp_detail::MapBattleResolutionCall("SequenceHelper.ItemGainSilent",u,item);
}
void ItemChapterLimitedToTransporter(Unit* u,bool equipped){fates::decomp_detail::MapBattleResolutionCall("SequenceHelper.ItemChapterLimitedToTransporter",u,equipped);}
bool IsGainItemWrong(const Unit* u,const unit::Item* item){return fates::decomp_detail::MapBattleResolutionValue<bool>("SequenceHelper.IsGainItemWrong",u,item);}
void GoldGain(ProcInst* p,int forceType,int amount,const char* msg,bool wait){fates::decomp_detail::MapBattleResolutionCall("SequenceHelper.GoldGain",p,forceType,amount,msg,wait);}
void ItemGain(ProcInst* p,Unit* u,const unit::Item* item,bool show){fates::decomp_detail::MapBattleResolutionCall("SequenceHelper.ItemGain",p,u,item,show);}
namespace detail {
namespace {inline void G(const char* n){fates::decomp_detail::MapBattleResolutionCall(n);}}
void GainItemProcess::ShowMessage(){G("SequenceHelper.GainItem.ShowMessage");}
void GainItemProcess::SkipRollback(){G("SequenceHelper.GainItem.SkipRollback");}
void GainItemProcess::Branch(){G("SequenceHelper.GainItem.Branch");}
void GainItemProcess::GainImpl(){G("SequenceHelper.GainItem.GainImpl");}
void GainItemProcess::Send(){G("SequenceHelper.GainItem.Send");}
void GainMaterialProcess::ShowMessage(){G("SequenceHelper.GainMaterial.ShowMessage");}
void GainMaterialProcess::GainImpl(){G("SequenceHelper.GainMaterial.GainImpl");}
void GainMaterialFromTrickProcess::GainImpl(){G("SequenceHelper.GainMaterialFromTrick.GainImpl");}
}
} // namespace map::SequenceHelper
