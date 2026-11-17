#include "capture/data.hpp"
#include "game_data.hpp"
namespace capture {
Data::Data():document_(Json::parse(embedded_game_data)) {
 for(const auto& j:document_.at("species")) {
  Species s; s.id=j.at("id"); s.family=j.at("family"); s.name=j.at("name"); s.capture_rule=j.at("captureRule");
  const auto& b=j.at("base");
  s.base={b.at("hp"),b.at("atk"),b.at("def"),b.at("evasion"),b.at("accuracy"),b.at("critical")};
  for(int i=0;i<3;++i) {
   const std::array<const char*,3> keys{"hp","atk","def"};
   s.targets[i]=j.at("targets").at(keys[i]);s.growth[i]=j.at("growth").at(keys[i]);
  }
  s.steps={j.at("steps").at("evasion"),j.at("steps").at("accuracy"),j.at("steps").at("critical")};
  species_.push_back(std::move(s));
 }
}
}
