#include "fates/game/force.hpp"
#include "fates/detail/unit_ownership_runtime.hpp"

void Force::Initialize(Type type){ type_=type; fates::decomp_detail::UnitOwnershipCall("Force.Initialize",this,type); }
std::uint32_t Force::GetMaskSameForce(Type type){
    switch(static_cast<int>(type)){case 0:case 3:case 4:return 0x19;case 1:return 0x02;case 2:return 0x04;default:return 0;}
}
std::uint32_t Force::GetMaskSameForceWithLost(Type type){
    switch(static_cast<int>(type)){case 0:case 3:case 4:case 6:return 0x59;case 1:return 0x02;case 2:return 0x04;default:return 0;}
}
std::uint32_t Force::GetMaskSameForceWithDefection(Type type){
    switch(static_cast<int>(type)){case 0:case 3:case 4:case 5:return 0x39;case 1:return 0x02;case 2:return 0x04;default:return 0;}
}
std::uint32_t Force::GetMaskSameForceWithDefectionLost(Type type){
    switch(static_cast<int>(type)){case 0:case 3:case 4:case 5:case 6:return 0x79;case 1:return 0x02;case 2:return 0x04;default:return 0;}
}
void Force::TransferForSortie(Type destination,bool preserveSortieState){fates::decomp_detail::UnitOwnershipCall("Force.TransferForSortie",this,destination,preserveSortieState);}
void Force::Remove(Unit* unit){fates::decomp_detail::UnitOwnershipCall("Force.Remove",this,unit);}
void Force::JoinLast(Unit* unit){fates::decomp_detail::UnitOwnershipCall("Force.JoinLast",this,unit);}
void Force::Transfer(Type destination,bool preserveState){fates::decomp_detail::UnitOwnershipCall("Force.Transfer",this,destination,preserveState);}
void Force::JoinFirst(Unit* unit){fates::decomp_detail::UnitOwnershipCall("Force.JoinFirst",this,unit);}
Force* Force::Get(Type type){return fates::decomp_detail::UnitOwnershipValue<Force*>("Force.Get",type);}
Unit* Force::GetUnitFromSkill(const char* id) const{return fates::decomp_detail::UnitOwnershipValue<Unit*>("Force.GetUnitFromSkill.id",this,id);}
Unit* Force::GetUnitFromSkill(unsigned long long mask) const{return fates::decomp_detail::UnitOwnershipValue<Unit*>("Force.GetUnitFromSkill.mask",this,mask);}
Unit* Force::GetUnitFromPerson(const Person* p) const{return fates::decomp_detail::UnitOwnershipValue<Unit*>("Force.GetUnitFromPerson.ptr",this,p);}
Unit* Force::GetUnitFromPerson(const char* id) const{return fates::decomp_detail::UnitOwnershipValue<Unit*>("Force.GetUnitFromPerson.id",this,id);}
Unit* Force::GetUnitFromPersonIdentifier(const game::packet::Unit* id) const{return fates::decomp_detail::UnitOwnershipValue<Unit*>("Force.GetUnitFromPersonIdentifier",this,id);}
int Force::GetCount() const{return fates::decomp_detail::UnitOwnershipValue<int>("Force.GetCount",this);}
bool Force::IsAllied(Type other) const{
    const int a=static_cast<int>(type_), b=static_cast<int>(other);
    // PROVEN exact retail relation: same force is allied; 0 and 2 are also
    // mutually allied. We deliberately do not assign player/enemy labels here.
    return a==b || (a==0 && b==2) || (a==2 && b==0);
}
