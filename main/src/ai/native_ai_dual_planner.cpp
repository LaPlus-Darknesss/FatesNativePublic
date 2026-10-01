#include "fates/ai/native_ai_dual_planner.hpp"
#include "fates/runtime/native_rng.hpp"
#include <algorithm>
#include <cstdlib>
namespace fates::ai::native {
std::optional<std::uint32_t> DualAttackScoreExact(const AiDualBattleFacts& f) noexcept {
    const auto base=std::uint32_t(f.power)*std::uint32_t(f.hit);
    if(f.attack_count<=0 || base==0)return std::nullopt;
    const auto critical=base*std::uint32_t(f.critical)*(std::uint32_t(f.damage_rate)-100u);
    return base*100u+critical/100u;
}
AiDualPlan PlanWholeForceDualExact(const AiDualPlanningInput& in,const AiDualPlanningServices& services) {
    AiDualPlan out{};out.next_ai_random=in.ai_random;auto random=in.ai_random;
    auto fail=[&](AiDualPlanStatus status){out.status=status;out.choice.reset();return out;};
    auto query=[&](AiDualQueryKind k,std::uint32_t u,std::uint32_t a=0,std::uint32_t b=0){out.queries.push_back({k,u,a,b});};
    auto roll=[&]()->std::optional<bool> {
        if(!random.initialized)return std::nullopt;
        const auto v=fates::runtime::native::RandomValue(random,2);++out.attempted_ai_draws;
        query(AiDualQueryKind::Random,0,2,v);return v==0;
    };
    // The original gates precede map and force traversal.
    if(in.actor_no_dual || !in.can_dual || in.difficulty==0){out.status=AiDualPlanStatus::Complete;return out;}
    auto inside=[&](int x,int y){return x>=in.min_x && x<in.max_x && y>=in.min_y && y<in.max_y;};
    if(in.actor==kNoDualUnit || in.target==kNoDualUnit || in.actor==in.target || in.weapon_index>9 ||
       in.min_x<0 || in.min_y<0 || in.max_x>32 || in.max_y>32 || in.min_x>=in.max_x || in.min_y>=in.max_y ||
       !inside(in.actor_x,in.actor_y) || !inside(in.attack_x,in.attack_y) || in.force_order.size()>250)return out;
    for(std::size_t i=0;i<in.force_order.size();++i) {
        const auto& u=in.force_order[i];
        if(u.slot==kNoDualUnit || (u.partner_slot && (*u.partner_slot==kNoDualUnit || *u.partner_slot==u.slot)))return out;
        for(std::size_t j=0;j<i;++j)if(in.force_order[j].slot==u.slot)return out;
    }
    struct Cell {std::int16_t x,y;std::int32_t score;};std::vector<Cell> cells;
    auto image=[&](int x,int y){return in.image[std::size_t(y*32+x)];};
    for(int y=std::max<int>(in.min_y,in.attack_y-1);y<std::min<int>(in.max_y,in.attack_y+2);++y)
        for(int x=std::max<int>(in.min_x,in.attack_x-1);x<std::min<int>(in.max_x,in.attack_x+2);++x) {
            if(std::abs(x-in.attack_x)+std::abs(y-in.attack_y)!=1)continue;
            const auto cell=image(x,y);if(cell==AiDualCell::Unknown)return fail(AiDualPlanStatus::MissingWorld);
            if(cell!=AiDualCell::Empty || (x==in.actor_x && y==in.actor_y))continue;
            cells.push_back({std::int16_t(x),std::int16_t(y),255});
        }
    out.landing_cells=std::uint32_t(cells.size());
    for(auto& cell:cells)
        for(int y=std::max<int>(in.min_y,cell.y-2);y<std::min<int>(in.max_y,cell.y+3);++y)
            for(int x=std::max<int>(in.min_x,cell.x-2);x<std::min<int>(in.max_x,cell.x+3);++x) {
                if(std::abs(x-cell.x)+std::abs(y-cell.y)!=2)continue;
                const auto value=image(x,y);if(value==AiDualCell::Unknown)return fail(AiDualPlanStatus::MissingWorld);
                if(value==AiDualCell::Hostile)--cell.score;
            }
    std::uint32_t best=0;
    if(!cells.empty())for(const auto& member:in.force_order) {
        if(member.slot==in.actor || (member.policy_flags&0x0f000000u) || (member.public_flags&0xc5u) || member.no_dual)continue;
        query(AiDualQueryKind::Permission,member.slot,in.target);
        const auto permission=services.AttackPermission(member.slot,in.target);
        if(!permission)return fail(AiDualPlanStatus::MissingService);if(!*permission)continue;
        query(AiDualQueryKind::Command,member.slot,in.target);
        const auto command=services.ActiveCrossfireCommand(member.slot,in.target);
        if(!command)return fail(AiDualPlanStatus::MissingService);if(!*command)continue;
        query(AiDualQueryKind::Movement,member.slot,2,0xffffffffu);
        const auto field=services.Movement(member.slot);if(!field)return fail(AiDualPlanStatus::MissingService);
        for(unsigned lane=0;lane<2;++lane) {
            if(lane && (!(member.policy_flags&0x80u) || !member.partner_slot))break;
            const auto source=lane?*member.partner_slot:member.slot;const Cell* selected=nullptr;
            for(const auto& cell:cells) {
                if(field->cost[std::size_t(cell.y*32+cell.x)]<0)continue;
                if(lane) {
                    query(AiDualQueryKind::Terrain,source,std::uint32_t(cell.x),std::uint32_t(cell.y));
                    const auto cost=services.TerrainCost(source,cell.x,cell.y);
                    if(!cost)return fail(AiDualPlanStatus::MissingService);if(*cost<0)continue;
                }
                if(selected) {
                    if(selected->score>cell.score)continue;
                    if(selected->score==cell.score) {
                        ++out.position_ties;const auto pick=roll();if(!pick)return fail(AiDualPlanStatus::MissingAiRandom);if(!*pick)continue;
                    }
                }
                selected=&cell;
            }
            if(!selected)continue;
            for(std::uint8_t index=0;index<5;++index) {
                query(AiDualQueryKind::Equip,source,index);
                const auto equip=services.CanEquip(source,index);
                if(!equip)return fail(AiDualPlanStatus::MissingService);if(!*equip)continue;
                query(AiDualQueryKind::Preview,source,index,in.weapon_index);
                const auto facts=services.Preview({in.actor,in.target,source,in.attack_x,in.attack_y,in.weapon_index,index});
                if(!facts)return fail(AiDualPlanStatus::MissingService);
                const auto score=DualAttackScoreExact(*facts);if(!score || *score<best)continue;
                if(*score==best) {
                    ++out.weapon_ties;const auto pick=roll();if(!pick)return fail(AiDualPlanStatus::MissingAiRandom);if(!*pick)continue;
                }
                best=*score;out.choice=AiDualChoice{member.slot,selected->x,selected->y,std::uint8_t(lane*5+index),best};
            }
        }
    }
    out.status=AiDualPlanStatus::Complete;out.next_ai_random=random;return out;
}
}
