#include "fates/map/image.hpp"
#include "fates/detail/tactical_map_runtime.hpp"
namespace map {
void Image::Initialize(){ fates::decomp_detail::TacticalMapCall("Image.Initialize"); }
void Image::Deserialize(Stream* s){ fates::decomp_detail::TacticalMapCall("Image.Deserialize",s); }
void Image::Serialize(Stream* s){ fates::decomp_detail::TacticalMapCall("Image.Serialize",s); }
void Image::Update(bool force){ fates::decomp_detail::TacticalMapCall("Image.Update",force); }
void Image::Finalize(){ fates::decomp_detail::TacticalMapCall("Image.Finalize"); }
bool Image::Talk::IsTalk(const ::Unit* a,const ::Unit* b){
    return fates::decomp_detail::TacticalMapValue<bool>("Image.Talk.IsTalk",a,b);
}
void Image::Talk::Update(const ::Unit* u){ fates::decomp_detail::TacticalMapCall("Image.Talk.Update",u); }
void Image::Unit::Add(const ::Unit* u,bool selected,int x,int y){ fates::decomp_detail::TacticalMapCall("Image.Unit.Add",u,selected,x,y); }
void Image::Unit::Delete(const ::Unit* u,int x,int y){ fates::decomp_detail::TacticalMapCall("Image.Unit.Delete",u,x,y); }
void Image::Unit::Update(){ fates::decomp_detail::TacticalMapCall("Image.Unit.Update"); }
const ::Unit* Image::Unit::GetUnit(int x,int y){ return fates::decomp_detail::TacticalMapValue<const ::Unit*>("Image.Unit.GetUnit",x,y); }
void Image::Danger::Add(const ::Unit* u,const map::trick::EnumeratorCannon* e){ fates::decomp_detail::TacticalMapCall("Image.Danger.Add",u,e); }
void Image::Danger::Update(){ fates::decomp_detail::TacticalMapCall("Image.Danger.Update"); }
} // namespace map
