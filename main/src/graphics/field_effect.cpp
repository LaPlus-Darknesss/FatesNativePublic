#include "fates/graphics/field_effect.hpp"
#include "fates/detail/camera_execution_runtime.hpp"
#include <stdexcept>

namespace {
namespace native=fates::map::native;
GameEffect Effect(native::FieldEffectHandle handle) {return GameEffect(static_cast<std::uint32_t>(handle.value));}
nn::math::MTX34 Matrix(const native::FieldMatrix& source) {
    nn::math::MTX34 result;
    for(unsigned row=0;row<3;++row)for(unsigned column=0;column<4;++column)result.m[row][column]=source.values[row*4+column];
    return result;
}
// Preserve the existing GameEffect adapter. This bridge shares the verified
// Fates node policy; it does not close GameEffect's remaining platform internals.
class EffectBackend final:public native::FieldEffectBackend {
public:
    native::FieldEffectHandle CreateEffect(const std::optional<std::string>& label) override {
        if(!scene_read_){scene_=fates::decomp_detail::GetFieldScene();scene_read_=true;}
        const auto effect=GameEffect::Create(label?label->c_str():nullptr,scene_);
        return {effect.Handle()>>12?effect.Handle():0};
    }
    std::optional<std::string> SecondaryEffectLabel(native::FieldEffectHandle handle) override {
        if(const char* label=fates::decomp_detail::GetSecondaryEffectLabel(Effect(handle)))return label;
        return {};
    }
    void DeleteEffect(native::FieldEffectHandle h) override {Effect(h).Delete();}
    void SetEffectGroup(native::FieldEffectHandle h,unsigned group) override {Effect(h).SetGroup(static_cast<EffectGroup::Type>(group));}
    void SetEffectVisible(native::FieldEffectHandle h,bool visible) override {Effect(h).SetVisible(visible);}
    void SetEffectStep(native::FieldEffectHandle h,float step) override {Effect(h).SetStepFrame(step);}
    void SetEffectLocation(native::FieldEffectHandle h,unsigned location) override {Effect(h).SetLocation(static_cast<EffectDataLocation::Type>(location));}
    void SetEffectTransform(native::FieldEffectHandle h,const native::FieldMatrix& matrix) override {Effect(h).SetMatrix(Matrix(matrix));}
private:
    bool scene_read_{};
    SceneSystem* scene_{};
};
}
FieldEffectNode::FieldEffectNode()=default;
FieldEffectNode::~FieldEffectNode() {EffectBackend backend;native::FreeFieldEffect(state_,backend);}
void FieldEffectNode::Load(const char* label,const MapPose& pose) {
    native::FieldEffectPlacement placement;placement.pose=pose;if(label)placement.label=label;
    EffectBackend backend;native::LoadFieldEffect(state_,backend,placement);
}
void FieldEffectNode::SetTransform(const nn::math::MTX34& matrix) {
    native::FieldMatrix parent;
    for(unsigned row=0;row<3;++row)for(unsigned column=0;column<4;++column)parent.values[row*4+column]=matrix.m[row][column];
    EffectBackend backend;
    if(!native::TransformFieldEffect(state_,backend,parent))throw std::invalid_argument("FieldEffectNode transform outside finite native domain");
}
void FieldEffectNode::SetLevel(PartsConst::Level level) {EffectBackend backend;native::SetFieldEffectLevel(state_,backend,static_cast<int>(level));}
void FieldEffectNode::SetColor(const Color8& color) {for(const auto h:state_.handles)Effect(h).SetColor(color);}
void FieldEffectList::FadeOut() {for(auto* node:nodes)if(node)for(const auto h:node->state_.handles)Effect(h).FadeOut(1000);}
FieldEffectList::~FieldEffectList()=default; // nodes are removed by the FieldObject owner
