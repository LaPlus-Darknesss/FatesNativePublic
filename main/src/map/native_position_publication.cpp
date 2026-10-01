#include "fates/map/native_position_publication.hpp"
#include "fates/map/native_terrain_image.hpp"
#include "fates/runtime/native_runtime.hpp"
#include <memory>
namespace fates::map::native {
namespace {using S=PositionPublicationStatus;}
PositionPublicationStatus PublishUnitPositionExact(int key,int x,int y,PositionPublicationServices& svc) {
    if(key<=0||key>250)return S::Ignored;
    const auto first=svc.UnitForKey(key);if(!first)return S::MissingUnit;
    if(first->force==9)return S::Ignored;
    const auto unit=svc.UnitForKey(key);if(!unit)return S::MissingUnit;
    if(unit->force>=3||(unit->flags&4))return S::Ignored;
    if(x<0||y<0||x>=32||y>=32)return S::InvalidCoordinates;
    const auto key_at=svc.OccupantKey(x,y);if(!key_at)return S::MissingImage;
    if(*key_at){const auto occupant=svc.OccupantUnit(x,y);if(!occupant)return S::MissingOccupant;if(*occupant!=unit->slot)return S::Ignored;}
    if(svc.MapPresent()&&!svc.DeleteImage(unit->slot))return S::MissingImage;
    if(!svc.WriteCoordinates(unit->slot,std::uint8_t(x),std::uint8_t(y)))return S::MissingUnit;
    const auto actor=svc.HasActor(unit->slot);if(!actor)return S::MissingActorPresence;
    if(*actor&&!svc.UpdateActor(unit->slot,true))return S::MissingActorPosition;
    if(!svc.AddImage(unit->slot,false))return S::MissingImage;
    if(!svc.UpdateDanger())return S::MissingDanger;
    return S::Ok;
}
PositionPublicationStatus PublishUnitPositionFreeSpaceExact(int key,int x,int y,PositionPublicationServices& svc) {
    if(key<=0||key>250)return S::Ignored;
    const auto first=svc.UnitForKey(key);if(!first)return S::MissingUnit;if(first->force==9)return S::Ignored;
    const auto unit=svc.UnitForKey(key);if(!unit)return S::MissingUnit;
    const auto position=svc.RescuePosition(unit->slot,x,y,true,false);if(!position)return S::MissingRescue;
    return PublishUnitPositionExact(key,position->first,position->second,svc);
}
namespace {
namespace rn=fates::runtime::native;
std::int8_t Signed(std::int16_t v){const auto b=std::uint8_t(v);return std::int8_t(b<128?int(b):int(b)-256);}
struct Services:PositionPublicationServices {
    rn::NativeRuntime& r;const CurrentActorHeightQuery* scene;CurrentPositionPublicationResult& result;
    std::optional<UnitImageGrid> image;
    Services(rn::NativeRuntime& r,const CurrentActorHeightQuery* s,CurrentPositionPublicationResult& o):r(r),scene(s),result(o){}
    bool Image() {
        if(image)return true;const auto view=ReadCurrentUnitImage(r.game);result.image_status=view.status;
        if(view.status!=UnitImageStatus::Ok)return false;image=*view.image;return true;
    }
    std::optional<UnitImageUnit> ImageUnit(std::uint16_t slot) {
        if(slot>=r.game.units.size())return std::nullopt;const auto& u=r.game.units[slot];
        if(!u.occupied||u.x<-128||u.x>255||u.y<-128||u.y>255)return std::nullopt;
        return UnitImageUnit{std::uint8_t(slot+1),Signed(u.x),Signed(u.y),u.flags};
    }
    std::optional<PositionPublicationUnit> UnitForKey(int key) override {
        if(key<=0||key>int(r.game.units.size()))return std::nullopt;const auto& u=r.game.units[key-1];
        if(!u.occupied&&u.force_type!=9)return std::nullopt;
        return PositionPublicationUnit{std::uint16_t(key-1),u.force_type,u.flags};
    }
    std::optional<std::uint8_t> OccupantKey(int x,int y) override {return Image()?UnitImageKeyAt(*image,x,y):std::nullopt;}
    std::optional<std::uint16_t> OccupantUnit(int x,int y) override {
        const auto key=OccupantKey(x,y);if(!key||!*key||*key>250||!r.game.units[*key-1].occupied)return std::nullopt;return std::uint16_t(*key-1);
    }
    bool MapPresent() override {return r.game.map_active;}
    bool DeleteImage(std::uint16_t slot) override {
        if(!Image())return false;const auto unit=ImageUnit(slot);if(!unit){result.image_status=UnitImageStatus::InvalidUnit;return false;}
        result.image_status=DeleteUnitImageExact(*image,*unit);return result.image_status==UnitImageStatus::Ok;
    }
    bool WriteCoordinates(std::uint16_t slot,std::uint8_t x,std::uint8_t y) override {
        if(slot>=r.game.units.size()||!r.game.units[slot].occupied)return false;auto& u=r.game.units[slot];u.x=x;u.y=y;u.has_position=true;return true;
    }
    std::optional<bool> HasActor(std::uint16_t slot) override {
        const auto& u=r.game.units[slot];if(!u.transfer.bound||u.transfer.person_id!=u.person_id)return std::nullopt;
        return u.transfer.value.map_actor==rn::UnitMapActorPresence::Present;
    }
    bool UpdateActor(std::uint16_t slot,bool partner) override {
        result.actor_status=UpdateCurrentActorPositionStaged(r,slot,partner,scene);return result.actor_status==ActorPositionStatus::Ok;
    }
    bool AddImage(std::uint16_t slot,bool selected) override {
        if(!Image())return false;const auto unit=ImageUnit(slot);if(!unit){result.image_status=UnitImageStatus::InvalidUnit;return false;}
        result.image_status=AddUnitImageExact(*image,*unit,selected);
        if(result.image_status!=UnitImageStatus::Ok)return false;
        // Rebind the incrementally edited image to the just-published coordinate
        // inputs. A whole-force rebuild could overwrite unrelated overlap cells.
        result.image_status=RestoreCurrentUnitImage(r.game,*image);return result.image_status==UnitImageStatus::Ok;
    }
    bool UpdateDanger() override {result.danger=RefreshCurrentDangerImage(r);return result.danger.status==DangerStatus::Ok;}
    std::optional<std::pair<int,int>> RescuePosition(std::uint16_t slot,int x,int y,bool allow,bool prefer) override {
        result.rescue=QueryRescuePositionWithCurrentTacticalImage(r,slot,x,y,allow,prefer);
        if(result.rescue.terrain_status!=TerrainImageStatus::Ok||result.rescue.unit_status!=UnitImageStatus::Ok||result.rescue.position.status!=RescuePositionStatus::Ok)return std::nullopt;
        return std::pair{result.rescue.position.x,result.rescue.position.y};
    }
};
CurrentPositionPublicationResult Publish(rn::NativeRuntime& r,int key,int x,int y,const CurrentActorHeightQuery* scene,bool free_space) {
    auto candidate=std::make_unique<rn::NativeRuntime>(r);CurrentPositionPublicationResult result{};Services svc(*candidate,scene,result);
    result.status=free_space?PublishUnitPositionFreeSpaceExact(key,x,y,svc):PublishUnitPositionExact(key,x,y,svc);
    if(result.status==S::Ok)r.game=std::move(candidate->game);
    return result;
}
}
CurrentPositionPublicationResult PublishCurrentUnitPosition(fates::runtime::native::NativeRuntime& r,int key,int x,int y,const CurrentActorHeightQuery* s){return Publish(r,key,x,y,s,false);}
CurrentPositionPublicationResult PublishCurrentUnitPositionFreeSpace(fates::runtime::native::NativeRuntime& r,int key,int x,int y,const CurrentActorHeightQuery* s){return Publish(r,key,x,y,s,true);}
}
