#pragma once
#include "fates/graphics/auto_camera.hpp"
class ShadowCamera { public: ~ShadowCamera(); };
class StarginCamera : public AutoCamera { public: void ChangeCut() override; };
