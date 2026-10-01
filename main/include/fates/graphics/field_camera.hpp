#pragma once
#include "fates/graphics/icamera.hpp"
class FieldCamera : public ICamera {
public: FieldCamera(); ~FieldCamera() override; void UpdateProj(); void UpdateView(); void Tick(float); bool IsBindControl() const;
private: void* cameraAnim_{}; void* control_{};
};
