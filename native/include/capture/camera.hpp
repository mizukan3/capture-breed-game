#pragma once
#include "types.hpp"
#include <array>
namespace capture {
struct CameraRig {
 std::array<double,3> target{},eye{};double half_width{},half_height{},pitch{};
};
CameraRig camera_rig(double x,double z,double aspect=1,int quadrant=0,double zoom=0,double angle=0,double framing=3.6);
bool within_view(double x,double z,double cx,double cz,double width=390,double height=360,double zoom=0,double angle=0);
std::array<float,16> view_projection(const CameraRig&);
Cell screen_direction(Cell,int quadrant);
}
