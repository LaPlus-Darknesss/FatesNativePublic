#pragma once
#include <vector>
#include "fates/graphics/camera_state.hpp"
class CameraStateMachine {
public:
    void Tick(); void ChangeState(CameraState* state); void ClearState(); void ChangeInput(); void ChangeToNormal(); void ChangeToWinCut(int id); void ChangeToDeathCut(int id); void ChangeCut();
    CameraState* GetCurrentState(); const CameraState* GetCurrentState() const; bool IsAllowedToEnd() const;
private: std::vector<CameraState*> states_{};
};
