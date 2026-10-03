#pragma once
#include "fates/io/native_tex_files.hpp"
#include "fates/runtime/native_object_registry.hpp"
#include "fates/runtime/native_message_lookup.hpp"
#include "fates/runtime/native_talk_text.hpp"
#include <bitset>
#include "fates/presentation/native_talk_color.hpp"
#include "fates/presentation/native_talk_position.hpp"

namespace fates::presentation::native {
struct TalkWindowIdentity final {const std::uint32_t serial;};
using TalkWindowHandle=std::shared_ptr<const TalkWindowIdentity>;
struct TalkNameIdentity final {const std::uint64_t serial;};
using TalkNameHandle=std::shared_ptr<const TalkNameIdentity>;
struct TalkMemberIdentity {
    runtime::native::ObjectHandle handle;
    runtime::native::ObjectIdentity identity;
};
struct TalkStringStorage {
    TalkMemberIdentity movable;
    TalkVectorBits position{};
    std::optional<std::uint8_t> font;
    std::uint8_t display{},length{};
    std::array<char16_t,65> words{};
    std::bitset<65> known;
    TalkColorBytes color{};
};
struct TalkNamePlateStorage {
    TalkMemberIdentity movable;
    TalkVectorBits position{};
    TalkNameHandle name;
    std::shared_ptr<const std::vector<char16_t>> words;
    std::array<TalkMemberIdentity,2> color_members;
    std::array<TalkColorBytes,2> colors;
};
// Explicit already-initialized global colors. Unknown palette is not replaced
// by convenient colors or treated as a completed application initializer.
struct TalkWindowPalette {
    TalkColorBytes white,black,frame;
};
struct TalkWindowStorage {
    TalkWindowHandle identity;
    TalkMemberIdentity movable,offset,color_member;
    TalkVectorBits position{},offset_position{};
    io::native::FileBaseHandle texture;
    std::optional<std::uint8_t> talk_type,location;
    std::uint8_t state{},font{1};
    std::optional<std::array<std::uint8_t,3>> text_layout; // x25,y26,lineHeight27
    TalkColorBytes frame_color{},font_color{};
    std::array<TalkStringStorage,8> strings;
    std::uint8_t strings_visible{1};
    TalkNamePlateStorage name_plate;
    std::uint32_t current{},line{},x_offset{};
    std::uint8_t next_icon{255},new_string{1},active{},face_pending{},name_override{};
    std::optional<std::uint8_t> font_size; // Original +24, separate from font selector +23.
    std::optional<std::uint8_t> name_effect_enabled; // Original +5DC = FIRST BYTE of the face identifier.
    // Legacy name_effect_enabled is retained as the one authoritative first
    // byte, not duplicated in an independent flag or another name cache.
    // The remaining original31bytes (+5DD..+5FB) are constructor-uninitialized.
    std::array<std::uint8_t,31> face_identifier_tail{};
    std::bitset<31> face_identifier_known;
    // Existing face_pending is the legacy name of original +5D6 (key-wait byte),
    // constructor-known zero. Preserve that already-tested storage/API identity.
    std::optional<std::uint8_t> speaker_active{}; // Original+5D5; constructor untouched, distinct from open+5D3.
    std::optional<std::uint8_t> first_message; // Original +5D4, constructor untouched.
    // This component owns fresh, genuinely face-null windows. Face attachment,
    // effects, font selection/metrics and TalkManager execution are not admitted.
};
struct TalkWindowView {TalkWindowHandle window;std::uint8_t slot{};};
struct TalkNameView {TalkWindowHandle window;TalkNameHandle allocation;std::size_t offset{};};
struct TalkWindowSource {
    enum class Kind:std::uint8_t {Null,Words,Message,WindowString,Name,Expanded,UnitEdit};
    Kind kind{Kind::Null};
    std::shared_ptr<const std::vector<char16_t>> words;
    std::shared_ptr<runtime::native::NativeMessageLookup> messages;
    runtime::native::MessageLookupResult message;
    std::shared_ptr<const runtime::native::NativeTalkText> expanded;
    TalkWindowView string_view;
    TalkNameView name_view;
    std::shared_ptr<runtime::native::NativeUnitNames> unit_names;
    runtime::native::UnitEditNameReference edit_name_source{};
    std::size_t offset{};
    static TalkWindowSource Text(std::u16string_view);
    static TalkWindowSource Words(std::span<const char16_t>);
    static TalkWindowSource Message(std::shared_ptr<runtime::native::NativeMessageLookup>,runtime::native::MessageLookupResult);
};
enum class TalkWindowStatus:std::uint8_t {
    Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,InvalidHandle,
    PaletteUnavailable,LayoutUnavailable,InvalidSource,StaleSource,UnknownWord,
    StorageOverflow,IdentityExhausted,Unavailable
};
struct TalkWindowRead {TalkWindowStatus status;char16_t value{};};
// Actual window constructor's storage/handles and text/name operations, with
// no-attached-face Reset and original non-deleting destructor. Reuses the shared
// ObjectHandleRegistry and concrete TexFile; no second registry/cache/parser.
class NativeTalkWindow final:public runtime::native::ProcessCallbacks,public TalkPositionOwner,public TalkColorOwner {
public:
    static TalkWindowStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<runtime::native::ObjectHandleRegistry>,
        std::shared_ptr<io::native::NativeFileController>,std::shared_ptr<io::native::NativeFileBase>,
        std::shared_ptr<io::native::NativeTexFiles>,std::shared_ptr<NativeTalkWindow>&);
    ~NativeTalkWindow();
    NativeTalkWindow(const NativeTalkWindow&)=delete;
    NativeTalkWindow& operator=(const NativeTalkWindow&)=delete;
    TalkWindowStatus PublishPalette(std::optional<TalkWindowPalette>,runtime::native::ProcessAccess* =nullptr);
    TalkWindowStatus Construct(TalkWindowHandle&,runtime::native::ProcessAccess* =nullptr);
    TalkWindowStatus Reset(TalkWindowHandle,runtime::native::ProcessAccess* =nullptr);
    // Initialize / InitializeInSkip when the supplied FID pointer is truly null.
    // Current window owner admits actual constructor-null attached faces only.
    // Retains name allocation/tail bytes; skipped init alone sets the open byte.
    TalkWindowStatus InitializeWithoutFace(TalkWindowHandle,std::int32_t,std::uint32_t,bool,runtime::native::ProcessAccess&);

