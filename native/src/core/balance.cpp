#include "capture/data.hpp"
#include <algorithm>
#include <cmath>
namespace capture {
int growth_value(int a,int b,int level,double p) {
 const double progress=(std::clamp(level,1,100)-1)/99.0;
 return static_cast<int>(std::floor(a+(b-a)*std::pow(progress,p)));
}
Stats species_stats(const Data& data,int id,int level) {
 const auto& s=data.species(id);const int l=std::clamp(level,1,100),step=l/20;
 return {growth_value(s.base[0],s.targets[0],l,s.growth[0]),
 growth_value(s.base[1],s.targets[1],l,s.growth[1]),
 growth_value(s.base[2],s.targets[2],l,s.growth[2]),
 std::min(100,s.base[3]+s.steps[0]*step),std::min(100,s.base[4]+s.steps[1]*step),
 std::min(100,s.base[5]+s.steps[2]*step),50,0};
}
Stats player_stats(int l) {
 return {growth_value(15,300,l),growth_value(5,300,l),growth_value(5,300,l),
 5+static_cast<int>(std::floor(l/20.0)),100,5+2*static_cast<int>(std::floor(l/20.0)),50,0};
}
int normal_damage(int a,int d) {
 if(a+d==0)return 1;
 return std::max(1,static_cast<int>(std::floor(1+static_cast<double>(a)*a/(a+d))));
}
int xp_needed(int l,bool monster) { return (monster?15:5)*l*(l+1)/2; }
Stats apply_modifiers(Stats s,Modifiers m) {
 constexpr double stages[]{.5,.75,1.,1.25,1.5};
 s.attack=static_cast<int>(std::floor(s.attack*stages[std::clamp(m.attack_stage,-2,2)+2]));
 s.defense=static_cast<int>(std::floor(s.defense*stages[std::clamp(m.defense_stage,-2,2)+2]));
 if(m.soft)s.defense=0;
 if(m.fortress)s.defense*=5;
 if(m.blind)s.accuracy=static_cast<int>(std::floor(s.accuracy*.25));
 if(m.gravity)s.evasion=static_cast<int>(std::floor(s.evasion*.1));
 if(m.flinch)s.evasion=0;
 if(m.dodge)s.evasion=100;
 if(m.decoy)s.evasion=0;
 s.guard_rate=std::clamp(s.guard_rate+m.guard_rate,0,100);return s;
}
bool capture_condition(const Data& d,int id,int hp,int max_hp,bool boss,bool summoned,bool decoy,bool fear,bool trait_fear) {
 const auto& rule=d.species(id).capture_rule;
 return hp>0&&!boss&&!summoned&&!decoy&&rule!="none"&&(rule=="fear"?(fear||trait_fear):hp<=max_hp*.5);
}
}
