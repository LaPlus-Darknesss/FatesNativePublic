#include "fates/game/chapter.hpp"

#include "fates/detail/core_game_data_runtime.hpp"
#include "fates/engine/ident_hash.hpp"

#include <cstddef>

namespace {

Chapter* gChapters = nullptr;
int gChapterCount = 0;

} // namespace

void Chapter::Initialize(const void* data, int count) {
    gChapters = const_cast<Chapter*>(
        static_cast<const Chapter*>(data));
    gChapterCount = count;
}

Chapter* Chapter::Get(const char* identifier) {
    return static_cast<Chapter*>(
        fates::decomp_detail::gChapterIdentHash->GetSurely(identifier));
}

Chapter* Chapter::Get(std::uint8_t chapterId) {
    return reinterpret_cast<Chapter*>(
        reinterpret_cast<std::byte*>(gChapters) +
        static_cast<std::ptrdiff_t>(chapterId) * 0x1C);
}

int Chapter::GetNum() {
    return gChapterCount;
}

void Chapter::Finalize() {
    if (gChapters != nullptr) {
        gChapterCount = 0;
        gChapters = nullptr;
    }
}