    TalkWindowStatus ResetSystem(TalkWindowHandle,runtime::native::ProcessAccess* =nullptr);
    TalkWindowStatus ResetStrings(TalkWindowHandle,runtime::native::ProcessAccess* =nullptr);
    TalkWindowStatus NextString(TalkWindowHandle,runtime::native::ProcessAccess* =nullptr);
    TalkWindowStatus NextLine(TalkWindowHandle,runtime::native::ProcessAccess* =nullptr);
    TalkWindowStatus AddLetter(TalkWindowHandle,const TalkWindowSource&,runtime::native::ProcessAccess* =nullptr);
    TalkWindowStatus SetName(TalkWindowHandle,const TalkWindowSource&,bool explicit_override=true,runtime::native::ProcessAccess* =nullptr);
    // Carried data writes for the still-unowned StartOpen/width/control routines.
    // These do NOT execute or claim those higher operations.
    TalkWindowStatus RestoreTextLayout(TalkWindowHandle,std::optional<std::array<std::uint8_t,3>>,runtime::native::ProcessAccess* =nullptr);
    TalkWindowStatus RestoreCursor(TalkWindowHandle,std::uint32_t current,std::uint32_t line,std::uint32_t x_offset,std::uint8_t new_string,runtime::native::ProcessAccess* =nullptr);
    TalkWindowStatus RestoreTypeLocation(TalkWindowHandle,std::optional<std::uint8_t>,std::optional<std::uint8_t>,runtime::native::ProcessAccess* =nullptr);
    TalkWindowStatus RestoreColors(TalkWindowHandle,TalkColorBytes frame,TalkColorBytes font,runtime::native::ProcessAccess* =nullptr);
    TalkWindowStatus RestoreString(TalkWindowView,const TalkStringStorage&,runtime::native::ProcessAccess* =nullptr);
    std::optional<std::uint32_t> Side(TalkWindowHandle) const;
    std::optional<TalkWindowStorage> Observe(TalkWindowHandle) const;
    std::vector<TalkWindowHandle> Handles() const;
    std::optional<TalkStringStorage> ObserveString(TalkWindowView) const;
    std::optional<TalkNameView> NameView(TalkWindowHandle) const;
    TalkWindowRead Read(const TalkWindowSource&,std::size_t index=0) const;
    std::optional<runtime::native::ProcessCall> DestroyCall(runtime::native::ProcessHandle,TalkWindowHandle) const;
    bool UsesObjectRegistry(const runtime::native::ObjectHandleRegistry&) const noexcept override;
    bool UsesScheduler(const runtime::native::NativeProcessScheduler&) const noexcept override;
    std::optional<TalkColorBytes> ReadColor(runtime::native::ObjectIdentity) const override;
    bool WriteColorChannel(runtime::native::ObjectIdentity,std::uint8_t,std::uint8_t,runtime::native::ProcessAccess&) override;
    std::optional<TalkVectorBits> ReadPosition(runtime::native::ObjectIdentity) const override;
    bool WritePosition(runtime::native::ObjectIdentity,TalkVectorBits,runtime::native::ProcessAccess&) override;
    // Two exact portions of NextPage, separated by real child construction.
    // They cannot themselves acknowledge or finish a blocking scroll child.
    std::optional<TalkWindowPalette> ObservePalette() const;
    // Exact original effect write prefixes, not host snapshots or completion flags.
    // Exact ChangeFontColor prefix and width accumulation, not font selection or layout synthesis.
    TalkWindowStatus WriteFontColor(TalkWindowHandle,TalkColorBytes,runtime::native::ProcessAccess&);
    TalkWindowStatus AddMeasuredXOffset(TalkWindowHandle,std::uint32_t,runtime::native::ProcessAccess&);
    TalkWindowStatus StartOpeningState(TalkWindowHandle,runtime::native::ProcessAccess&);
    TalkWindowStatus RestoreSpeakerActive(TalkWindowHandle,std::optional<std::uint8_t>,runtime::native::ProcessAccess* =nullptr);
    TalkWindowStatus ClearFaceState(TalkWindowHandle,bool clear_open,runtime::native::ProcessAccess&);
    TalkWindowStatus ClearNextIcon(TalkWindowHandle,runtime::native::ProcessAccess&); // $Wv writes0, HideNextIcon writes255.
    TalkWindowStatus HideNextIcon(TalkWindowHandle,runtime::native::ProcessAccess&);
    // Legacy next_icon is original+5D0, the signed shout index. Outer Draw
    // writes this same byte after its reached delta calculation.
    TalkWindowStatus WriteShoutIndex(TalkWindowHandle,std::uint8_t,runtime::native::ProcessAccess&);
    TalkWindowStatus CloseEffectState(TalkWindowHandle,bool skip,runtime::native::ProcessAccess&);
    // Explicit carried face-identifier storage, not FaceInstance attachment or
    // a claim to execute FadeInFace. All32 original bytes and knownness retained.
    TalkWindowStatus RestoreFaceIdentifier(TalkWindowHandle,std::span<const std::uint8_t>,std::bitset<32>,runtime::native::ProcessAccess* =nullptr);
    TalkWindowStatus RestoreStringsVisible(TalkWindowHandle,std::uint8_t,runtime::native::ProcessAccess* =nullptr);
    TalkWindowStatus RestoreNameEffectEnabled(TalkWindowHandle,std::optional<std::uint8_t>,runtime::native::ProcessAccess* =nullptr);
    TalkWindowStatus RestoreEffectFlags(TalkWindowHandle,std::uint8_t active,std::optional<std::uint8_t> first_message,std::uint8_t key_wait,runtime::native::ProcessAccess* =nullptr);
    TalkWindowStatus SetStringAlpha(TalkWindowView,std::uint8_t,runtime::native::ProcessAccess&);
    TalkWindowStatus StartPageState(TalkWindowHandle,runtime::native::ProcessAccess&);
    TalkWindowStatus FinishPageState(TalkWindowHandle,runtime::native::ProcessAccess&);
    TalkWindowStatus ScrollString(TalkWindowView,std::uint32_t elapsed,std::uint32_t duration,std::uint8_t fade,runtime::native::ProcessAccess&);
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    struct State;struct Continuation;
    explicit NativeTalkWindow(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
