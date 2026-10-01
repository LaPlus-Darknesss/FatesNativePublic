#pragma once
#include <vector>
#include "fates/graphics/game_effect.hpp"
#include "fates/map/native_field_object.hpp"
namespace PartsConst { enum class Level:int; }
using MapPose=fates::map::native::FieldPose;
class FieldEffectNode {
public:
    FieldEffectNode(); ~FieldEffectNode(); void SetTransform(const nn::math::MTX34&); void Load(const char*,const MapPose&); void SetColor(const Color8&); void SetLevel(PartsConst::Level);
private: friend class FieldEffectList; fates::map::native::FieldEffectState state_;
};
class FieldEffectList { public: void FadeOut(); ~FieldEffectList(); std::vector<FieldEffectNode*> nodes{}; };
