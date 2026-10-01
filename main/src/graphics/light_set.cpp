#include "fates/graphics/light_set.hpp"
#include <algorithm>
LightSet::LightSet()=default;
LightSet::~LightSet()=default;
bool LightSet::Add(const nw::h3d::res::LightContent* l){
    if(l==nullptr || !fates::decomp_detail::IsLightEnabled(l)) return false;
    switch(fates::decomp_detail::GetLightKind(l)){
    case 1: if(!hemisphere_){hemisphere_=l;return true;} break;
    case 2: if(!ambient_){ambient_=l;return true;} break;
    case 5: case 6: case 7: if(vertexCount_<8){vertex_[vertexCount_++]=l;return true;} break;
    case 9: case 10: case 11: if(fragmentCount_<8){fragment_[fragmentCount_++]=l;return true;} break;
    } return false;
}
void LightSet::Reset(){ hemisphere_=ambient_=nullptr; vertexCount_=fragmentCount_=0; commands_.attitudeWordCount=0; }
bool LightSet::Remove(const nw::h3d::res::LightContent* l){
    if (l == nullptr) {
        return false;
    }
    const int k = fates::decomp_detail::GetLightKind(l);
    if(k==1 && hemisphere_==l){hemisphere_=nullptr;return true;} if(k==2 && ambient_==l){ambient_=nullptr;return true;}
    auto erase=[&](auto& a,int& n){for(int i=0;i<n;i++) if(a[i]==l){for(int j=i+1;j<n;j++)a[j-1]=a[j];a[--n]=nullptr;return true;}return false;};
    if (k >= 5 && k <= 7) {
        return erase(vertex_, vertexCount_);
    }
    if (k >= 9 && k <= 11) {
        return erase(fragment_, fragmentCount_);
    }
    return false;
}
void LightSet::Commit(nw::h3d::SceneState& s,const nn::math::MTX34& v,const nn::math::MTX34& w){ fates::decomp_detail::CommitLightSet(s,setIndex_,hemisphere_,ambient_,vertex_.data(),vertexCount_,fragment_.data(),fragmentCount_,v,w,commands_); }
Color8 LightSet::GetSkyColor() const { return hemisphere_?fates::decomp_detail::GetHemisphereSkyColor(hemisphere_):Color8{0x80,0x80,0x80,0xff}; }
float LightSet::GetSkyWeight() const { return hemisphere_?fates::decomp_detail::GetHemisphereSkyWeight(hemisphere_):0.5f; }
Color8 LightSet::GetGroundColor() const { return hemisphere_?fates::decomp_detail::GetHemisphereGroundColor(hemisphere_):Color8{0x80,0x80,0x80,0xff}; }
Color8 LightSet::GetAmbientColor() const { Color8 c{}; for(int i=0;i<fragmentCount_;i++) if(fates::decomp_detail::GetLightKind(fragment_[i])==9) fates::decomp_detail::AccumulateLightColor(c,fates::decomp_detail::GetDirectionalAmbientColor(fragment_[i])); if(ambient_) fates::decomp_detail::AccumulateLightColor(c,fates::decomp_detail::GetAmbientLightColor(ambient_)); return c; }
Color8 LightSet::GetDiffuseColor() const { Color8 c{}; for(int i=0;i<fragmentCount_;i++) if(fates::decomp_detail::GetLightKind(fragment_[i])==9) fates::decomp_detail::AccumulateLightColor(c,fates::decomp_detail::GetDirectionalDiffuseColor(fragment_[i])); return c; }
const nw::h3d::res::LightContent* LightSet::GetDirectionaLight() const { for(int i=0;i<fragmentCount_;i++) if(fates::decomp_detail::GetLightKind(fragment_[i])==9) return fragment_[i]; return nullptr; }
void LightSet::MakeUniformCommand(nn::gr::CTR::CommandBufferJumpHelper& h,const nn::gr::CTR::BindSymbolVSInteger& i,const nn::gr::CTR::BindSymbolVSFloat& a,const nn::gr::CTR::BindSymbolVSFloat& b,const nn::gr::CTR::BindSymbolVSFloat& c,const nn::gr::CTR::BindSymbolVSFloat& d) const { if(hemisphere_) fates::decomp_detail::EmitHemisphereUniformCommand(h,hemisphere_,i,a,b,c,d); }
void LightSet::MakeAttitudeCommand(nn::gr::CTR::CommandBufferJumpHelper& h) const { if(commands_.attitudeWordCount) fates::decomp_detail::EmitLightAttitudeCommand(h,commands_); }
bool LightSet::IsExist() const { return hemisphere_||ambient_||vertexCount_>0||fragmentCount_>0; }
