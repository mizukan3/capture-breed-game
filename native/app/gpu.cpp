#include "gpu.hpp"
#include <SDL3_image/SDL_image.h>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
namespace capture::platform {
namespace {
void require(bool b,const char* what){if(!b)throw std::runtime_error(std::string(what)+": "+SDL_GetError());}
std::string bytes(const std::string& p) {
 size_t size{};void* data=SDL_LoadFile(p.c_str(),&size);require(data!=nullptr,p.c_str());
 std::string r(static_cast<const char*>(data),size);SDL_free(data);return r;
}
}
Gpu::Gpu(SDL_Window* window,std::string assets,bool debug):window_(window),assets_(std::move(assets)) {
 device_=SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV|SDL_GPU_SHADERFORMAT_MSL,debug,nullptr);
 require(device_!=nullptr,"GPU device (Vulkan or Metal)");
 try {
  require(SDL_ClaimWindowForGPUDevice(device_,window_),"claim window");
  require(SDL_SetGPUSwapchainParameters(device_,window_,SDL_GPU_SWAPCHAINCOMPOSITION_SDR,SDL_GPU_PRESENTMODE_VSYNC),"vsync");
  make_pipelines();
  SDL_GPUSamplerCreateInfo sampler{};sampler.min_filter=SDL_GPU_FILTER_LINEAR;sampler.mag_filter=SDL_GPU_FILTER_LINEAR;sampler.mipmap_mode=SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
  sampler.address_mode_u=sampler.address_mode_v=sampler.address_mode_w=SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
  sampler_=SDL_CreateGPUSampler(device_,&sampler);require(sampler_!=nullptr,"UI sampler");
  constexpr Uint32 capacity=32768*sizeof(UiVertex);
  SDL_GPUBufferCreateInfo b{};b.usage=SDL_GPU_BUFFERUSAGE_VERTEX;b.size=capacity;
  ui_buffer_=SDL_CreateGPUBuffer(device_,&b);require(ui_buffer_!=nullptr,"UI buffer");
  SDL_GPUTransferBufferCreateInfo t{};t.usage=SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;t.size=capacity;
  ui_transfer_=SDL_CreateGPUTransferBuffer(device_,&t);require(ui_transfer_!=nullptr,"UI transfer buffer");
 }catch(...){SDL_DestroyGPUDevice(device_);device_=nullptr;throw;}
}
Gpu::~Gpu() {
 if(!device_)return;
 SDL_WaitForGPUIdle(device_);
 if(depth_)SDL_ReleaseGPUTexture(device_,depth_);
 if(ui_buffer_)SDL_ReleaseGPUBuffer(device_,ui_buffer_);
 if(ui_transfer_)SDL_ReleaseGPUTransferBuffer(device_,ui_transfer_);
 if(sampler_)SDL_ReleaseGPUSampler(device_,sampler_);
 if(scene_pipeline_)SDL_ReleaseGPUGraphicsPipeline(device_,scene_pipeline_);
 if(ui_pipeline_)SDL_ReleaseGPUGraphicsPipeline(device_,ui_pipeline_);
 SDL_ReleaseWindowFromGPUDevice(device_,window_);SDL_DestroyGPUDevice(device_);
}
SDL_GPUShader* Gpu::shader(const std::string& name,SDL_GPUShaderStage stage,Uint32 samplers,Uint32 uniforms) {
 const bool metal=(SDL_GetGPUShaderFormats(device_)&SDL_GPU_SHADERFORMAT_MSL)!=0;
 const auto src=bytes(assets_+"/shaders/"+(metal?"all.metal":name+".spv"));
 std::string entry;
 if(metal){entry=name.starts_with("scene")?"scene_":"ui_";entry+=stage==SDL_GPU_SHADERSTAGE_VERTEX?"vertex":"fragment";}else entry="main";
 SDL_GPUShaderCreateInfo info{};info.code_size=src.size();info.code=reinterpret_cast<const Uint8*>(src.c_str());info.entrypoint=entry.c_str();
 info.format=metal?SDL_GPU_SHADERFORMAT_MSL:SDL_GPU_SHADERFORMAT_SPIRV;info.stage=stage;info.num_samplers=samplers;info.num_uniform_buffers=uniforms;
 auto* out=SDL_CreateGPUShader(device_,&info);require(out!=nullptr,name.c_str());return out;
}
void Gpu::make_pipelines() {
 auto create=[&](bool ui) {
  const std::string prefix=ui?"ui":"scene";
  auto* vs=shader(prefix+".vert",SDL_GPU_SHADERSTAGE_VERTEX,0,1);
  SDL_GPUShader* fs{};
  try{fs=shader(prefix+".frag",SDL_GPU_SHADERSTAGE_FRAGMENT,ui?1:0,ui?0:1);}catch(...){SDL_ReleaseGPUShader(device_,vs);throw;}
  SDL_GPUVertexBufferDescription vb{};vb.slot=0;vb.pitch=ui?sizeof(UiVertex):sizeof(Vertex);vb.input_rate=SDL_GPU_VERTEXINPUTRATE_VERTEX;
  std::array<SDL_GPUVertexAttribute,3> a{};
  a[0]={0,0,ui?SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2:SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,0};
  a[1]={1,0,ui?SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2:SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,ui?8u:12u};
  a[2]={2,0,SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,16};
  SDL_GPUColorTargetDescription target{};target.format=SDL_GetGPUSwapchainTextureFormat(device_,window_);
  if(ui){auto& b=target.blend_state;b.enable_blend=true;b.src_color_blendfactor=SDL_GPU_BLENDFACTOR_SRC_ALPHA;b.dst_color_blendfactor=SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;b.color_blend_op=SDL_GPU_BLENDOP_ADD;b.src_alpha_blendfactor=SDL_GPU_BLENDFACTOR_ONE;b.dst_alpha_blendfactor=SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;b.alpha_blend_op=SDL_GPU_BLENDOP_ADD;}
  SDL_GPUGraphicsPipelineCreateInfo p{};p.vertex_shader=vs;p.fragment_shader=fs;p.primitive_type=SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
  p.vertex_input_state={&vb,1,a.data(),ui?3u:2u};
  p.rasterizer_state.fill_mode=SDL_GPU_FILLMODE_FILL;p.rasterizer_state.cull_mode=SDL_GPU_CULLMODE_NONE;p.rasterizer_state.front_face=SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
  p.multisample_state.sample_count=SDL_GPU_SAMPLECOUNT_1;p.target_info.color_target_descriptions=&target;p.target_info.num_color_targets=1;
  p.target_info.has_depth_stencil_target=!ui;p.target_info.depth_stencil_format=SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
  p.depth_stencil_state.enable_depth_test=!ui;p.depth_stencil_state.enable_depth_write=!ui;p.depth_stencil_state.compare_op=SDL_GPU_COMPAREOP_LESS;
  auto* pipeline=SDL_CreateGPUGraphicsPipeline(device_,&p);
  SDL_ReleaseGPUShader(device_,vs);SDL_ReleaseGPUShader(device_,fs);require(pipeline!=nullptr,"graphics pipeline");return pipeline;
 };
 scene_pipeline_=create(false);ui_pipeline_=create(true);
}
Mesh Gpu::upload(const std::vector<Vertex>& v) {
 if(v.empty())return {};
 require(v.size()<=std::numeric_limits<Uint32>::max()/sizeof(Vertex),"mesh size");
 const auto size=static_cast<Uint32>(v.size()*sizeof(Vertex));SDL_GPUBufferCreateInfo b{};b.size=size;b.usage=SDL_GPU_BUFFERUSAGE_VERTEX;
 Mesh result{SDL_CreateGPUBuffer(device_,&b),static_cast<Uint32>(v.size())};require(result.buffer!=nullptr,"mesh buffer");
 SDL_GPUTransferBufferCreateInfo t{};t.usage=SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;t.size=size;auto* transfer=SDL_CreateGPUTransferBuffer(device_,&t);
 if(!transfer){release(result);require(false,"mesh transfer");}
 void* ptr=SDL_MapGPUTransferBuffer(device_,transfer,false);
 if(!ptr){SDL_ReleaseGPUTransferBuffer(device_,transfer);release(result);require(false,"map mesh transfer");}
 std::memcpy(ptr,v.data(),size);SDL_UnmapGPUTransferBuffer(device_,transfer);
 auto* cmd=SDL_AcquireGPUCommandBuffer(device_);require(cmd!=nullptr,"mesh command");auto* pass=SDL_BeginGPUCopyPass(cmd);require(pass!=nullptr,"mesh copy pass");
 SDL_GPUTransferBufferLocation source{transfer,0};SDL_GPUBufferRegion destination{result.buffer,0,size};SDL_UploadToGPUBuffer(pass,&source,&destination,false);SDL_EndGPUCopyPass(pass);
 const bool ok=SDL_SubmitGPUCommandBuffer(cmd);SDL_ReleaseGPUTransferBuffer(device_,transfer);require(ok,"submit mesh");uploaded_bytes+=size;return result;
}
Mesh Gpu::load_mesh(const std::string& p) {
 const auto data=bytes(p);require(data.size()>=8&&data.substr(0,4)=="CDM1","mesh magic");
 const auto* d=reinterpret_cast<const Uint8*>(data.data());const Uint32 count=Uint32(d[4])|(Uint32(d[5])<<8)|(Uint32(d[6])<<16)|(Uint32(d[7])<<24);
 require(count%6==0&&data.size()==8+static_cast<std::size_t>(count)*4,"mesh payload");
 std::vector<Vertex> vertices;vertices.resize(count/6);
 for(std::size_t i=0;i<count;++i){Uint32 bits{};std::memcpy(&bits,d+8+i*4,4);bits=SDL_Swap32LE(bits);float value{};std::memcpy(&value,&bits,4);std::memcpy(reinterpret_cast<char*>(vertices.data())+i*4,&value,4);}
 return upload(vertices);
}
void Gpu::release(Mesh& m){if(m.buffer)SDL_ReleaseGPUBuffer(device_,m.buffer);m={};}
Texture Gpu::upload_surface(SDL_Surface* surface) {
 std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)> converted(SDL_ConvertSurface(surface,SDL_PIXELFORMAT_RGBA32),SDL_DestroySurface);require(converted!=nullptr,"RGBA conversion");
 const Uint32 w=static_cast<Uint32>(converted->w),h=static_cast<Uint32>(converted->h);
 SDL_GPUTextureCreateInfo t{};t.type=SDL_GPU_TEXTURETYPE_2D;t.format=SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;t.usage=SDL_GPU_TEXTUREUSAGE_SAMPLER;t.width=w;t.height=h;t.layer_count_or_depth=1;t.num_levels=1;t.sample_count=SDL_GPU_SAMPLECOUNT_1;
 Texture result{SDL_CreateGPUTexture(device_,&t),static_cast<int>(w),static_cast<int>(h)};require(result.texture!=nullptr,"UI texture");
 SDL_GPUTransferBufferCreateInfo bi{};bi.usage=SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;bi.size=w*h*4;auto* transfer=SDL_CreateGPUTransferBuffer(device_,&bi);require(transfer!=nullptr,"texture transfer");
 auto* ptr=static_cast<Uint8*>(SDL_MapGPUTransferBuffer(device_,transfer,false));require(ptr!=nullptr,"map texture");
 for(Uint32 y=0;y<h;++y)std::memcpy(ptr+y*w*4,static_cast<Uint8*>(converted->pixels)+y*converted->pitch,w*4);
 SDL_UnmapGPUTransferBuffer(device_,transfer);auto* cmd=SDL_AcquireGPUCommandBuffer(device_);require(cmd!=nullptr,"texture command");
 auto* copy=SDL_BeginGPUCopyPass(cmd);require(copy!=nullptr,"texture copy");
 SDL_GPUTextureTransferInfo source{transfer,0,w,h};SDL_GPUTextureRegion destination{result.texture,0,0,0,0,0,w,h,1};
 SDL_UploadToGPUTexture(copy,&source,&destination,false);SDL_EndGPUCopyPass(copy);const bool ok=SDL_SubmitGPUCommandBuffer(cmd);SDL_ReleaseGPUTransferBuffer(device_,transfer);require(ok,"submit texture");uploaded_bytes+=bi.size;return result;
}
Texture Gpu::load_image(const std::string& path) {
 std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)> surface(IMG_Load(path.c_str()),SDL_DestroySurface);require(surface!=nullptr,"image load");return upload_surface(surface.get());
}
void Gpu::release(Texture& t){if(t.texture)SDL_ReleaseGPUTexture(device_,t.texture);t={};}
SDL_GPUCommandBuffer* Gpu::begin(SDL_GPUTexture*& texture,Uint32& w,Uint32& h) {
 auto* cmd=SDL_AcquireGPUCommandBuffer(device_);require(cmd!=nullptr,"frame command");
 if(!SDL_WaitAndAcquireGPUSwapchainTexture(cmd,window_,&texture,&w,&h)){SDL_CancelGPUCommandBuffer(cmd);require(false,"acquire swapchain");}
 if(!texture){SDL_CancelGPUCommandBuffer(cmd);return nullptr;}return cmd;
}
void Gpu::draw(SDL_GPUCommandBuffer* cmd,SDL_GPUTexture* target,Uint32 w,Uint32 h,const SDL_FRect& viewport,
 const std::vector<std::pair<const Mesh*,SceneUniform>>& instances,const std::vector<UiVertex>& ui,const std::vector<UiBatch>& batches,float logical_width,float logical_height) {
 draw_calls=0;
 if(w!=depth_width_||h!=depth_height_){if(depth_)SDL_ReleaseGPUTexture(device_,depth_);SDL_GPUTextureCreateInfo d{};d.type=SDL_GPU_TEXTURETYPE_2D;d.format=SDL_GPU_TEXTUREFORMAT_D32_FLOAT;d.usage=SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;d.width=w;d.height=h;d.layer_count_or_depth=1;d.num_levels=1;d.sample_count=SDL_GPU_SAMPLECOUNT_1;depth_=SDL_CreateGPUTexture(device_,&d);require(depth_!=nullptr,"depth texture");depth_width_=w;depth_height_=h;}
 require(ui.size()<=32768,"UI batch capacity");const auto size=static_cast<Uint32>(ui.size()*sizeof(UiVertex));
 if(size){void* ptr=SDL_MapGPUTransferBuffer(device_,ui_transfer_,true);require(ptr!=nullptr,"map UI");std::memcpy(ptr,ui.data(),size);SDL_UnmapGPUTransferBuffer(device_,ui_transfer_);auto* copy=SDL_BeginGPUCopyPass(cmd);SDL_GPUTransferBufferLocation src{ui_transfer_,0};SDL_GPUBufferRegion dst{ui_buffer_,0,size};SDL_UploadToGPUBuffer(copy,&src,&dst,true);SDL_EndGPUCopyPass(copy);uploaded_bytes+=size;}
 SDL_GPUColorTargetInfo color{};color.texture=target;color.clear_color={.063f,.106f,.153f,1};color.load_op=SDL_GPU_LOADOP_CLEAR;color.store_op=SDL_GPU_STOREOP_STORE;
 SDL_GPUDepthStencilTargetInfo depth{};depth.texture=depth_;depth.clear_depth=1;depth.load_op=SDL_GPU_LOADOP_CLEAR;depth.store_op=SDL_GPU_STOREOP_DONT_CARE;depth.stencil_load_op=SDL_GPU_LOADOP_DONT_CARE;depth.stencil_store_op=SDL_GPU_STOREOP_DONT_CARE;
 auto* pass=SDL_BeginGPURenderPass(cmd,&color,1,&depth);require(pass!=nullptr,"scene pass");SDL_BindGPUGraphicsPipeline(pass,scene_pipeline_);
 SDL_GPUViewport vp{viewport.x,viewport.y,viewport.w,viewport.h,0,1};SDL_SetGPUViewport(pass,&vp);
 const std::array<float,4> fog{.098f,.169f,.165f,1};SDL_PushGPUFragmentUniformData(cmd,0,fog.data(),sizeof(fog));
 for(const auto& [mesh,uniform]:instances)if(mesh&&mesh->buffer){SDL_PushGPUVertexUniformData(cmd,0,&uniform,sizeof(uniform));SDL_GPUBufferBinding b{mesh->buffer,0};SDL_BindGPUVertexBuffers(pass,0,&b,1);SDL_DrawGPUPrimitives(pass,mesh->vertices,1,0,0);++draw_calls;}
 SDL_EndGPURenderPass(pass);
 color.load_op=SDL_GPU_LOADOP_LOAD;pass=SDL_BeginGPURenderPass(cmd,&color,1,nullptr);require(pass!=nullptr,"UI pass");SDL_BindGPUGraphicsPipeline(pass,ui_pipeline_);
 SDL_GPUViewport full{0,0,static_cast<float>(w),static_cast<float>(h),0,1};SDL_SetGPUViewport(pass,&full);
 const std::array<float,4> screen{logical_width,logical_height,0,0};SDL_PushGPUVertexUniformData(cmd,0,screen.data(),sizeof(screen));
 SDL_GPUBufferBinding binding{ui_buffer_,0};SDL_BindGPUVertexBuffers(pass,0,&binding,1);
 for(const auto& batch:batches){SDL_GPUTextureSamplerBinding sampler{batch.texture,sampler_};SDL_BindGPUFragmentSamplers(pass,0,&sampler,1);SDL_DrawGPUPrimitives(pass,batch.count,1,batch.first,0);++draw_calls;}
 SDL_EndGPURenderPass(pass);require(SDL_SubmitGPUCommandBuffer(cmd),"submit frame");
}
}
