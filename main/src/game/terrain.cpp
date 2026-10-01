#include "fates/game/terrain.hpp"
#include "fates/detail/unit_ownership_runtime.hpp"
void Terrain::Initialize(const char* a){fates::decomp_detail::UnitOwnershipCall("Terrain.Initialize",a);}
const char* Terrain::GetFieldName(){return fates::decomp_detail::UnitOwnershipValue<const char*>("Terrain.GetFieldName");}
const void* Terrain::GetImageData(){return fates::decomp_detail::UnitOwnershipValue<const void*>("Terrain.GetImageData");}
const void* Terrain::TryGetImageData(){return fates::decomp_detail::UnitOwnershipValue<const void*>("Terrain.TryGetImageData");}
const Terrain* Terrain::Get(const char* id){return fates::decomp_detail::UnitOwnershipValue<const Terrain*>("Terrain.Get.id",id);}
const Terrain* Terrain::Get(int id){return fates::decomp_detail::UnitOwnershipValue<const Terrain*>("Terrain.Get.numeric",id);}
const Terrain* Terrain::TryGet(const char* id){return fates::decomp_detail::UnitOwnershipValue<const Terrain*>("Terrain.TryGet",id);}
void Terrain::Finalize(){fates::decomp_detail::UnitOwnershipCall("Terrain.Finalize");}
bool Terrain::IsCastleUnitDispos() const{return fates::decomp_detail::UnitOwnershipValue<bool>("Terrain.IsCastleUnitDispos",this);}
const wchar_t* Terrain::GetName() const{return fates::decomp_detail::UnitOwnershipValue<const wchar_t*>("Terrain.GetName.Mess",this);}
