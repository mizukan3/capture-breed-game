#pragma once
#include "data.hpp"
#include <functional>
#include <map>
#include <optional>
#include <string>
namespace capture {
class KeyValueStore {
public:
 virtual ~KeyValueStore()=default;
 virtual std::optional<std::string> get(const std::string& key) const=0;
 virtual void put(const std::string& key,const std::string& value)=0;
 virtual void erase(const std::string& key)=0;
};
class MemoryStore:public KeyValueStore {
public:
 std::map<std::string,std::string> entries;
 std::optional<std::string> get(const std::string&) const override;
 void put(const std::string&,const std::string&) override;
 void erase(const std::string&) override;
};
using SaveValidator=std::function<bool(const Json&)>;
using SaveClock=std::function<std::string()>;
struct SaveCandidate {
 bool valid{},missing{},legacy{};
 std::optional<std::string> payload;
 Json state,meta;
 std::int64_t sequence{-1};
 std::string reason;
};
struct SaveInspection {
 std::string source{"none"};std::optional<std::string> payload;Json state;
 std::int64_t sequence{};
 bool recovered{},legacy{},has_backup{},primary_valid{},corrupt{},written{};
 SaveCandidate primary,backup,staging;
};
class SaveStore {
 KeyValueStore& storage_;
 std::string key_;
 SaveValidator validate_;
 SaveClock clock_;
 std::optional<Json> parse_state(const std::string&) const;
 SaveCandidate candidate(const std::string&,const std::string&,bool allow_legacy=false) const;
 std::string metadata(const std::string&,std::int64_t) const;
 void clear_staging() noexcept;
public:
 SaveStore(KeyValueStore&,std::string key,SaveValidator,SaveClock);
 SaveInspection inspect() const;
 SaveInspection write(const std::string& payload);
 bool restore_backup();
 void remove() noexcept;
};
std::string slot_save_key(const std::string& base,int slot);
Json native_save_envelope(const std::string& payload,const std::string& saved_at);
std::optional<Json> parse_native_envelope(const std::string& raw,const SaveValidator&);
Json inspection_json(const SaveInspection&);
}
