#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "fates/presentation/d3d11_field_readback.hpp"
#include "fates/graphics/portable_vertex.hpp"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <map>
#include <sstream>

namespace fates::presentation::portable {
using Microsoft::WRL::ComPtr;
namespace {
bool Fail(std::string& error,const char* message){error=message;return false;}
bool Hr(HRESULT result,const char* where,std::string& error) {
    if(SUCCEEDED(result))return true;std::ostringstream text;text<<where<<" failed: 0x"<<std::hex<<static_cast<unsigned long>(result);error=text.str();return false;
}
template<class Values> bool Finite(const Values& values){return std::all_of(values.begin(),values.end(),[](float v){return std::isfinite(v);});}
struct Constants {std::array<float,12> world,view;std::array<float,4> projection,color;};
static_assert(sizeof(Constants)==128);
constexpr char Shader[]=R"(
cbuffer DrawState:register(b0){float4 wr0;float4 wr1;float4 wr2;float4 vr0;float4 vr1;float4 vr2;float4 pr;float4 color;}
float4 VS(float3 position:POSITION):SV_POSITION {
    float4 local=float4(position,1);
    float4 world=float4(dot(wr0,local),dot(wr1,local),dot(wr2,local),1);
    float3 view=float3(dot(vr0,world),dot(vr1,world),dot(vr2,world));
    return float4(pr.x*view.x,pr.y*view.y,pr.z*view.z+pr.w,view.z);
}
float4 PS():SV_TARGET{return color;}
)";
bool Compile(const char* entry,const char* profile,ComPtr<ID3DBlob>& output,std::string& error) {
    ComPtr<ID3DBlob> log;
    const auto result=D3DCompile(Shader,sizeof(Shader)-1,nullptr,nullptr,nullptr,entry,profile,D3DCOMPILE_ENABLE_STRICTNESS,0,&output,&log);
    if(FAILED(result)&&log){error.assign(static_cast<const char*>(log->GetBufferPointer()),log->GetBufferSize());return false;}
    return Hr(result,"D3D shader compile",error);
}
}
struct D3D11FieldReadback::Impl {
    struct Mesh {
        ComPtr<ID3D11Buffer> vertices,indices;
        std::vector<std::pair<UINT,UINT>> ranges;
        UINT index_count{};
    };
    struct Model {
        std::shared_ptr<const assets::PortableModelAsset> asset;
        std::vector<Mesh> meshes;
    };
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<ID3D11VertexShader> vertex_shader;
    ComPtr<ID3D11PixelShader> pixel_shader;
    ComPtr<ID3D11InputLayout> layout;
    ComPtr<ID3D11Buffer> constants;
    ComPtr<ID3D11DepthStencilState> depth_state;
    std::map<const assets::PortableModelAsset*,std::shared_ptr<Model>> models;
    std::shared_ptr<const FieldPresentationFrame> submitted_frame;
    bool Upload(std::shared_ptr<const assets::PortableModelAsset> asset,std::shared_ptr<Model>& output,std::string& error) {
        if(!asset||!asset->resource||!asset->resource->file)return Fail(error,"Draw model has no retained BCH owner");
        auto next=std::make_shared<Model>();next->asset=std::move(asset);
        for(const auto& mesh:next->asset->geometry.meshes) {
            assets::StaticFieldVertices decoded;
            if(!assets::DecodeStaticFieldVertices(*next->asset->resource->file,mesh,decoded,error))return false;
            std::vector<std::uint16_t> indices;Mesh gpu;
            for(const auto& range:mesh.ranges) {
                if(range.indices.size()%3)return Fail(error,"Diagnostic triangle-list profile refuses incomplete triangle range");
                if(indices.size()+range.indices.size()>std::numeric_limits<UINT>::max()/sizeof(std::uint16_t))return Fail(error,"Index upload exceeds D3D11 buffer domain");
                if(!range.indices.empty())gpu.ranges.emplace_back(static_cast<UINT>(indices.size()),static_cast<UINT>(range.indices.size()));
                indices.insert(indices.end(),range.indices.begin(),range.indices.end());
            }
            if(indices.empty())continue;
            if(decoded.vertices.empty()||decoded.vertices.size()>std::numeric_limits<UINT>::max()/sizeof(assets::StaticFieldVertex))return Fail(error,"Vertex upload exceeds D3D11 buffer domain");
            if(std::any_of(indices.begin(),indices.end(),[&](auto index){return index>=decoded.vertices.size();}))return Fail(error,"Draw index exceeds decoded vertices");
            D3D11_BUFFER_DESC desc{};desc.ByteWidth=static_cast<UINT>(decoded.vertices.size()*sizeof(assets::StaticFieldVertex));
            desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_VERTEX_BUFFER;
            D3D11_SUBRESOURCE_DATA data{decoded.vertices.data(),0,0};
            if(!Hr(device->CreateBuffer(&desc,&data,&gpu.vertices),"Vertex upload",error))return false;
            desc.ByteWidth=static_cast<UINT>(indices.size()*sizeof(std::uint16_t));desc.BindFlags=D3D11_BIND_INDEX_BUFFER;data.pSysMem=indices.data();
            if(!Hr(device->CreateBuffer(&desc,&data,&gpu.indices),"Index upload",error))return false;
            gpu.index_count=static_cast<UINT>(indices.size());next->meshes.push_back(std::move(gpu));
        }
        output=std::move(next);return true;
    }
};
D3D11FieldReadback::D3D11FieldReadback(std::unique_ptr<Impl> impl):impl_(std::move(impl)){}
D3D11FieldReadback::~D3D11FieldReadback()=default;
std::unique_ptr<D3D11FieldReadback> D3D11FieldReadback::Create(FieldReadbackDevice mode,std::string& error) {
    if(mode!=FieldReadbackDevice::Warp&&mode!=FieldReadbackDevice::Hardware){error="Unknown D3D device request";return {};}
    auto impl=std::make_unique<Impl>();D3D_FEATURE_LEVEL level{};
    if(!Hr(D3D11CreateDevice(nullptr,mode==FieldReadbackDevice::Warp?D3D_DRIVER_TYPE_WARP:D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&impl->device,&level,&impl->context),"D3D11 device",error))return {};
    ComPtr<ID3DBlob> vs,ps;if(!Compile("VS","vs_4_0",vs,error)||!Compile("PS","ps_4_0",ps,error))return {};
    if(!Hr(impl->device->CreateVertexShader(vs->GetBufferPointer(),vs->GetBufferSize(),nullptr,&impl->vertex_shader),"Vertex shader",error)||
       !Hr(impl->device->CreatePixelShader(ps->GetBufferPointer(),ps->GetBufferSize(),nullptr,&impl->pixel_shader),"Pixel shader",error))return {};
    const D3D11_INPUT_ELEMENT_DESC input{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0};
    if(!Hr(impl->device->CreateInputLayout(&input,1,vs->GetBufferPointer(),vs->GetBufferSize(),&impl->layout),"Vertex layout",error))return {};
    D3D11_BUFFER_DESC cb{};cb.ByteWidth=sizeof(Constants);cb.Usage=D3D11_USAGE_DEFAULT;cb.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
    if(!Hr(impl->device->CreateBuffer(&cb,nullptr,&impl->constants),"Draw constant buffer",error))return {};
    D3D11_DEPTH_STENCIL_DESC depth{};depth.DepthEnable=TRUE;depth.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;depth.DepthFunc=D3D11_COMPARISON_LESS;
    if(!Hr(impl->device->CreateDepthStencilState(&depth,&impl->depth_state),"Diagnostic depth state",error))return {};
    error.clear();return std::unique_ptr<D3D11FieldReadback>(new D3D11FieldReadback(std::move(impl)));
}
bool D3D11FieldReadback::Draw(std::shared_ptr<const FieldPresentationFrame> frame,const FieldReadbackCamera& camera,
    unsigned width,unsigned height,const GeometryDiagnosticStyle& style,assets::TextureImage& output,FieldReadbackStats& stats,std::string& error) {
    if(!width||!height||width>4096||height>4096||std::uint64_t(width)*9!=std::uint64_t(height)*16)return Fail(error,"Native readback requires a bounded 16:9 canvas");
    if(!Finite(camera.view.values)||!Finite(std::array{camera.focal_x,camera.focal_y,camera.near_z,camera.far_z})||
       camera.focal_x<=0||camera.focal_y<=0||camera.near_z<=0||camera.far_z<=camera.near_z)return Fail(error,"Invalid explicit positive-Z camera");
    if(!Finite(style.clear)||!Finite(style.color))return Fail(error,"Nonfinite diagnostic color");
    const float a=camera.far_z/(camera.far_z-camera.near_z),b=-camera.near_z*camera.far_z/(camera.far_z-camera.near_z);
    if(!std::isfinite(a)||!std::isfinite(b))return Fail(error,"Camera projection overflow");
    auto& impl=*impl_;std::map<const assets::PortableModelAsset*,std::shared_ptr<Impl::Model>> next_models;
    if(frame) {
        if(!frame->assets||!frame->assets->snapshot)return Fail(error,"Draw frame has no immutable scene owner");
        for(const auto& actor:frame->actors) {
            if(!actor.key||actor.key.world!=frame->assets->snapshot->world||!actor.asset.model||!Finite(actor.world_matrix.values))return Fail(error,"Invalid retained actor draw binding");
            const auto key=actor.asset.model.get();if(next_models.contains(key))continue;
            if(const auto it=impl.models.find(key);it!=impl.models.end())next_models.emplace(key,it->second);
            else {std::shared_ptr<Impl::Model> model;if(!impl.Upload(actor.asset.model,model,error))return false;next_models.emplace(key,std::move(model));}
        }
    }
    D3D11_TEXTURE2D_DESC desc{};desc.Width=width;desc.Height=height;desc.MipLevels=desc.ArraySize=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=D3D11_BIND_RENDER_TARGET;
    ComPtr<ID3D11Texture2D> color,depth,readback;ComPtr<ID3D11RenderTargetView> target;ComPtr<ID3D11DepthStencilView> depth_view;
    if(!Hr(impl.device->CreateTexture2D(&desc,nullptr,&color),"Color target",error)||!Hr(impl.device->CreateRenderTargetView(color.Get(),nullptr,&target),"Color view",error))return false;
    desc.Format=DXGI_FORMAT_D32_FLOAT;desc.BindFlags=D3D11_BIND_DEPTH_STENCIL;
    if(!Hr(impl.device->CreateTexture2D(&desc,nullptr,&depth),"Depth target",error)||!Hr(impl.device->CreateDepthStencilView(depth.Get(),nullptr,&depth_view),"Depth view",error))return false;
    desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    if(!Hr(impl.device->CreateTexture2D(&desc,nullptr,&readback),"Readback target",error))return false;
    D3D11_RASTERIZER_DESC raster{};raster.FillMode=style.wireframe?D3D11_FILL_WIREFRAME:D3D11_FILL_SOLID;raster.CullMode=D3D11_CULL_NONE;raster.DepthClipEnable=TRUE;
    ComPtr<ID3D11RasterizerState> raster_state;if(!Hr(impl.device->CreateRasterizerState(&raster,&raster_state),"Diagnostic raster state",error))return false;
    auto* context=impl.context.Get();context->ClearState();
    context->ClearRenderTargetView(target.Get(),style.clear.data());context->ClearDepthStencilView(depth_view.Get(),D3D11_CLEAR_DEPTH,1,0);
    auto* target_raw=target.Get();context->OMSetRenderTargets(1,&target_raw,depth_view.Get());context->OMSetDepthStencilState(impl.depth_state.Get(),0);
    const D3D11_VIEWPORT viewport{0,0,static_cast<float>(width),static_cast<float>(height),0,1};context->RSSetViewports(1,&viewport);context->RSSetState(raster_state.Get());
    context->IASetInputLayout(impl.layout.Get());context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(impl.vertex_shader.Get(),nullptr,0);context->PSSetShader(impl.pixel_shader.Get(),nullptr,0);
    auto* cb=impl.constants.Get();context->VSSetConstantBuffers(0,1,&cb);context->PSSetConstantBuffers(0,1,&cb);
    FieldReadbackStats next_stats;next_stats.retained_models=next_models.size();
    if(frame)for(const auto& actor:frame->actors) {
        Constants constants{actor.world_matrix.values,camera.view.values,{camera.focal_x,camera.focal_y,a,b},style.color};
        context->UpdateSubresource(impl.constants.Get(),0,nullptr,&constants,0,0);++next_stats.actors;
        for(const auto& mesh:next_models.at(actor.asset.model.get())->meshes) {
            auto* vertices=mesh.vertices.Get();const UINT stride=sizeof(assets::StaticFieldVertex),offset=0;
            context->IASetVertexBuffers(0,1,&vertices,&stride,&offset);context->IASetIndexBuffer(mesh.indices.Get(),DXGI_FORMAT_R16_UINT,0);
            for(const auto& range:mesh.ranges){context->DrawIndexed(range.second,range.first,0);++next_stats.ranges;}
            ++next_stats.meshes;next_stats.indices+=mesh.index_count;
        }
    }
    context->CopyResource(readback.Get(),color.Get());D3D11_MAPPED_SUBRESOURCE mapped{};
    if(!Hr(context->Map(readback.Get(),0,D3D11_MAP_READ,0,&mapped),"Field readback",error)){context->ClearState();return false;}
    assets::TextureImage next;next.width=width;next.height=height;next.rgba.resize(std::size_t(width)*height*4);
    for(unsigned y=0;y<height;++y)std::memcpy(next.rgba.data()+std::size_t(y)*width*4,static_cast<const std::uint8_t*>(mapped.pData)+std::size_t(y)*mapped.RowPitch,std::size_t(width)*4);
    context->Unmap(readback.Get(),0);context->ClearState();
    impl.models=std::move(next_models);impl.submitted_frame=std::move(frame);output=std::move(next);stats=next_stats;error.clear();return true;
}
}
#endif
