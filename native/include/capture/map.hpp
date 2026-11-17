#pragma once
#include "data.hpp"
#include "rng.hpp"
#include <vector>
namespace capture {
struct Room { int x{},z{},w{},h{},cx{},cz{}; };
struct EnemySpawn { int uid{},species{},level{1}; Cell cell; bool boss{}; };
struct Drop { int uid{},item_id{};Cell cell; };
struct Distances {std::vector<int> values;std::vector<Cell> order;};
struct Floor {
 int number{1},size{},next_uid{11}; std::uint32_t rng_state{};
 Cell spawn,stairs;std::vector<std::uint8_t> tiles,visible,visited;
 std::vector<Room> rooms;std::vector<EnemySpawn> enemies;std::vector<Drop> drops;std::vector<int> seen;
 bool walkable(Cell) const noexcept;
 bool can_step(Cell from,Cell to) const noexcept;
 bool line_of_sight(Cell from,Cell to) const noexcept;
 Distances reachable(Cell from) const;
 void reveal(Cell from);
};
// The fresh game consumes UID 10 for its initial capsule. Later floors pass the current UID.
Floor generate_floor(std::uint32_t seed,int number,int next_uid=11);
Json floor_snapshot(const Floor&,const Data&);
Json stats_json(Stats);
}
