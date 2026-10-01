#pragma once
#include "fates/graphics/camera_param.hpp"
class CamUtil {
public:
    static nn::math::VEC3 CalcHitPos(const nn::math::VEC3&); static nn::math::VEC3 CalcHitArea(const nn::math::VEC3&,const nn::math::VEC3&);
    static bool IsLongRange(); static bool IsCloseRange(); static bool IsCutChanged(const nn::math::VEC3&,const nn::math::VEC3&);
    static void CalcTPSCamera(CameraParam*,int); static void CalcBossCamera(CameraParam*,const char*); static void CalcHeadCamera(CameraParam*,const nn::math::VEC3&,const nn::math::VEC3&); static void CalcSideCamera(CameraParam*,bool); static void CalcFixedCamera(CameraParam*,bool); static void CalcIntroCamera(CameraParam*); static void CalcRotateCamera(CameraParam*,const char*,const nn::math::VEC3&,bool); static void CalcTargetCamera(CameraParam*,const char*,const nn::math::VEC3&,int,bool);
    static nn::math::VEC3 GetUnitFlatDir(); static nn::math::VEC3 GetUnitHeadPos(int); static nn::math::VEC3 GetUnitRootDir(bool); static const char* GetBossCameraName(); static nn::math::VEC3 GetUnitHeadRotate(int); static bool IsFlip(const nn::math::VEC3&,const nn::math::VEC3&);
};
