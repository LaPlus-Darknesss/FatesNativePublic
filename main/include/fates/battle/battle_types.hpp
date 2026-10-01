#pragma once
#include <cstdint>
#include "fates/graphics/basic_types.hpp"
namespace nn::math { struct VEC2 { float x{}, y{}; }; }
namespace nw::ut { using Color8 = ::Color8; }
struct VEC2XZ { float x{}, z{}; };
namespace AnimClip { enum Type : int {}; }
namespace TimeSpace { enum Type : int {}; }
class AnimObj; class SceneSystem; class Unit; class Person; class Job; class ProcInst;
class BattlePhase; class BattleUnitResName; class BattleUnitParams;
namespace map { class BattleCalculator; class BattleInfo; }
