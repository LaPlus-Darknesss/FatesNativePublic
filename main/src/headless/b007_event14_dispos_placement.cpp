#include "fates/headless/b007_event14_dispos_placement.hpp"

#include <limits>
#include <queue>
#include <tuple>
#include <vector>

namespace fates::headless {
namespace {
constexpr int kOriginX = 20;
constexpr int kOriginY = 14;
constexpr int kPreferredX = 15;
constexpr int kPreferredY = 12;
constexpr int kMovementLimit = 100;
constexpr int kUnreachable = -1;

int Index(const B007Event14PlacementGrid& g, int x, int y) {
    return y * g.width + x;
}

bool InBackingGrid(const B007Event14PlacementGrid& g, int x, int y) {
    return x >= 0 && y >= 0 && x < g.width && y < g.height;
}

bool InActiveRect(const B007Event14PlacementGrid& g, int x, int y) {
    return x >= g.active_min_x && y >= g.active_min_y &&
           x < g.active_max_x && y < g.active_max_y;
}

bool ValidShape(const B007Event14PlacementGrid& g) {
    if (g.width <= 0 || g.height <= 0 ||
        g.active_min_x < 0 || g.active_min_y < 0 ||
        g.active_max_x > g.width || g.active_max_y > g.height ||
        g.active_min_x >= g.active_max_x || g.active_min_y >= g.active_max_y) {
        return false;
    }
    const auto n = static_cast<std::size_t>(g.width) * static_cast<std::size_t>(g.height);
    return g.enter_costs.size() == n && g.destination_ok.size() == n &&
           g.occupied.size() == n && g.strict_blocked.size() == n &&
           InBackingGrid(g, kOriginX, kOriginY) && InBackingGrid(g, kPreferredX, kPreferredY);
}

struct Node {
    int cost;
    int x;
    int y;
};
struct Greater {
    bool operator()(const Node& a, const Node& b) const { return a.cost > b.cost; }
};

std::vector<std::int16_t> BuildImage(
    const B007Event14PlacementGrid& g, int sx, int sy, bool use_strict_blockers) {
    std::vector<std::int16_t> out(static_cast<std::size_t>(g.width * g.height), kUnreachable);
    std::priority_queue<Node, std::vector<Node>, Greater> q;
    out[Index(g, sx, sy)] = 0;
    q.push({0, sx, sy});
    constexpr int dx[4] = {-1, 1, 0, 0};
    constexpr int dy[4] = {0, 0, 1, -1};
    while (!q.empty()) {
        const auto n = q.top(); q.pop();
        if (out[Index(g, n.x, n.y)] != n.cost) continue;
        for (int i = 0; i < 4; ++i) {
            const int x = n.x + dx[i], y = n.y + dy[i];
            if (!InActiveRect(g, x, y)) continue;
            const int cell = Index(g, x, y);
            if (use_strict_blockers && g.strict_blocked[cell] != 0) continue;
            const auto step = g.enter_costs[cell];
            if (step < 0) continue;
            const int next = n.cost + step;
            auto& dst = out[cell];
            if (next <= kMovementLimit && (dst < 0 || next < dst)) {
                dst = static_cast<std::int16_t>(next);
                q.push({next, x, y});
            }
        }
    }
    return out;
}

struct Candidate {
    bool found{};
    int x{-1};
    int y{-1};
    int preferred_cost{-1};
    int origin_cost{-1};
    std::uint32_t score{};
};

Candidate SelectCandidate(
    const B007Event14PlacementGrid& g,
    std::span<const std::int16_t> preferred,
    std::span<const std::int16_t> origin) {
    Candidate selected;
    // Retail scans y outer / x inner. It replaces on equal score. One quirk is
    // preserved literally: the first eligible candidate seeds score=0 instead
    // of computing its packed score; only later candidates compute/compare it.
    for (int y = 0; y < g.height; ++y) {
        for (int x = 0; x < g.width; ++x) {
            if (!InActiveRect(g, x, y)) continue;
            const int cell = Index(g, x, y);
            if (g.occupied[cell] != 0 || g.destination_ok[cell] == 0) continue;
            const int pc = preferred[cell], oc = origin[cell];
            if (pc < 0 || oc < 0) continue;
            std::uint32_t score = 0;
            if (selected.found) {
                score = static_cast<std::uint32_t>(255 - oc) +
                        (static_cast<std::uint32_t>(255 - pc) << 8);
                if (score < selected.score) continue;
            }
            selected = {true, x, y, pc, oc, score};
        }
    }
    return selected;
}

B007Event14PlacementResult Failure(B007Event14PlacementStatus s) {
    B007Event14PlacementResult r;
    r.status = s;
    r.fresh_unit_should_clear_on_failure = true;
    return r;
}
} // namespace

B007Event14PlacementResult ResolveB007Event14PlacementExact(
    const B007Event14PlacementGrid& g) {
    if (!ValidShape(g)) return Failure(B007Event14PlacementStatus::InvalidInput);
    if (!InActiveRect(g, kPreferredX, kPreferredY)) {
        return Failure(B007Event14PlacementStatus::RejectedPreferredOutsideActiveRect);
    }
    const int preferred_cell = Index(g, kPreferredX, kPreferredY);
    if (g.occupied[preferred_cell] == 0) {
        if (g.destination_ok[preferred_cell] == 0) {
            return Failure(B007Event14PlacementStatus::RejectedPreferredInvalidTerrain);
        }
        B007Event14PlacementResult r;
        r.status = B007Event14PlacementStatus::PreferredCoordinate;
        r.final_x = kPreferredX; r.final_y = kPreferredY;
        r.actor_start_x = kOriginX; r.actor_start_y = kOriginY;
        return r;
    }

    // Occupied preferred tile is the *only* Event14 path that enters the
    // alternate-placement search. The preferred image ignores occupancy.
    const auto preferred = BuildImage(g, kPreferredX, kPreferredY, false);
    Candidate c;
    bool relaxed = false;
    auto origin = BuildImage(g, kOriginX, kOriginY, true);
    c = SelectCandidate(g, preferred, origin);
    if (!c.found) {
        relaxed = true;
        origin = BuildImage(g, kOriginX, kOriginY, false);
        c = SelectCandidate(g, preferred, origin);
    }
    if (!c.found) return Failure(B007Event14PlacementStatus::RejectedNoAlternate);

    // Retail rebuilds the strict origin image before deciding whether coord1
    // can remain the Actor's presentation start. If not, start collapses to
    // the selected final tile rather than inventing a route through blockers.
    const auto strict = BuildImage(g, kOriginX, kOriginY, true);
    const bool strict_reachable = strict[Index(g, c.x, c.y)] >= 0;

    B007Event14PlacementResult r;
    r.status = relaxed ? B007Event14PlacementStatus::AlternateRelaxed
                       : B007Event14PlacementStatus::AlternateStrict;
    r.final_x = c.x; r.final_y = c.y;
    r.actor_start_x = strict_reachable ? kOriginX : c.x;
    r.actor_start_y = strict_reachable ? kOriginY : c.y;
    r.preferred_path_cost = c.preferred_cost;
    r.origin_path_cost = c.origin_cost;
    r.packed_score = c.score;
    r.used_relaxed_retry = relaxed;
    return r;
}

bool B007DisposWaitIsBlocked(bool proc_dispos_active) {
    return proc_dispos_active;
}

} // namespace fates::headless
