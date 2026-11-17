#pragma once
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <numeric>
namespace capture::platform {
class Profiler {
 std::array<double,240> frame_{},cpu_{};
 std::size_t next_{},count_{};
 Uint64 start_{},previous_{};
public:
 struct Stats {double fps{},mean_ms{},p95_ms{},cpu_mean_ms{};std::size_t frames{};};
 void reset(){next_=count_=0;start_=previous_=0;frame_.fill(0);cpu_.fill(0);}
 void begin(){start_=SDL_GetPerformanceCounter();}
 void end() {
  const auto now=SDL_GetPerformanceCounter();const double hz=static_cast<double>(SDL_GetPerformanceFrequency());
  if(previous_) {
   cpu_[next_]=(now-start_)*1000/hz;frame_[next_]=(now-previous_)*1000/hz;
   next_=(next_+1)%frame_.size();count_=std::min(count_+1,frame_.size());
  }
  previous_=now;
 }
 Stats stats() const {
  if(!count_)return {};
  auto sorted=frame_;std::sort(sorted.begin(),sorted.begin()+static_cast<std::ptrdiff_t>(count_));
  const double mean=std::accumulate(frame_.begin(),frame_.begin()+static_cast<std::ptrdiff_t>(count_),0.)/count_;
  return {mean>0?1000/mean:0,mean,sorted[static_cast<std::size_t>((count_-1)*.95)],std::accumulate(cpu_.begin(),cpu_.begin()+static_cast<std::ptrdiff_t>(count_),0.)/count_,count_};
 }
};
}
