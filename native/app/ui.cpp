#include "ui.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
namespace capture::platform {
Ui::Ui(Gpu& g,std::string font):gpu_(g),font_path_(std::move(font)) {
 vertices.reserve(8192);batches.reserve(256);
 auto* surface=SDL_CreateSurface(1,1,SDL_PIXELFORMAT_RGBA32);
 if(!surface)throw std::runtime_error(SDL_GetError());
 std::memset(surface->pixels,255,static_cast<std::size_t>(surface->pitch));white_=gpu_.upload_surface(surface);SDL_DestroySurface(surface);
}
Ui::~Ui(){for(auto& [key,c]:cache_)gpu_.release(c.texture);gpu_.release(white_);if(font_)TTF_CloseFont(font_);}
void Ui::begin(float density) {
 vertices.clear();batches.clear();++clock_;
 if(!font_||std::abs(density-density_)>.05f) {
  for(auto& [key,c]:cache_)gpu_.release(c.texture);
  cache_.clear();
  if(font_)TTF_CloseFont(font_);
  font_=TTF_OpenFont(font_path_.c_str(),16*density);
  if(!font_)throw std::runtime_error(std::string("Japanese font: ")+SDL_GetError());
  for(const Uint32 codepoint:{0x6355u,0x7372u,0x914du,0x5408u,0x63a2u,0x7d22u,0x8005u})
   if(!TTF_FontHasGlyph(font_,codepoint))throw std::runtime_error("Japanese font glyph missing");
  density_=density;
 }
}
void Ui::quad(SDL_GPUTexture* texture,SDL_FRect r,UiColor c) {
 const auto first=static_cast<Uint32>(vertices.size());
 auto v=[&](float x,float y,float u,float vv){return UiVertex{x,y,u,vv,c.r,c.g,c.b,c.a};};
 const auto a=v(r.x,r.y,0,0),b=v(r.x+r.w,r.y,1,0),cc=v(r.x+r.w,r.y+r.h,1,1),d=v(r.x,r.y+r.h,0,1);
 for(auto vertex:{a,b,cc,a,cc,d})vertices.push_back(vertex);
 if(!batches.empty()&&batches.back().texture==texture)batches.back().count+=6;
 else batches.push_back({texture,first,6});
}
Ui::Cached& Ui::cached(const std::string& text) {
 auto existing=cache_.find(text);if(existing!=cache_.end()){existing->second.used=clock_;return existing->second;}
 if(cache_.size()>=128){auto oldest=std::min_element(cache_.begin(),cache_.end(),[](const auto& a,const auto& b){return a.second.used<b.second.used;});gpu_.release(oldest->second.texture);cache_.erase(oldest);}
 auto* surface=TTF_RenderText_Blended(font_,text.c_str(),text.size(),{255,255,255,255});
 if(!surface)throw std::runtime_error(std::string("UTF-8 text: ")+SDL_GetError());
 if(cache_.size()>=128) {
  auto oldest=cache_.end();
  for(auto it=cache_.begin();it!=cache_.end();++it)if(it->second.used<clock_&&(oldest==cache_.end()||it->second.used<oldest->second.used))oldest=it;
  if(oldest!=cache_.end()){gpu_.release(oldest->second.texture);cache_.erase(oldest);}
 }
 Cached item{gpu_.upload_surface(surface),clock_};SDL_DestroySurface(surface);return cache_.emplace(text,std::move(item)).first->second;
}
void Ui::text(const std::string& value,float x,float y,UiColor color,float max_width) {
 if(value.empty())return;
 const auto& c=cached(value);float w=c.texture.width/density_,h=c.texture.height/density_;
 if(max_width>0&&w>max_width){h*=max_width/w;w=max_width;}
 quad(c.texture.texture,{x,y,w,h},color);
}
void Ui::button(SDL_FRect r,const std::string& label,bool selected) {
 rect(r,selected?UiColor{.23f,.28f,.29f,1}:UiColor{.102f,.165f,.22f,1});
 rect({r.x,r.y,r.w,1},selected?gold:UiColor{.169f,.231f,.286f,1});
 const auto& c=cached(label);const float w=c.texture.width/density_,h=c.texture.height/density_;
 text(label,r.x+(r.w-w)/2,r.y+(r.h-h)/2,selected?gold:text_color);
}
}
