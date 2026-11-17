#include "capture/map.hpp"
#include <algorithm>
#include <stdexcept>
namespace capture {
bool Floor::walkable(Cell a) const noexcept {
 return a.x>=0&&a.z>=0&&a.x<size&&a.z<size&&tiles[static_cast<std::size_t>(a.z*size+a.x)]!=0;
}
bool Floor::can_step(Cell a,Cell b) const noexcept {
 const int dx=b.x-a.x,dz=b.z-a.z;
 return distance(a,b)==1&&walkable(b)&&(!dx||!dz||(walkable({a.x+dx,a.z})&&walkable({a.x,a.z+dz})));
}
bool Floor::line_of_sight(Cell a,Cell b) const noexcept {
 const int dx=std::abs(b.x-a.x),dz=std::abs(b.z-a.z),sx=(b.x>a.x)-(b.x<a.x),sz=(b.z>a.z)-(b.z<a.z);
 int x=a.x,z=a.z,ix=0,iz=0;
 for(int n=0;n<160;++n) {
  if(x==b.x&&z==b.z)return true;
  const int decision=(1+2*ix)*dz-(1+2*iz)*dx;
  if(decision==0){if(!walkable({x+sx,z})||!walkable({x,z+sz}))return false;x+=sx;z+=sz;++ix;++iz;}
  else if(decision<0){x+=sx;++ix;}else{z+=sz;++iz;}
  if(x==b.x&&z==b.z)return true;
  if(!walkable({x,z}))return false;
 }
 return false;
}
Distances Floor::reachable(Cell from) const {
 Distances result;result.values.assign(static_cast<std::size_t>(size*size),-1);
 result.order.reserve(static_cast<std::size_t>(size*size));result.order.push_back(from);
 result.values[static_cast<std::size_t>(from.z*size+from.x)]=0;
 for(std::size_t n=0;n<result.order.size();++n) {
  const Cell p=result.order[n];
  for(Cell d:directions) {
   const Cell a{p.x+d.x,p.z+d.z};
   if(!can_step(p,a))continue;
   const auto index=static_cast<std::size_t>(a.z*size+a.x);
   if(result.values[index]>=0)continue;
   result.values[index]=result.values[static_cast<std::size_t>(p.z*size+p.x)]+1;
   result.order.push_back(a);
  }
 }
 return result;
}
void Floor::reveal(Cell p) {
 std::fill(visible.begin(),visible.end(),std::uint8_t{0});
 for(int z=std::max(0,p.z-7);z<=std::min(size-1,p.z+7);++z)
 for(int x=std::max(0,p.x-7);x<=std::min(size-1,p.x+7);++x)
 if((x-p.x)*(x-p.x)+(z-p.z)*(z-p.z)<=49&&line_of_sight(p,{x,z})) {
  const auto i=static_cast<std::size_t>(z*size+x);visible[i]=1;visited[i]=1;
 }
 for(const auto& e:enemies)if(visible[static_cast<std::size_t>(e.cell.z*size+e.cell.x)]&&std::find(seen.begin(),seen.end(),e.species)==seen.end())seen.push_back(e.species);
}
Floor generate_floor(std::uint32_t seed,int number,int next_uid) {
 if(number<1||number>6)throw std::invalid_argument("floor must be 1..6");
 Rng rng(seed);Floor f;f.next_uid=next_uid;f.number=number;const bool boss=number==6;f.size=boss?21:29;
 f.tiles.assign(static_cast<std::size_t>(f.size*f.size),0);f.visible=f.tiles;f.visited=f.tiles;
 auto carve=[&](int x,int z,int w,int h){for(int zz=z;zz<z+h;++zz)for(int xx=x;xx<x+w;++xx)f.tiles[static_cast<std::size_t>(zz*f.size+xx)]=1;};
 if(boss){carve(2,2,17,17);f.rooms.push_back({2,2,17,17,10,10});}
 else {
  carve(3,3,9,8);f.rooms.push_back({3,3,9,8,7,7});
  for(int n=0;n<160&&f.rooms.size()<8;++n) {
   // Each draw is sequenced explicitly; C++ function-argument evaluation order is not JS order.
   const int w=4+rng.bounded(5);const int h=4+rng.bounded(4);
   const int x=2+rng.bounded(f.size-w-4);const int z=2+rng.bounded(f.size-h-4);
   const bool overlaps=std::any_of(f.rooms.begin(),f.rooms.end(),[&](const Room& r){return x<r.x+r.w+1&&x+w+1>r.x&&z<r.z+r.h+1&&z+h+1>r.z;});
   if(overlaps)continue;
   const Room room{x,z,w,h,x+w/2,z+h/2};
   const Room prior=f.rooms[static_cast<std::size_t>(rng.bounded(static_cast<int>(f.rooms.size())))];
   carve(x,z,w,h);int xx=prior.cx,zz=prior.cz;
   while(xx!=room.cx){f.tiles[static_cast<std::size_t>(zz*f.size+xx)]=1;xx+=(room.cx>xx)-(room.cx<xx);}
   while(zz!=room.cz){f.tiles[static_cast<std::size_t>(zz*f.size+xx)]=1;zz+=(room.cz>zz)-(room.cz<zz);}
   f.tiles[static_cast<std::size_t>(zz*f.size+xx)]=1;f.rooms.push_back(room);
  }
 }
 f.spawn={7,boss?10:7};const auto reachable=f.reachable(f.spawn);
 std::vector<Cell> points,interiors;points.reserve(reachable.order.size());
 for(Cell p:reachable.order)if(distance(p,f.spawn)>1) {
  points.push_back(p);
  if(std::any_of(f.rooms.begin(),f.rooms.end(),[&](const Room& r){return p.x>r.x&&p.x<r.x+r.w-1&&p.z>r.z&&p.z<r.z+r.h-1;}))interiors.push_back(p);
 }
 std::stable_sort(interiors.begin(),interiors.end(),[&](Cell a,Cell b){return reachable.values[static_cast<std::size_t>(b.z*f.size+b.x)]<reachable.values[static_cast<std::size_t>(a.z*f.size+a.x)];});
 f.stairs=boss?Cell{16,10}:interiors.empty()?Cell{9,7}:interiors.front();
 std::vector<std::uint8_t> used(f.tiles.size(),0);used[static_cast<std::size_t>(f.spawn.z*f.size+f.spawn.x)]=1;used[static_cast<std::size_t>(f.stairs.z*f.size+f.stairs.x)]=1;
 if(boss)f.enemies.push_back({f.next_uid++,4,1,{10,10},true});
 else {
  std::vector<int> ids;
  if(number==1)ids={0};
 if(number==2)ids={0,1};
 if(number==3)ids={0,1,2};
 if(number==5)ids={0,1,2,3};
  if(number==4)for(int n=0;n<3;++n) {
   double roll=rng.random()*3;int id=2;for(int i=0;i<3;++i){roll-=1;
 if(roll<0){id=i;break;}}ids.push_back(id);
  }
  for(int id:ids) {
   std::vector<Cell> available;for(Cell p:points)if(distance(p,f.spawn)>=4&&!used[static_cast<std::size_t>(p.z*f.size+p.x)])available.push_back(p);
   const int i=rng.bounded(static_cast<int>(available.size()));
   if(available.empty())throw std::runtime_error("no enemy placement");
   const Cell p=available[static_cast<std::size_t>(i)];used[static_cast<std::size_t>(p.z*f.size+p.x)]=1;
   f.enemies.push_back({f.next_uid++,id,1,p,false});
  }
  const int gear=number==1?3:number==2?4:rng.random()<.5?3:4;
  std::vector<int> item_ids{gear,0};
 if(number==1)item_ids.push_back(0);
  if(number==1)item_ids.push_back(1);
  else {const double r=rng.random();
 if(r>=.95){item_ids.push_back(1);item_ids.push_back(1);}else if(r>=.85)item_ids.push_back(1);}
  const double crystal=rng.random();
 if(crystal>=.995){item_ids.push_back(6);item_ids.push_back(6);}else if(crystal>=.945)item_ids.push_back(6);
  for(int id:item_ids) {
   std::vector<Cell> available;for(Cell p:points)if(!used[static_cast<std::size_t>(p.z*f.size+p.x)])available.push_back(p);
   const int i=rng.bounded(static_cast<int>(available.size()));
   if(available.empty())throw std::runtime_error("no loot placement");
   const Cell p=available[static_cast<std::size_t>(i)];used[static_cast<std::size_t>(p.z*f.size+p.x)]=1;
   f.drops.push_back({f.next_uid++,id,p});
  }
 }
 f.rng_state=rng.state();f.reveal(f.spawn);return f;
}
Json stats_json(Stats s) {return {{"maxHp",s.max_hp},{"atk",s.attack},{"def",s.defense},{"evasion",s.evasion},{"accuracy",s.accuracy},{"critical",s.critical},{"maxMp",s.max_mp}};}
Json floor_snapshot(const Floor& f,const Data& d) {
 auto rooms=Json::array(),enemies=Json::array(),drops=Json::array(),visible=Json::array(),visited=Json::array();
 for(const auto& r:f.rooms)rooms.push_back({{"x",r.x},{"z",r.z},{"w",r.w},{"h",r.h},{"cx",r.cx},{"cz",r.cz}});
 for(const auto& e:f.enemies){const auto s=species_stats(d,e.species,e.level);enemies.push_back({{"uid",e.uid},{"species",e.species},{"level",e.level},{"x",e.cell.x},{"z",e.cell.z},{"hp",s.max_hp},{"maxHp",s.max_hp},{"atk",s.attack},{"def",s.defense},{"boss",e.boss}});}
 for(const auto& a:f.drops)drops.push_back({{"uid",a.uid},{"x",a.cell.x},{"z",a.cell.z},{"itemId",a.item_id},{"gold",0}});
 for(auto x:f.visible)visible.push_back(x!=0);
 for(auto x:f.visited)visited.push_back(x!=0);
 return {{"size",f.size},{"tiles",f.tiles},{"visible",visible},{"visited",visited},{"rooms",rooms},
 {"stairs",{{"x",f.stairs.x},{"z",f.stairs.z}}},{"spawn",{{"x",f.spawn.x},{"z",f.spawn.z}}},
 {"enemies",enemies},{"drops",drops},{"rng",f.rng_state},{"uid",f.next_uid},{"seen",f.seen}};
}
}
