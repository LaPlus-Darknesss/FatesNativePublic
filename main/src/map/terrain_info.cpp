#include "fates/map/terrain_info.hpp"
#include "fates/detail/unit_ownership_runtime.hpp"
namespace map {
void TerrainInfo::Initialize(){fates::decomp_detail::UnitOwnershipCall("TerrainInfo.Initialize");}
TerrainInfo* TerrainInfo::Get(){return fates::decomp_detail::UnitOwnershipValue<TerrainInfo*>("TerrainInfo.Get");}
void TerrainInfo::Draw(){fates::decomp_detail::UnitOwnershipCall("TerrainInfo.Draw",this);}
void TerrainInfo::HideForDesc(){fates::decomp_detail::UnitOwnershipCall("TerrainInfo.HideForDesc",this);}
void TerrainInfo::Hide(){fates::decomp_detail::UnitOwnershipCall("TerrainInfo.Hide",this);}
void TerrainInfo::Show(int x,int y){fates::decomp_detail::UnitOwnershipCall("TerrainInfo.Show",this,x,y);}
void TerrainInfo::Finalize(){fates::decomp_detail::UnitOwnershipCall("TerrainInfo.Finalize");}
void TerrainInfo::DrawTransformAccessForTargetSelect(float a){fates::decomp_detail::UnitOwnershipCall("TerrainInfo.DrawTransformAccessForTargetSelect",this,a);}
}
