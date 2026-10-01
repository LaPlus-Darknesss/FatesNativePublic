#include "fates/campaign/native_chapter_complete_advance.hpp"
#include <algorithm>
namespace fates::campaign::native {
ChapterCompleteDecision ResolveMainSequenceChapterComplete(std::uint32_t flags,std::uint8_t type,RouteIndex route,
 std::uint8_t b,std::uint8_t c,std::uint8_t r,bool ending){
 ChapterCompleteDecision out{};
 if ((flags&0x2u)!=0){out.route=ChapterCompleteRoute::ResetCurrentSpotMobs;return out;}
 if (type==static_cast<std::uint8_t>(ChapterType::DragonsGate)){out.route=ChapterCompleteRoute::ReturnNoAdvance;return out;}
 out.update_current_spot=true;
 out.next_chapter_index=SelectRouteNextChapter(b,c,r,route);
 if (out.next_chapter_index!=0 && ending){out.route=ChapterCompleteRoute::JumpEndingLabel10;return out;}
 if (out.next_chapter_index!=0){out.set_spot_current_to_next=true;out.route=ChapterCompleteRoute::Advance;}
 else if ((flags&0x1000u)==0){out.route=ChapterCompleteRoute::JumpLabel2;return out;}
 else out.route=ChapterCompleteRoute::Advance;
 out.update_world_mobs=(flags&0x2000u)!=0;
 return out;
}
CompletedSpotMark MarkCompletedSpot(int level){ return {2,static_cast<std::int16_t>(std::clamp(level,-32768,32767))}; }
bool UnlockOrdinaryRequirementSpot(std::uint8_t state,bool req,std::uint8_t type,std::uint16_t married){
 return state==0 && req && married==0 && type!=2 && type!=7;
}
bool WorldMobImmediateSpawnPassEnabled(RouteIndex route){return route!=RouteIndex::Conquest;}
int WorldMobImmediateSpawnSuccessCap(){return 2;}
int WorldMobSpawnWeight(int i){if(i<0)return 0;if(i<4)return 20;if(i<6)return 15;if(i<8)return 5;if(i<12)return 3;return 1;}
bool WorldMobCombatConsumesDraw(int n){return n>1;}
MobCombatOutcome ResolveWorldMobCombatRoll(int roll){if(roll==0)return MobCombatOutcome::ResetFirst;if(roll==1)return MobCombatOutcome::ResetSecond;return MobCombatOutcome::ResetBoth;}
bool WorldMobFallbackSpawnAllowed(RouteIndex route,bool current,int sum){return route!=RouteIndex::Conquest && !current && sum>0;}
MobDisposDrawPlan ResolveMobDisposDrawPlan(bool both,int jobs){return {both,true,jobs>0,jobs>0};}
} // namespace fates::campaign::native
