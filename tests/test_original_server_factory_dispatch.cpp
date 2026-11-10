#include "sin_compat/original_server_factory_dispatch.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
namespace f=sin_compat::original_server_factory_dispatch;
struct Host {
    std::vector<std::string> calls;
    std::uint16_t symbol=9,slot=0;
    std::uintptr_t result=73;
    std::uint32_t bad=0;
    std::uint16_t intern_classname(const char* key){calls.emplace_back("intern");assert(std::string(key)=="flak");return symbol;}
    std::uint16_t find_registry_index(std::uint16_t key,std::uint32_t n){calls.emplace_back("find");assert(key==symbol&&n==0);return slot;}
    void report_invalid_registry_slot(std::uint16_t id){calls.emplace_back("invalid");bad=id;}
    void report_missing_primary(std::uint32_t address,const char* key){calls.emplace_back("miss0");assert(address==f::kPrimaryMissingDiagnosticVA&&std::string(key)=="flak");}
    void report_missing_destroy(std::uint32_t address,const char* key){calls.emplace_back("miss4");assert(address==f::kDestroyMissingDiagnosticVA&&std::string(key)=="flak");}
    std::uintptr_t invoke_primary_factory(std::uintptr_t factory,const char* key){calls.emplace_back("virtual0");assert(factory==51&&std::string(key)=="flak");return result;}
    void invoke_destroy_factory(std::uintptr_t factory,std::uintptr_t arg){calls.emplace_back("virtual4");assert(factory==51&&arg==0x1234u);}
};
int main(){
  static_assert(f::original_record_factory_byte_offset(0)==12,"offset0");
  static_assert(f::original_record_factory_byte_offset(1)==28,"stride16");
  static_assert(f::original_record_factory_byte_offset(65535)==1048572,"unsigned 16-bit");
  std::vector<f::RegistryRecord> records{{0,0,0,51},{0,0,0,99}};
  Host host{};
  assert(f::original_primary_factory_dispatch("flak",records,host)==73);
  assert((host.calls==std::vector<std::string>{"intern","find","virtual0"}));
  host.calls.clear();f::original_destroy_factory_dispatch("flak",0x1234,records,host);
  assert((host.calls==std::vector<std::string>{"intern","find","virtual4"}));
  host.calls.clear();host.slot=f::kNotFound;
  assert(f::original_primary_factory_dispatch("flak",records,host)==0);
  assert((host.calls==std::vector<std::string>{"intern","find","miss0"}));
  host.calls.clear();f::original_destroy_factory_dispatch("flak",0x1234,records,host);
  assert((host.calls==std::vector<std::string>{"intern","find","miss4"}));
  host.calls.clear();host.slot=13;
  assert(f::original_primary_factory_dispatch("flak",records,host)==0);
  assert(host.bad==13);
  assert((host.calls==std::vector<std::string>{"intern","find","invalid","miss0"}));
  host.calls.clear();host.slot=1;records[1].factory=0;
  assert(f::original_primary_factory_dispatch("flak",records,host)==0);
  assert((host.calls==std::vector<std::string>{"intern","find","miss0"}));
}