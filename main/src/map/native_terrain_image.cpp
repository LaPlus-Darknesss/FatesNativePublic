#include "fates/map/native_terrain_image.hpp"
#include "fates/runtime/native_runtime.hpp"
namespace fates::map::native {
namespace {
using S=TerrainImageStatus;
using Game=fates::runtime::native::NativeGameState;
using Runtime=fates::runtime::native::NativeRuntime;
using Map=fates::runtime::native::TerrainMapDefinition;
std::array<std::uint32_t,6> Metadata(const Map& map) noexcept {
    return {map.width,map.height,map.min_x,map.min_y,map.max_x,map.max_y};
}
TerrainImageGeometry Geometry(const Map& map) noexcept {
    return {std::uint8_t(map.width),std::uint8_t(map.height),std::uint8_t(map.min_x),
        std::uint8_t(map.min_y),std::uint8_t(map.max_x),std::uint8_t(map.max_y)};
}
bool Valid(TerrainImageGeometry geometry) noexcept {return geometry.width<=32&&geometry.height<=32;}
}
TerrainImageStatus RefreshTerrainImageExact(TerrainImageGrid& receiver,const Map& map,
    const CastleTerrainPlane* castle,const TerrainImageGrid* global) {
    const auto geometry=Geometry(map);
    if(!Valid(geometry))return S::InvalidDimensions;
    const bool singleton=!global||global==&receiver;
    const auto& source=castle?castle->cells:map.grid;
    const auto global_geometry=singleton?geometry:global->geometry;
    const auto& global_terrain=singleton?source:global->planes.terrain;
    if(!Valid(global_geometry))return S::InvalidDimensions;
    for(unsigned y=0;y<global_geometry.height;++y)for(unsigned x=0;x<global_geometry.width;++x)
        if(global_terrain[y*32+x]>=map.terrain_types.size())return S::MissingTileDefinition;
    // All validation precedes writes; no active-bounds gate belongs to refresh.
    receiver.geometry=geometry;receiver.planes.terrain=source;receiver.planes.refresh_copy=source;
    for(unsigned y=0;y<global_geometry.height;++y)for(unsigned x=0;x<global_geometry.width;++x)
        receiver.planes.cost_indices[y*32+x]=map.terrain_types[global_terrain[y*32+x]].movement_cost_index;
    return S::Ok;
}
struct TerrainImageAccess {
    static TerrainImageGeometry Geometry(const Game& game) noexcept {
        const auto& s=game.tactical_image;
        return {s.grid_.width,s.grid_.height,s.active_[0],s.active_[1],s.active_[2],s.active_[3]};
    }
    static void SetGeometry(Game& game,TerrainImageGeometry g) noexcept {
        auto& s=game.tactical_image;s.grid_.width=g.width;s.grid_.height=g.height;
        s.active_={g.min_x,g.min_y,g.max_x,g.max_y};s.terrain_geometry_changed_=false;
    }
    static bool Context(const Game& game) noexcept {
        const auto& s=game.tactical_image;
        return s.terrain_map_active_==game.map_active&&s.terrain_chapter_==game.campaign.current_chapter_index;
    }
    static void Initialize(Game& game) noexcept {
        auto& s=game.tactical_image;s.terrain_={};SetGeometry(game,{});
        s.terrain_initialized_=true;s.terrain_bound_=false;
        s.terrain_map_active_=game.map_active;s.terrain_chapter_=game.campaign.current_chapter_index;
    }
    static void Invalidate(Game& game) noexcept {
        game.tactical_image.terrain_initialized_=false;game.tactical_image.terrain_bound_=false;
    }
    static TerrainImageView Read(const Runtime& runtime) noexcept {
        const auto& s=runtime.game.tactical_image;
        if(!s.terrain_initialized_)return {S::UninitializedImage};
        if(!Context(runtime.game)||s.terrain_geometry_changed_)return {S::StaleImage};
        if(!s.terrain_bound_)return {S::UnboundImage};
        const auto* map=runtime.definitions.terrain_map();
        if(!map||Metadata(*map)!=s.terrain_source_metadata_||map->grid!=s.terrain_source_grid_||
           map->terrain_types.size()!=s.terrain_source_tiles_.size())return {S::StaleImage};
        for(unsigned i=0;i<map->terrain_types.size();++i){const auto& tile=map->terrain_types[i];
            if(s.terrain_source_tiles_[i]!=std::pair{tile.movement_cost_index,tile.flags_0x18})return {S::StaleImage};}
        return {S::Ok,Geometry(runtime.game),&s.terrain_};
    }
    static S Refresh(Runtime& runtime,const CastleTerrainPlane* castle) {
        auto& s=runtime.game.tactical_image;
        if(!s.terrain_initialized_)return S::UninitializedImage;
        if(!Context(runtime.game))return S::StaleImage;
        const auto* map=runtime.definitions.terrain_map();if(!map)return S::MissingTerrainDefinition;
        TerrainImageGrid next{Geometry(runtime.game),s.terrain_};
        const auto status=RefreshTerrainImageExact(next,*map,castle);if(status!=S::Ok)return status;
        std::vector<std::pair<std::uint8_t,std::uint32_t>> tiles;tiles.reserve(map->terrain_types.size());
        for(const auto& tile:map->terrain_types)tiles.emplace_back(tile.movement_cost_index,tile.flags_0x18);
        // Allocation/validation finish before publishing either shared geometry or planes.
        s.terrain_source_tiles_=std::move(tiles);s.terrain_source_metadata_=Metadata(*map);s.terrain_source_grid_=map->grid;
        s.terrain_=next.planes;SetGeometry(runtime.game,next.geometry);s.terrain_bound_=true;return S::Ok;
    }
};
void InitializeCurrentTerrainImage(Game& game) noexcept {TerrainImageAccess::Initialize(game);}
void InvalidateCurrentTerrainImage(Game& game) noexcept {TerrainImageAccess::Invalidate(game);}
TerrainImageStatus RefreshCurrentTerrainImage(Runtime& runtime,const CastleTerrainPlane* castle) {return TerrainImageAccess::Refresh(runtime,castle);}
TerrainImageView ReadCurrentTerrainImage(const Runtime& runtime) noexcept {return TerrainImageAccess::Read(runtime);}
CurrentTacticalRescueResult QueryRescuePositionWithCurrentTacticalImage(const Runtime& runtime,
    std::uint16_t slot,int x,int y,bool allow,bool prefer) {
    const auto view=ReadCurrentTerrainImage(runtime);if(view.status!=S::Ok)return {view.status};
    const auto g=view.geometry;RescueTerrainImage terrain{};terrain.active={g.min_x,g.min_y,g.max_x,g.max_y};
    if(!ValidMovementBounds(terrain.active)||g.max_x>g.width||g.max_y>g.height)return {S::InvalidActiveBounds};
    terrain.terrain_cost_indices=view.planes->cost_indices;
    const auto& tiles=runtime.definitions.terrain_map()->terrain_types;
    // Original flag8 lookup is used only for active-area selection. Padding IDs
    // need not resolve, and padding cost bytes are the retained original plane.
    for(unsigned y=g.min_y;y<g.max_y;++y)for(unsigned x=g.min_x;x<g.max_x;++x){
        const auto i=y*32+x;const auto key=view.planes->terrain[i];
        if(key>=tiles.size())return {S::MissingTileDefinition};
        terrain.terrain_blocks_move_image[i]=(tiles[key].flags_0x18&8)!=0;
    }
    const auto result=QueryRescuePositionWithCurrentImage(runtime,slot,terrain,x,y,allow,prefer);
    return {S::Ok,result.image_status,result.position};
}
}
