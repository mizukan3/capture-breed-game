#pragma once
#include "gpu.hpp"
#include <SDL3_ttf/SDL_ttf.h>
#include <memory>
#include <string>
#include <unordered_map>
namespace capture::platform {
struct UiColor {float r{},g{},b{},a{1};};
inline constexpr UiColor text_color{.89f,.914f,.91f,1},gold{.894f,.702f,.424f,1},muted{.596f,.667f,.714f,1};
class Ui {
 struct Cached {Texture texture;std::uint64_t used{};};
 Gpu& gpu_;
 std::string font_path_;
 TTF_Font* font_{};
 Texture white_;
 std::unordered_map<std::string,Cached> cache_;
 float density_{1};
 std::uint64_t clock_{};
 void quad(SDL_GPUTexture*,SDL_FRect,UiColor);
 Cached& cached(const std::string&);
public:
 std::vector<UiVertex> vertices;
 std::vector<UiBatch> batches;
 Ui(Gpu&,std::string font_path);
 ~Ui();
 void begin(float density);
 void rect(SDL_FRect r,UiColor c){quad(white_.texture,r,c);}
 void image(Texture t,SDL_FRect r){quad(t.texture,r,{1,1,1,1});}
 void text(const std::string&,float x,float y,UiColor c=text_color,float max_width=0);
 void button(SDL_FRect,const std::string&,bool selected=false);
 std::size_t cache_size() const{return cache_.size();}
};
}
