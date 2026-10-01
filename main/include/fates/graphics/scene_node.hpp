#pragma once
#include <cstdint>
#include "fates/graphics/basic_types.hpp"

class ICamera; class SceneSystem;
class SceneNode {
public:
    SceneNode();
    virtual ~SceneNode();
    void SetVisible(int slot,bool visible);
    bool IsVisible(int slot) const;
    void SetLocalBox(const AABB& box);
    void SetLocalBox(const nn::math::VEC3& center,const nn::math::VEC3& size);
    void ResetLocalBox();
    void SetWorldTranslate(const nn::math::VEC3& position);
    void UpdateCamera(const ICamera* camera);
    void Attach(SceneSystem* scene);
    void Detach();
private:
    friend class SceneSystem;
    static constexpr std::uint8_t kRejected=0x10, kLocalBoxDirty=0x20, kWorldDirty=0x40;
    SceneNode* prev_{}; SceneNode* next_{}; SceneSystem* scene_{};
    std::uint8_t flags_{};
    float sortDistance_{};
    AABB localBox_{}; AABB worldBox_{}; nn::math::MTX34 world_{};
};
