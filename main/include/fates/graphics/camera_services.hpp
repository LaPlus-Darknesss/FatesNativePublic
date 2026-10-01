#pragma once
#include "fates/graphics/basic_types.hpp"
class CameraTrack;
class CameraProduct {
public:
    ~CameraProduct(); void Offset(const nn::math::VEC3&,float,const nn::math::VEC3&); CameraTrack* Find(const char*) const;
private: void* opaqueRoot_{};
};
class CameraRotation {
public:
    CameraRotation(); bool Tick(); void Wait(); void Change(const char*);
private: nn::math::VEC3 from_{}; nn::math::VEC3 to_{}; nn::math::VEC3 current_{}; float moveFrame_{}; float moveFrames_{}; float waitFrame_{}; float waitFrames_{};
};
class CameraButtonProc { public: void Persistent(); void Tick(); ~CameraButtonProc()=default; private: void* group_{}; bool disabled_{}; bool persistent_{}; };
class CameraButtonGroup { public: void Draw(); void Tick(); ~CameraButtonGroup()=default; private: int previousIndex_=-1; int flashFrames_{}; };
class CameraButtonController { public: static void Disable(); static void SetDisabled(bool); static void Hide(); static void Show(); static void Create(); static void Delete(); static bool IsDisable(); };
