#include "fates/map/native_field_world.hpp"
#include "fates/runtime/native_runtime.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace fates::map::native {
namespace rn=fates::runtime::native;
namespace {
bool ValidRange(const HeightMapRange& r) {return r.min_x<=r.max_x&&r.min_y<=r.max_y;}
void Advance(std::uint64_t& value) {if(value==UINT64_MAX)throw std::overflow_error("Field revision exhausted");++value;}
FieldWorldBackend& Require(const std::shared_ptr<FieldWorldBackend>& backend,const std::shared_ptr<rn::ObjectHandleRegistry>& registry) {
    if(!backend||!registry)throw std::invalid_argument("Field world requires backend and object registry");return *backend;
}
}
struct NativeFieldWorld::Node {
    std::shared_ptr<rn::ObjectHandleRegistry> registry;
    rn::ObjectIdentity identity;
    rn::ObjectHandle handle;
    std::shared_ptr<const FieldSourcePlacement> placement;
    std::unique_ptr<FieldObjectInstance> object;
    ~Node(){object.reset();registry->Remove(handle);}
};
NativeFieldWorld::NativeFieldWorld(rn::NativeRuntime& runtime,std::shared_ptr<FieldWorldBackend> backend,
    std::shared_ptr<rn::ObjectHandleRegistry> registry):runtime_(runtime),backend_(std::move(backend)),registry_(std::move(registry)),configuration_(Require(backend_,registry_)) {
    chapter_=runtime_.game.campaign.current_chapter_index;map_active_=runtime_.game.map_active;
    runtime_.game.field_scene={identity_,0,0,{}};InvalidateCurrentHeightGeometry(runtime_.game);
}
NativeFieldWorld::~NativeFieldWorld() {
    Free();
    if(runtime_.game.field_scene.owner==identity_)runtime_.game.field_scene={};
}
void NativeFieldWorld::Touch(bool height) {
    Advance(revision_);
    if(runtime_.game.field_scene.owner==identity_) {
        auto& current=runtime_.game.field_scene;current.revision=revision_;current.terrain_revision=terrain_revision_;current.snapshot.reset();
        if(height)InvalidateCurrentHeightGeometry(runtime_.game);
    }
}
void NativeFieldWorld::Service(FieldWorldServiceKind kind,std::optional<std::string> name,int value) {
    backend_->Perform({kind,std::move(name),value,*range_,{},configuration_.Configuration(),time_});
}
void NativeFieldWorld::Parameters(){Service(FieldWorldServiceKind::UpdateParameters);}
FieldSourceProvider NativeFieldWorld::Provider(){return [this](std::string_view name){return backend_->ReadFieldSource(name);};}
FieldObjectKey NativeFieldWorld::Key(const Node& node) const {return {identity_,node.identity};}
NativeFieldWorld::Node* NativeFieldWorld::FindNode(const FieldObjectKey& key) const {
    if(key.world!=identity_||!key.instance)return nullptr;
    for(const auto& node:objects_)if(node->identity==key.instance)return node.get();return nullptr;
}
void NativeFieldWorld::OnSourceEvent(const FieldSourceEvent& event) {
    using Kind=FieldSourceEventKind;
    if(event.kind==Kind::DestroyObject) {
        const auto it=std::find_if(objects_.begin(),objects_.end(),[&](const auto& node){return node->placement==event.placement;});
        if(it==objects_.end())throw std::logic_error("Field source/live membership mismatch");
        auto removed=std::move(*it);objects_.erase(it);Touch(true);
        backend_->ObserveSource(event);removed.reset();return;
    }
    backend_->ObserveSource(event);
    if(event.kind==Kind::ConstructAndLoadObject) {
        if(constructing_)throw std::logic_error("Reentrant field construction");
        auto node=std::make_unique<Node>();node->registry=registry_;node->identity=registry_->NewIdentity();node->placement=event.placement;
        // Original ObjectBase registration precedes actor member construction.
        registry_->Entry(node->handle,node->identity);
        auto object_backend=backend_->CreateObjectBackend(node->identity);
        node->object=std::make_unique<FieldObjectInstance>(event.placement->source.part,event.placement->name,event.placement->pose,
            std::move(object_backend),range_,[this]{pending_=true;Touch(true);});
        node->object->Load();constructing_=std::move(node);
    } else if(event.kind==Kind::SetDisposition) {
        const auto it=std::find_if(objects_.begin(),objects_.end(),[&](const auto& node){return node->placement==event.placement;});
        if(it==objects_.end())throw std::logic_error("Disposition before field membership");
        (*it)->object->SetDispos(true);
    }
}
void NativeFieldWorld::OnPlacementInserted(const std::shared_ptr<const FieldSourcePlacement>& placement) {
    if(!constructing_||constructing_->placement!=placement)throw std::logic_error("Field insertion without constructed object");
    objects_.push_back(std::move(constructing_));Touch(true);
}
void NativeFieldWorld::ClearField(){sources_.FreeField(this);field_name_.reset();loaded_=false;Touch(true);}
void NativeFieldWorld::ClearGeometry() {
    ClearHeightGeometryExact(height_);Advance(terrain_revision_);Touch(true);
    Service(FieldWorldServiceKind::ClearGeometryTree);Service(FieldWorldServiceKind::ClearCollisionTree);
    Service(FieldWorldServiceKind::DeleteEffectGroup,{},1);
}
void NativeFieldWorld::FreeScene() {
    Service(FieldWorldServiceKind::FreeScene);scene_selectors_.fill(-1);environment_name_.reset();scene_resource_path_.reset();Touch(false);
}
bool NativeFieldWorld::LoadScene(const char* name) {
    // Retain an input alias before FreeScene drops the old descriptor.
    const std::optional<std::string> requested=name?std::optional<std::string>(name):std::nullopt;
    FreeScene();if(!requested)return false;
    auto path="scene/"+*requested+".bch.lz";path.resize(std::min<std::size_t>(path.size(),63));
    auto read=backend_->ReadSceneResource(path);
    if(read.ready)backend_->Perform({FieldWorldServiceKind::LoadSceneResource,{},0,*range_,read.resource,configuration_.Configuration(),time_});
    else Service(FieldWorldServiceKind::LoadFallbackScene,"scene/Dummy.bch.lz");
    Service(FieldWorldServiceKind::UpdateColor);backend_->ReleaseSceneResource(read.resource);
    environment_name_=*requested;scene_resource_path_=read.ready?path:"scene/Dummy.bch.lz";Touch(false);return true;
}
bool NativeFieldWorld::LoadPlant(const char* name) {
    const std::optional<std::string> requested=name?std::optional<std::string>(name):std::nullopt;
    if(plant_){Service(FieldWorldServiceKind::DestroyPlant);plant_=false;}
    if(requested) {
        Service(FieldWorldServiceKind::CreatePlant,*requested);plant_=true;
        Service(FieldWorldServiceKind::SelectPlant,{},-9);Service(FieldWorldServiceKind::AttachPlant);Parameters();
    }
    Touch(false);return true;
}
void NativeFieldWorld::Free() {
    Service(FieldWorldServiceKind::DetachTrick);Service(FieldWorldServiceKind::FreeTrick);
    if(plant_){Service(FieldWorldServiceKind::DestroyPlant);plant_=false;}
    ClearField();ClearGeometry();
    if(sky_){Service(FieldWorldServiceKind::DestroySky);sky_=false;}
    FreeScene();Service(FieldWorldServiceKind::StopReverb);configuration_.Free();Service(FieldWorldServiceKind::FreeWeather);
    *range_={};pending_=false;Touch(true);
}
bool NativeFieldWorld::Load(const char* input) {
    if(!input)return false;
    const std::string request(input);Free();
    chapter_=runtime_.game.campaign.current_chapter_index;map_active_=runtime_.game.map_active;
    requested_=request.substr(0,63);Service(FieldWorldServiceKind::StopReverb);configuration_.Free();
    const bool requested_exists=configuration_.Load(request);Service(FieldWorldServiceKind::StartReverb);
    Service(FieldWorldServiceKind::InvalidateArea);Service(FieldWorldServiceKind::ResetOverrides);
    auto config=configuration_.Configuration();
    // The requested-exists/read-failed combination dereferences null in retail.
    // Native refusal names that domain; do not silently take the missing-file path.
    if(requested_exists&&!config)throw std::invalid_argument("Existing field configuration has no decoded record");
    std::optional<std::string> chosen=requested_exists?config->field_name:std::optional<std::string>(request);
    if(!config)return false;
    if(!LoadScene(config->environment_name?config->environment_name->c_str():nullptr))return false;
    config=configuration_.Configuration();if(!config)return false;
    if(sky_){Service(FieldWorldServiceKind::DestroySky);sky_=false;}
    if(config->sky_name) {
        Service(FieldWorldServiceKind::CreateSky,*config->sky_name);sky_=true;
        Service(FieldWorldServiceKind::SelectSky,{},-13);Service(FieldWorldServiceKind::AttachSky);
    }
    Service(FieldWorldServiceKind::LoadWeather);Service(FieldWorldServiceKind::SetWeather,{},config->Weather());
    if(!keep_time_)time_.current=config->TimeZone();
    if(!std::isfinite(time_.current))throw std::invalid_argument("Nonfinite field time");
    time_.start=time_.target=time_.current;time_.elapsed=time_.duration=0.0f;Parameters();
    ClearField();ClearGeometry();
    if(!chosen)throw std::invalid_argument("Null field name in readable configuration");
    field_name_=*chosen;const bool success=sources_.LoadField(*chosen,Provider(),this).loaded;
    if(success) {
        Parameters();UpdateAll();LoadPlant(chosen->c_str());Service(FieldWorldServiceKind::LoadTrick);
        Service(FieldWorldServiceKind::AttachTrick);Service(FieldWorldServiceKind::SelectTrick,{},-7);loaded_=true;
    }
    Touch(false);return success;
}
bool NativeFieldWorld::LoadChapter() {
    const auto* chapter=runtime_.definitions.FindChapter(runtime_.game.campaign.current_chapter_index);
    if(!chapter)return false;return Load(chapter->battlefield_present?chapter->battlefield.c_str():nullptr);
}
bool NativeFieldWorld::Update() {
    if(!pending_&&!ValidRange(*range_))return false;
    if(pending_) {
        for(const auto& node:objects_)node->object->UpdateDispos();
        Service(FieldWorldServiceKind::UpdateArea);pending_=false;
    }
    if(ValidRange(*range_)) {
        std::vector<WorldHeightContribution> contributions;contributions.reserve(objects_.size());
        for(const auto& node:objects_)contributions.push_back({node->object->State().flags,node->object->Geometry().Active().height.list.get()});
        const auto status=UpdateHeightPopulationExact(height_,*range_,contributions);
        if(status!=HeightPopulationStatus::Ok)throw std::invalid_argument("Unadmitted field height population");
        Advance(terrain_revision_);Service(FieldWorldServiceKind::UpdateTrick);*range_={};
    }
    Touch(true);return true;
}
void NativeFieldWorld::UpdateAll(){*range_={0,0,31,31};pending_=true;Update();}
bool NativeFieldWorld::EntryData(std::string_view name) {
    std::vector<FieldSourceEvent> events;const bool result=sources_.EntryData(name,Provider(),events,this);Touch(false);return result;
}
FieldObjectKey NativeFieldWorld::CreateObject(const FieldReferData& ref) {
    std::vector<FieldSourceEvent> events;const auto p=sources_.CreateObject(ref,events,this);
    return p?Key(*objects_.back()):FieldObjectKey{};
}
FieldObjectKey NativeFieldWorld::CreateObject(const char* name) {
    std::vector<FieldSourceEvent> events;const auto p=sources_.CreateObject(name,Provider(),events,this);
    return p?Key(*objects_.back()):FieldObjectKey{};
}
bool NativeFieldWorld::DeleteObject(const FieldObjectKey& key) {
    const auto* node=FindNode(key);if(!node)return false;
    // Copy before the synchronous destroy callback releases the node itself.
    const auto placement=node->placement;std::vector<FieldSourceEvent> events;return sources_.DeleteObject(placement,events,this);
}
FieldObjectKey NativeFieldWorld::FindObject(const char* name) const {
    if(!name)return {}; // native null-input admission, retail dereferences
    const auto hash=rn::HashIdentifierExact(name);const Node* found=nullptr;
    for(auto it=objects_.rbegin();it!=objects_.rend();++it) {
        const auto& state=(*it)->object->State();
        if((state.flags&8)&&state.name_hash==hash.nameHash&&state.bucket==hash.bucketHash%127) {
            if(found)return {};found=it->get();
        }
    }
    return found?Key(*found):FieldObjectKey{};
}
FieldObjectKey NativeFieldWorld::GetObject(rn::ObjectHandle handle) const {
    const auto identity=registry_->Get(handle);if(!identity)return {};
    const FieldObjectKey key{identity_,identity};return FindNode(key)?key:FieldObjectKey{};
}
const FieldObjectInstance* NativeFieldWorld::ReadObject(const FieldObjectKey& key) const {
    const auto* node=FindNode(key);return node?node->object.get():nullptr;
}
bool NativeFieldWorld::SetState(const FieldObjectKey& key,int value){auto* node=FindNode(key);if(!node)return false;node->object->SetState(value);return true;}
bool NativeFieldWorld::SetLevel(const FieldObjectKey& key,int value){auto* node=FindNode(key);if(!node)return false;const int before=node->object->State().level;node->object->SetLevel(value);if(before!=node->object->State().level)Touch(false);return true;}
bool NativeFieldWorld::SetPose(const FieldObjectKey& key,const FieldPose& value){auto* node=FindNode(key);if(!node)return false;node->object->SetPose(value);return true;}
bool NativeFieldWorld::SetDispos(const FieldObjectKey& key,bool value){auto* node=FindNode(key);if(!node)return false;node->object->SetDispos(value);return true;}
bool NativeFieldWorld::SetEscape(const FieldObjectKey& key,bool value){auto* node=FindNode(key);if(!node)return false;node->object->SetEscape(value);return true;}
bool NativeFieldWorld::SetVisible(const FieldObjectKey& key,bool value){auto* node=FindNode(key);if(!node)return false;node->object->SetVisible(value);Touch(false);return true;}
FieldSceneSnapshot NativeFieldWorld::Snapshot() const {
    FieldSceneSnapshot result{identity_,revision_,terrain_revision_,chapter_,map_active_,loaded_,pending_||ValidRange(*range_),requested_,field_name_,environment_name_,scene_resource_path_,configuration_.Configuration()};
    for(const auto& resource:sources_.Resources())result.source_resources.push_back(resource->name);
    for(const auto& node:objects_) {
        const auto& state=node->object->State();
        result.objects.push_back({Key(*node),node->handle,node->placement->source.resource->name,node->object->Part(),
            state.display_name,state.pose,state.state,state.level,state.flags,state.part_bounds,state.actor_bounds,state.actors});
    }
    return result;
}
bool NativeFieldWorld::PublishCurrent() {
    const auto& game=runtime_.game;
    if(game.field_scene.owner!=identity_||game.campaign.current_chapter_index!=chapter_||game.map_active!=map_active_||pending_||ValidRange(*range_))return false;
    auto snapshot=std::make_shared<const FieldSceneSnapshot>(Snapshot());
    // Republishing unchanged geometry must not invalidate current Unit poses.
    if(ReadCurrentHeightGeometry(runtime_).status!=HeightStatus::Ok&&RestoreCurrentHeightGeometry(runtime_,height_)!=HeightStatus::Ok)return false;
    runtime_.game.field_scene={identity_,revision_,terrain_revision_,std::move(snapshot)};return true;
}
}
