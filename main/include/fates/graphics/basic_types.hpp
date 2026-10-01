#pragma once

#include <cstdint>

// Small ABI/value shells used by first-party Fates graphics code.  They model
// the value layout needed by readable source without importing Nintendo SDK
// implementations into the Fates-owned source lane.
namespace nn::math {
struct VEC3 { float x{}, y{}, z{}; };
struct MTX34 { float m[3][4]{}; };
struct MTX44 { float m[4][4]{}; };
}

struct Color8 {
    std::uint8_t r{}, g{}, b{}, a{};
};

struct AABB {
    nn::math::VEC3 min{};
    nn::math::VEC3 max{};
};
