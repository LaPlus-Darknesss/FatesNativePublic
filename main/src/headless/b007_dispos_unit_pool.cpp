#include "fates/headless/b007_dispos_unit_pool.hpp"
#include "fates/map/native_event_movement_semantics.hpp"

#include <algorithm>
#include <limits>

namespace fates::headless {
namespace {
constexpr std::size_t kFe14PointerBase = 0x20;
constexpr std::size_t kFactionStride = 12;
constexpr std::size_t kSpawnStride = 0x8c;
constexpr std::size_t kMaxFactionCount = 128;

constexpr unsigned char kPlayerMale[] = {0x50,0x49,0x44,0x5f,0x83,0x76,0x83,0x8c,0x83,0x43,0x83,0x84,0x81,0x5b,0x92,0x6a};
constexpr unsigned char kPlayerFemale[] = {0x50,0x49,0x44,0x5f,0x83,0x76,0x83,0x8c,0x83,0x43,0x83,0x84,0x81,0x5b,0x8f,0x97};
constexpr unsigned char kFelicia[] = {0x50,0x49,0x44,0x5f,0x83,0x74,0x83,0x46,0x83,0x8a,0x83,0x56,0x83,0x41};
constexpr unsigned char kJakob[] = {0x50,0x49,0x44,0x5f,0x83,0x57,0x83,0x87,0x81,0x5b,0x83,0x4a,0x81,0x5b};
constexpr unsigned char kElise[] = {0x50,0x49,0x44,0x5f,0x83,0x47,0x83,0x8a,0x81,0x5b,0x83,0x5b};
constexpr unsigned char kSilas[] = {0x50,0x49,0x44,0x5f,0x83,0x54,0x83,0x43,0x83,0x89,0x83,0x58};

std::string_view Raw(const unsigned char* p, std::size_t n) {
    return {reinterpret_cast<const char*>(p), n};
}

std::uint32_t U32(std::span<const std::uint8_t> b, std::size_t o) {
    return static_cast<std::uint32_t>(b[o]) |
           (static_cast<std::uint32_t>(b[o + 1]) << 8) |
           (static_cast<std::uint32_t>(b[o + 2]) << 16) |
           (static_cast<std::uint32_t>(b[o + 3]) << 24);
}

bool ReadRelativeString(std::span<const std::uint8_t> b, std::uint32_t rel, std::string& out) {
    out.clear();
    if (rel == 0) return true;
    const std::size_t start = kFe14PointerBase + static_cast<std::size_t>(rel);
    if (start >= b.size()) return false;
    std::size_t end = start;
    while (end < b.size() && b[end] != 0) ++end;
    if (end >= b.size()) return false;
    out.assign(reinterpret_cast<const char*>(b.data() + start), end - start);
    return true;
}

bool IsPlayerPlaceholder(std::string_view pid) {
    return pid == B007PidPlayerMale() || pid == B007PidPlayerFemale();
}

std::int32_t UnitGetByPid(void* user, const char* pid) {
    if (!pid) return 0;
    return static_cast<B007UnitPoolProjection*>(user)->GetByPid(pid);
}
bool UnitIsPid(void* user, std::int32_t h, const char* pid) {
    return pid && static_cast<B007UnitPoolProjection*>(user)->IsPid(h, pid);
}
std::int32_t UnitGetForce(void* user, std::int32_t h) {
    return static_cast<B007UnitPoolProjection*>(user)->GetForce(h);
}
std::int32_t UnitGetX(void* user, std::int32_t h) {
    return static_cast<B007UnitPoolProjection*>(user)->GetX(h);
}
std::int32_t UnitGetY(void* user, std::int32_t h) {
    return static_cast<B007UnitPoolProjection*>(user)->GetY(h);
}
void UnitMovePosition(void* user, std::int32_t h, std::int32_t x, std::int32_t y,
                      std::uint32_t flags) {
    static_cast<B007UnitPoolProjection*>(user)->QueueMovePosition(h, x, y, flags);
}
bool UnitIsMoveWait(void* user) {
    return static_cast<B007UnitPoolProjection*>(user)->IsMoveWait();
}
void DisposRunGroup(void* user, const char* group, std::int32_t mode) {
    if (!group) return;
    static_cast<B007UnitPoolProjection*>(user)->ApplyExistingRosterGroup(
        group, static_cast<std::uint32_t>(mode));
}
} // namespace

std::string_view B007PidPlayerMale() { return Raw(kPlayerMale, sizeof(kPlayerMale)); }
std::string_view B007PidPlayerFemale() { return Raw(kPlayerFemale, sizeof(kPlayerFemale)); }
std::string_view B007PidFelicia() { return Raw(kFelicia, sizeof(kFelicia)); }
std::string_view B007PidJakob() { return Raw(kJakob, sizeof(kJakob)); }
std::string_view B007PidElise() { return Raw(kElise, sizeof(kElise)); }
std::string_view B007PidSilas() { return Raw(kSilas, sizeof(kSilas)); }

const Fe14DisposGroupProjection* Fe14DisposFileProjection::FindGroup(std::string_view name) const {
    const auto it = std::find_if(groups.begin(), groups.end(), [name](const auto& g) {
        return g.name == name;
    });
    return it == groups.end() ? nullptr : &*it;
}

bool ParseFe14DisposProjection(std::span<const std::uint8_t> decoded,
                               Fe14DisposFileProjection& out) {
    out.groups.clear();
    if (decoded.size() < kFe14PointerBase + kFactionStride) return false;

    for (std::size_t i = 0; i < kMaxFactionCount; ++i) {
        const std::size_t f = kFe14PointerBase + i * kFactionStride;
        if (f + kFactionStride > decoded.size()) return false;
        const std::uint32_t name_rel = U32(decoded, f);
        if (name_rel == 0) return !out.groups.empty();
        const std::uint32_t table_rel = U32(decoded, f + 4);
        const std::uint32_t count = U32(decoded, f + 8);
        if (count > 0x1000u) return false;

        Fe14DisposGroupProjection group;
        if (!ReadRelativeString(decoded, name_rel, group.name) || group.name.empty()) return false;
        const std::size_t table = kFe14PointerBase + static_cast<std::size_t>(table_rel);
        if (table > decoded.size() || static_cast<std::size_t>(count) >
                (decoded.size() - table) / kSpawnStride) return false;

        group.spawns.reserve(count);
        for (std::uint32_t j = 0; j < count; ++j) {
            const std::size_t r = table + static_cast<std::size_t>(j) * kSpawnStride;
            Fe14DisposSpawnProjection s;
            if (!ReadRelativeString(decoded, U32(decoded, r), s.pid)) return false;
            if (!ReadRelativeString(decoded, U32(decoded, r + 4), s.job)) return false;
            s.team = decoded[r + 8];
            s.level = decoded[r + 9];
            s.coord1_x = static_cast<std::int8_t>(decoded[r + 0x0a]);
            s.coord1_y = static_cast<std::int8_t>(decoded[r + 0x0b]);
            s.coord2_x = static_cast<std::int8_t>(decoded[r + 0x0c]);
            s.coord2_y = static_cast<std::int8_t>(decoded[r + 0x0d]);
            s.spawn_flags = U32(decoded, r + 0x10);
            for (std::size_t k = 0; k < 5; ++k) {
                if (!ReadRelativeString(decoded, U32(decoded, r + 0x14 + k * 8), s.items[k])) return false;
                s.item_flags[k] = decoded[r + 0x18 + k * 8];
                for (std::size_t difficulty=0; difficulty<3; ++difficulty) {
                    const auto byte=decoded[r + 0x19 + k * 8 + difficulty];
                    s.item_difficulty_adjustments[k][difficulty]=
                        static_cast<std::int8_t>(byte<128 ? int(byte) : int(byte)-256);
                }
                if (!ReadRelativeString(decoded, U32(decoded, r + 0x3c + k * 4), s.skills[k])) return false;
            }
            s.skill_flags = U32(decoded, r + 0x50);
            if (!ReadRelativeString(decoded, U32(decoded, r + 0x54), s.ai_action)) return false;
            if (!ReadRelativeString(decoded, U32(decoded, r + 0x58), s.ai_action_param)) return false;
            if (!ReadRelativeString(decoded, U32(decoded, r + 0x5c), s.ai_mission)) return false;
            if (!ReadRelativeString(decoded, U32(decoded, r + 0x60), s.ai_mission_param)) return false;
            if (!ReadRelativeString(decoded, U32(decoded, r + 0x64), s.ai_attack)) return false;
            if (!ReadRelativeString(decoded, U32(decoded, r + 0x68), s.ai_attack_param)) return false;
            if (!ReadRelativeString(decoded, U32(decoded, r + 0x6c), s.ai_movement)) return false;
            if (!ReadRelativeString(decoded, U32(decoded, r + 0x70), s.ai_movement_param)) return false;
            s.battle_rate = decoded[r + 0x74];
            s.priority = decoded[r + 0x75];
            s.ai_raw_76_78 = {decoded[r + 0x76], decoded[r + 0x77], decoded[r + 0x78]};
            for (std::size_t k = 0; k < 5; ++k) s.move_limit[k] = decoded[r + 0x79 + k];
            s.ai_policy = U32(decoded, r + 0x80);
            s.runtime_state = decoded[r + 0x84];
            group.spawns.push_back(std::move(s));
        }
        out.groups.push_back(std::move(group));
    }
    return false;
}

bool B007UnitPoolProjection::AddExisting(std::string_view pid, bool player) {
    if (pid.empty() || units_.size() >= 250) return false;
    HeadlessUnitProjection u;
    u.handle = static_cast<std::int32_t>(units_.size() + 1);
    u.pid.assign(pid.data(), pid.size());
    u.is_player = player;
    units_.push_back(std::move(u));
    return true;
}

bool B007UnitPoolProjection::SeedEndingScenario(B007PlayerSex sex) {
    units_.clear();
    move_intents_.clear();
    event_moves_.clear();
    if (!AddExisting(sex == B007PlayerSex::Male ? B007PidPlayerMale() : B007PidPlayerFemale(), true)) return false;
    if (!AddExisting(B007PidElise(), false)) return false;
    if (!AddExisting(B007PidSilas(), false)) return false;
    if (!AddExisting(sex == B007PlayerSex::Male ? B007PidFelicia() : B007PidJakob(), false)) return false;
    return true;
}

HeadlessUnitProjection* B007UnitPoolProjection::FindHandle(std::int32_t handle) {
    if (handle < 1 || handle > static_cast<std::int32_t>(units_.size())) return nullptr;
    return &units_[static_cast<std::size_t>(handle - 1)];
}
const HeadlessUnitProjection* B007UnitPoolProjection::FindHandle(std::int32_t handle) const {
    if (handle < 1 || handle > static_cast<std::int32_t>(units_.size())) return nullptr;
    return &units_[static_cast<std::size_t>(handle - 1)];
}
HeadlessUnitProjection* B007UnitPoolProjection::FindPlayer() {
    const auto it = std::find_if(units_.begin(), units_.end(), [](const auto& u) { return u.is_player; });
    return it == units_.end() ? nullptr : &*it;
}
const HeadlessUnitProjection* B007UnitPoolProjection::FindPlayer() const {
    const auto it = std::find_if(units_.begin(), units_.end(), [](const auto& u) { return u.is_player; });
    return it == units_.end() ? nullptr : &*it;
}

std::int32_t B007UnitPoolProjection::GetByPid(std::string_view pid) const {
    const auto it = std::find_if(units_.begin(), units_.end(), [pid](const auto& u) {
        return u.pid == pid;
    });
    return it == units_.end() ? 0 : it->handle;
}

bool B007UnitPoolProjection::IsPid(std::int32_t handle, std::string_view pid) const {
    const auto* u = FindHandle(handle);
    return u && u->unique && u->pid == pid;
}

std::int32_t B007UnitPoolProjection::GetPlayer() const {
    const auto female = GetByPid(B007PidPlayerFemale());
    return female != 0 ? female : GetByPid(B007PidPlayerMale());
}

bool B007UnitPoolProjection::PlayerIsMale() const {
    const auto h = GetPlayer();
    return h != 0 && IsPid(h, B007PidPlayerMale());
}

std::int32_t B007UnitPoolProjection::GetForce(std::int32_t handle) const {
    const auto* u = FindHandle(handle);
    return u ? static_cast<std::int32_t>(u->force) : 0;
}
std::int32_t B007UnitPoolProjection::GetX(std::int32_t handle) const {
    const auto* u = FindHandle(handle);
    return u && u->has_position ? static_cast<std::int32_t>(u->x) : -1;
}
std::int32_t B007UnitPoolProjection::GetY(std::int32_t handle) const {
    const auto* u = FindHandle(handle);
    return u && u->has_position ? static_cast<std::int32_t>(u->y) : -1;
}

bool B007UnitPoolProjection::ApplyExistingRosterGroup(std::string_view group_name,
                                                      std::uint32_t mode) {
    if (!dispos_ || mode != kRetailEndingEventDisposMode) return false;
    const auto* group = dispos_->FindGroup(group_name);
    if (!group) return false;

    // B007 Event11/12/Event13 use fixed coordinates. This projection refuses
    // more general Dispos assignment/search behavior rather than approximating it.
    for (const auto& spawn : group->spawns) {
        if (spawn.coord1_x != spawn.coord2_x || spawn.coord1_y != spawn.coord2_y ||
            spawn.coord2_x < 0 || spawn.coord2_y < 0) return false;
        HeadlessUnitProjection* u = nullptr;
        if (IsPlayerPlaceholder(spawn.pid)) u = FindPlayer();
        else {
            const auto h = GetByPid(spawn.pid);
            u = FindHandle(h);
        }
        if (!u) return false;
        u->force = spawn.team;
        u->x = spawn.coord2_x;
        u->y = spawn.coord2_y;
        u->has_position = true;
    }
    return true;
}

void B007UnitPoolProjection::QueueMovePosition(std::int32_t handle, std::int32_t x,
                                               std::int32_t y, std::uint32_t flags) {
    auto* u = FindHandle(handle);
    if (!u || !u->has_position) return;
    move_intents_.push_back({handle, x, y, flags});

    HeadlessEventMoveProjection move;
    move.handle = handle;
    move.start_x = u->x;
    move.start_y = u->y;
    move.goal_x = x;
    move.goal_y = y;
    move.script_flags = flags;
    move.primary_deploy_flags = fates::map::native::ResolveEventDeployFlags(flags);
    move.retry_deploy_flags = fates::map::native::ResolveEventDeployRetryFlags(move.primary_deploy_flags);
    move.actor_move_flags = fates::map::native::ResolveActorEventMoveFlags(flags);
    move.bounds_gate_bypassed = fates::map::native::UnitMovePositionBypassesMapBounds(flags);
    move.occupied_target_relocation_bypassed =
        fates::map::native::UnitMovePositionBypassesOccupiedTargetRelocation(flags);
    move.destination_reserved = true;

    // The B007 integration slice owns the event-movement lifecycle and exact
    // final Unit state, but it does not invent the path returned by Deploy's
    // movement image + Route::Seek. Until that boundary is closed, route-shape
    // and its per-backtrack RNG draws remain explicitly unresolved.
    if (flags != kRetailB007MoveFlags || IsMoveWait()) {
        move.status = HeadlessEventMoveStatus::Rejected;
    } else {
        move.status = HeadlessEventMoveStatus::Active;
    }
    event_moves_.push_back(move);
}

bool B007UnitPoolProjection::IsMoveWait() const {
    return std::any_of(event_moves_.begin(), event_moves_.end(), [](const auto& m) {
        return m.status == HeadlessEventMoveStatus::Active;
    });
}

bool B007UnitPoolProjection::CompleteActiveMove() {
    const auto it = std::find_if(event_moves_.begin(), event_moves_.end(), [](const auto& m) {
        return m.status == HeadlessEventMoveStatus::Active;
    });
    if (it == event_moves_.end()) return false;
    auto* u = FindHandle(it->handle);
    if (!u) {
        it->status = HeadlessEventMoveStatus::Rejected;
        return false;
    }
    u->x = static_cast<std::int8_t>(it->goal_x);
    u->y = static_cast<std::int8_t>(it->goal_y);
    u->has_position = true;
    it->status = HeadlessEventMoveStatus::Complete;
    return true;
}

bool B007UnitPoolProjection::HasUnresolvedRouteDeterminism() const {
    return std::any_of(event_moves_.begin(), event_moves_.end(), [](const auto& m) {
        return m.status != HeadlessEventMoveStatus::Rejected &&
               (!m.route_shape_resolved || m.route_rng_draw_count < 0);
    });
}

event::native::TypedEventRuntime B007UnitPoolProjection::MakeTypedRuntime() {
    event::native::TypedEventRuntime r;
    r.user = this;
    r.dispos.run_group = &DisposRunGroup;
    r.unit.get_by_pid = &UnitGetByPid;
    r.unit.is_pid = &UnitIsPid;
    r.unit.get_force = &UnitGetForce;
    r.unit.get_x = &UnitGetX;
    r.unit.get_y = &UnitGetY;
    r.unit.move_position = &UnitMovePosition;
    r.unit.is_move_wait = &UnitIsMoveWait;
    return r;
}

} // namespace fates::headless
