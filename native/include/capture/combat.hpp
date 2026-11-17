#pragma once
#include "data.hpp"
#include "rng.hpp"
namespace capture {
enum class AttackKind {miss,guard,hit};
struct AttackResult {AttackKind kind;int damage;bool critical;};
AttackResult resolve_attack(Rng&,Stats attacker,Stats defender,bool sure_hit=false,bool unguardable=false,bool frozen=false);
}
