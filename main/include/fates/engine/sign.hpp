#pragma once

#include <cstdint>

namespace sign {

void Initialize();
std::uint32_t CalculateCrc32(const void* data, int size);
std::uint16_t GetSum16(const char* text);

} // namespace sign
