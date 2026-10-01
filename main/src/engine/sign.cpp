#include "fates/engine/sign.hpp"

#include "fates/detail/metadata_runtime.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace sign {
namespace {
std::array<std::uint32_t, 256>* gCrcTable = nullptr;
constexpr std::uint32_t kCrcPolynomial = 0xEDB88320u;
}

void Initialize() {
    if (gCrcTable != nullptr) {
        return;
    }

    gCrcTable = new std::array<std::uint32_t, 256>{};
    for (std::uint32_t value = 0; value < 256; ++value) {
        std::uint32_t crc = value;
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 1u) != 0
                ? kCrcPolynomial ^ (crc >> 1)
                : crc >> 1;
        }
        (*gCrcTable)[value] = crc;
    }
}

std::uint32_t CalculateCrc32(const void* data, int size) {
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    std::uint32_t crc = 0xFFFFFFFFu;

    for (int i = 0; i < size; ++i) {
        crc = (*gCrcTable)[(bytes[i] ^ crc) & 0xFFu] ^ (crc >> 8);
    }
    return ~crc;
}

std::uint16_t GetSum16(const char* text) {
    std::int16_t sum = 0;
    if (text == nullptr) {
        return 0;
    }

    while (*text != '\0') {
        const auto value = static_cast<std::int8_t>(*text++);
        sum = static_cast<std::int16_t>(sum * 3 + value);
    }
    return static_cast<std::uint16_t>(sum);
}

} // namespace sign

namespace fates::decomp_detail {

std::uint16_t GetStringSum16(const char* text) {
    return sign::GetSum16(text);
}

} // namespace fates::decomp_detail
