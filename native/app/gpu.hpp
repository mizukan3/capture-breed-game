#pragma once
#include <SDL3/SDL.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
namespace capture::platform {
struct Vertex {float x,y,z,r,g,b;};
struct UiVertex {float x,y,u,v,r,g,b,a;};
struct Mesh {SDL_GPUBuffer* buffer{};Uint32 vertices{};};
struct Texture {SDL_GPUTexture* texture{};int width{},height{};};
struct alignas(16) SceneUniform {
 std::array<float,16> matrix{};
 std::array<float,4> model{},center{};
 std::array<std::array<float,4>,8> torches{};
};
struct UiBatch {SDL_GPUTexture* texture{};Uint32 first{},count{};};
class Gpu {
 SDL_Window* window_{};
 SDL_GPUDevice* device_{};
 SDL_GPUGraphicsPipeline *scene_pipeline_{},*ui_pipeline_{};
 SDL_GPUSampler* sampler_{};
 SDL_GPUTexture* depth_{};
 SDL_GPUBuffer* ui_buffer_{};
 SDL_GPUTransferBuffer* ui_transfer_{};
 Uint32 depth_width_{},depth_height_{};
 std::string assets_;
 SDL_GPUShader* shader(const std::string&,SDL_GPUShaderStage,Uint32 samplers,Uint32 uniforms);
 void make_pipelines();
public:
 std::uint64_t uploaded_bytes{},draw_calls{};
 explicit Gpu(SDL_Window*,std::string assets,bool debug);
 ~Gpu();
 Gpu(const Gpu&)=delete;Gpu& operator=(const Gpu&)=delete;
 SDL_GPUDevice* device() const{return device_;}
 Mesh upload(const std::vector<Vertex>&);
 Mesh load_mesh(const std::string&);
 void release(Mesh&);
 Texture upload_surface(SDL_Surface*);
 Texture load_image(const std::string&);
 void release(Texture&);
 SDL_GPUCommandBuffer* begin(SDL_GPUTexture*&,Uint32&,Uint32&);
 void draw(SDL_GPUCommandBuffer*,SDL_GPUTexture*,Uint32,Uint32,const SDL_FRect& viewport,
  const std::vector<std::pair<const Mesh*,SceneUniform>>& instances,
  const std::vector<UiVertex>&,const std::vector<UiBatch>&,float logical_width,float logical_height);
 const char* driver() const {return SDL_GetGPUDeviceDriver(device_);}
};
}
