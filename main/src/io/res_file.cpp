#include "fates/io/res_file.hpp"

#include "fates/detail/resource_file_runtime.hpp"
#include "fates/io/file_object.hpp"

using fates::decomp_detail::ResourceCategory;

namespace {

const void* GetResource(const FileObject* object, ResourceCategory category, int index) {
    if (index == fates::decomp_detail::kInvalidResourceIndex) {
        return nullptr;
    }
    return fates::decomp_detail::GetResourceAt(object, category, index);
}

int FindResource(const FileObject* object, ResourceCategory category, const char* identifier) {
    if (identifier == nullptr) {
        return fates::decomp_detail::kInvalidResourceIndex;
    }
    return fates::decomp_detail::FindResourceIndex(object, category, identifier);
}

int CountResource(const FileObject* object, ResourceCategory category) {
    return fates::decomp_detail::GetResourceCount(object, category);
}

} // namespace

ResFile::ResFile() = default;

ResFile::~ResFile() {
    Free();
}

void ResFile::Link(const ResFile& source) {
    if (!source.IsDone()) {
        return;
    }

    FileObject* const destinationObject = GetFileObject();
    const FileObject* const sourceObject = source.GetFileObject();
    if (destinationObject == nullptr || sourceObject == nullptr) {
        return;
    }

    fates::decomp_detail::LinkResourceFileObjects(destinationObject, sourceObject);
}

const nw::h3d::res::AnimContent* ResFile::GetCamAnim(int index) const { return static_cast<const nw::h3d::res::AnimContent*>(GetResource(GetFileObject(), ResourceCategory::CameraAnimation, index)); }
const void* ResFile::GetFogAnim(int index) const {
    return GetResource(GetFileObject(), ResourceCategory::FogAnimation, index);
}
const nw::h3d::res::AnimContent* ResFile::GetMatAnim(int index) const { return static_cast<const nw::h3d::res::AnimContent*>(GetResource(GetFileObject(), ResourceCategory::MaterialAnimation, index)); }
const nw::h3d::res::TextureContent* ResFile::GetTexture(int index) const {
    if (index == fates::decomp_detail::kInvalidResourceIndex) {
        return nullptr;
    }
    return fates::decomp_detail::GetTextureContent(GetFileObject(), index);
}
const nw::h3d::res::AnimContent* ResFile::GetVisAnim(int index) const { return static_cast<const nw::h3d::res::AnimContent*>(GetResource(GetFileObject(), ResourceCategory::VisibilityAnimation, index)); }
bool ResFile::HasAnyAnim(const char* identifier) const {
    return identifier != nullptr &&
        fates::decomp_detail::HasAnyModelAnimation(GetFileObject(), identifier);
}

int ResFile::GetFogCount() const { return CountResource(GetFileObject(), ResourceCategory::Fog); }
int ResFile::GetFogIndex(const char* identifier) const { return FindResource(GetFileObject(), ResourceCategory::Fog, identifier); }
const nw::h3d::res::AnimContent* ResFile::GetSkelAnim(int index) const { return static_cast<const nw::h3d::res::AnimContent*>(GetResource(GetFileObject(), ResourceCategory::SkeletalAnimation, index)); }
const nw::h3d::res::AnimContent* ResFile::GetLightAnim(int index) const { return static_cast<const nw::h3d::res::AnimContent*>(GetResource(GetFileObject(), ResourceCategory::LightAnimation, index)); }
int ResFile::GetLightCount() const { return CountResource(GetFileObject(), ResourceCategory::Light); }
int ResFile::GetLightIndex(const char* identifier) const { return FindResource(GetFileObject(), ResourceCategory::Light, identifier); }
int ResFile::GetModelCount() const { return CountResource(GetFileObject(), ResourceCategory::Model); }
int ResFile::GetSceneCount() const { return CountResource(GetFileObject(), ResourceCategory::Scene); }
int ResFile::GetSceneIndex(const char* identifier) const { return FindResource(GetFileObject(), ResourceCategory::Scene, identifier); }
int ResFile::GetCamAnimIndex(const char* identifier) const { return FindResource(GetFileObject(), ResourceCategory::CameraAnimation, identifier); }
int ResFile::GetFogAnimIndex(const char* identifier) const { return FindResource(GetFileObject(), ResourceCategory::FogAnimation, identifier); }
int ResFile::GetMatAnimCount() const { return CountResource(GetFileObject(), ResourceCategory::MaterialAnimation); }
int ResFile::GetMatAnimIndex(const char* identifier) const { return FindResource(GetFileObject(), ResourceCategory::MaterialAnimation, identifier); }
int ResFile::GetTextureIndex(const char* identifier) const { return FindResource(GetFileObject(), ResourceCategory::Texture, identifier); }
int ResFile::GetVisAnimCount() const { return CountResource(GetFileObject(), ResourceCategory::VisibilityAnimation); }
int ResFile::GetVisAnimIndex(const char* identifier) const { return FindResource(GetFileObject(), ResourceCategory::VisibilityAnimation, identifier); }
int ResFile::GetSkelAnimCount() const { return CountResource(GetFileObject(), ResourceCategory::SkeletalAnimation); }
int ResFile::GetSkelAnimIndex(const char* identifier) const { return FindResource(GetFileObject(), ResourceCategory::SkeletalAnimation, identifier); }
int ResFile::GetLightAnimIndex(const char* identifier) const { return FindResource(GetFileObject(), ResourceCategory::LightAnimation, identifier); }

const void* ResFile::GetFog(int index) const { return GetResource(GetFileObject(), ResourceCategory::Fog, index); }
const nw::h3d::res::LightContent* ResFile::GetLight(int index) const { return static_cast<const nw::h3d::res::LightContent*>(GetResource(GetFileObject(), ResourceCategory::Light, index)); }
const nw::h3d::res::ModelContent* ResFile::GetModel(int index) const {
    if (index == fates::decomp_detail::kInvalidResourceIndex) {
        return nullptr;
    }
    return fates::decomp_detail::GetModelContent(GetFileObject(), index);
}
const void* ResFile::GetScene(int index) const { return GetResource(GetFileObject(), ResourceCategory::Scene, index); }
const nw::h3d::res::CameraContent* ResFile::GetCamera(int index) const { return static_cast<const nw::h3d::res::CameraContent*>(GetResource(GetFileObject(), ResourceCategory::Camera, index)); }

int ResFile::FindModelIndex(const char* identifier) const {
    return FindResource(GetFileObject(), ResourceCategory::Model, identifier);
}
