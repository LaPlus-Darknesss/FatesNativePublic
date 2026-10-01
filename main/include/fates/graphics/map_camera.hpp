#pragma once
#include "fates/graphics/camera_base.hpp"
class Stream; class ProcInst;
namespace map {
class Camera : public game::graphics::CameraBase {
public:
    static void Initialize(); static Camera* Get(); static void Finalize();
    void Deserialize(Stream*); void Serialize(Stream*) const;
    float GetPlayAreaH() const; float GetPlayAreaW() const; float GetPlayAreaX() const; float GetPlayAreaY() const;
    ~Camera() override=default;
private: float playAreaH_{}; float playAreaW_{}; float playAreaX_{}; float playAreaY_{};
};
}
namespace castle {
class FocusCamera : public game::graphics::CameraBase {
public:
    enum class TargetType:int { Primary=0, Secondary=1, Tertiary=2 };
    static FocusCamera* Get(); static FocusCamera* TryGet(); static void Create(ProcInst*); static void Destroy();
    void ForceMoveToTargetPos(); void SetTargetPos(TargetType,const nn::math::VEC3&); void SetTargetType(TargetType);
    float GetPlayAreaH() const; float GetPlayAreaW() const; float GetPlayAreaX() const; float GetPlayAreaY() const;
    ~FocusCamera() override=default;
private: nn::math::VEC3 targets_[3]{}; TargetType targetType_{TargetType::Primary}; float playAreaH_{}; float playAreaW_{}; float playAreaX_{}; float playAreaY_{};
};
class FocusCameraProc { public: void Persistent(); ~FocusCameraProc()=default; private: FocusCamera* camera_{}; };
}
