#pragma once
#include "fates/map/native_field_geometry.hpp"
#include "fates/runtime/native_identifier.hpp"
#include <memory>
#include <string_view>
#include <functional>

namespace fates::map::native {
using FieldBounds=std::array<float,6>;
FieldBounds EmptyFieldBounds() noexcept;
bool IsEmptyFieldBounds(const FieldBounds&) noexcept;
// PROVEN: model name uses its argument directly. Idle/access names remap it
// through model_indices, even when the caller has already mapped the level.
std::string FieldModelName(const FieldHeightPart&,int model_index);
std::string FieldAnimationName(const FieldHeightPart&,int model_index,int state,bool access);
std::string FieldResourcePath(std::string_view container,bool texture);
unsigned FieldTransferFlags(int mode) noexcept;
int DefaultFieldLevel(unsigned selector); // admitted retail selectors 0 and 1

struct FieldResource {virtual ~FieldResource()=default;};
using FieldResourceRef=std::shared_ptr<FieldResource>;
struct FieldResourceRead {FieldResourceRef resource;bool ready{};};
// Backend identity, including its generation. Zero represents an original
// handle with zero generation; this is distinct from querying whether it lives.
struct FieldEffectHandle {std::uint64_t value{};explicit operator bool() const{return value!=0;}bool operator==(const FieldEffectHandle&) const=default;};

// Opaque effect-manager operations shared with the friendly FieldEffectNode.
class FieldEffectBackend {
public:
    virtual ~FieldEffectBackend()=default;
    virtual FieldEffectHandle CreateEffect(const std::optional<std::string>& label)=0;
    virtual std::optional<std::string> SecondaryEffectLabel(FieldEffectHandle)=0;
    virtual void DeleteEffect(FieldEffectHandle)=0;
    virtual void SetEffectGroup(FieldEffectHandle,unsigned)=0;
    virtual void SetEffectVisible(FieldEffectHandle,bool)=0;
    virtual void SetEffectStep(FieldEffectHandle,float)=0;
    virtual void SetEffectLocation(FieldEffectHandle,unsigned)=0;
    virtual void SetEffectTransform(FieldEffectHandle,const FieldMatrix&)=0;
};

// Synchronous, non-reentrant per-object backend. No method may call back into
// its owner or throw. Resources retain backend ownership; actors are the four fixed
// members, not four selected LOD resources. Allocation success is admitted.
// ReadResource wraps the already reconstructed FileBase Open/Entry/Close
// protocol with TryImmediate (2). A failed read may still retain a binding.
// OPAQUE: BCH/model internals, scene graph, animation/RNG, effect manager and
// collision-tree storage. All Fates ordering above those services is native.
class FieldObjectBackend : public FieldEffectBackend {
public:
    virtual ~FieldObjectBackend()=default;
    virtual unsigned DefaultLevelSelector() const=0;
    virtual int TransferMode(bool texture)=0;
    virtual FieldResourceRead ReadResource(std::string_view path,unsigned flags)=0;
    virtual void ReleaseResource(FieldResourceRef&,bool texture)=0; // also accepts empty
    virtual void LinkResources(const FieldResourceRef&,const FieldResourceRef&)=0;
    virtual void ConstructActor(unsigned)=0;
    virtual void DestroyActor(unsigned)=0;
    virtual bool LoadModel(unsigned,const FieldResourceRef&,std::string_view)=0;
    virtual void SetupActor(unsigned,std::int16_t raw_selector)=0;
    virtual void CleanupActor(unsigned)=0;
    virtual void FreeActor(unsigned)=0;
    virtual void AttachActor(unsigned)=0;
    virtual void DetachActor(unsigned)=0;
    virtual void SetActorModelFlags(unsigned,std::uint16_t bits)=0;
    virtual void ResetActorLocalBounds(unsigned)=0;
    virtual FieldMatrix ActorModelMatrix(unsigned) const=0;
    virtual FieldBounds ActorLocalBounds(unsigned) const=0;
    virtual void SetActorTransform(unsigned,const FieldMatrix&)=0; // includes dirty 0x40
    virtual void SetActorVisible(unsigned,bool)=0;
    virtual void SetAnimationMode(unsigned,std::uint8_t)=0;
    virtual void StopAnimations(unsigned)=0;
    virtual void ResetAnimations(unsigned)=0;
    virtual bool HasAnimation(const FieldResourceRef&,std::string_view)=0;
    virtual bool TryPlayAnimations(unsigned,const FieldResourceRef&,std::string_view)=0;
    virtual void SetAnimationLoop(unsigned,bool)=0;
    virtual void RandomizeAnimation(unsigned)=0; // original RandomAnim RNG boundary
    virtual void SetAnimationStep(unsigned,float)=0;
    virtual void SetAnimationToEnd(unsigned)=0;
    virtual void UpdateActor(unsigned)=0;
    virtual bool HasEffectDefinition(const std::optional<std::string>& label)=0;
    virtual void MarkWorldPending()=0;
    // Act only on the family named by the event. The immutable binding snapshot
    // is supplied explicitly; observing intermediate owner mutations is forbidden.
    virtual void GeometryEvent(FieldGeometryEvent,const FieldGeometrySnapshot&)=0;
};

struct FieldEffectState {
    FieldPose pose;
    int level{-1};
    std::array<FieldEffectHandle,2> handles;
};
// Native FieldEffectNode policy. Empty handles still receive unconditional
// Delete/Group/Location calls. Transform tests generation, not IsAlive.
void LoadFieldEffect(FieldEffectState&,FieldEffectBackend&,const FieldEffectPlacement&);
void SetFieldEffectLevel(FieldEffectState&,FieldEffectBackend&,int);
bool TransformFieldEffect(const FieldEffectState&,FieldEffectBackend&,const FieldMatrix&);
void FreeFieldEffect(FieldEffectState&,FieldEffectBackend&);

struct FieldActorState {
    // Owner-issued requests/results. Initial visibility, selector, animation
    // state and other model flags remain backend-owned; absent means unwritten.
    std::optional<std::string> model_name;
    bool model_loaded{},attached{};
    std::optional<bool> visibility_request;
    std::optional<std::int16_t> selector_request;
    std::uint16_t added_model_flags{};
    std::optional<std::uint8_t> animation_mode_request;
    FieldMatrix world_matrix;
};
struct FieldObjectState {
    std::optional<std::string> display_name; // absent preserves null-name distinction
    std::uint32_t name_hash{},bucket{}; // full input, not the truncated display name
    FieldPose pose;
    int state{},level{-1};
    std::uint16_t flags{};
    FieldBounds part_bounds{EmptyFieldBounds()},actor_bounds{EmptyFieldBounds()};
    FieldResourceRef primary,texture;
    std::array<FieldActorState,4> actors;
    std::vector<FieldEffectState> effects;
    std::int8_t access_state{-1},access_level{-1};
    std::uint64_t access_handle{};
    std::uint32_t access_progress{};
};
// Owns mutable FieldObject policy and geometry. It neither chooses the current
// chapter nor registers itself in the field-source registry. Those memberships
// belong to the world owner. Backend and pending range outlive all member calls.
class FieldObjectInstance {
public:
    // Admission: type 0..6, model indices -1..3, finite arithmetic, ASCII part
    // identifier for the currently validated SDK lowercase domain. Display names
    // retain all original byte values. Invalid inputs throw before actor creation.
    FieldObjectInstance(std::shared_ptr<const FieldHeightPart>,std::optional<std::string_view> name,
        const FieldPose&,std::shared_ptr<FieldObjectBackend>,std::shared_ptr<HeightMapRange>,
        std::function<void()> world_pending={});
    ~FieldObjectInstance();
    FieldObjectInstance(const FieldObjectInstance&)=delete;
    FieldObjectInstance& operator=(const FieldObjectInstance&)=delete;
    // Borrowed state, valid until mutation. Effect/backend handles do not become
    // independently live resources by copying this view. World revisions and
    // instance generations are supplied by the enclosing world, not this class.
    const FieldObjectState& State() const noexcept{return state_;}
    const FieldGeometryInstance& Geometry() const noexcept{return geometry_;}
    const std::shared_ptr<const FieldHeightPart>& Part() const noexcept{return part_;}
    void Load();
    void Free();
    void SetState(int); // 0..2
    void SetLevel(int); // -1 resolves current global selector; otherwise 0..3
    void SetPose(const FieldPose&);
    void SetTranslate(const std::array<float,3>&);
    void SetDispos(bool);
    void SetEscape(bool);
    void SetVisible(bool);
    void UpdateTransform();
    FieldGeometryUpdate UpdateDispos();
private:
    void MarkDirty();
    void ResetAccess() noexcept;
    void ClearEffects();
    void UpdateEffects(int);
    FieldMatrix ComposedMatrix(const FieldPose&,int level) const;
    void ValidatePose(const FieldPose&,int level) const;
    void SetFlag(std::uint16_t,bool);
    std::shared_ptr<const FieldHeightPart> part_;
    std::shared_ptr<FieldObjectBackend> backend_;
    std::shared_ptr<HeightMapRange> pending_;
    FieldGeometryInstance geometry_;
    FieldObjectState state_;
    std::function<void()> world_pending_;
};
}
