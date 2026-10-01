#pragma once
#include <vector>
#include "fates/graphics/basic_types.hpp"
class CameraCurve { public: static float GetValue(int type,float a,float b,float c,float d); };
class CameraInput {
public:
    enum class Type:int { None=0, Rotate=1, Orbit=2 };
    CameraInput(); void Reset(); void Tick(Type type,float dt,float horizontal,float vertical);
    const nn::math::VEC3& Rotation() const { return rotation_; } float Zoom() const { return zoom_; }
private: nn::math::VEC3 rotation_{}; float zoom_{}; Type type_{Type::None}; bool latched_{};
};
class CameraTrack {
public:
    struct Key { float frame{}; nn::math::VEC3 eye{}; nn::math::VEC3 at{}; float fovy{}; float roll{}; };
    virtual ~CameraTrack();
    std::vector<Key> keys{};
};
