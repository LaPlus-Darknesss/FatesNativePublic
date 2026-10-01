#include "fates/game/family.hpp"
#include "fates/detail/unit_ownership_runtime.hpp"
namespace unit {
void Family::ParentSingle::SetFromPacket(const game::packet::FamilyParent* p){fates::decomp_detail::UnitOwnershipCall("Family.ParentSingle.SetFromPacket",this,p);}
void Family::ParentSingle::Set(Unit* p){fates::decomp_detail::UnitOwnershipCall("Family.ParentSingle.Set",this,p);}
void Family::ParentSingle::ToPacket(game::packet::FamilyParent* p) const{fates::decomp_detail::UnitOwnershipCall("Family.ParentSingle.ToPacket",this,p);}
int Family::ParentSingle::GetMaxLevel() const{return 3;}
int Family::ParentSingle::GetLevelPoint(int level) const{switch(level){case 1:return 1;case 2:return 5;case 3:return 10;case 4:return 99;default:return 0;}}
int Family::ParentSingle::GetLevel(int points) const{return points<1?0:(points<5?1:(points<10?2:3));}
int Family::ParentSingle::GetGrow(int capabilityIndex) const{return fates::decomp_detail::UnitOwnershipValue<int>("Family.ParentSingle.GetGrow",this,capabilityIndex);}
int Family::BrotherSingle::GetMaxLevel() const{return 3;}
int Family::BrotherSingle::GetLevelPoint(int level) const{switch(level){case 1:return 1;case 2:return 5;case 3:return 10;case 4:return 99;default:return 0;}}
int Family::BrotherSingle::GetLevel(int points) const{return points<1?0:(points<5?1:(points<10?2:3));}
void Family::Deserialize(Stream* s){fates::decomp_detail::UnitOwnershipCall("Family.Deserialize",this,s);}
void Family::ClearChapterReliance(){fates::decomp_detail::UnitOwnershipCall("Family.ClearChapterReliance",this);}
void Family::Clear(){retailPayload_.fill(std::byte{});}
Family::ParentSingle* Family::GetParent(const Unit* u){return fates::decomp_detail::UnitOwnershipValue<ParentSingle*>("Family.GetParent.unit",this,u);}
Family::ParentSingle* Family::GetParent(const Person* p){return fates::decomp_detail::UnitOwnershipValue<ParentSingle*>("Family.GetParent.person",this,p);}
Family& Family::operator=(const Family& o){retailPayload_=o.retailPayload_;return *this;}
int Family::GetGrow(const Person* p,int c) const{return fates::decomp_detail::UnitOwnershipValue<int>("Family.GetGrow",this,p,c);}
int Family::GetDualSupport(BattleCapability::Type c,int l) const{return fates::decomp_detail::UnitOwnershipValue<int>("Family.GetDualSupport",this,c,l);}
const Person* Family::GetGrandFather() const{return fates::decomp_detail::UnitOwnershipValue<const Person*>("Family.GetGrandFather",this);}
const Person* Family::GetGrandMother() const{return fates::decomp_detail::UnitOwnershipValue<const Person*>("Family.GetGrandMother",this);}
int Family::GetDoubleCapability(Capability::Type c,int l) const{return fates::decomp_detail::UnitOwnershipValue<int>("Family.GetDoubleCapability",this,c,l);}
bool Family::IsWrong() const{return fates::decomp_detail::UnitOwnershipValue<bool>("Family.IsWrong.enemy_only_parent_job",this);}
int Family::GetLimit(int c) const{return fates::decomp_detail::UnitOwnershipValue<int>("Family.GetLimit",this,c);}
const Family::ParentSingle* Family::GetParent(const Unit* u) const{return fates::decomp_detail::UnitOwnershipValue<const ParentSingle*>("Family.GetParent.const_unit",this,u);}
void Family::Serialize(Stream* s) const{fates::decomp_detail::UnitOwnershipCall("Family.Serialize",this,s);}
}
