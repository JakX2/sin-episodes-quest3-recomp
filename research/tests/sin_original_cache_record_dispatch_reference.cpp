#include "sin_compat/original_cache_record_dispatch.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <string>
#include <vector>
using namespace sin_compat::record_dispatch;
using namespace sin_compat::client_shutdown;
int main() {
  static_assert(original_record_stride==24);
  static_assert(scene_new_record_object_bytes==28);
  static_assert(model_sounds_new_record_object_bytes==24);
  assert(record_value_offset==4 && record_handler_index_offset==12);
  assert(record_aux_pointer_offset==20 && invalid_record_index==0xffff);
  int lookups=0,inserts=0;
  auto existing=find_or_insert_record([&](){++lookups;return std::uint16_t(7);},
                                       [&](){++inserts;return std::uint16_t(5);});
  assert(existing==7 && lookups==1 && inserts==0);
  auto new_record=find_or_insert_record([&](){++lookups;return invalid_record_index;},
                                         [&](){++inserts;return std::uint16_t(9);});
  assert(new_record==9 && lookups==2 && inserts==1);
  RecordWords32 slot{};
  slot.words[record_handler_index_offset/4]=3;
  slot.words[record_aux_pointer_offset/4]=0x1234u;
  int handler=42;
  int handler_queries=0, handler_invocations=0;
  dispatch_record(slot,0xcafeu,
      [&](std::uint32_t idx)->int&{assert(idx==3);++handler_queries;return handler;},
      [&](int& h,RecordWords32& entry){++handler_invocations;assert(h==42);
          assert(entry.words[1]==0xcafe && entry.words[5]==0x1234u);});
  assert(handler_queries==1 && handler_invocations==1);
  std::vector<std::string> calls;
  run_late_shutdown(
     [&](){calls.push_back("pre");},
     [&](std::uint32_t addr,std::uint8_t offset){
         calls.push_back(std::to_string(addr)+":"+std::to_string(offset));},
     true,
     [&](std::uint32_t addr,std::uint8_t slot){
         assert(addr==optional_interface_global_va && slot==0x0c);
         calls.push_back("optional");},
     [&](std::uint8_t offset){assert(offset==0x14);calls.push_back("final");});
  assert(calls.size()==11 && calls.front()=="pre" && calls[9]=="optional"
         && calls.back()=="final");
  for(std::size_t i=0;i<8;++i){
      assert(calls[i+1]==std::to_string(original_late_calls[i].original_global_va)+
                          ":"+std::to_string(original_late_calls[i].virtual_byte_offset));
  }
  calls.clear();
  run_late_shutdown([&](){calls.push_back("pre");},
      [&](std::uint32_t,std::uint8_t){calls.push_back("call");},false,
      [&](std::uint32_t,std::uint8_t){assert(false);},
      [&](std::uint8_t){calls.push_back("final");});
  assert(calls.size()==10 && calls.back()=="final");
  int owned=7;void* object=&owned;
  release_optional_global(object,[&](void* p,int arg){assert(p==&owned && arg==1);calls.push_back("released");});
  assert(!object && calls.back()=="released");
  release_optional_global(object,[](void*,int){assert(false);});
}
