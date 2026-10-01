#include "fates/io/compress.hpp"

#include "fates/detail/file_platform_runtime.hpp"

#include <cstdint>
#include <cstring>

namespace {

const std::uint32_t* Words(const void* source) {
    return static_cast<const std::uint32_t*>(source);
}

} // namespace

unsigned int Compress::Uncompress(const void* source, void* destination) {
    const std::uint32_t* words = Words(source);
    std::uint32_t descriptor = *words;

    if ((descriptor & 0x02u) != 0) {
        ++words;
        descriptor = *words;
    }

    const std::uint32_t* sizeWord = words;
    if ((descriptor & 0x02u) != 0) {
        ++sizeWord;
    }

    const unsigned int outputSize = *sizeWord >> 8;
    if ((descriptor & 0x10u) == 0) {
        std::memcpy(destination, words + 1, outputSize);
    } else {
        fates::decomp_detail::UncompressPlatformLz(words, destination);
    }
    return outputSize;
}

unsigned int Compress::GetOverlapSize(const void* source) {
    return *Words(source) >> 8;
}

unsigned int Compress::GetUncompressSize(const void* source) {
    const std::uint32_t* words = Words(source);
    if ((*words & 0x02u) != 0) {
        ++words;
    }
    return *words >> 8;
}
