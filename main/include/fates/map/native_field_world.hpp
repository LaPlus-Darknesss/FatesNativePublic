#pragma once
#include "fates/map/native_field_scene.hpp"
#include "fates/map/native_field_sources.hpp"

namespace fates::map::native {
enum class FieldWorldServiceKind {
    DetachTrick,FreeTrick,DestroyPlant,ClearGeometryTree,ClearCollisionTree,
    DeleteEffectGroup,DestroySky,FreeScene,StopReverb,StartReverb,FreeWeather,
    InvalidateArea,ResetOverrides,LoadSceneResource,LoadFallbackScene,UpdateColor,
    CreateSky,SelectSky,AttachSky,LoadWeather,SetWeather,UpdateParameters,
    UpdateArea,UpdateTrick,CreatePlant,SelectPlant,AttachPlant,LoadTrick,
    AttachTrick,SelectTrick
};
struct FieldTimeState {float elapsed{},duration{},start{12.0f},target{12.0f},current{12.0f};};
struct FieldWorldService {
    FieldWorldServiceKind kind;
    std::optional<std::string> name;
    int value{};
    HeightMapRange range;
    FieldResourceRef resource;
    std::shared_ptr<const FieldConfiguration> configuration;
    FieldTimeState time;
};
// Initialized platform/scene context supplied by the host. The original outer
// world constructor's renderer/camera/shadow allocations remain a boundary.
// Calls are synchronous, nonthrowing and non-reentrant. Perform owns only the
// named service, never World load/update ordering or numeric height population.
class FieldWorldBackend : public FieldConfigurationBackend {
public:
    virtual std::shared_ptr<const FieldSourceArchive> ReadFieldSource(std::string_view)=0;
    virtual std::shared_ptr<FieldObjectBackend> CreateObjectBackend(fates::runtime::native::ObjectIdentity)=0;
    virtual FieldResourceRead ReadSceneResource(std::string_view path)=0; // TryImmediate 2
    virtual void ReleaseSceneResource(FieldResourceRef&)=0;
    virtual void Perform(const FieldWorldService&)=0;
    virtual void ObserveSource(const FieldSourceEvent&) {} // diagnostic call-entry observer
};
// Owns live FieldData membership and the load/free/update control of the current
// field. Runtime, shared registry and backend outlive calls. Tick/camera/animation
// progression, FieldArea/Trick internals and UpdateParam mathematics remain named
// services; this is not a claim that the whole FieldWorld class is complete.
class NativeFieldWorld final : private FieldSourceObserver {
public:
    NativeFieldWorld(fates::runtime::native::NativeRuntime&,std::shared_ptr<FieldWorldBackend>,
        std::shared_ptr<fates::runtime::native::ObjectHandleRegistry>);
    ~NativeFieldWorld();
    NativeFieldWorld(const NativeFieldWorld&)=delete;
    NativeFieldWorld& operator=(const NativeFieldWorld&)=delete;
    bool Load(const char* field_name); // null returns false without Free
    bool LoadChapter(); // immutable Chapter.battlefield; no chapter-specific paths
    void Free();
    bool LoadScene(const char*);
    void FreeScene();
    bool LoadPlant(const char*); // always true in the admitted allocation domain
    bool Update();
    void UpdateAll();
    bool EntryData(std::string_view);
    FieldObjectKey CreateObject(const FieldReferData&);
    FieldObjectKey CreateObject(const char* part_name);
    bool DeleteObject(const FieldObjectKey&);
    FieldObjectKey FindObject(const char* name) const; // unique disposed hash-pair match
    FieldObjectKey GetObject(fates::runtime::native::ObjectHandle) const;
    const FieldObjectInstance* ReadObject(const FieldObjectKey&) const;
    bool SetState(const FieldObjectKey&,int);
    bool SetLevel(const FieldObjectKey&,int);
    bool SetPose(const FieldObjectKey&,const FieldPose&);
    bool SetDispos(const FieldObjectKey&,bool);
    bool SetEscape(const FieldObjectKey&,bool);
    bool SetVisible(const FieldObjectKey&,bool);
    void SetKeepTime(bool value) noexcept {keep_time_=value;}
    const FieldTimeState& Time() const noexcept {return time_;}
    const FieldSourceRegistry& Sources() const noexcept {return sources_;}
    const HeightMapGeometry& HeightGeometry() const noexcept {return height_;}
    HeightMapRange PendingRange() const noexcept {return *range_;}
    bool Pending() const noexcept {return pending_;}
    FieldSceneSnapshot Snapshot() const;
    // Refuses displaced/chapter-stale owners and unsettled geometry. Publication
    // carries the exact current height to existing map/actor-position consumers.
    bool PublishCurrent();
private:
    struct Node;
    Node* FindNode(const FieldObjectKey&) const;
    FieldObjectKey Key(const Node&) const;
    void OnSourceEvent(const FieldSourceEvent&) override;
    void OnPlacementInserted(const std::shared_ptr<const FieldSourcePlacement>&) override;
    void Touch(bool invalidate_height);
    void ClearField();
    void ClearGeometry();
    void Parameters();
    void Service(FieldWorldServiceKind,std::optional<std::string> name={},int value=0);
    FieldSourceProvider Provider();
    fates::runtime::native::NativeRuntime& runtime_;
    std::shared_ptr<FieldWorldBackend> backend_;
    std::shared_ptr<fates::runtime::native::ObjectHandleRegistry> registry_;
    std::shared_ptr<const FieldWorldIdentity> identity_{std::make_shared<FieldWorldIdentity>()};
    FieldConfigurationOwner configuration_;
    FieldSourceRegistry sources_;
    std::vector<std::unique_ptr<Node>> objects_;
    std::unique_ptr<Node> constructing_;
    std::shared_ptr<HeightMapRange> range_{std::make_shared<HeightMapRange>()};
    HeightMapGeometry height_;
    std::uint64_t revision_{},terrain_revision_{};
    std::uint8_t chapter_{};
    bool map_active_{},pending_{},sky_{},plant_{},keep_time_{},loaded_{};
    std::array<int,4> scene_selectors_{-1,-1,-1,-1};
    FieldTimeState time_;
    std::string requested_;
    std::optional<std::string> field_name_,environment_name_,scene_resource_path_;
};
}
