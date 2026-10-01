#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <utility>

namespace fates::map::native {

inline constexpr std::size_t kTerrainTileRecordBytes = 0x28;
inline constexpr std::size_t kTerrainDeltaCells = 0x400;
inline constexpr std::string_view kDragonVeinPrivateSkillRetailId = "SPID_竜脈";
inline constexpr std::string_view kKnightMaleRetailJobId = "JID_アーマーナイト男";
inline constexpr std::string_view kKnightMaleParagonSymbol = "KNIGHT_M";
inline constexpr std::string_view kKnightLocalizedClassName = "Knight";

struct TerrainTileResolved {
    std::uint8_t id{};
    std::uint8_t change_id_1{};
    std::uint8_t change_id_2{};
    std::uint8_t change_id_3{};
    std::uint8_t move_cost_index{};
    std::int8_t defense_bonus{};
    std::int8_t avoid_bonus{};
    std::int8_t healing_bonus{};
    std::uint32_t retail_flags{};
};

struct TerrainInfoState {
    int x{-1};
    int y{-1};
    bool has_terrain{false};
    std::uint8_t terrain_id{};
};

struct Rect {
    int x{};
    int y{};
    int width{};
    int height{};
};

struct DeployPropagation {
    bool column{};
    bool row{};
};

struct DragonVeinScoreInput {
    bool situation_blocked{};             // retail Situation flag 0x00100000
    std::uint8_t chapter_type{};          // exact retail byte; type labels stay conservative
    bool user_flag_0x20{};
    std::uint32_t stored_value_bits{};      // same retail word is signed in one branch and unsigned in another
    int capability_deduction{};           // resolved sum from qualifying Force-1 units
    int trick_count{};                    // resolved Enumerator count
};

std::size_t TerrainCostStride(std::uint32_t width) noexcept;
std::size_t TerrainTileOffset(std::uint8_t terrain_id) noexcept;
void HideTerrainInfo(TerrainInfoState& state) noexcept;
void ShowTerrainInfo(TerrainInfoState& state, int x, int y, std::uint8_t terrain_id) noexcept;
bool IsCastleUnitDispos(const TerrainTileResolved& terrain, int knight_movement_cost) noexcept;

bool ShouldSerializeTerrainDelta(const TerrainTileResolved& base, std::uint8_t current_id) noexcept;
bool CanApplyTerrainDelta(const TerrainTileResolved& base, int delta_id) noexcept;
bool IsShowDeploy(std::uint8_t trick_type, std::uint8_t trick_flags) noexcept;
bool IsCannon(std::uint8_t trick_type) noexcept;
std::optional<int> CannonWeaponExpKind(std::uint8_t trick_type, bool kind5_blocked) noexcept;
bool CanUseCannon(std::uint8_t trick_type, int weapon_exp_kind3, int weapon_exp_kind4,
                  int weapon_exp_kind5, bool kind5_blocked) noexcept;
bool IsDestroyTargetCastleOffense(std::uint8_t trick_type) noexcept;
std::pair<int,int> CannonAccess(const Rect& rect, std::uint8_t direction) noexcept;
std::pair<int,int> CannonUnitDirection(const Rect& rect, std::pair<int,int> access) noexcept;
bool IsTrickAccess(std::uint8_t trick_type, const Rect& rect, int x, int y,
                   bool current_terrain_has_change_id_1,
                   std::pair<int,int> resolved_cannon_access) noexcept;
bool IsBreakable(std::uint8_t breakable_value, bool any_tile_has_retail_flag_0x2) noexcept;
bool ShouldMarkDeploy(std::uint32_t terrain_flags, std::uint8_t trick_flags) noexcept;
DeployPropagation DeployPropagationForTerrain(std::uint32_t terrain_flags) noexcept;

bool CanSetTerrainTid(bool map_exists, bool inside_actual_bounds,
                      const TerrainTileResolved* current, const TerrainTileResolved* target) noexcept;
bool TerrainIsTid(bool map_exists, bool inside_actual_bounds, bool target_exists,
                  std::uint8_t current_id, std::uint8_t target_id) noexcept;
bool TerrainHasFlag(bool map_exists, bool inside_actual_bounds,
                    std::uint32_t terrain_flags, std::uint32_t mask) noexcept;
bool ShouldRemoveObstacle(std::uint32_t terrain_flags) noexcept;

bool CanDragonVein(std::uint64_t private_skill_category_mask,
                   std::uint64_t combined_unit_category_mask,
                   bool retail_unit_flag_0x10000000,
                   bool person_is_download) noexcept;
int CalculateDragonVeinScore(const DragonVeinScoreInput& in) noexcept;

} // namespace fates::map::native
