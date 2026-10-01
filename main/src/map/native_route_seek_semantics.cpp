#include "fates/map/native_route_seek_semantics.hpp"

#include <algorithm>
#include <limits>
#include <queue>
#include <stdexcept>
#include <tuple>

namespace fates::map::native {
namespace {
constexpr std::int16_t kUnreachable = -1;
constexpr std::uint8_t kRouteTerminator = 0x80u;
struct QueueNode { int cost; int x; int y; };
struct Greater { bool operator()(const QueueNode& a, const QueueNode& b) const { return a.cost > b.cost; } };
int Index(int width, int x, int y) { return y * width + x; }
}

std::vector<std::int16_t> BuildPositiveCostMovementImage(
    int width, int height, int startX, int startY, int maxCost,
    std::span<const std::int8_t> enterCosts) {
    if (width <= 0 || height <= 0 || static_cast<std::size_t>(width * height) != enterCosts.size())
        throw std::invalid_argument("movement image dimensions");
    if (startX < 0 || startY < 0 || startX >= width || startY >= height)
        throw std::invalid_argument("movement image start");
    std::vector<std::int16_t> image(static_cast<std::size_t>(width * height), kUnreachable);
    std::priority_queue<QueueNode, std::vector<QueueNode>, Greater> q;
    image[Index(width,startX,startY)] = 0; q.push({0,startX,startY});
    constexpr int dx[4] = {-1,1,0,0};
    constexpr int dy[4] = {0,0,1,-1};
    while (!q.empty()) {
        const auto n=q.top();q.pop();
        if (image[Index(width,n.x,n.y)] != n.cost) continue;
        for (int i=0;i<4;++i) {
            const int x=n.x+dx[i], y=n.y+dy[i];
            if (x<0||y<0||x>=width||y>=height) continue;
            const auto step=enterCosts[Index(width,x,y)];
            if (step < 0) continue;
            const int next=n.cost+step;
            auto& cell=image[Index(width,x,y)];
            if (next <= maxCost && (cell < 0 || next < cell)) { cell=static_cast<std::int16_t>(next);q.push({next,x,y}); }
        }
    }
    return image;
}

EventRouteSeekResult SeekEventRouteExact(
    int width, int height, std::span<const std::int16_t> moveImage,
    int goalX, int goalY, const std::function<void(std::uint32_t)>& consumeSystemRngDraw) {
    if (width <= 0 || height <= 0 || static_cast<std::size_t>(width * height) != moveImage.size())
        throw std::invalid_argument("route image dimensions");
    if (goalX < 0 || goalY < 0 || goalX >= width || goalY >= height)
        throw std::invalid_argument("route goal");
    int x=goalX,y=goalY;
    if (moveImage[Index(width,x,y)] < 0) throw std::invalid_argument("unreachable route goal");
    std::vector<std::uint8_t> reversed;
    EventRouteSeekResult result;
    while (moveImage[Index(width,x,y)] > 0) {
        struct Candidate { int value; std::uint8_t bit; int x; int y; };
        std::vector<Candidate> candidates;
        // Retail scan order is left, right, down, up (bits 1,2,4,8).
        constexpr std::uint8_t bits[4]={1,2,4,8};
        constexpr int dx[4]={-1,1,0,0};
        constexpr int dy[4]={0,0,1,-1};
        int best=0xff;
        for (int i=0;i<4;++i) {
            const int nx=x+dx[i], ny=y+dy[i];
            if (nx<0||ny<0||nx>=width||ny>=height) continue;
            const auto v=moveImage[Index(width,nx,ny)];
            const int raw=v<0?0xff:v;
            if (raw < best) { best=raw;candidates.clear(); }
            if (raw == best) candidates.push_back({raw,bits[i],nx,ny});
        }
        if (candidates.empty() || best==0xff) throw std::runtime_error("route backtrack stalled");
        const auto tieCount=static_cast<std::uint32_t>(candidates.size());
        result.backtrackTieCounts.push_back(static_cast<std::uint8_t>(tieCount));
        ++result.systemRngDrawCount;
        if (consumeSystemRngDraw) consumeSystemRngDraw(tieCount);
        // SetForEvent calls Seek(..., alternate=false), so retail overwrites the
        // random selector with zero *after* consuming the draw.
        const auto c=candidates.front();
        std::uint8_t step=0;
        if (c.bit & 1u) step|=2u;
        if (c.bit & 2u) step|=1u;
        if (c.bit & 8u) step|=4u;
        if (c.bit & 4u) step|=8u;
        reversed.push_back(step); x=c.x; y=c.y;
        if (reversed.size()>0x7d) throw std::runtime_error("retail route capacity exceeded");
    }
    result.route.assign(reversed.rbegin(),reversed.rend());
    result.route.push_back(kRouteTerminator);
    return result;
}

} // namespace fates::map::native
