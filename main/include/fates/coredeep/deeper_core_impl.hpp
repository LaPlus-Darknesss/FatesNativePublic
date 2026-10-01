#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace fates::coredeep {

using DeepWord = std::intptr_t;

enum class DeepFamily : std::uint8_t { FieldGeometry, Utility, Audio, Process, Presentation, ContentState, Core, Movie, MapUi };

struct DeepSpec { const char* retail_symbol; std::uint32_t retail_address; const char* retail_signature; DeepFamily family; };

struct DeepRuntime { using InvokeFn = DeepWord (*)(void* user, const DeepSpec& spec, const DeepWord* args, std::size_t argc); void* user=nullptr; InvokeFn invoke=nullptr; };

const DeepSpec* GetDeepSpecs();
std::size_t GetDeepSpecCount();
const DeepSpec* FindDeepSpec(std::uint32_t retail_address);
DeepWord InvokeDeep(DeepRuntime&, std::uint32_t retail_address, const DeepWord* args, std::size_t argc);

// Layout-independent retail geometry contracts proven in this closure.
struct MapRangeRect { int min_x; int min_y; int max_x; int max_y; };
void ClampMapRange31(MapRangeRect& r);
MapRangeRect MakeMapRangeXYWH(int x, int y, int width, int height);
void ExpandMapRange31(MapRangeRect& r, int x_amount, int y_amount);

struct Aabb3 { float min_x; float min_y; float min_z; float max_x; float max_y; float max_z; };
void AabbAddPoint(Aabb3& box, float x, float y, float z);
void AabbAddBox(Aabb3& box, const Aabb3& other);
void AabbOffset(Aabb3& box, float x, float y, float z);
void AabbExpand(Aabb3& box, float x, float y, float z);
void AabbExpandY(Aabb3& box, float y);
bool AabbContainsFinite(const Aabb3& outer, const Aabb3& inner);

// One source-facing function per retail implementation. Exact signatures remain in DeepSpec/evidence.
DeepWord MapRange__Clamp(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord MapRange__Reset_2(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord HeightData__Clear(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord util__Curve__Accelfloat(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord util__Curve__Decelfloat(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord AABB__Add(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord AABB__Add_2(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord sound__Effector__Effector_2(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord HeightMap__Commit(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord PolyData__GetPos(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord ProcInst__FindNext(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord FieldSound__StartReverb(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord ProcSoundMonitor__GetInstance(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord TalkCodeDisposerItem__GetColor8(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord GP__GetFloat(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord map__sound__anonymous_namespace__TSound__SetName(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord Icon__Anime__anonymous_namespace__GetIndex(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord MapRange__Add(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord MapRange__Reset_4(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord FieldArea__InvalidateArea(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord FieldData__LoadField(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord FieldData__LoadParam(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord HitResult__Reset(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord AABB__IsInside(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord PolyData__GetAABB(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord HeightData__Commit(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord HomeButton__SetCallbacks(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord ProcThread__ProcThread(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord MapDataFile__TryRead(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord SoundHandle__Play(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord ContentsUtil__GetContentID(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord ContentsUtil__Mount(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord FieldWeather__Free(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord FieldWeather__Load(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord FieldWeather__Update(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord FieldWeather__SetType(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord FieldWorldImpl__UpdateColor(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord FieldWorldImpl__LoadPlant(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord FieldWorldImpl__LoadScene(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord GameFontHelper__EquipSkill__GetCloseWidth(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord GameFontHelper__Food__GetCloseWidth(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord GameFontHelper__Item__GetWidthForSignal(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord GameFontHelper__Gemstone__GetCloseWidth(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord FieldWeatherImpl__Draw(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord ProcSoundMonitor__ResetPosition(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord SoundDuplicationChecker__Initialize(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord is__app__movie__MovieThread__SubTitleLeave(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord map__SortiePosition__SetMax(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord map__Panel__Panel_2(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord AABB__Setup(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord AABB__Expand_2(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord AABB__Offset(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord AABB__ExpandY(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord Font__GetMaxHeight(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord game__graphics__SharedTexture__Get(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord game__graphics__Window__DrawVariableWindow(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord game__graphics__Wallpaper__DrawLower(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord UIFont__SetCurrent(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord UIFont__GetMaxWidth(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord UIFont__PopFont(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord UIFont__PushFont(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord castle__JukeBox__GetBgmLabel(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord GfxUtil__GetRotRad(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord ColsTree__Cleanup(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord GameFont__anonymous_namespace__DrawImpl(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord MapRange__Expand(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord FieldData__Tick(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord FieldData__SetName(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord HitResult__Move(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord FieldParam__GetTimeZone(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord MapDataFile__GetPartsList(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord RandomSound__GetSoundItem(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord ShadowActor__GetShadowAABB(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord AABB__GetRadius(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord ISFont__GetGlyphWidth(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord ColsTree__GetCurrent(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord ColsTree__CalcHit(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord FieldArea__GetNearArea(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord FieldData__FindObject_4(DeepRuntime&, const DeepWord* args, std::size_t argc);
DeepWord HeightMap__CalcHit(DeepRuntime&, const DeepWord* args, std::size_t argc);

} // namespace fates::coredeep
