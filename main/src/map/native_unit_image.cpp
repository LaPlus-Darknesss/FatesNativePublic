#include "fates/map/native_unit_image.hpp"
#include "fates/runtime/native_runtime.hpp"
#include "fates/runtime/native_force_order.hpp"
#include <vector>

namespace fates::map::native {
namespace {
using S=UnitImageStatus;
using Game=fates::runtime::native::NativeGameState;
constexpr std::uint32_t kImageExclusionMask=0x24004u;
bool ValidDimensions(int width,int height) noexcept {return width>=0&&height>=0&&width<=32&&height<=32;}
bool ValidKey(unsigned key) noexcept {return key>=1&&key<=250;}
bool Inside(const UnitImageGrid& image,int x,int y) noexcept {
    return x>=0&&y>=0&&x<image.width&&y<image.height;
}
bool StoredByte(int value) noexcept {return value>=-128&&value<=255;}
std::int8_t SignedByte(int value) noexcept {
    const auto byte=static_cast<std::uint8_t>(value);
    return static_cast<std::int8_t>(byte<128?int(byte):int(byte)-256);
}
S CurrentUnit(const Game& game,std::uint16_t slot,int x,int y,UnitImageUnit& out) noexcept {
    if(slot>=game.units.size()||!game.units[slot].occupied)return S::InvalidUnit;
    const auto& unit=game.units[slot];
    if((x==-1&&!StoredByte(unit.x))||(y==-1&&!StoredByte(unit.y)))return S::InvalidCoordinates;
    out={std::uint8_t(slot+1),SignedByte(unit.x),SignedByte(unit.y),unit.flags};return S::Ok;
}
}

UnitImageStatus AddUnitImageExact(UnitImageGrid& image,UnitImageUnit unit,bool selected,int x,int y) noexcept {
    if(!ValidDimensions(image.width,image.height))return S::InvalidDimensions;
    if(!ValidKey(unit.key))return S::InvalidUnit;
    if(x==-1)x=unit.x;if(y==-1)y=unit.y;
    if(Inside(image,x,y)&&(selected||(unit.flags&kImageExclusionMask)==0))image.cells[y*32+x]=unit.key;
    return S::Ok;
}
UnitImageStatus DeleteUnitImageExact(UnitImageGrid& image,UnitImageUnit unit,int x,int y) noexcept {
    if(!ValidDimensions(image.width,image.height))return S::InvalidDimensions;
    if(!ValidKey(unit.key))return S::InvalidUnit;
    if(x==-1)x=unit.x;if(y==-1)y=unit.y;
    if(Inside(image,x,y)&&image.cells[y*32+x]==unit.key)image.cells[y*32+x]=0;
    return S::Ok;
}
UnitImageStatus RebuildUnitImageExact(UnitImageGrid& image,std::span<const UnitImageUnit> units) noexcept {
    if(!ValidDimensions(image.width,image.height))return S::InvalidDimensions;
    for(const auto& unit:units)if(!ValidKey(unit.key))return S::InvalidUnit;
    image.cells.fill(0);
    for(const auto& unit:units)AddUnitImageExact(image,unit,false);
    return S::Ok;
}
std::optional<std::uint8_t> UnitImageKeyAt(const UnitImageGrid& image,int x,int y) noexcept {
    if(!ValidDimensions(image.width,image.height)||x<0||y<0||x>=32||y>=32)return std::nullopt;
    return image.cells[y*32+x];
}

// Binding metadata is native validity state, not a reconstruction of an original
// cache. It prevents implicit image regeneration and stale slot reuse.
struct UnitImageAccess {
    static NativeTacticalImageState::Source Source(const Game& game,unsigned slot) noexcept {
        const auto& unit=game.units[slot];NativeTacticalImageState::Source result{};
        result.occupied=unit.occupied;result.generation=game.unit_slot_generations[slot];
        if(unit.occupied){result.person=unit.person_id;result.force=unit.force_type;
            result.x=unit.x;result.y=unit.y;result.image_flags=unit.flags&kImageExclusionMask;}
        return result;
    }
    static NativeTacticalImageState::Order Order(const Game& game,unsigned force) noexcept {
        const auto& order=force==0?game.player_force_order:game.other_force_orders[force-1];
        return {order.bound,order.count,order.slots,order.person_ids};
    }
    static void Bind(Game& game,const UnitImageGrid& image) noexcept {
        auto& state=game.tactical_image;
        if(state.grid_.width!=image.width||state.grid_.height!=image.height)state.terrain_geometry_changed_=true;
        state.grid_=image;
        state.map_active_=game.map_active;state.chapter_=game.campaign.current_chapter_index;
        for(unsigned i=0;i<state.sources_.size();++i)state.sources_[i]=Source(game,i);
        for(unsigned f=0;f<3;++f)state.orders_[f]=Order(game,f);
        state.bound_=true;
    }
    static UnitImageView Read(const Game& game) noexcept {
        const auto& state=game.tactical_image;if(!state.bound_)return {S::UnboundImage};
        if(state.map_active_!=game.map_active||state.chapter_!=game.campaign.current_chapter_index)return {S::StaleImage};
        for(unsigned i=0;i<state.sources_.size();++i)if(state.sources_[i]!=Source(game,i))return {S::StaleImage};
        for(unsigned f=0;f<3;++f)if(state.orders_[f]!=Order(game,f))return {S::StaleImage};
        return {S::Ok,&state.grid_};
    }
    static UnitImageGrid& Grid(Game& game) noexcept {return game.tactical_image.grid_;}
    static void Invalidate(Game& game) noexcept {game.tactical_image.bound_=false;}
};
UnitImageStatus RebuildCurrentUnitImage(Game& game,int width,int height) {
    using namespace fates::runtime::native;
    if(!ValidDimensions(width,height))return S::InvalidDimensions;
    std::vector<UnitImageUnit> ordered;ordered.reserve(250);
    for(unsigned force=0;force<3;++force) {
        bool empty=true;for(const auto& unit:game.units)if(unit.occupied&&unit.force_type==force){empty=false;break;}
        if(empty)continue; // an empty list is unambiguous and needs no binding write
        const auto* order=GetVerifiedForceOrder(game,std::uint8_t(force));if(!order)return S::MissingForceOrder;
        for(unsigned i=0;i<order->count;++i){UnitImageUnit unit{};const auto status=CurrentUnit(game,order->slots[i],-1,-1,unit);
            if(status!=S::Ok)return status;ordered.push_back(unit);}
    }
    UnitImageGrid image{};image.width=std::uint8_t(width);image.height=std::uint8_t(height);
    const auto status=RebuildUnitImageExact(image,ordered);if(status!=S::Ok)return status;
    UnitImageAccess::Bind(game,image);return S::Ok;
}
UnitImageStatus RestoreCurrentUnitImage(Game& game,const UnitImageGrid& image) {
    if(!ValidDimensions(image.width,image.height))return S::InvalidDimensions;
    // Stored cells outside full dimensions still belong to the backing image.
    for(auto key:image.cells)if(key&&(!ValidKey(key)||!game.units[key-1].occupied))return S::InvalidOccupant;
    UnitImageAccess::Bind(game,image);return S::Ok;
}
void InvalidateCurrentUnitImage(Game& game) noexcept {UnitImageAccess::Invalidate(game);}
UnitImageView ReadCurrentUnitImage(const Game& game) noexcept {return UnitImageAccess::Read(game);}
UnitImageStatus AddCurrentUnitToImage(Game& game,std::uint16_t slot,bool selected,int x,int y) noexcept {
    const auto view=ReadCurrentUnitImage(game);if(view.status!=S::Ok)return view.status;
    UnitImageUnit unit{};const auto status=CurrentUnit(game,slot,x,y,unit);if(status!=S::Ok)return status;
    return AddUnitImageExact(UnitImageAccess::Grid(game),unit,selected,x,y);
}
UnitImageStatus DeleteCurrentUnitFromImage(Game& game,std::uint16_t slot,int x,int y) noexcept {
    const auto view=ReadCurrentUnitImage(game);if(view.status!=S::Ok)return view.status;
    UnitImageUnit unit{};const auto status=CurrentUnit(game,slot,x,y,unit);if(status!=S::Ok)return status;
    return DeleteUnitImageExact(UnitImageAccess::Grid(game),unit,x,y);
}
CurrentImageRescueResult QueryRescuePositionWithCurrentImage(const fates::runtime::native::NativeRuntime& runtime,
    std::uint16_t slot,const RescueTerrainImage& terrain,int x,int y,bool allow,bool prefer) {
    const auto view=ReadCurrentUnitImage(runtime.game);if(view.status!=S::Ok)return {view.status};
    if(!ValidMovementBounds(terrain.active)||terrain.active.max_x>view.image->width||terrain.active.max_y>view.image->height)
        return {S::InvalidTerrainBounds};
    RescueMapImage image{};static_cast<RescueTerrainImage&>(image)=terrain;
    for(unsigned i=0;i<1024;++i)image.occupants[i]=view.image->cells[i];
    return {S::Ok,QueryRescuePositionForCurrentUnit(runtime,slot,image,x,y,allow,prefer)};
}
}
