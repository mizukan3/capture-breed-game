#include "capture/save_store.hpp"
#include "capture/checksum.hpp"
#include <iostream>
#include <stdexcept>
using namespace capture;
namespace {
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
struct FailingStore:MemoryStore {
 int fail_at{},writes{};
 void put(const std::string& k,const std::string& v) override {
  if(++writes==fail_at)throw std::runtime_error("injected disk error");MemoryStore::put(k,v);
 }
};
}
int main() {
 try{
  const SaveValidator valid=[](const Json& s){return s.is_object()&&s.contains("turn")&&s.at("turn").is_number_integer();};
  const auto clock=[](){return "2000-01-01T00:00:00.000Z";};
  const std::string first=R"({"turn":1,"name":"ピョン😀"})",second=R"({"turn":2,"name":"ヤミピョン"})";
  MemoryStore storage;SaveStore saves(storage,"slot-1",valid,clock);
  check(saves.inspect().source=="none","empty slot");
  auto i=saves.write(first);check(i.primary_valid&&i.has_backup&&i.sequence==1,"first double write");
  check(!saves.write(first).written,"unchanged save skip");
  i=saves.write(second);check(i.sequence==2&&i.backup.payload==first,"previous generation preserved");
  storage.put("slot-1","broken");i=saves.inspect();check(i.recovered&&i.source=="backup"&&i.payload==first,"corrupt primary recovery");
  check(saves.restore_backup()&&saves.inspect().primary_valid,"restore backup");
  for(int failure=1;failure<=6;++failure) {
   FailingStore disk;SaveStore s(disk,"slot",valid,clock);s.write(first);disk.writes=0;disk.fail_at=failure;
   bool threw=false;try{s.write(second);}catch(...){threw=true;}check(threw,"fault reached");
   auto recovered=s.inspect();check(recovered.payload.has_value()&&valid(recovered.state),"interrupted save remains recoverable");
   check(*recovered.payload==first||*recovered.payload==second,"recovery picks whole payload");
  }
  auto envelope=native_save_envelope(first,clock());check(parse_native_envelope(envelope.dump(),valid).has_value(),"native envelope Unicode");
  envelope["payload"]=second;check(!parse_native_envelope(envelope.dump(),valid),"reject checksum mismatch");
  saves.remove();check(saves.inspect().source=="none","delete all save generations");
  check(slot_save_key("base",0)=="base-slot-1"&&slot_save_key("base",99)=="base-slot-5","slot limits");
  std::cout<<"Save recovery and injected failures passed\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
