#include "fates/map/native_map_cursor.hpp"
#include "fates/runtime/native_runtime.hpp"
#include <cmath>
#include <limits>

namespace fates::map::native {
using namespace runtime::native;using S=MapCursorStatus;
namespace {
std::int32_t Truncate(float v) {
    if(std::isnan(v))return 0;
    if(v>=2147483648.0f)return std::numeric_limits<std::int32_t>::max();
    if(v<=-2147483648.0f)return std::numeric_limits<std::int32_t>::min();
    return static_cast<std::int32_t>(v);
}
float Center(std::int32_t v){volatile float r=static_cast<float>(v)+0.5f;return r;}
std::int32_t Clamp(std::int32_t v,std::int32_t lo,std::int32_t hi){return v<lo?lo:v>hi?hi:v;}
std::int32_t SignedByte(std::int16_t v){const auto b=static_cast<std::uint8_t>(v);return b<128?b:std::int32_t(b)-256;}
}
S NativeMapCursor::Create(std::shared_ptr<NativeRuntime> r,std::shared_ptr<NativeProcessScheduler> scheduler,
    std::shared_ptr<NativeMapCamera> camera,std::shared_ptr<NativeMapCursor>& out) {
    if(!r)return S::NullRuntime;
    if(!scheduler || !scheduler->root(2) || !camera || camera->runtime()!=r || !camera->UsesScheduler(*scheduler))return S::MismatchedDomain;
    auto next=std::shared_ptr<NativeMapCursor>(new NativeMapCursor);
    next->runtime_=std::move(r);next->scheduler_=scheduler;next->camera_=std::move(camera);out=std::move(next);return S::Ready;
}
S NativeMapCursor::Validate() const {
    auto scheduler=scheduler_.lock();if(!scheduler || !scheduler->root(2))return S::Retired;
    if(!current_)return S::UnknownCurrent;
    const auto& g=runtime_->game;
    if(!g.map_active || !world_ || g.field_scene.owner!=world_ || g.campaign.current_chapter_index!=chapter_)return S::StaleField;
    return S::Ready;
}
S NativeMapCursor::RestoreCarried(MapCursorMotion motion) {
    auto scheduler=scheduler_.lock();if(!scheduler || !scheduler->root(2))return S::Retired;
    if(scheduler->busy())return S::Busy;
    const auto& g=runtime_->game;const auto& f=g.field_scene;
    if(!g.map_active || !f.owner || !f.snapshot || f.snapshot->world!=f.owner ||
        f.snapshot->chapter!=g.campaign.current_chapter_index)return S::StaleField;
    for(auto p:{motion.saved,motion.current,motion.target})if(!std::isfinite(p.x)||!std::isfinite(p.y))return S::InvalidCoordinate;
    current_=motion;world_=f.owner;chapter_=g.campaign.current_chapter_index;return S::Ready;
}
std::optional<MapCursorMotion> NativeMapCursor::Observe() const {return Validate()==S::Ready?current_:std::nullopt;}
TurnCursorSelection NativeMapCursor::SelectTurnFirst(std::uint8_t force) const {
    TurnCursorSelection out;out.status=Validate();if(out.status!=S::Ready)return out;
    const auto& g=runtime_->game;const auto& phase=g.phase;const auto& situation=phase.situation;
    if(force>=3 || phase.stage==PhaseAccessStage::Unbound || phase.chapter_index!=chapter_){out.status=S::InvalidPhase;return out;}
    out.x=Truncate(current_->target.x);out.y=Truncate(current_->target.y);
    const auto type=situation.control[force];if(type==2)return out;
    if(type==3) {
        for(std::uint8_t i=0;i<3;++i)if(situation.control[i]==1) {
            const auto& remembered=situation.remembered_cursor[i];
            if(!remembered){out.status=S::UnknownRememberedPosition;return out;}
            if((*remembered)[0]!=-1){out.x=(*remembered)[0];out.y=(*remembered)[1];}
            else {out=SelectTurnFirst(i);if(out.status!=S::Ready)return out;}
            break;
        }
    } else if(type==1) {
        if(!g.game_config_flags){out.status=S::UnknownConfig;return out;}
        bool use_units=(*g.game_config_flags&8)!=0;
        if(!use_units) {
            const auto& remembered=situation.remembered_cursor[force];
            if(!remembered){out.status=S::UnknownRememberedPosition;return out;}
            use_units=(*remembered)[0]==-1;
            if(!use_units){out.x=(*remembered)[0];out.y=(*remembered)[1];}
        }
        if(use_units) {
            const auto mask=runtime_->definitions.protagonist_mask();
            if(!mask){out.status=S::MissingPrivateSkill;return out;}
            out.lookup=FindForceUnitFromPrivateSkill(*runtime_,force,*mask);
            if(out.lookup->status!=ForceSkillLookupStatus::Ok) {
                out.status=out.lookup->status==ForceSkillLookupStatus::MissingOrder?S::MissingForceOrder:S::UnitStateUnavailable;return out;
            }
            auto slot=out.lookup->slot;const auto* order=GetVerifiedForceOrder(g,force);
            if(!slot && order->count)slot=order->slots[0];
            if(slot) {
                const auto* unit=&g.units[*slot];
                if(unit->flags&4) {
                    if(!unit->pair.bound || unit->pair.partner_slot>=g.units.size() ||
                        !g.units[unit->pair.partner_slot].occupied){out.status=S::UnitStateUnavailable;return out;}
                    unit=&g.units[unit->pair.partner_slot];
                }
                if(unit->x<-128 || unit->x>255 || unit->y<-128 || unit->y>255){out.status=S::InvalidCoordinate;return out;}
                out.x=SignedByte(unit->x);out.y=SignedByte(unit->y);
            }
        }
    }
    const auto* terrain=runtime_->definitions.terrain_map();
    if(!terrain || terrain->min_x>255 || terrain->max_x>255 || terrain->max_y>255){out.status=S::StaleField;return out;}
    out.x=Clamp(out.x,terrain->min_x,static_cast<std::int32_t>(terrain->max_x)-1);
    // E0 uses Terrain+2 for BOTH lower bounds, even when min_y differs.
    out.y=Clamp(out.y,terrain->min_x,static_cast<std::int32_t>(terrain->max_y)-1);
    out.scroll=true;return out;
}
S NativeMapCursor::TurnScroll(ProcessAccess& access,TurnCursorSelection& out) {
    auto scheduler=scheduler_.lock();if(!scheduler || !access.BelongsTo(*scheduler))return S::MismatchedDomain;
    auto selected=SelectTurnFirst(runtime_->game.phase.situation.active_force);
    out=selected;if(selected.status!=S::Ready || !selected.scroll)return selected.status;
    // Camera preflight/commit cannot invoke external callbacks. Refusal is
    // atomic across the cursor+camera transaction; no observable partial state.
    if(camera_->SetDestination(access,selected.x,selected.y)!=MapCameraStatus::Ready)return S::CameraUnavailable;
    current_->target=current_->current=current_->saved={Center(selected.x),Center(selected.y)};
    return S::Ready;
}
bool NativeMapCursor::UsesRuntime(const NativeRuntime& r) const noexcept{return runtime_.get()==&r;}
bool NativeMapCursor::UsesScheduler(const NativeProcessScheduler& s) const noexcept{return scheduler_.lock().get()==&s;}
}
