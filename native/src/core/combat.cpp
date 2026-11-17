#include "capture/combat.hpp"
#include <algorithm>
#include <cmath>
namespace capture {
AttackResult resolve_attack(Rng& rng,Stats a,Stats d,bool sure_hit,bool unguardable,bool frozen) {
 if(!sure_hit&&rng.random()*100>=std::clamp(a.accuracy-d.evasion,0,100))return {AttackKind::miss,0,false};
 // Preserve this draw even for a zero guard rate.
 if(!unguardable&&rng.random()*100<d.guard_rate)return {AttackKind::guard,0,false};
 const bool critical=rng.random()*100<a.critical;
 int n=normal_damage(a.attack,d.defense);if(critical)n=static_cast<int>(std::floor(n*1.5));if(frozen)n=static_cast<int>(std::floor(n*1.5));
 return {AttackKind::hit,n,critical};
}
}
