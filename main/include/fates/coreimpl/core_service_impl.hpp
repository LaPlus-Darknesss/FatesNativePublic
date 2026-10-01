#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace fates::coreimpl {

using ImplWord = std::intptr_t;

enum class ImplFamily : std::uint8_t { SerializationState, FieldGeometry, Audio, PresentationUi, GameplayState, Utility, Process, Movie, MapSubsystem };

struct ImplSpec { const char* retail_symbol; std::uint32_t retail_address; const char* retail_signature; ImplFamily family; };

struct ImplRuntime { using InvokeFn = ImplWord (*)(void* user, const ImplSpec& spec, const ImplWord* args, std::size_t argc); void* user=nullptr; InvokeFn invoke=nullptr; };

const ImplSpec* GetImplSpecs();
std::size_t GetImplSpecCount();
const ImplSpec* FindImplSpec(std::uint32_t retail_address);
ImplWord InvokeImpl(ImplRuntime&, std::uint32_t retail_address, const ImplWord* args, std::size_t argc);

// Layout-independent contracts directly proven by the retail bodies.
struct StreamCursor { std::uint8_t* data=nullptr; std::size_t position=0; };
std::uint8_t StreamReadByte(StreamCursor&);
std::uint16_t StreamReadShortLE(StreamCursor&);
std::uint32_t StreamReadLongLE(StreamCursor&);
void StreamReadBlock(StreamCursor&, std::uint8_t* dst, std::size_t size);
const char* StreamReadCString(StreamCursor&);
void StreamWriteByte(StreamCursor&, std::uint8_t);
void StreamWriteShortLE(StreamCursor&, std::uint16_t);
void StreamWriteLongLE(StreamCursor&, std::uint32_t);
void StreamWriteBlock(StreamCursor&, const std::uint8_t* src, std::size_t size);
void StreamWriteCString(StreamCursor&, const char* text);

struct Aabb3 { float min_x, min_y, min_z, max_x, max_y, max_z; };
bool AabbIntersects(const Aabb3&, const Aabb3&);

struct MapRangeRect { int min_x, min_y, max_x, max_y; };
void MapRangeAddPoint(MapRangeRect&, int x, int y);
std::size_t PackedFlagByteCount(std::size_t flag_count);

struct Color8 { std::uint8_t r,g,b,a; };
Color8 MultiplyColor8(const Color8&, const Color8&);

struct MapPoseWords { std::array<float,9> value{}; };
MapPoseWords CopyMapPose(const MapPoseWords&);

std::size_t RandomSeedSerializedWordCount();

