#pragma once
#include "fates/graphics/camera_param.hpp"
class CameraStateMachine;
class CameraState {
public:
    CameraState(); virtual ~CameraState()=default;
    virtual void ChangeInput(); virtual void ChangeCamera(int,const char*); virtual void ChangeToFocus(int); virtual void ChangeToNormal(); virtual void ChangeToWinCut(int); virtual void ChangeToDeathCut(int); virtual void ChangeCut();
    virtual void Enter(){} virtual void Tick(){} virtual void Leave(){}
    CameraParam* GetParam(); const CameraParam* GetParam() const;
    void HitCheck(CameraParam* param);
    bool exiting{}; bool entered{}; bool allowEnd{true};
protected: CameraParam param_{};
};
