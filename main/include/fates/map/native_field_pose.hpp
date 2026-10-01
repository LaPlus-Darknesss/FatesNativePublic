#pragma once
#include <array>
namespace fates::map::native {
// Original MapPose is nine floats; the friendly graphics alias shares this type.
struct FieldPose {
    std::array<float,3> scale{1,1,1};
    std::array<float,3> rotation_degrees{};
    std::array<float,3> position{};
    bool operator==(const FieldPose&) const = default;
};
// Row-major affine 3x4, acting on column vectors. Translation is column 3.
struct FieldMatrix {
    std::array<float,12> values{1,0,0,0,0,1,0,0,0,0,1,0};
    bool operator==(const FieldMatrix&) const = default;
};
}
