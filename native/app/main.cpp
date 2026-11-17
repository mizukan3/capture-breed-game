#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "audio.hpp"
#include "geometry.hpp"
#include "profiler.hpp"
#include "ui.hpp"
#include "capture/camera.hpp"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <numbers>
#include <sstream>
#include <unordered_map>
using namespace capture;
using namespace capture::platform;
namespace {
enum class Action {previous_floor,next_floor,next_seed,rotate,zoom,sound,metrics,quit};
struct Button {SDL_FRect rect;Action action;};
struct App {
 SDL_Window* window{};
 std::unique_ptr<Gpu> gpu;
 std::unique_ptr<Ui> ui;
 std::unique_ptr<Audio> audio;
 Data data;Floor floor;
 std::unordered_map<std::string,Mesh> models;
 Mesh world;Texture icon;
 std::vector<Vec3> torches;
 std::vector<std::pair<const Mesh*,SceneUniform>> instances;
 std::vector<Button> buttons;
 Profiler profiler;
 std::uint32_t seed{13};int number{1},quadrant{},zoom{};
 float cx{7},cz{7},ui_scale{1},logical_width{},logical_height{};
 bool suspended{},show_metrics{true},quit{};
 int smoke_frames{},frames{};
 std::filesystem::path settings_path;
 std::string metric_line;
 Uint64 next_metric{};
 App(int argc,char** argv) {
  for(int i=1;i<argc;++i){const std::string arg=argv[i];if(arg.starts_with("--smoke-frames="))smoke_frames=std::max(1,std::stoi(arg.substr(15)));}
  char* pref=SDL_GetPrefPath("CaptureBreed","NativePort");
  if(pref){settings_path=std::filesystem::path(reinterpret_cast<const char8_t*>(pref))/"workbench-settings.json";SDL_free(pref);}
  if(!smoke_frames)load_settings();
  window=SDL_CreateWindow("つかまえて配合！ SDL3移植検証",1100,760,SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIGH_PIXEL_DENSITY);
  if(!window)throw std::runtime_error(SDL_GetError());
  const char* base=SDL_GetBasePath();std::string assets=base?std::string(base)+"assets":"assets";
#ifdef SDL_PLATFORM_ANDROID
  assets="assets";
#endif
#ifdef NDEBUG
  constexpr bool debug=false;
#else
  constexpr bool debug=true;
#endif
  gpu=std::make_unique<Gpu>(window,assets,debug);
  ui=std::make_unique<Ui>(*gpu,assets+"/fonts/NotoSansJP.ttf");audio=std::make_unique<Audio>();
  icon=gpu->load_image(assets+"/icon.svg");
  for(const auto& name:{"explorer","monster-0","monster-1","monster-2","monster-3","monster-4","item-0","item-1","item-2","item-3","item-4","item-5","item-6"})
   models.emplace(name,gpu->load_mesh(assets+"/meshes/"+name+".mesh"));
  instances.reserve(32);buttons.reserve(8);regenerate();
  SDL_Log("SDL_GPU backend=%s; reference=v0.23.10/v53; v0.23.14 unverified",gpu->driver());
 }
 ~App() {
  save_settings();audio.reset();ui.reset();
  if(gpu){for(auto& [key,m]:models)gpu->release(m);gpu->release(world);gpu->release(icon);}
  gpu.reset();if(window)SDL_DestroyWindow(window);
 }
 void load_settings() {
  try{std::ifstream in(settings_path);if(!in)return;Json j;in>>j;seed=j.value("seed",13u);number=std::clamp(j.value("floor",1),1,6);quadrant=std::clamp(j.value("quadrant",0),0,3);zoom=std::clamp(j.value("zoom",0),0,1);}catch(const std::exception& e){SDL_Log("Settings ignored: %s",e.what());}
 }
 void save_settings() noexcept {
  if(smoke_frames||settings_path.empty())return;
  try {
   const auto temp=std::filesystem::path(settings_path).concat(".tmp");
   const Json j{{"schema",1},{"seed",seed},{"floor",number},{"quadrant",quadrant},{"zoom",zoom}};
   {std::ofstream out(temp,std::ios::trunc|std::ios::binary);out<<j.dump(2);out.flush();if(!out)throw std::runtime_error("settings write failed");}
   std::error_code ec;std::filesystem::rename(temp,settings_path,ec);
   if(ec){const auto backup=std::filesystem::path(settings_path).concat(".previous");std::filesystem::remove(backup,ec);if(std::filesystem::exists(settings_path))std::filesystem::rename(settings_path,backup);std::filesystem::rename(temp,settings_path);}
  }catch(const std::exception& e){SDL_Log("Settings save failed: %s",e.what());}
 }
 void rebuild_world() {
  auto geometry=build_world(floor,data,quadrant);auto next=gpu->upload(geometry.vertices);gpu->release(world);world=next;torches=std::move(geometry.torches);
 }
 void regenerate(){floor=generate_floor(seed,number);cx=static_cast<float>(floor.spawn.x);cz=static_cast<float>(floor.spawn.z);rebuild_world();}
 void act(Action action) {
  audio->ui();
  switch(action) {
   case Action::previous_floor:number=number==1?6:number-1;regenerate();break;
   case Action::next_floor:number=number==6?1:number+1;regenerate();break;
   case Action::next_seed:{Rng rng(seed);rng.random();seed=rng.state();if(!seed)seed=13;regenerate();break;}
   case Action::rotate:quadrant=(quadrant+1)%4;rebuild_world();break;
   case Action::zoom:zoom=1-zoom;break;
   case Action::sound:audio->enable(!audio->enabled());audio->ui();break;
   case Action::metrics:show_metrics=!show_metrics;break;
   case Action::quit:quit=true;break;
  }
  save_settings();
 }
 void click(float x,float y){for(const auto& b:buttons)if(x>=b.rect.x&&y>=b.rect.y&&x<b.rect.x+b.rect.w&&y<b.rect.y+b.rect.h){act(b.action);break;}}
 void event(const SDL_Event& e) {
  switch(e.type) {
   case SDL_EVENT_QUIT:case SDL_EVENT_WINDOW_CLOSE_REQUESTED:quit=true;break;
   case SDL_EVENT_WILL_ENTER_BACKGROUND:case SDL_EVENT_WINDOW_MINIMIZED:suspended=true;audio->pause(true);save_settings();break;
   case SDL_EVENT_DID_ENTER_FOREGROUND:case SDL_EVENT_WINDOW_RESTORED:suspended=false;audio->pause(false);break;
   case SDL_EVENT_MOUSE_BUTTON_UP:if(e.button.which!=SDL_TOUCH_MOUSEID&&e.button.button==SDL_BUTTON_LEFT)click(e.button.x/ui_scale,e.button.y/ui_scale);break;
   case SDL_EVENT_FINGER_UP:{int w{},h{};SDL_GetWindowSize(window,&w,&h);click(e.tfinger.x*w/ui_scale,e.tfinger.y*h/ui_scale);break;}
   case SDL_EVENT_KEY_DOWN:
    if(e.key.repeat)break;
    switch(e.key.key) {
     case SDLK_ESCAPE:quit=true;break;
     case SDLK_PAGEUP:act(Action::previous_floor);break;
     case SDLK_PAGEDOWN:act(Action::next_floor);break;
     case SDLK_R:act(Action::next_seed);break;
     case SDLK_E:act(Action::rotate);break;
     case SDLK_Z:act(Action::zoom);break;
     case SDLK_M:act(Action::sound);break;
     case SDLK_F3:act(Action::metrics);break;
     case SDLK_UP:cz-=1;break;case SDLK_DOWN:cz+=1;break;case SDLK_LEFT:cx-=1;break;case SDLK_RIGHT:cx+=1;break;
     default:break;
    }break;
   default:break;
  }
 }
 void minimap(SDL_FRect rect) {
  ui->rect(rect,{.06f,.105f,.15f,1});const float unit=std::min(rect.w,rect.h)/floor.size;
  for(int z=0;z<floor.size;++z)for(int x=0;x<floor.size;++x)if(floor.walkable({x,z}))ui->rect({rect.x+x*unit,rect.y+z*unit,unit,unit},{.31f,.37f,.34f,1});
  auto dot=[&](Cell p,UiColor color){ui->rect({rect.x+p.x*unit,rect.y+p.z*unit,unit,unit},color);};
  dot(floor.stairs,gold);for(const auto& e:floor.enemies)dot(e.cell,{.93f,.56f,.52f,1});dot(floor.spawn,{.46f,.79f,.63f,1});
 }
 void draw() {
  profiler.begin();
  int w{},h{};SDL_GetWindowSize(window,&w,&h);ui_scale=std::max(1.f,SDL_GetWindowDisplayScale(window));
  logical_width=w/ui_scale;logical_height=h/ui_scale;
  SDL_Rect safe{};if(!SDL_GetWindowSafeArea(window,&safe))safe={0,0,w,h};
  const SDL_FRect area{safe.x/ui_scale+8,safe.y/ui_scale+8,std::max(1.f,safe.w/ui_scale-16),std::max(1.f,safe.h/ui_scale-16)};
  const bool wide=area.w>=850;const bool compact=area.h<450;
  const float header=compact?44.f:68.f,footer=compact?108.f:144.f;
  SDL_FRect view{area.x,area.y+header,area.w-(wide?242.f:0.f),std::max(16.f,area.h-header-footer)};
  SDL_GPUTexture* target{};Uint32 pixel_w{},pixel_h{};auto* cmd=gpu->begin(target,pixel_w,pixel_h);if(!cmd)return;
  const float density=pixel_w/std::max(1.f,logical_width);
  ui->begin(density);ui->image(icon,{area.x,area.y+4,30,30});
  ui->text("つかまえて配合！",area.x+40,area.y+1,gold);
  ui->text("SDL3 移植検証・ゲーム本体は開発中",area.x+40,area.y+24,muted,area.w-44);
  const std::string title=std::to_string(number)+"F  シード "+std::to_string(seed);
  ui->text(title,view.x+8,view.y+6,gold);
  buttons.clear();const std::array<std::string,8> labels{"前の階","次の階","別シード","回転",zoom?"標準":"拡大",audio->enabled()?"音 OFF":"音 ON","計測","終了"};
  const std::array<Action,8> actions{Action::previous_floor,Action::next_floor,Action::next_seed,Action::rotate,Action::zoom,Action::sound,Action::metrics,Action::quit};
  const float gap=6,button_w=(area.w-3*gap)/4,button_h=44,by=area.y+area.h-94;
  for(int i=0;i<8;++i){const SDL_FRect rect{area.x+(i%4)*(button_w+gap),by+(i/4)*(button_h+6),button_w,button_h};buttons.push_back({rect,actions[i]});ui->button(rect,labels[i],i==6&&show_metrics);}
  if(wide) {
   const float x=view.x+view.w+16;ui->text("地形・配置の確認",x,view.y,gold);
   minimap({x,view.y+32,216,216});
   ui->text("元HTML: v0.23.10 / v53",x,view.y+264,muted);
   ui->text("v0.23.14との照合待ち",x,view.y+290,muted);
   ui->text("矢印: カメラを移動",x,view.y+328);
   ui->text("E: 回転  Z: 拡大",x,view.y+354);
  }else if(view.w>=300&&view.h>=180)minimap({view.x+view.w-82,view.y+30,74,74});
  if(show_metrics) {
   if(SDL_GetTicks()>=next_metric){next_metric=SDL_GetTicks()+500;const auto p=profiler.stats();std::ostringstream out;out<<std::fixed<<std::setprecision(1)<<"frame "<<p.mean_ms<<" ms / p95 "<<p.p95_ms<<" ms";metric_line=out.str();}
   ui->text(metric_line,area.x,by-28,muted,area.w);
  }
  const double angle=quadrant*std::numbers::pi/2;
  const auto camera=camera_rig(cx,cz,view.w/view.h,quadrant,zoom,angle,view.h<250?2.65:3.6);
  SceneUniform uniform;uniform.matrix=view_projection(camera);uniform.center={cx,cz,1,0};
  for(int n=0;n<8;++n)uniform.torches[n]=n<static_cast<int>(torches.size())?std::array<float,4>{static_cast<float>(torches[n][0]),static_cast<float>(torches[n][1]),static_cast<float>(torches[n][2]),0}:std::array<float,4>{-100,-100,-100,0};
  instances.clear();instances.push_back({&world,uniform});
  auto model=[&](const std::string& key,Cell p,float yaw,float scale,float y){if(!within_view(p.x,p.z,cx,cz,view.w,view.h,zoom,angle))return;auto u=uniform;u.model={static_cast<float>(p.x),y,static_cast<float>(p.z),yaw};u.center[2]=scale;instances.push_back({&models.at(key),u});};
  model("explorer",floor.spawn,std::numbers::pi_v<float>/2,1,.012f);
  for(const auto& e:floor.enemies)model("monster-"+std::to_string(e.species),e.cell,e.boss?-std::numbers::pi_v<float>/2:std::numbers::pi_v<float>,e.boss?1.4f:1.12f,.012f);
  for(const auto& d:floor.drops)model("item-"+std::to_string(d.item_id),d.cell,static_cast<float>(angle+.28),1,.025f);
  const SDL_FRect pixels{view.x/logical_width*pixel_w,view.y/logical_height*pixel_h,view.w/logical_width*pixel_w,view.h/logical_height*pixel_h};
  gpu->draw(cmd,target,pixel_w,pixel_h,pixels,instances,ui->vertices,ui->batches,logical_width,logical_height);
  profiler.end();++frames;
  if(smoke_frames&&frames>=smoke_frames){SDL_Log("GPU smoke passed: frames=%d draws=%llu uploaded=%llu text-cache=%zu (no performance claim)",frames,static_cast<unsigned long long>(gpu->draw_calls),static_cast<unsigned long long>(gpu->uploaded_bytes),ui->cache_size());quit=true;}
 }
};
}
SDL_AppResult SDL_AppInit(void** state,int argc,char** argv) {
 SDL_SetAppMetadata("Capture Breed Native Port","0.1.0","com.capturebreed.nativeport");
 if(!SDL_Init(SDL_INIT_VIDEO))return SDL_APP_FAILURE;
 if(!SDL_InitSubSystem(SDL_INIT_AUDIO))SDL_Log("Audio unavailable: %s",SDL_GetError());
 if(!TTF_Init()){SDL_Log("SDL_ttf: %s",SDL_GetError());return SDL_APP_FAILURE;}
 try{*state=new App(argc,argv);return SDL_APP_CONTINUE;}
 catch(const std::exception& e){SDL_Log("Initialization failed: %s",e.what());return SDL_APP_FAILURE;}
}
SDL_AppResult SDL_AppEvent(void* state,SDL_Event* event) {
 if(!state)return SDL_APP_FAILURE;
 try{auto& app=*static_cast<App*>(state);app.event(*event);return app.quit?SDL_APP_SUCCESS:SDL_APP_CONTINUE;}
 catch(const std::exception& e){SDL_Log("Input failed: %s",e.what());return SDL_APP_FAILURE;}
}
SDL_AppResult SDL_AppIterate(void* state) {
 if(!state)return SDL_APP_FAILURE;
 try{auto& app=*static_cast<App*>(state);if(app.suspended){SDL_Delay(40);return SDL_APP_CONTINUE;}app.draw();return app.quit?SDL_APP_SUCCESS:SDL_APP_CONTINUE;}
 catch(const std::exception& e){SDL_Log("Frame failed: %s",e.what());return SDL_APP_FAILURE;}
}
void SDL_AppQuit(void* state,SDL_AppResult) {delete static_cast<App*>(state);TTF_Quit();SDL_Quit();}
