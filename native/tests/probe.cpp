#include "capture/camera.hpp"
#include "capture/checksum.hpp"
#include "capture/combat.hpp"
#include "capture/map.hpp"
#include <iostream>
#include <string>
using namespace capture;
namespace {
Stats read_stats(const Json& j) {
 return {j.value("maxHp",15),j.value("atk",5),j.value("def",5),j.value("evasion",5),j.value("accuracy",100),j.value("critical",5),50,j.value("guardRate",0)};
}
Json evaluate(const Json& j,const Data& d) {
 const auto op=j.at("op").get<std::string>();
 if(op=="data")return d.json();
 if(op=="stats")return stats_json(species_stats(d,j.at("species"),j.at("level")));
 if(op=="player"){auto r=stats_json(player_stats(j.at("level")));r.erase("maxMp");return r;}
 if(op=="damage")return normal_damage(j.at("attack"),j.at("defense"));
 if(op=="growth")return growth_value(j.at("initial"),j.at("target"),j.at("level"),j.value("p",1.2));
 if(op=="xp")return xp_needed(j.at("level"),j.value("monster",false));
 if(op=="rng"){Rng r(j.at("seed"));Json out=Json::array();for(int n=0;n<j.value("count",32);++n)out.push_back(r.random());return {{"values",out},{"state",r.state()}};}
 if(op=="floor")return floor_snapshot(generate_floor(j.at("seed"),j.at("floor")),d);
 if(op=="checksum")return save_checksum(j.at("text").get<std::string>());
 if(op=="camera"){const auto c=camera_rig(j.value("x",0.),j.value("z",0.),j.value("aspect",1.),0,j.value("zoom",0.),j.value("angle",0.),j.value("framing",3.6));return {{"target",c.target},{"eye",c.eye},{"halfWidth",c.half_width},{"halfHeight",c.half_height},{"pitch",c.pitch}};}
 if(op=="attack"){Rng rng(j.at("seed"));const auto r=resolve_attack(rng,read_stats(j.at("attacker")),read_stats(j.at("defender")),j.value("sureHit",false),j.value("unguardable",false),j.value("frozen",false));return {{"kind",r.kind==AttackKind::miss?"miss":r.kind==AttackKind::guard?"guard":"hit"},{"damage",r.damage},{"critical",r.critical},{"rng",rng.state()}};}
 if(op=="sight"){auto f=generate_floor(j.at("seed"),j.at("floor"));Json out=Json::array();for(int z=0;z<f.size;++z)for(int x=0;x<f.size;++x)out.push_back(f.line_of_sight({j.at("x"),j.at("z")},{x,z}));return out;}
 if(op=="capture")return capture_condition(d,j.at("species"),j.at("hp"),j.at("maxHp"),j.value("boss",false),j.value("summoned",false),j.value("decoy",false),j.value("fear",false),j.value("traitFear",false));
 throw std::invalid_argument("unknown probe operation");
}
}
int main() {
 try {const Data data;std::string line;while(std::getline(std::cin,line))if(!line.empty())std::cout<<evaluate(Json::parse(line),data).dump()<<'\n';}
 catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
