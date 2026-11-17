#pragma once
#include <cmath>
#include <cstdint>
namespace capture {
// Exact engine.js xorshift32. Zero is a valid replay state; do not reseed it.
class Rng {
 std::uint32_t state_;
public:
 explicit Rng(std::uint32_t seed):state_(seed){}
 double random() noexcept {
  auto v=state_; v^=v<<13; v^=v>>17; v^=v<<5; state_=v;
  return static_cast<double>(v)/4294967296.0;
 }
 int bounded(int n) noexcept { return static_cast<int>(std::floor(random()*n)); }
 std::uint32_t state() const noexcept { return state_; }
};
}
