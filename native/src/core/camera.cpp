#include "capture/camera.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
namespace capture {
CameraRig camera_rig(double x,double z,double aspect,int quadrant,double zoom,double angle,double framing) {
 (void)quadrant;const double width=std::max(3.55,framing*aspect)*(1+.32*zoom);
 const double pitch=std::numbers::pi*55/180,d=19;
 return {{x,.15,z},{x+std::sin(angle)*std::cos(pitch)*d,.15+std::sin(pitch)*d,z+std::cos(angle)*std::cos(pitch)*d},width,width/aspect,pitch};
}
bool within_view(double x,double z,double cx,double cz,double w,double h,double zoom,double angle) {
 if(!std::isfinite(x)||!std::isfinite(z))return false;
 const auto c=camera_rig(cx,cz,w/std::max(1.,h),0,zoom,angle,h<250?2.65:3.6);
 const double dx=x-cx,dz=z-cz,horizontal=dx*std::cos(angle)-dz*std::sin(angle);
 const double depth=dx*std::sin(angle)+dz*std::cos(angle),bottom=-depth*std::sin(c.pitch)-.15*std::cos(c.pitch),top=bottom+3*std::cos(c.pitch);
 return std::abs(horizontal)<=c.half_width+.8&&bottom<=c.half_height+.8&&top>=-c.half_height-.8;
}
std::array<float,16> view_projection(const CameraRig& c) {
 using V=std::array<double,3>;
 auto norm=[](V a){const double l=std::hypot(a[0],a[1],a[2]);for(auto& x:a)x/=l;return a;};
 auto cross=[](V a,V b)->V{return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};};
 auto dot=[](V a,V b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];};
 V z{};for(int i=0;i<3;++i)z[i]=c.eye[i]-c.target[i];z=norm(z);
 const V x=norm(cross({0,1,0},z)),y=cross(z,x);
 const std::array<double,16> view{x[0],y[0],z[0],0,x[1],y[1],z[1],0,x[2],y[2],z[2],0,-dot(x,c.eye),-dot(y,c.eye),-dot(z,c.eye),1};
 // SDL_GPU depth is [0,1], unlike the source WebGL [-1,1].
 constexpr double near=.1,far=80;
 const std::array<double,16> projection{1/c.half_width,0,0,0,0,1/c.half_height,0,0,0,0,-1/(far-near),0,0,0,-near/(far-near),1};
 std::array<float,16> result{};for(int col=0;col<4;++col)for(int row=0;row<4;++row)for(int k=0;k<4;++k)result[col*4+row]+=static_cast<float>(projection[k*4+row]*view[col*4+k]);
 return result;
}
Cell screen_direction(Cell a,int q) {for(int n=0;n<(q%4+4)%4;++n)a={a.z,-a.x};return a;}
}
