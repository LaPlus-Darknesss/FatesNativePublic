#pragma once

#include "fates/io/file_base.hpp"

namespace nw::h3d::res {
struct ModelContent;
struct TextureContent;
struct AnimContent;
struct CameraContent;
struct LightContent;
}

class Model;

class ResFile : public FileBase {
public:
    ResFile();
    ~ResFile();

    void Link(const ResFile& source);

    const nw::h3d::res::AnimContent* GetCamAnim(int index) const;
    const void* GetFogAnim(int index) const;
    const nw::h3d::res::AnimContent* GetMatAnim(int index) const;
    const nw::h3d::res::TextureContent* GetTexture(int index) const;
    const nw::h3d::res::AnimContent* GetVisAnim(int index) const;
    bool HasAnyAnim(const char* identifier) const;

    int GetFogCount() const;
    int GetFogIndex(const char* identifier) const;
    const nw::h3d::res::AnimContent* GetSkelAnim(int index) const;
    const nw::h3d::res::AnimContent* GetLightAnim(int index) const;
    int GetLightCount() const;
    int GetLightIndex(const char* identifier) const;
    int GetModelCount() const;
    int GetSceneCount() const;
    int GetSceneIndex(const char* identifier) const;
    int GetCamAnimIndex(const char* identifier) const;
    int GetFogAnimIndex(const char* identifier) const;
    int GetMatAnimCount() const;
    int GetMatAnimIndex(const char* identifier) const;
    int GetTextureIndex(const char* identifier) const;
    int GetVisAnimCount() const;
    int GetVisAnimIndex(const char* identifier) const;
    int GetSkelAnimCount() const;
    int GetSkelAnimIndex(const char* identifier) const;
    int GetLightAnimIndex(const char* identifier) const;

    const void* GetFog(int index) const;
    const nw::h3d::res::LightContent* GetLight(int index) const;
    const nw::h3d::res::ModelContent* GetModel(int index) const;
    const void* GetScene(int index) const;
    const nw::h3d::res::CameraContent* GetCamera(int index) const;

private:
    friend class Model;
    int FindModelIndex(const char* identifier) const;
};
