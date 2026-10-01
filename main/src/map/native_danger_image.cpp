#include "fates/map/native_danger_image.hpp"
#include "fates/map/native_terrain_semantics.hpp"
#include "fates/map/native_scenario_tricks.hpp"
#include "fates/runtime/native_force_order.hpp"
#include "fates/runtime/native_current_item_eligibility.hpp"
#include "fates/runtime/native_movement_rules.hpp"
#include "fates/battle/native_battle_conditions.hpp"
#include <algorithm>
#include <cstdlib>
namespace fates::map::native {
namespace {
using S=DangerStatus;
int Signed(std::uint8_t byte) noexcept {return byte<128?int(byte):int(byte)-256;}
void PaintBand(DeploymentBits& plane,MovementBounds bounds,int x,int y,int inner,int outer) {
    for(int ty=std::max(bounds.min_y,y-outer);ty<std::min(bounds.max_y,y+outer+1);++ty)
        for(int tx=std::max(bounds.min_x,x-outer);tx<std::min(bounds.max_x,x+outer+1);++tx) {
            const int distance=std::abs(x-tx)+std::abs(y-ty);
            if(distance>=inner&&distance<=outer)plane[(ty*32+tx)/8]|=std::uint8_t(1u<<(tx&7));
        }
}
void Merge(DeploymentBits& target,const DeploymentBits& source) noexcept {
    for(unsigned i=0;i<target.size();++i)target[i]|=source[i];
}
}
std::uint32_t DangerStaffFlagsExact(std::uint8_t selector) noexcept {
    return selector==0?0x50000u:(selector==2?0x30000u:0x10000u);
}
std::vector<DangerCannonRecord> EnumerateDangerCannonsExact(std::span<const DangerCannonRecord> registry) {
    std::vector<DangerCannonRecord> result;
    for(const auto& record:registry)if((record.fields[14]&1)&&IsCannon(record.fields[4]))result.push_back(record);
    return result;
}
DangerStatus AddDangerImageExact(DangerImage& image,DeploymentFields& deploy,TerrainImageGeometry geometry,
    std::uint8_t config,std::uint32_t unit_flags,std::span<const DangerCannonRecord> cannons,DangerAddServices& svc) {
    const MovementBounds bounds{geometry.min_x,geometry.min_y,geometry.max_x,geometry.max_y};
    if(geometry.width>32||geometry.height>32||geometry.max_x>geometry.width||geometry.max_y>geometry.height||!ValidMovementBounds(bounds))return S::InvalidGeometry;
    auto next=image;auto fields=deploy;
    if(!svc.UnitMove(fields,-1,0x8002,DangerStaffFlagsExact(config)))return S::MissingDeployment;
    for(const auto& record:cannons) {
        const auto use=svc.MountedCannonUsable(record);if(!use)return S::MissingCannonUse;if(!*use)continue;
        const auto& f=record.fields;
        const auto access=CannonAccess({Signed(f[0]),Signed(f[1]),Signed(f[2]),Signed(f[3])},f[13]);
        const auto value=svc.MoveImageGet(fields,access.first,access.second);if(!value)return S::MissingMovementCell;
        if(*value>=0)PaintBand(fields.ranges.attack,bounds,Signed(f[0]),Signed(f[1]),std::max(0,int(f[10])-int(f[12])),int(f[11])+int(f[12]));
    }
    const auto category=svc.IntrinsicCategory();if(!category)return S::MissingIntrinsicCategory;
    if(*category) {
        const auto available=svc.IntrinsicItemAvailable();if(!available)return S::MissingIntrinsicItem;
        if(*available) {
            const auto inner=svc.IntrinsicInner();if(!inner)return S::MissingIntrinsicRange;
            const auto area1=svc.IntrinsicArea();if(!area1)return S::MissingIntrinsicRange;
            const auto outer=svc.IntrinsicOuter();if(!outer)return S::MissingIntrinsicRange;
            const auto area2=svc.IntrinsicArea();if(!area2)return S::MissingIntrinsicRange;
            // Actual cannon helpers return the small nonnegative ranges below.
            if(*inner<0||*inner>255||*outer<0||*outer>255||*area1<0||*area1>255||*area2<0||*area2>255)return S::MissingIntrinsicRange;
            const int lo=std::max(0,*inner-*area1),hi=*outer+*area2;
            for(int y=bounds.min_y;y<bounds.max_y;++y)for(int x=bounds.min_x;x<bounds.max_x;++x) {
                const auto value=svc.MoveImageGet(fields,x,y);if(!value)return S::MissingMovementCell;
                if(*value>=0)PaintBand(fields.ranges.attack,bounds,x,y,lo,hi);
            }
        }
    }
    if(unit_flags&0x10000){Merge(next.selected_attack,fields.ranges.attack);Merge(next.selected_rod,fields.ranges.rod);}
    Merge(next.all_attack,fields.ranges.attack);Merge(next.all_rod,fields.ranges.rod);next.flags|=2;
    deploy=fields;image=next;return S::Ok;
}
bool DangerUpdateServices::Allied(const DangerUnit& unit,std::uint8_t human) {
    return fates::runtime::native::ForcesAlliedExact(unit.force,human);
}
DangerStatus UpdateDangerImageExact(DangerImage& image,std::uint8_t human,
    std::span<const DangerUnit> units,DangerUpdateServices& svc) {
    DangerImage next{};next.flags=image.flags;
    for(const auto& unit:units) {
        if(unit.flags&0x24004u)continue;
        if(svc.Allied(unit,human))continue;
        if(!(next.flags&1)&&!(unit.flags&0x10000))continue;
        const auto status=svc.Add(next,unit);if(status!=S::Ok)return status;
    }
    next.flags|=2;image=next;return S::Ok;
}
CurrentDangerResult BuildCurrentDangerImage(DangerImage& out,const fates::runtime::native::NativeRuntime& r,
    std::optional<std::uint8_t> config) {
    namespace rn=fates::runtime::native;CurrentDangerResult result{};
    const auto& game=r.game;const auto& phase=game.phase;
    auto fail=[&](S s){result.status=s;return result;};
    if(!game.map_active||phase.stage==rn::PhaseAccessStage::Unbound||phase.chapter_index!=game.campaign.current_chapter_index||phase.situation.human_force>=3)return fail(S::MissingPhase);
    if(!game.scenario_tricks.bound)return fail(S::MissingScenario);
    if(!ScenarioScriptsMatch(r))return fail(S::StaleScenario);
    std::vector<DangerCannonRecord> registry;for(const auto& d:game.scenario_tricks.registry)registry.push_back({d.fields});
    const auto cannons=EnumerateDangerCannonsExact(registry);result.mounted_cannons=std::uint32_t(cannons.size());
    std::vector<DangerUnit> units;
    for(std::uint8_t force=0;force<3;++force) {
        bool empty=true;for(const auto& u:game.units)if(u.occupied&&u.force_type==force){empty=false;break;}
        if(empty)continue;
        const auto* order=rn::GetVerifiedForceOrder(game,force);if(!order)return fail(S::MissingForceOrder);
        for(unsigned i=0;i<order->count;++i){const auto slot=order->slots[i];const auto& u=game.units[slot];units.push_back({slot,u.force_type,u.flags});}
    }
    struct Update: DangerUpdateServices {
        const rn::NativeRuntime& r;std::optional<std::uint8_t> config;std::span<const DangerCannonRecord> cannons;CurrentDangerResult& result;
        Update(const rn::NativeRuntime& r,std::optional<std::uint8_t> c,std::span<const DangerCannonRecord> v,CurrentDangerResult& o):r(r),config(c),cannons(v),result(o){}
        S Add(DangerImage& image,const DangerUnit& unit) override {
            result.unit_slot=unit.slot;if(!config)return S::MissingConfiguration;
            const auto terrain=ReadCurrentTerrainImage(r);if(terrain.status!=TerrainImageStatus::Ok)return S::MissingDefinition;
            struct AddServices: DangerAddServices {
                Update& owner;std::uint16_t slot;S error{S::Ok};
                AddServices(Update& o,std::uint16_t s):owner(o),slot(s){}
                const rn::UnitState& Unit() const {return owner.r.game.units[slot];}
                bool UnitMove(DeploymentFields& fields,int power,std::uint32_t flags,std::uint32_t extra) override {
                    const auto& u=Unit();
                    if(u.x<-128||u.x>255||u.y<-128||u.y>255){error=S::InvalidUnit;return false;}
                    owner.result.deployment=BuildCurrentUnitDeployment(fields,owner.r,slot,Signed(std::uint8_t(u.x)),Signed(std::uint8_t(u.y)),power,flags,extra);
                    return owner.result.deployment.operation.status==UnitDeploymentStatus::Ok;
                }
                std::optional<bool> MountedCannonUsable(const DangerCannonRecord& record) override {
                    const auto type=record.fields[4];if(!IsCannon(type))return false;
                    const auto& u=Unit();
                    if(type==0x1b){if(!u.enhance.bound){error=S::MissingEnhance;return std::nullopt;}if(u.enhance.flags[0]&8)return false;}
                    const auto& d=owner.r.definitions;const auto* p=d.FindPerson(u.person_id);const auto* j=d.FindJob(u.job_id);
                    if(!p||!j){error=S::MissingDefinition;return std::nullopt;}
                    const auto limits=rn::ResolveCurrentWeaponExpLimits(*p,*j,u,d.weapon_rank_thresholds());
                    const auto group=*CannonWeaponExpKind(type,false);return std::min(limits[group],u.weapon_exp[group])>0;
                }
                std::optional<int> MoveImageGet(const DeploymentFields& fields,int x,int y) override {
                    if(x<0||y<0||x>=32||y>=32){error=S::MissingMovementCell;return std::nullopt;}
                    const auto t=ReadCurrentTerrainImage(owner.r);if(t.status!=TerrainImageStatus::Ok){error=S::MissingDefinition;return std::nullopt;}
                    const auto id=t.planes->terrain[y*32+x];const auto& tiles=owner.r.definitions.terrain_map()->terrain_types;
                    if(id>=tiles.size()){error=S::MissingDefinition;return std::nullopt;}
                    return (tiles[id].flags_0x18&8)?-1:int(fields.movement[y*32+x]);
                }
                std::optional<bool> IntrinsicCategory() override {
                    const auto& rules=owner.r.definitions.movement_rules();if(!rules){error=S::MissingDefinition;return std::nullopt;}
                    const auto category=fates::battle::native::ProjectUnitBattleCategory(owner.r,Unit());
                    if(!category){error=S::MissingDefinition;return std::nullopt;}return (*category&rules->shooter_category)!=0;
                }
                std::optional<bool> IntrinsicItemAvailable() override {
                    const auto value=BuildCurrentCannonItemAvailability(owner.r,slot,true);
                    if(value.status!=DeploymentRangeStatus::Ok){owner.result.deployment.range_status=value.status;error=S::MissingEligibility;return std::nullopt;}
                    return value.available;
                }
                std::optional<bool> Modification() {
                    const auto* skill=owner.r.definitions.FindSkill("SEID_\x96\x43\x90\x67\x89\xfc\x91\xa2");
                    if(!skill||skill->id>32767){error=S::MissingDefinition;return std::nullopt;}
                    const auto value=rn::ProjectCurrentEquippedSkill(owner.r,Unit(),std::int16_t(skill->id));if(!value)error=S::MissingSkill;return value;
                }
                std::optional<int> IntrinsicInner() override {const auto s=Modification();return s?std::optional<int>(*s?2:3):std::nullopt;}
                std::optional<int> IntrinsicOuter() override {const auto s=Modification();return s?std::optional<int>(*s?3:4):std::nullopt;}
                std::optional<int> IntrinsicArea() override {const auto s=Modification();return s?std::optional<int>(*s?2:1):std::nullopt;}
            } svc(*this,unit.slot);
            DeploymentFields fields{};result.unit_slot=unit.slot;
            const auto status=AddDangerImageExact(image,fields,terrain.geometry,*config,unit.flags,cannons,svc);
            if(status==S::Ok)++result.added_units;
            return svc.error==S::Ok?status:svc.error;
        }
    } svc(r,config,cannons,result);
    result.status=UpdateDangerImageExact(out,phase.situation.human_force,units,svc);return result;
}
namespace {
// Capture the semantic inputs of the existing providers, not their implementation
// layouts. This deliberately reuses their validity checks. Equivalent resolved
// inputs may retain a valid image; generation/phase/context changes never do.
// No movement search or threat rasterization occurs while checking this record.
struct InputRecord {
    std::vector<std::uint64_t> words;
    template<class T>void Put(T v){words.push_back(static_cast<std::uint64_t>(v));}
    template<class T,std::size_t N>void Put(const std::array<T,N>& a){for(const auto& v:a)Put(v);}
    template<class T>void Put(const std::optional<T>& v){Put(v.has_value());if(v)Put(*v);}
};
std::vector<std::uint64_t> CurrentDangerInputs(const fates::runtime::native::NativeRuntime& r,std::uint32_t flags) {
    namespace rn=fates::runtime::native;InputRecord stamp;auto& g=r.game;const auto& d=r.definitions;
    stamp.Put(flags&1u);stamp.Put(g.map_active);stamp.Put(g.campaign.current_chapter_index);stamp.Put(g.campaign.game_user_flags);
    stamp.Put(g.phase.stage);stamp.Put(g.phase.chapter_index);stamp.Put(g.phase.revision);stamp.Put(g.phase.situation.human_force);
    stamp.Put(g.phase.situation.active_force);stamp.Put(g.phase.situation.control);stamp.Put(g.danger_staff_range_selector);
    stamp.Put(g.game_user_difficulty.bound);stamp.Put(g.game_user_difficulty.value);stamp.Put(g.game_user_difficulty.revision);
    stamp.Put(g.scenario_tricks.bound);stamp.Put(ScenarioScriptsMatch(r));stamp.Put(g.scenario_tricks.registry.size());
    for(const auto& record:g.scenario_tricks.registry)stamp.Put(record.fields);
    for(unsigned f=0;f<3;++f){const auto& order=f?g.other_force_orders[f-1]:g.player_force_order;
        stamp.Put(order.bound);stamp.Put(order.count);stamp.Put(order.slots);stamp.Put(order.person_ids);}
    const auto terrain=ReadCurrentTerrainImage(r);stamp.Put(terrain.status);std::array<bool,256> cost_indices{};
    if(terrain.status==TerrainImageStatus::Ok){const auto t=terrain.geometry;
        for(auto v:{t.width,t.height,t.min_x,t.min_y,t.max_x,t.max_y})stamp.Put(v);
        stamp.Put(terrain.planes->terrain);stamp.Put(terrain.planes->cost_indices);
        for(auto c:terrain.planes->cost_indices)cost_indices[c]=true;
        const auto& tiles=d.terrain_map()->terrain_types;stamp.Put(tiles.size());
        for(const auto& tile:tiles){stamp.Put(tile.flags_0x18);stamp.Put(tile.movement_cost_index);stamp.Put(tile.change_id_1);stamp.Put(tile.change_id_2);}
    }
    stamp.Put(d.movement_costs().size());for(const auto& row:d.movement_costs()){stamp.Put(row.size());for(auto v:row)stamp.Put(v);}
    const auto occupancy=ReadCurrentUnitImage(g);stamp.Put(occupancy.status);
    if(occupancy.status==UnitImageStatus::Ok){stamp.Put(occupancy.image->width);stamp.Put(occupancy.image->height);stamp.Put(occupancy.image->cells);}
    auto skill=[&](const rn::UnitState& u,const char* name)->std::optional<bool>{const auto* s=d.FindSkill(name);return s&&s->id<=32767?rn::ProjectCurrentEquippedSkill(r,u,std::int16_t(s->id)):std::nullopt;};
    for(unsigned slot=0;slot<g.units.size();++slot){const auto& u=g.units[slot];stamp.Put(g.unit_slot_generations[slot]);stamp.Put(u.occupied);
        if(!u.occupied)continue;
        stamp.Put(u.person_id);stamp.Put(u.job_id);stamp.Put(u.force_type);stamp.Put(u.flags);stamp.Put(u.x);stamp.Put(u.y);
        stamp.Put(u.ai.runtime_tuning_bound);stamp.Put(u.ai.policy_flags);stamp.Put(rn::ProjectCurrentMovementProhibition(r,u));
        const bool selected=u.force_type<3&&!(u.flags&0x24004)&&!rn::ForcesAlliedExact(u.force_type,g.phase.situation.human_force)&&((flags&1)||(u.flags&0x10000));
        stamp.Put(selected);if(!selected)continue;
        const auto power=rn::ProjectCurrentMovementPower(r,u);stamp.Put(power.status);stamp.Put(power.value);
        stamp.Put(rn::ProjectCurrentMovementCostFree(r,u));stamp.Put(rn::ProjectCurrentMovementBaseCostFallback(r,u));
        stamp.Put(skill(u,"SEID_\x82\xb7\x82\xe8\x94\xb2\x82\xaf"));
        const auto* job=d.FindJob(u.job_id);stamp.Put(job!=nullptr);if(job)stamp.Put(job->movement_cost_index);
        stamp.Put(u.pair.partner_slot);stamp.Put(u.pair.bound);
        if(u.pair.partner_slot<g.units.size()){
            const auto& p=g.units[u.pair.partner_slot];stamp.Put(p.occupied);stamp.Put(p.pair.bound);stamp.Put(p.pair.partner_slot);
            for(unsigned c=0;c<cost_indices.size();++c)if(cost_indices[c])stamp.Put(rn::ProjectCurrentTerrainCost(r,p,std::uint8_t(c)));
        }
        // Cost-free, fallback and Pass add only100,1000000,4000/remove2. None is
        // consumed by GetRangeBit. Its complete relevant mask is8002|staffflags.
        const auto range_flags=0x8002u|DangerStaffFlagsExact(g.danger_staff_range_selector.value_or(0));
        const auto ranges=BuildCurrentDeploymentRanges(r,std::uint16_t(slot),range_flags,range_flags);
        stamp.Put(ranges.status);stamp.Put(ranges.item_range_status);
        stamp.Put(ranges.masks.attack);stamp.Put(ranges.masks.rod);stamp.Put(ranges.masks.without_partner_attack);stamp.Put(ranges.masks.without_partner_rod);stamp.Put(ranges.masks.attack_max);stamp.Put(ranges.masks.rod_max);
        stamp.Put(u.enhance.bound);stamp.Put(u.enhance.flags[0]);
        const auto* person=d.FindPerson(u.person_id);stamp.Put(person!=nullptr);
        if(person&&job){const auto limits=rn::ResolveCurrentWeaponExpLimits(*person,*job,u,d.weapon_rank_thresholds());for(unsigned group:{3u,4u,5u})stamp.Put(std::min(limits[group],u.weapon_exp[group]));}
        const auto category=fates::battle::native::ProjectUnitBattleCategory(r,u);stamp.Put(category);
        const auto& rules=d.movement_rules();stamp.Put(bool(rules));if(rules)stamp.Put(rules->shooter_category);
        const auto cannon=BuildCurrentCannonItemAvailability(r,std::uint16_t(slot),true);stamp.Put(cannon.status);stamp.Put(cannon.available);
        stamp.Put(skill(u,"SEID_\x96\x43\x90\x67\x89\xfc\x91\xa2"));
    }
    return std::move(stamp.words);
}
}
struct DangerImageAccess {
    using Game=fates::runtime::native::NativeGameState;using Runtime=fates::runtime::native::NativeRuntime;
    static void Initialize(Game& game){auto& s=game.tactical_image;s.danger_={};s.danger_initialized_=true;s.danger_bound_=false;s.danger_inputs_.clear();}
    static void Invalidate(Game& game) noexcept {game.tactical_image.danger_bound_=false;}
    static S Whole(Game& game,bool value) noexcept {auto& s=game.tactical_image;if(!s.danger_initialized_)return S::UnboundImage;s.danger_.flags=(s.danger_.flags&~1u)|unsigned(value);return S::Ok;}
    static CurrentDangerResult Refresh(Runtime& r){auto& s=r.game.tactical_image;if(!s.danger_initialized_)return {S::UnboundImage};
        auto next=s.danger_;auto result=BuildCurrentDangerImage(next,r,r.game.danger_staff_range_selector);if(result.status!=S::Ok)return result;
        auto inputs=CurrentDangerInputs(r,next.flags);s.danger_=next;s.danger_inputs_=std::move(inputs);s.danger_bound_=true;return result;
    }
    static CurrentDangerView Read(const Runtime& r){const auto& s=r.game.tactical_image;if(!s.danger_initialized_||!s.danger_bound_)return {S::UnboundImage};
        if(CurrentDangerInputs(r,s.danger_.flags)!=s.danger_inputs_)return {S::StaleImage};return {S::Ok,&s.danger_};
    }
};
void InitializeCurrentDangerImage(fates::runtime::native::NativeGameState& g){DangerImageAccess::Initialize(g);}
void InvalidateCurrentDangerImage(fates::runtime::native::NativeGameState& g) noexcept {DangerImageAccess::Invalidate(g);}
DangerStatus SetCurrentWholeDanger(fates::runtime::native::NativeGameState& g,bool whole) noexcept {return DangerImageAccess::Whole(g,whole);}
CurrentDangerResult RefreshCurrentDangerImage(fates::runtime::native::NativeRuntime& r){return DangerImageAccess::Refresh(r);}
CurrentDangerView ReadCurrentDangerImage(const fates::runtime::native::NativeRuntime& r){return DangerImageAccess::Read(r);}
}
