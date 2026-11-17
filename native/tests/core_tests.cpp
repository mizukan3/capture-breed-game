#include "capture/map.hpp"
#include "capture/checksum.hpp"
#include "capture/combat.hpp"
#include <iostream>
#include <stdexcept>
using namespace capture;
void check(bool b,const char* message){if(!b)throw std::runtime_error(message);}
int main() {
 try {
  Data d;check(d.species_count()==5,"reference species");check(player_stats(1).max_hp==15,"initial HP");
  check(normal_damage(5,3)==4,"damage formula");check(normal_damage(0,0)==1,"zero stats damage");
  check(!capture_condition(d,4,1,25),"boss capture forbidden");check(capture_condition(d,0,4,8),"half HP capture boundary");
  Floor f;f.size=3;f.tiles={0,0,0,0,1,0,0,0,1};check(!f.can_step({1,1},{2,2}),"no diagonal corner cutting");
  check(!f.line_of_sight({1,1},{2,2}),"corner blocks sight");
  const auto floor=generate_floor(13,6);check(floor.size==21&&floor.enemies.size()==1&&floor.enemies[0].species==4,"boss floor");
  check(save_checksum("")=="811c9dc5","empty checksum");
  bool invalid=false;try{save_checksum(std::string("\xc0\xaf",2));}catch(...){invalid=true;}check(invalid,"reject overlong UTF-8");
  Rng rng(0);check(rng.random()==0&&rng.state()==0,"zero replay state");
  std::cout<<"Core invariants passed\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
