#include "fates/leafcore/core_leaf_impl.hpp"
#include <limits>

namespace fates::leafcore {

int ParseDecimalUtf16(const std::uint16_t* text) {
    if (text == nullptr) return 0;
    const bool negative = (*text == static_cast<std::uint16_t>('-'));
    if (negative) ++text;
    int value = 0;
    while (*text >= static_cast<std::uint16_t>('0') && *text <= static_cast<std::uint16_t>('9')) {
        value = value * 10 + static_cast<int>(*text - static_cast<std::uint16_t>('0'));
        ++text;
    }
    return negative ? -value : value;
}

Color8 SaturatingAddColor8(Color8 lhs, Color8 rhs) {
    const auto add = [](std::uint8_t a, std::uint8_t b) -> std::uint8_t {
        const unsigned sum = static_cast<unsigned>(a) + static_cast<unsigned>(b);
        return static_cast<std::uint8_t>(sum > 255u ? 255u : sum);
    };
    return {add(lhs.r,rhs.r), add(lhs.g,rhs.g), add(lhs.b,rhs.b), add(lhs.a,rhs.a)};
}

std::array<float,3> AabbCenter(const Aabb3& box) {
    return {(box.min_x + box.max_x) * 0.5f,
            (box.min_y + box.max_y) * 0.5f,
            (box.min_z + box.max_z) * 0.5f};
}

std::uint32_t FindU16Index(const std::uint16_t* values, std::size_t count, std::uint16_t value) {
    if (values == nullptr) return std::numeric_limits<std::uint32_t>::max();
    for (std::size_t i=0; i<count; ++i) {
        if (values[i] == value) return static_cast<std::uint32_t>(i);
    }
    return std::numeric_limits<std::uint32_t>::max();
}

} // namespace fates::leafcore
