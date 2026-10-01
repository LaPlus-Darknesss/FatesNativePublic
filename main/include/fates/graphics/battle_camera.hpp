#pragma once
#include <array>
#include "fates/graphics/camera_param.hpp"
#include "fates/graphics/camera_state_machine.hpp"
#include "fates/graphics/camera_motion.hpp"
namespace CameraConst { enum class Mode:int { Event=0,Auto=1,Head=2,TPS=3,Side=4 }; }
class BattleCamera {
public:
    BattleCamera(); ~BattleCamera(); void ResetInput(); bool ChangeMode(CameraConst::Mode,bool force,bool remember); void CameraShake(); bool ChangeFocus(int,bool force); void ResetUnitPos(); void UpdateUnitPos(); void Tick(); void Quake(float amplitude,int frames); void Update(); bool CanChangeMode() const; float GetFocusFactor() const; float GetFocusLength() const;
private: CameraParam param_{}; CameraStateMachine states_{}; CameraInput input_{}; CameraConst::Mode mode_{CameraConst::Mode::Event}; int focusUnit_{-1}; int quakeFrames_{}; float quakeAmplitude_{}; std::array<nn::math::VEC3,4> unitPos_{}; bool active_{true};
};
