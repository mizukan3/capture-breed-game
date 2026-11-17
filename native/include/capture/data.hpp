#pragma once
#include "types.hpp"
#include <nlohmann/json.hpp>
#include <array>
#include <string>
#include <vector>
namespace capture {
using Json=nlohmann::json;
struct Species {
 int id{},family{};
 std::string name,capture_rule;
 std::array<int,6> base{};
 std::array<int,3> targets{},steps{};
 std::array<double,3> growth{};
};
class Data {
 Json document_;
 std::vector<Species> species_;
public:
 Data();
 const Json& json() const noexcept { return document_; }
 const Species& species(int id) const { return species_.at(static_cast<std::size_t>(id)); }
 std::size_t species_count() const noexcept { return species_.size(); }
};
int growth_value(int initial,int target,int level,double exponent=1.2);
Stats species_stats(const Data&,int id,int level=1);
Stats player_stats(int level);
int normal_damage(int attack,int defense);
int xp_needed(int level,bool monster=false);
struct Modifiers {
 int attack_stage{},defense_stage{},guard_rate{};
 bool soft{},fortress{},blind{},gravity{},flinch{},dodge{},decoy{};
};
Stats apply_modifiers(Stats,Modifiers);
bool capture_condition(const Data&,int species,int hp,int max_hp,bool boss=false,
 bool summoned=false,bool decoy=false,bool fear=false,bool trait_fear=false);
}
