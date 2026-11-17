#include "capture/map.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <numeric>
#include <vector>
using namespace capture;
int main(int argc,char** argv) {
 using Clock=std::chrono::steady_clock;
 Data data;std::vector<double> micros;micros.reserve(600);
 std::size_t max_bytes=0;std::uint64_t checksum=0;
 for(int n=0;n<612;++n) {
  const auto start=Clock::now();
  const auto f=generate_floor(static_cast<std::uint32_t>((n+1)*2654435761u),n%6+1);
  const auto end=Clock::now();
  if(n<12)continue;
  micros.push_back(std::chrono::duration<double,std::micro>(end-start).count());
  const auto bytes=f.tiles.capacity()+f.visible.capacity()+f.visited.capacity()+f.rooms.capacity()*sizeof(Room)+f.enemies.capacity()*sizeof(EnemySpawn)+f.drops.capacity()*sizeof(Drop)+f.seen.capacity()*sizeof(int);
  max_bytes=std::max(max_bytes,bytes);checksum+=f.rng_state;
 }
 auto sorted=micros;std::sort(sorted.begin(),sorted.end());
 Json result{{"benchmark","floor-generation"},{"samples",micros.size()},{"warmup",12},
 {"mean_us",std::accumulate(micros.begin(),micros.end(),0.)/micros.size()},
 {"p95_us",sorted[static_cast<std::size_t>((sorted.size()-1)*.95)]},
 {"max_retained_vector_capacity_bytes",max_bytes},{"rng_checksum",checksum},
 {"limitations","CI runner only; excludes Data, temporary BFS allocations, allocator overhead, GPU and process RSS. Not a gameplay FPS claim."}};
 std::cout<<result.dump(2)<<'\n';
 if(argc>1){std::ofstream out(argv[1],std::ios::binary);out<<result.dump(2)<<'\n';if(!out)return 1;}
}
