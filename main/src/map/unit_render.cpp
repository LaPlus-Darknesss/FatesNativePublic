#include "fates/map/unit_render.hpp"
#include "fates/detail/unit_ownership_runtime.hpp"
#include "fates/graphics/render_state.hpp"
namespace map {
UnitRender::Data::Data(){retailRecord.fill(std::byte{});}
UnitRender::UnitRender():SceneNode(){}
void UnitRender::Entry(const UnitActor* a){fates::decomp_detail::UnitOwnershipCall("UnitRender.Entry",this,a);}
void UnitRender::Reset(float s){fates::decomp_detail::UnitOwnershipCall("UnitRender.Reset",this,s);}
void UnitRender::OnUpdate(RenderState&){/* PROVEN retail no-op virtual. */}
void UnitRender::OnDrawOpa(RenderState&){/* PROVEN retail no-op virtual. */}
void UnitRender::OnDrawXlu(RenderState& s){
    // PROVEN high-level behavior: build a pointer list for queued 0x10-byte
    // icon records, sort it, bind FieldWorld's camera, then batch-draw visible
    // unit icons in the translucent primitive pass. ObjectManager/UnitIcon/
    // primitive command details remain a map-presentation boundary.
    fates::decomp_detail::UnitOwnershipCall("UnitRender.OnDrawXlu.sorted_icon_batch",this,&s);
}
bool UnitSortCompare(UnitRender::Data* a,UnitRender::Data* b,void* c){return fates::decomp_detail::UnitOwnershipValue<bool>("UnitRender.Data.KeySort",a,b,c);}
}
