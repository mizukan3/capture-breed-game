#include "geometry.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
namespace capture::platform {
Color rgb(const std::string& s){return {std::stoi(s.substr(1,2),nullptr,16)/255.,std::stoi(s.substr(3,2),nullptr,16)/255.,std::stoi(s.substr(5,2),nullptr,16)/255.};}
Color tint(Color a,double t){for(auto& x:a)x=std::min(1.,x*t);return a;}
void Geometry::triangle(Vec3 a,Vec3 b,Vec3 c,Color color) {
 Vec3 u{},v{};for(int i=0;i<3;++i){u[i]=b[i]-a[i];v[i]=c[i]-a[i];}
 Vec3 n{u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]};
 const double len=std::hypot(n[0],n[1],n[2]);if(len)for(auto& x:n)x/=len;
 const double light=std::sqrt(.4*.4+1+.6*.6),dot=(n[0]*-.4+n[1]+n[2]*.6)/light;
 const auto co=tint(color,.68+std::max(0.,dot)*.32);
 for(auto p:{a,b,c})vertices.push_back({static_cast<float>(p[0]),static_cast<float>(p[1]),static_cast<float>(p[2]),static_cast<float>(co[0]),static_cast<float>(co[1]),static_cast<float>(co[2])});
}
void Geometry::quad(Vec3 a,Vec3 b,Vec3 c,Vec3 d,Color t){triangle(a,b,c,t);triangle(a,c,d,t);}
void Geometry::box(double x,double y,double z,double w,double h,double d,Color t) {
 const double x0=x-w/2,x1=x+w/2,y0=y-h/2,y1=y+h/2,z0=z-d/2,z1=z+d/2;
 quad({x0,y1,z0},{x0,y1,z1},{x1,y1,z1},{x1,y1,z0},t);
 quad({x0,y0,z1},{x0,y1,z1},{x0,y1,z0},{x0,y0,z0},t);
 quad({x1,y0,z0},{x1,y1,z0},{x1,y1,z1},{x1,y0,z1},t);
 quad({x0,y0,z1},{x1,y0,z1},{x1,y1,z1},{x0,y1,z1},t);
 quad({x1,y0,z0},{x0,y0,z0},{x0,y1,z0},{x1,y1,z0},t);
 quad({x0,y0,z0},{x1,y0,z0},{x1,y0,z1},{x0,y0,z1},t);
}
void Geometry::bevel_box(double x,double y,double z,double w,double h,double d,Color t,double bevel) {
 box(x,y-bevel/2,z,w,h-bevel,d,t);
 const double lx=x-w/2,rx=x+w/2,nz=z-d/2,fz=z+d/2;
 const std::array<std::array<double,2>,8> ring{{{lx+bevel,nz},{rx-bevel,nz},{rx,nz+bevel},{rx,fz-bevel},{rx-bevel,fz},{lx+bevel,fz},{lx,fz-bevel},{lx,nz+bevel}}};
 auto inner=ring;for(auto& p:inner){p[0]+=p[0]<x?bevel:-bevel;p[1]+=p[1]<z?bevel:-bevel;}
 for(int n=0;n<8;++n){const int b=(n+1)%8;const auto a=ring[n],c=ring[b],p=inner[n],q=inner[b];const double top=y+h/2;
  quad({a[0],top-bevel,a[1]},{p[0],top,p[1]},{q[0],top,q[1]},{c[0],top-bevel,c[1]},t);
  triangle({x,top,z},{q[0],top,q[1]},{p[0],top,p[1]},tint(t,1.03));
 }
}
void Geometry::ellipsoid(double x,double y,double z,double rx,double ry,double rz,Color t,int segments,int rings) {
 auto pos=[&](int i,int j)->Vec3{const double th=std::numbers::pi*i/rings,ph=std::numbers::pi*2*j/segments;return {x+rx*std::sin(th)*std::cos(ph),y+ry*std::cos(th),z+rz*std::sin(th)*std::sin(ph)};};
 for(int i=0;i<rings;++i)for(int j=0;j<segments;++j)quad(pos(i,j),pos(i,j+1),pos(i+1,j+1),pos(i+1,j),t);
}
WorldGeometry build_world(const Floor& f,const Data& data,int quadrant) {
 Geometry g;WorldGeometry result;
 const auto theme=data.json().at("themes").at("moss");
 const Color floor=rgb(theme.at("floor")),wall=rgb(theme.at("wall")),accent=rgb(theme.at("accent"));
 auto floor_stone=[&](int x,int z){
  const auto co=tint(floor,1.24+((x*17+z*23)%11)*.012);
  g.bevel_box(x,-.14,z,.965,.25,.965,co,.045);const int hash=(x*17+z*13)%13;
  if(hash<4){const auto seam=tint(co,.71);g.box(x+.12,.001,z-.03,.025,.009,.61,seam);g.box(x-.15,.002,z+.16,.5,.009,.017,seam);}
  if(hash==6){g.box(x-.2,.003,z+.14,.24,.01,.012,tint(co,.65));g.box(x-.08,.003,z+.2,.012,.01,.13,tint(co,.65));}
 };
 auto wall_stone=[&](int x,int z,double height,bool rear){
  const auto co=tint(wall,.94+((x*19+z*7)%7)*.023);const int layers=std::max(1,static_cast<int>(std::floor(height/.3+.5)));const double dh=height/layers;
  for(int n=0;n<layers;++n){const double y=-.1+dh*(n+.5),offset=n%2?.06:-.06;g.bevel_box(x-.25+offset,y,z,.49,dh-.018,.965,co,.025);g.bevel_box(x+.25+offset,y,z,.49,dh-.018,.965,tint(co,1.035),.025);}
  g.bevel_box(x,height-.07,z,1.04,.13,1.025,tint(co,1.14),.035);
  if(rear&&(x*7+z*11)%17==0)g.box(x,height*.5,z+.49,.07,height*.44,.012,tint(co,.62));
 };
 const double angle=quadrant*std::numbers::pi/2;
 for(int z=1;z<f.size-1;++z)for(int x=1;x<f.size-1;++x) {
  if(f.walkable({x,z})){floor_stone(x,z);continue;}
  std::vector<Cell> edges;for(int i=0;i<4;++i){const Cell d=directions[i];if(f.walkable({x+d.x,z+d.z}))edges.push_back(d);}
  if(edges.empty())continue;
  const bool front=std::any_of(edges.begin(),edges.end(),[&](Cell d){return d.x*std::sin(angle)+d.z*std::cos(angle)>.5;});
  const bool rear=!front;const double h=front?.56:1.06;wall_stone(x,z,h,rear);
  if((x*3+z*5)%11==0&&rear){g.bevel_box(x,h+.17,z,.61,.45,.61,tint(wall,.96));g.bevel_box(x,h+.41,z,.78,.11,.78,tint(wall,1.18));}
  const Cell edge=edges.front();
  if(rear&&(x*7+z*3)%9==0){
   const double tx=x+edge.x*.54,tz=z+edge.z*.54,ty=h*.66+.12;
   g.box(tx,ty-.1,tz,.075,.31,.075,rgb("#72563d"));
   g.ellipsoid(tx,ty,tz,.13,.075,.13,rgb("#8c795d"),8,3);
   g.box(tx-edge.x*.09,ty-.17,tz-edge.z*.09,std::abs(edge.x)*.23+.075,.065,std::abs(edge.z)*.23+.075,rgb("#635b4b"));
   result.torches.push_back({tx,ty+.14,tz});
  }
  if((x*11+z*7)%8==0)g.ellipsoid(x+edge.x*.49,.13,z+edge.z*.49,std::abs(edge.x)*.02+.15,.04,std::abs(edge.z)*.02+.15,tint(accent,.68),6,3);
 }
 const double x=f.stairs.x,z=f.stairs.z;
 g.box(x,-.06,z,.88,.15,.88,rgb("#28393b"));
 for(int n=0;n<5;++n)g.box(x,.04-n*.035,z-.34+n*.16,.78,.08,.15,rgb("#e8c18a"));
 g.box(x-.47,.12,z,.09,.3,.99,wall);g.box(x+.47,.12,z,.09,.3,.99,wall);
 for(int j=0;j<20;++j){const double a=j*std::numbers::pi*2/20,b=(j+1)*std::numbers::pi*2/20,r=.68,rr=r-.028;
  g.quad({x+r*std::cos(a),.014,z+r*std::sin(a)},{x+rr*std::cos(a),.014,z+rr*std::sin(a)},{x+rr*std::cos(b),.014,z+rr*std::sin(b)},{x+r*std::cos(b),.014,z+r*std::sin(b)},rgb("#f4cf7e"));
 }
 std::stable_sort(result.torches.begin(),result.torches.end(),[&](Vec3 a,Vec3 b){return std::hypot(a[0]-f.spawn.x,a[2]-f.spawn.z)<std::hypot(b[0]-f.spawn.x,b[2]-f.spawn.z);});
 result.vertices=std::move(g.vertices);return result;
}
}
