#include "fates/coreimpl/core_service_impl.hpp"
#include <cstring>

namespace fates::coreimpl {

std::uint8_t StreamReadByte(StreamCursor& s) { return s.data[s.position++]; }
std::uint16_t StreamReadShortLE(StreamCursor& s) {
    const std::uint16_t v = static_cast<std::uint16_t>(s.data[s.position]) |
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(s.data[s.position+1]) << 8);
    s.position += 2; return v;
}
std::uint32_t StreamReadLongLE(StreamCursor& s) {
    const std::uint32_t v = static_cast<std::uint32_t>(s.data[s.position]) |
        (static_cast<std::uint32_t>(s.data[s.position+1]) << 8) |
        (static_cast<std::uint32_t>(s.data[s.position+2]) << 16) |
        (static_cast<std::uint32_t>(s.data[s.position+3]) << 24);
    s.position += 4; return v;
}
void StreamReadBlock(StreamCursor& s, std::uint8_t* dst, std::size_t size) {
    for (std::size_t i=0;i<size;++i) dst[i]=s.data[s.position++];
}
const char* StreamReadCString(StreamCursor& s) {
    const char* out=reinterpret_cast<const char*>(s.data+s.position);
    while (s.data[s.position] != 0u) ++s.position;
    ++s.position; return out;
}
void StreamWriteByte(StreamCursor& s, std::uint8_t v) { s.data[s.position++]=v; }
void StreamWriteShortLE(StreamCursor& s, std::uint16_t v) {
    s.data[s.position++]=static_cast<std::uint8_t>(v);
    s.data[s.position++]=static_cast<std::uint8_t>(v>>8);
}
void StreamWriteLongLE(StreamCursor& s, std::uint32_t v) {
    s.data[s.position++]=static_cast<std::uint8_t>(v);
    s.data[s.position++]=static_cast<std::uint8_t>(v>>8);
    s.data[s.position++]=static_cast<std::uint8_t>(v>>16);
    s.data[s.position++]=static_cast<std::uint8_t>(v>>24);
}
void StreamWriteBlock(StreamCursor& s, const std::uint8_t* src, std::size_t size) {
    for (std::size_t i=0;i<size;++i) s.data[s.position++]=src[i];
}
void StreamWriteCString(StreamCursor& s, const char* text) {
    if (text==nullptr || *text=='\0') { s.data[s.position++]=0u; return; }
    while (*text!='\0') s.data[s.position++]=static_cast<std::uint8_t>(*text++);
    s.data[s.position++]=0u;
}

bool AabbIntersects(const Aabb3& a, const Aabb3& b) {
    return b.max_x >= a.min_x && b.max_y >= a.min_y && b.max_z >= a.min_z &&
           a.max_x >= b.min_x && a.max_y >= b.min_y && a.max_z >= b.min_z;
}

void MapRangeAddPoint(MapRangeRect& r, int x, int y) {
    if (x < r.min_x) r.min_x=x;
    if (y < r.min_y) r.min_y=y;
    if (x > r.max_x) r.max_x=x;
    if (y > r.max_y) r.max_y=y;
}

std::size_t PackedFlagByteCount(std::size_t flag_count) { return (flag_count + 7u) >> 3u; }

Color8 MultiplyColor8(const Color8& a, const Color8& b) {
    const auto mul=[](std::uint8_t x,std::uint8_t y)->std::uint8_t {
        return static_cast<std::uint8_t>((static_cast<std::uint32_t>(x) * (static_cast<std::uint32_t>(y)+1u)) >> 8u);
    };
    return {mul(a.r,b.r),mul(a.g,b.g),mul(a.b,b.b),mul(a.a,b.a)};
}

MapPoseWords CopyMapPose(const MapPoseWords& in) { return in; }
std::size_t RandomSeedSerializedWordCount() { return 4u; }

} // namespace fates::coreimpl
