#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace capture {
struct Cell { int x{}, z{}; bool operator==(const Cell&) const = default; };
inline constexpr std::array<Cell,8> directions{{
 {0,-1},{1,0},{0,1},{-1,0},{-1,-1},{1,-1},{1,1},{-1,1}
}};
inline int distance(Cell a,Cell b) { return std::max(std::abs(a.x-b.x),std::abs(a.z-b.z)); }
struct Stats {
 int max_hp{}, attack{}, defense{}, evasion{}, accuracy{}, critical{}, max_mp{50}, guard_rate{};
 bool operator==(const Stats&) const = default;
};
}
