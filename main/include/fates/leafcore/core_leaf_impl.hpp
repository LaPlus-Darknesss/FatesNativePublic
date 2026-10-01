#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace fates::leafcore {

using LeafWord = std::intptr_t;

enum class LeafFamily : std::uint8_t { FieldCollision, AudioPlayback, ContentIo, Presentation, Movie, CoreUtility };

struct LeafSpec { const char* retail_symbol; std::uint32_t retail_address; const char* retail_signature; LeafFamily family; };

struct LeafRuntime { using InvokeFn = LeafWord (*)(void* user, const LeafSpec& spec, const LeafWord* args, std::size_t argc); void* user=nullptr; InvokeFn invoke=nullptr; };

const LeafSpec* GetLeafSpecs();
std::size_t GetLeafSpecCount();
const LeafSpec* FindLeafSpec(std::uint32_t retail_address);
LeafWord InvokeLeaf(LeafRuntime&, std::uint32_t retail_address, const LeafWord* args, std::size_t argc);

// Layout-independent retail contracts proven in this closure.
int ParseDecimalUtf16(const std::uint16_t* text);

struct Color8 { std::uint8_t r; std::uint8_t g; std::uint8_t b; std::uint8_t a; };
Color8 SaturatingAddColor8(Color8 lhs, Color8 rhs);

struct Aabb3 { float min_x; float min_y; float min_z; float max_x; float max_y; float max_z; };
std::array<float,3> AabbCenter(const Aabb3& box);

std::uint32_t FindU16Index(const std::uint16_t* values, std::size_t count, std::uint16_t value);

// One source-facing function per retail leaf. Exact signatures remain in LeafSpec/evidence.
LeafWord Decimalize(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord Font__Draw_3(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord UIFont__GetWidth(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord Color8__operator_4(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord SoundHandle__PlayCore(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord CreateGlobalFile(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord MultiSound__Play(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord MovieViewer__GetProc(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord RandomSound__Play(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord MovieSubtitle__Draw(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord MovieSubtitle__FadeIn(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord MovieSubtitle__FadeOut(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord MovieSubtitle__SetMessId(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord ContentsReader__MountContent(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord ContentsReader__Load(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord GameFontHelper__EquipSkill__Draw(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord GameFontHelper__Food__Draw(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord GameFontHelper__Item__DrawForSignal(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord GameFontHelper__Gemstone__Draw(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord ContentsManager__LoadGlobal(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord ContentsManager__LoadLocal(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord FieldWeatherImpl__Load(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord FieldWeatherImpl__Update(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord GameFont__GetMaxWidth(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord Icon__System__Draw_2(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord MovieSubtitle__Tick(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord UIFont__GetMaxHeight(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord UIFont__Draw_2(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord castle__Food__GetData(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord castle__Gemstone__GetData(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord Graphics__GetClearType(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord Graphics__SetClearType(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord FieldUtil__GetRound(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord MapDataFile__GetFileList(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord MapDataFile__GetReferList(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord ContentsReader__GetListIndex(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord ParameterSound__Play(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord AABB__GetCenter(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord ISFont__GetGlyphHeight(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord ISFont__GetGlyph_2(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord castle__Food__Data__GetName(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord castle__Gemstone__Data__GetName(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord ColsTree__CalcHit_2(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord PolyData__GetIntersect(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord PolyData__GetNormal(LeafRuntime&, const LeafWord* args, std::size_t argc);
LeafWord HeightMap__GetPolyData(LeafRuntime&, const LeafWord* args, std::size_t argc);

} // namespace fates::leafcore