// One source-facing wrapper per promoted retail implementation function. Exact signatures stay in ImplSpec/evidence.
ImplWord Stream__WriteByte(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord Stream__ReadByte(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord MapPose__GetMatrixSRT(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord SoundPrivate__GetSoundPlayer(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord Stream__WriteLong(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord Stream__WriteShort(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord Stream__ReadLong(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord Stream__ReadShort(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord Font__SetCurrent(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord Stream__ReadBlock(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord versus__Config__Get(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord AABB__Reset(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord SoundPrivate__GetFreeHandleSE(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord anonymous_namespace__FadeCreate(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord ColsTree__Remove(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord MapRange__Reset_5(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord CastleDefendSettingSequence__GetInstance(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord util__MoveTime__SetTime(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord Stream__WriteBlock(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord Darkness__Stop(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord GameTime__ResetFrameStep(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord util__MoveTime__Evaluate(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord util__MoveTime__GetRate(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord HeightList__Copy(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord HeightList__Transform(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord MultiSound__GetSoundItem(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord SoundPrivate__Initialize(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord SoundPrivate__GetHandleLoopSE(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord SoundPrivate__GetPreparedHandleSE(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldWorldImpl__UpdateAll(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FlagManagerNoName__Reset(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord GP__GetInt(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord map__Intermediate__Tutorial__IsActive(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord AABB__Transform(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord Font__GetCurrent(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord Stream__ReadString(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord Stream__WriteString(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord ColsTree__Clear(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord ColsTree__Entry(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord DLCSound__AddArchive(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord DLCSound__RemoveArchive(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord GameFont__Draw(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord MapRange__Reset(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord PolyList__Copy(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord PolyList__Transform(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord ProcInst__ProcInst_3(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord HeightMap__Remove(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord MovieData__Initialize(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord MovieData__Finalize(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldWorldImpl__CalcGeometryHit(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldData__IsLoaded(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord Proc__KillByName(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldActor__Setup(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldActor__Update(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldActor__Cleanup(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldSound__StopReverb(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldTrick__Free(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldTrick__Load(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldTrick__Update(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord HomeButton__HomeUnbind(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord HomeButton__HomeBind(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord ObjectBase__EntryHandle(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord ObjectBase__RemoveHandle(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldShadow__Draw(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldShadow__Update(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord ContentsUtil__IsNeedMount(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldWeather__Draw(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord SoundActor3D__StopAll(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord SoundPrivate__GetHandleSE(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord SoundPrivate__LoadGroupAsync(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord SoundPrivate__GetFreeHandleME(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord SoundPrivate__StopAll(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord SoundPrivate__Construct(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord SoundPrivate__LoadGroup(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord SoundPrivate__SoundPrivate_3(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord IndirectSound__Initialize(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord IndirectSound__Finalize(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord ObjectManager__Get(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord OwnHeightList__OwnHeightList(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldWorldImpl__ChangeLoad(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldWorldImpl__UpdateParam(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldWorldImpl__Free(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldWorldImpl__Load(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldWorldImpl__Tick(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldWorldImpl__Resume(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldWorldImpl__FreeScene(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldWorldImpl__Serialize(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord ProcDelaySound__DeleteAll(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord SoundHandleBGM__PlayPosition(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FlagManagerNoName__Deserialize(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FlagManagerNoName__Serialize(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord GameUserGlobalData__IsUsedJapaneseVoice(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord is__app__movie__MovieThread__MovieLeave(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord is__app__movie__MovieThread__MovieThreadInitialize(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord MTX__GetTranslate(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord map__SortiePosition__Initialize(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord map__SortiePosition__Finalize(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord map__Panel__Initialize(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord map__Panel__Finalize(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord map__sound__Initialize(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord map__sound__Setup(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord map__sound__Finalize(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord map__Binder__Initialize(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord map__Binder__Finalize(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord map__Sender__Initialize(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord map__Sender__Finalize(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord map__Gradation__Initialize(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord map__Gradation__Finalize(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord Font__PopFont(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord Font__GetLines(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord Font__GetWidth(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord Font__PushFont_2(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord Icon__Anime__DrawButtonA(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord Icon__Anime__DrawWait(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord Proc__FindByDesc(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord game__graphics__Window__DrawDialog(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord IActor__FreeAll(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord GfxUtil__GetModDeg(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord MapPose__Copy(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord MapPose__Round(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord DLCSound__StopAllVoice(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord DLCSound__SetBiquadFilter(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord DLCSound__SetPlayerVolumeSe(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord DLCSound__SetPlayerVolumeBGM(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord DLCSound__UpdateSoundSetting(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord DLCSound__SetPlayerVolumeVoice(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord DLCSound__GetData(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord DLCSound__IsVoicing(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord DLCSound__StopAllSe(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord Darkness__Start(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord GameFont__GetWidth(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord GameFont__GetHeight(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord GameTime__SetFrameSlow(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord MapRange__Add_2(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord MapRange__Add_3(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord HeightMap__Clear(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord PolyList__Round(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord ProcInst__Jump(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord ProcInst__Next(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldArea__Update(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldData__RemoveData(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldData__DeleteObject(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldData__Resume(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldData__Update(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldData__Suspend(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldData__EntryData(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldData__FreeParam(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord HeightMap__Entry(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord HeightMap__Update(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldData__FreeField(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord RandomSeed__Serialize(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord SoundHandle__IsEqual(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord GameConfigData__Serialize(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord AABB__IsIntersect(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord Color8__operator_3(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord MapPose__IsIdentity(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord MapPose__GetMatrixRT(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldData__FindObject_2(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord FieldData__FindObject_3(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord PartsData__GetModelName(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord PartsData__GetAccessAnim(ImplRuntime&, const ImplWord* args, std::size_t argc);
ImplWord ReferList__GetIndex(ImplRuntime&, const ImplWord* args, std::size_t argc);

} // namespace fates::coreimpl
