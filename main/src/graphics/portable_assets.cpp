#include "fates/graphics/portable_assets.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

namespace fates::graphics::portable {
namespace {
bool RelativeResource(std::string_view path) {
    if(path.empty()||path.front()=='/'||path.find_first_of(":\\")!=path.npos||path.find('\0')!=path.npos)return false;
    while(!path.empty()) {
        const auto slash=path.find('/');const auto part=path.substr(0,slash);
        if(part.empty()||part=="."||part=="..")return false;
        if(slash==path.npos)break;path.remove_prefix(slash+1);if(path.empty())return false;
    }
    return true;
}
template<class T> bool UniqueNames(const std::vector<T>& records) {
    std::set<std::string> names;
    for(const auto& record:records)if(record.name&&!names.insert(*record.name).second)return false;
    return true;
}
}
ResourceResult PortableAssetStore::Read(std::string_view path) {
    if(!RelativeResource(path))return {AssetStatus::Invalid,{},"Resource path must be an exact relative mount path"};
    const std::string key(path);
    if(const auto it=resources_.find(key);it!=resources_.end())if(auto retained=it->second.lock())return {AssetStatus::Ready,std::move(retained),{}};
    if(!reader_)return {AssetStatus::Missing,{},"No resource reader"};
    std::vector<std::uint8_t> bytes;std::string error;
    if(!reader_(path,bytes,error))return {AssetStatus::Missing,{},std::move(error)};
    auto next=std::make_shared<PortableBchResource>();next->path=key;
    if(!BchFile::Decode(bytes,next->file,error)||!ReadBchModels(*next->file,next->models,error)||!ReadBchTextures(*next->file,next->textures,error))
        return {AssetStatus::Invalid,{},std::move(error)};
    if(!UniqueNames(next->models)||!UniqueNames(next->textures))return {AssetStatus::Ambiguous,{},"Duplicate exact resource names"};
    for(const auto& model:next->models)for(float value:model.matrix)if(!std::isfinite(value))
        return {AssetStatus::Invalid,{},"Nonfinite model matrix"};
    resources_[key]=next;return {AssetStatus::Ready,std::move(next),{}};
}
ModelResult PortableAssetStore::Model(const std::shared_ptr<const PortableBchResource>& resource,std::string_view name) {
    if(!resource||!resource->file)return {AssetStatus::Missing,{},"Model resource is absent"};
    const auto found=std::find_if(resource->models.begin(),resource->models.end(),[&](const auto& m){return m.name&&*m.name==name;});
    if(found==resource->models.end())return {AssetStatus::Missing,{},"Exact model name is absent"};
    const auto index=static_cast<unsigned>(found-resource->models.begin());
    for(const auto& weak:models_)if(auto live=weak.lock();live&&live->resource==resource&&live->index==index)return {AssetStatus::Ready,std::move(live),{}};
    std::vector<BchModelGeometry> geometry;std::vector<BchMaterial> materials;std::string error;
    if(!ReadBchGeometry(*resource->file,geometry,error)||!ReadBchMaterials(*resource->file,materials,error))return {AssetStatus::Invalid,{},std::move(error)};
    if(index>=geometry.size()||geometry[index].model.offset!=found->offset)return {AssetStatus::Invalid,{},"Geometry/model index mismatch"};
    auto next=std::make_shared<PortableModelAsset>();next->resource=resource;next->index=index;next->geometry=std::move(geometry[index]);
    for(auto& material:materials)if(material.model_index==index)next->materials.push_back(std::move(material));
    if(next->materials.size()!=next->geometry.material_count)return {AssetStatus::Invalid,{},"Model material count mismatch"};
    models_.push_back(next);return {AssetStatus::Ready,std::move(next),{}};
}
TextureResult PortableAssetStore::Texture(const std::shared_ptr<const PortableBchResource>& primary,
    const std::shared_ptr<const PortableBchResource>& linked,std::string_view name,bool color_2d_context) {
    const BchTextureDescriptor* descriptor=nullptr;std::shared_ptr<const PortableBchResource> source;
    for(const auto& resource:{primary,linked}) {
        if(!resource||resource==source)continue;
        for(const auto& texture:resource->textures)if(texture.name&&*texture.name==name) {
            if(descriptor)return {AssetStatus::Ambiguous,{},"Texture name exists in multiple linked resources"};
            descriptor=&texture;source=resource;
        }
    }
    if(!descriptor)return {AssetStatus::Missing,{},"Exact texture name is absent"};
    if(descriptor->format==0&&!color_2d_context)return {AssetStatus::Unsupported,{},"Format zero needs an admitted 2D color consumer"};
    for(const auto& weak:textures_)if(auto live=weak.lock();live&&live->resource==source&&live->descriptor.name==descriptor->name)return {AssetStatus::Ready,std::move(live),{}};
    const auto bytes=source->file->Bytes();
    if(descriptor->data_offset>bytes.size())return {AssetStatus::Invalid,{},"Texture storage exceeds resource"};
    auto next=std::make_shared<PortableTextureAsset>();next->resource=source;next->descriptor=*descriptor;std::string error;
    if(!DecodeTextureBase(bytes.subspan(descriptor->data_offset),descriptor->format,descriptor->width,descriptor->height,next->image,error))
        return {AssetStatus::Unsupported,{},std::move(error)};
    textures_.push_back(next);return {AssetStatus::Ready,std::move(next),{}};
}
void PortableAssetStore::CollectExpired() {
    std::erase_if(resources_,[](const auto& pair){return pair.second.expired();});
    std::erase_if(models_,[](const auto& weak){return weak.expired();});
    std::erase_if(textures_,[](const auto& weak){return weak.expired();});
}
}
