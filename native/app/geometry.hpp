#pragma once
#include "gpu.hpp"
#include "capture/map.hpp"
#include <array>
namespace capture::platform {
using Vec3=std::array<double,3>;
using Color=std::array<double,3>;
Color rgb(const std::string&);
Color tint(Color,double);
class Geometry {
public:
 std::vector<Vertex> vertices;
 Geometry(){vertices.reserve(65536);}
 void triangle(Vec3,Vec3,Vec3,Color);
 void quad(Vec3,Vec3,Vec3,Vec3,Color);
 void box(double,double,double,double,double,double,Color);
 void bevel_box(double,double,double,double,double,double,Color,double bevel=.035);
 void ellipsoid(double,double,double,double,double,double,Color,int segments=8,int rings=5);
};
struct WorldGeometry {std::vector<Vertex> vertices;std::vector<Vec3> torches;};
WorldGeometry build_world(const Floor&,const Data&,int quadrant);
}
