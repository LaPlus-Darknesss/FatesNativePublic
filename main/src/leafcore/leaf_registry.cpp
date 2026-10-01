#include "fates/leafcore/core_leaf_impl.hpp"
#include <algorithm>

namespace fates::leafcore {
namespace {
constexpr LeafSpec kSpecs[] = {
    {"Decimalize", 0x0016448Cu, "Decimalize", LeafFamily::CoreUtility},
    {"Font__Draw_3", 0x003CFB38u, "Font__Draw_3", LeafFamily::Presentation},
    {"UIFont__GetWidth", 0x0044D160u, "UIFont__GetWidth", LeafFamily::Presentation},
    {"Color8__operator_4", 0x0053D5F8u, "Color8__operator_4", LeafFamily::Presentation},
    {"SoundHandle__PlayCore", 0x001A5964u, "SoundHandle__PlayCore", LeafFamily::AudioPlayback},
    {"CreateGlobalFile", 0x001660BCu, "CreateGlobalFile", LeafFamily::ContentIo},
    {"MultiSound__Play", 0x0017D7FCu, "MultiSound__Play", LeafFamily::AudioPlayback},
    {"MovieViewer__GetProc", 0x0019FD74u, "MovieViewer__GetProc", LeafFamily::Movie},
    {"RandomSound__Play", 0x001A2F94u, "RandomSound__Play", LeafFamily::AudioPlayback},
    {"MovieSubtitle__Draw", 0x001C9594u, "MovieSubtitle__Draw", LeafFamily::Movie},
    {"MovieSubtitle__FadeIn", 0x001C9884u, "MovieSubtitle__FadeIn", LeafFamily::Movie},
    {"MovieSubtitle__FadeOut", 0x001C9898u, "MovieSubtitle__FadeOut", LeafFamily::Movie},
    {"MovieSubtitle__SetMessId", 0x001C98ACu, "MovieSubtitle__SetMessId", LeafFamily::Movie},
    {"ContentsReader__MountContent", 0x001CD04Cu, "ContentsReader__MountContent", LeafFamily::ContentIo},
    {"ContentsReader__Load", 0x001CD22Cu, "ContentsReader__Load", LeafFamily::ContentIo},
    {"GameFontHelper__EquipSkill__Draw", 0x001D088Cu, "GameFontHelper__EquipSkill__Draw", LeafFamily::Presentation},
    {"GameFontHelper__Food__Draw", 0x001D0A98u, "GameFontHelper__Food__Draw", LeafFamily::Presentation},
    {"GameFontHelper__Item__DrawForSignal", 0x001D0C14u, "GameFontHelper__Item__DrawForSignal", LeafFamily::Presentation},
    {"GameFontHelper__Gemstone__Draw", 0x001D1294u, "GameFontHelper__Gemstone__Draw", LeafFamily::Presentation},
    {"ContentsManager__LoadGlobal", 0x001DD950u, "ContentsManager__LoadGlobal", LeafFamily::ContentIo},
    {"ContentsManager__LoadLocal", 0x001DDDC4u, "ContentsManager__LoadLocal", LeafFamily::ContentIo},
    {"FieldWeatherImpl__Load", 0x001EAC94u, "FieldWeatherImpl__Load", LeafFamily::FieldCollision},
    {"FieldWeatherImpl__Update", 0x001EB07Cu, "FieldWeatherImpl__Update", LeafFamily::FieldCollision},
    {"GameFont__GetMaxWidth", 0x003CFA64u, "GameFont__GetMaxWidth", LeafFamily::Presentation},
    {"Icon__System__Draw_2", 0x003D0ADCu, "Icon__System__Draw_2", LeafFamily::Presentation},
    {"MovieSubtitle__Tick", 0x0041BC3Cu, "MovieSubtitle__Tick", LeafFamily::Movie},
    {"UIFont__GetMaxHeight", 0x0044C9B0u, "UIFont__GetMaxHeight", LeafFamily::Presentation},
    {"UIFont__Draw_2", 0x0044D084u, "UIFont__Draw_2", LeafFamily::Presentation},
    {"castle__Food__GetData", 0x0048BE3Cu, "castle__Food__GetData", LeafFamily::ContentIo},
    {"castle__Gemstone__GetData", 0x00497534u, "castle__Gemstone__GetData", LeafFamily::ContentIo},
    {"Graphics__GetClearType", 0x004EBB54u, "Graphics__GetClearType", LeafFamily::Presentation},
    {"Graphics__SetClearType", 0x004EBB64u, "Graphics__SetClearType", LeafFamily::Presentation},
    {"FieldUtil__GetRound", 0x004FF9A8u, "FieldUtil__GetRound", LeafFamily::FieldCollision},
    {"MapDataFile__GetFileList", 0x00508724u, "MapDataFile__GetFileList", LeafFamily::ContentIo},
    {"MapDataFile__GetReferList", 0x0050875Cu, "MapDataFile__GetReferList", LeafFamily::ContentIo},
    {"ContentsReader__GetListIndex", 0x0050A6CCu, "ContentsReader__GetListIndex", LeafFamily::ContentIo},
    {"ParameterSound__Play", 0x0050B250u, "ParameterSound__Play", LeafFamily::AudioPlayback},
    {"AABB__GetCenter", 0x00528DF8u, "AABB__GetCenter", LeafFamily::FieldCollision},
    {"ISFont__GetGlyphHeight", 0x0053D720u, "ISFont__GetGlyphHeight", LeafFamily::Presentation},
    {"ISFont__GetGlyph_2", 0x0053D748u, "ISFont__GetGlyph_2", LeafFamily::Presentation},
    {"castle__Food__Data__GetName", 0x00541650u, "castle__Food__Data__GetName", LeafFamily::ContentIo},
    {"castle__Gemstone__Data__GetName", 0x00542C44u, "castle__Gemstone__Data__GetName", LeafFamily::ContentIo},
    {"ColsTree__CalcHit_2", 0x0054588Cu, "ColsTree__CalcHit_2", LeafFamily::FieldCollision},
    {"PolyData__GetIntersect", 0x00546588u, "PolyData__GetIntersect", LeafFamily::FieldCollision},
    {"PolyData__GetNormal", 0x00546728u, "PolyData__GetNormal", LeafFamily::FieldCollision},
    {"HeightMap__GetPolyData", 0x00548784u, "HeightMap__GetPolyData", LeafFamily::FieldCollision},
};
}
const LeafSpec* GetLeafSpecs() { return kSpecs; }
std::size_t GetLeafSpecCount() { return sizeof(kSpecs)/sizeof(kSpecs[0]); }
const LeafSpec* FindLeafSpec(std::uint32_t retail_address) { const auto* b=kSpecs; const auto* e=kSpecs+GetLeafSpecCount(); const auto it=std::find_if(b,e,[retail_address](const LeafSpec& s){ return s.retail_address==retail_address; }); return it==e?nullptr:it; }
LeafWord InvokeLeaf(LeafRuntime& runtime, std::uint32_t retail_address, const LeafWord* args, std::size_t argc) { const LeafSpec* spec=FindLeafSpec(retail_address); if(spec==nullptr || runtime.invoke==nullptr) return 0; return runtime.invoke(runtime.user,*spec,args,argc); }
} // namespace fates::leafcore
