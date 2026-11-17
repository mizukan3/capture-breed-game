#include "capture/save_store.hpp"
#include "capture/checksum.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace capture {
std::optional<std::string> MemoryStore::get(const std::string& k) const{const auto it=entries.find(k);return it==entries.end()?std::nullopt:std::optional<std::string>(it->second);}
void MemoryStore::put(const std::string& k,const std::string& v){entries[k]=v;}
void MemoryStore::erase(const std::string& k){entries.erase(k);}
SaveStore::SaveStore(KeyValueStore& s,std::string key,SaveValidator v,SaveClock clock):storage_(s),key_(std::move(key)),validate_(std::move(v)),clock_(std::move(clock)){}
std::optional<Json> SaveStore::parse_state(const std::string& p) const {
 try{auto state=Json::parse(p);if(validate_(state))return state;}catch(...){}return std::nullopt;
}
SaveCandidate SaveStore::candidate(const std::string& payload_key,const std::string& meta_key,bool allow_legacy) const {
 SaveCandidate c;c.payload=storage_.get(payload_key);
 if(!c.payload){c.missing=true;return c;}
 const auto state=parse_state(*c.payload);const auto meta_raw=storage_.get(meta_key);
 Json meta;
 if(meta_raw)try {
  meta=Json::parse(*meta_raw);const double seq=meta.at("sequence").get<double>();
  if(meta.value("format","")!="turn-dungeon-save"||meta.at("schema")!=1||!std::isfinite(seq)||seq<0||seq>9007199254740991.||std::floor(seq)!=seq||!meta.at("checksum").is_string())meta=nullptr;
 }catch(...){meta=nullptr;}
 if(!state){c.reason="json";return c;}
 if(meta.is_null()){
  if(allow_legacy&&!meta_raw){c.valid=true;c.legacy=true;c.state=*state;c.sequence=0;return c;}
  c.reason="metadata";return c;
 }
 c.sequence=static_cast<std::int64_t>(meta.at("sequence").get<double>());
 if(meta.at("checksum")!=save_checksum(*c.payload)){c.reason="checksum";return c;}
 c.valid=true;c.state=*state;c.meta=std::move(meta);return c;
}
std::string SaveStore::metadata(const std::string& payload,std::int64_t sequence) const {
 return Json{{"format","turn-dungeon-save"},{"schema",1},{"sequence",sequence},{"checksum",save_checksum(payload)},{"savedAt",clock_()}}.dump();
}
SaveInspection SaveStore::inspect() const {
 SaveInspection i;i.primary=candidate(key_,key_+"-meta-v1",true);
 i.backup=candidate(key_+"-backup-v1",key_+"-backup-meta-v1");
 i.staging=candidate(key_+"-staging-v1",key_+"-staging-meta-v1");
 const SaveCandidate* chosen=nullptr;int priority=0;
 auto choose=[&](const SaveCandidate& c,const char* source,int rank){
  if(c.valid&&(!chosen||c.sequence>chosen->sequence||(c.sequence==chosen->sequence&&rank>priority))){chosen=&c;priority=rank;i.source=source;}
 };
 choose(i.primary,"primary",3);choose(i.backup,"backup",1);choose(i.staging,"staging",2);
 if(chosen){i.payload=chosen->payload;i.state=chosen->state;i.sequence=chosen->sequence;i.legacy=chosen->legacy;}
 i.recovered=i.source=="backup"||i.source=="staging";i.has_backup=i.backup.valid;i.primary_valid=i.primary.valid;
 i.corrupt=(!i.primary.valid&&!i.primary.missing)||(!i.backup.valid&&!i.backup.missing)||(!i.staging.valid&&!i.staging.missing);return i;
}
void SaveStore::clear_staging() noexcept {
 for(const auto& key:{key_+"-staging-v1",key_+"-staging-meta-v1"})try{storage_.erase(key);}catch(...){}
}
SaveInspection SaveStore::write(const std::string& payload) {
 if(!parse_state(payload))throw std::runtime_error("保存内容を検証できません。");
 const auto before=inspect();const auto sequence=std::max({std::int64_t{0},before.primary.valid?before.primary.sequence:0,before.backup.valid?before.backup.sequence:0,before.staging.valid?before.staging.sequence:0})+1;
 if(before.source=="primary"&&before.payload&&*before.payload==payload&&!before.legacy&&before.has_backup)return before;
 const auto meta=metadata(payload,sequence);
 try {
  storage_.put(key_+"-staging-v1",payload);storage_.put(key_+"-staging-meta-v1",meta);
  if(!candidate(key_+"-staging-v1",key_+"-staging-meta-v1").valid)throw std::runtime_error("一時保存の検証に失敗しました。");
  const auto& current=before.primary;
  if(current.valid&&current.payload&&*current.payload!=payload){storage_.put(key_+"-backup-v1",*current.payload);storage_.put(key_+"-backup-meta-v1",metadata(*current.payload,current.sequence));}
  else if(!before.has_backup){const auto& p=current.valid?*current.payload:payload;const auto seq=current.valid?current.sequence:sequence;storage_.put(key_+"-backup-v1",p);storage_.put(key_+"-backup-meta-v1",metadata(p,seq));}
  storage_.put(key_,payload);storage_.put(key_+"-meta-v1",meta);
  if(!candidate(key_,key_+"-meta-v1",true).valid)throw std::runtime_error("本保存の検証に失敗しました。");
  clear_staging();auto after=inspect();after.written=true;return after;
 }catch(...){throw std::runtime_error("自動保存に失敗しました。前回の復旧データは保持されています。");}
}
bool SaveStore::restore_backup() {
 const auto before=inspect();if(!before.backup.valid)return false;
 const auto& payload=*before.backup.payload;const auto meta=metadata(payload,std::max(before.sequence,before.backup.sequence)+1);
 try{
  storage_.put(key_+"-staging-v1",payload);storage_.put(key_+"-staging-meta-v1",meta);
  storage_.put(key_,payload);storage_.put(key_+"-meta-v1",meta);
  if(!candidate(key_,key_+"-meta-v1",true).valid)throw std::runtime_error("復旧後の検証に失敗しました。");
  clear_staging();return true;
 }catch(...){return false;}
}
void SaveStore::remove() noexcept {
 for(const auto& k:{key_,key_+"-meta-v1",key_+"-backup-v1",key_+"-backup-meta-v1",key_+"-staging-v1",key_+"-staging-meta-v1"})try{storage_.erase(k);}catch(...){}
}
std::string slot_save_key(const std::string& base,int slot){return base+"-slot-"+std::to_string(std::clamp(slot,1,5));}
Json native_save_envelope(const std::string& payload,const std::string& saved_at){return {{"format","turn-dungeon-native-save"},{"schema",1},{"checksum",save_checksum(payload)},{"savedAt",saved_at},{"payload",payload}};}
std::optional<Json> parse_native_envelope(const std::string& raw,const SaveValidator& validate) {
 try{
  auto e=Json::parse(raw);
  if(e.value("format","")!="turn-dungeon-native-save"||e.at("schema")!=1||!e.at("payload").is_string())return std::nullopt;
  const auto p=e.at("payload").get<std::string>();if(e.at("checksum")!=save_checksum(p))return std::nullopt;
  auto state=Json::parse(p);if(!validate(state))return std::nullopt;e["state"]=std::move(state);return e;
 }catch(...){return std::nullopt;}
}
Json inspection_json(const SaveInspection& i){return {{"source",i.source},{"payload",i.payload?Json(*i.payload):Json(nullptr)},{"state",i.state},{"sequence",i.sequence},{"recovered",i.recovered},{"legacy",i.legacy},{"hasBackup",i.has_backup},{"primaryValid",i.primary_valid},{"corrupt",i.corrupt}};}
}
