#include "fates/game/unit_pool.hpp"
#include "fates/detail/unit_ownership_runtime.hpp"

void UnitPool::Initialize(){
    // PROVEN retail capacity: 250 Unit records and ten Force records. Retail
    // initializes Unit indices 1..250, initializes force values 0..9, and
    // initially links every slot into force index 9 before later transfers.
    fates::decomp_detail::UnitOwnershipCall("UnitPool.Initialize.250_units_10_forces");
}
void UnitPool::Deserialize(Stream* s){fates::decomp_detail::UnitOwnershipCall("UnitPool.Deserialize",s);}
Unit* UnitPool::GetFromSkill(unsigned long long m,unsigned int f){return fates::decomp_detail::UnitOwnershipValue<Unit*>("UnitPool.GetFromSkill",m,f);}
Unit* UnitPool::GetFromPerson(const Person* p){return fates::decomp_detail::UnitOwnershipValue<Unit*>("UnitPool.GetFromPerson.ptr",p);}
Unit* UnitPool::GetFromPerson(const Person* p,unsigned int f){return fates::decomp_detail::UnitOwnershipValue<Unit*>("UnitPool.GetFromPerson.ptr.mask",p,f);}
Unit* UnitPool::GetFromPerson(const char* p){return fates::decomp_detail::UnitOwnershipValue<Unit*>("UnitPool.GetFromPerson.id",p);}
Unit* UnitPool::GetFromPerson(const char* p,unsigned int f){return fates::decomp_detail::UnitOwnershipValue<Unit*>("UnitPool.GetFromPerson.id.mask",p,f);}
Unit* UnitPool::GetFromPerson(std::uint16_t p,unsigned int f){return fates::decomp_detail::UnitOwnershipValue<Unit*>("UnitPool.GetFromPerson.numeric.mask",p,f);}
Unit* UnitPool::GetFromPersonOnlyGuest(const Person* p,unsigned int f){return fates::decomp_detail::UnitOwnershipValue<Unit*>("UnitPool.GetFromPersonOnlyGuest",p,f);}
Unit* UnitPool::GetFromPersonIdentifier(const game::packet::Unit* p,unsigned int f){return fates::decomp_detail::UnitOwnershipValue<Unit*>("UnitPool.GetFromPersonIdentifier",p,f);}
Unit* UnitPool::GetFromPersonIdentifierOnlyGuest(const Person* p,const unit::Identifier* i,unsigned int f){return fates::decomp_detail::UnitOwnershipValue<Unit*>("UnitPool.GetFromPersonIdentifierOnlyGuest",p,i,f);}
Unit* UnitPool::Get(int i){return fates::decomp_detail::UnitOwnershipValue<Unit*>("UnitPool.Get",i);}
void UnitPool::Reset(){fates::decomp_detail::UnitOwnershipCall("UnitPool.Reset.250_units_to_force9");}
Unit* UnitPool::GetLast(unsigned int m){return fates::decomp_detail::UnitOwnershipValue<Unit*>("UnitPool.GetLast",m);}
void UnitPool::Finalize(){fates::decomp_detail::UnitOwnershipCall("UnitPool.Finalize");}
int UnitPool::GetCount(unsigned int m){return fates::decomp_detail::UnitOwnershipValue<int>("UnitPool.GetCount.sum_10_forces",m);}
Unit* UnitPool::GetFirst(unsigned int m){return fates::decomp_detail::UnitOwnershipValue<Unit*>("UnitPool.GetFirst",m);}
Force* UnitPool::GetForce(int i){return fates::decomp_detail::UnitOwnershipValue<Force*>("UnitPool.GetForce",i);}
Unit* UnitPool::GetDirect(int i){return fates::decomp_detail::UnitOwnershipValue<Unit*>("UnitPool.GetDirect",i);}
Unit* UnitPool::GetPlayer(){return fates::decomp_detail::UnitOwnershipValue<Unit*>("UnitPool.GetPlayer");}
void UnitPool::Serialize(Stream* s,unsigned int m){fates::decomp_detail::UnitOwnershipCall("UnitPool.Serialize",s,m);}
