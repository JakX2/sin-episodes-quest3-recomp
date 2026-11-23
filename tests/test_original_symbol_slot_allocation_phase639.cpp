#include "sin_compat/original_symbol_slot_allocation_phase639.hpp"
#include "sin_compat/original_symbol_forward_phase638.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
namespace p=sin_compat::original_symbol_slot_allocation_phase639;
namespace f=sin_compat::original_symbol_forward_phase638;
namespace {
constexpr std::uint32_t kContext=0x1000u,kTable=0x2000u;
constexpr std::uint32_t kSlots=0x3000u,kRecord=0x6000u;
class Host final:public p::IHost, public f::IHost {
public:
  std::map<std::uint32_t,std::uint8_t> bytes;
  std::vector<std::string> events;
  bool reject_enum=false,reject_grow=false,reject_link=false,reject_get=false;
  bool fail_write_record_dword=false,fail_read_record_dword=false;
  bool relocate_on_grow=false,relocate_on_link=false,throw_in_enum=false;
  bool reenter_enum=false,reenter_grow=false;
  bool do_not_rewrite=false;
  p::AcquireSlot* active_acquire=nullptr;
  p::RegisterSlot* active_register=nullptr;
  p::Code nested=p::Code::Completed;
  std::uint16_t seen_new=0xffffu,seen_next=0xffffu;
  std::uint32_t linked_first=0,linked_second=0,enumerated_record=0;
  std::uint32_t tree_seen=0,source_seen=0;
  p::Result last_registered{};
  void set16(std::uint32_t addr,std::uint16_t value) {
    bytes[addr]=static_cast<std::uint8_t>(value);
    bytes[addr+1]=static_cast<std::uint8_t>(value>>8);
  }
  void set32(std::uint32_t addr,std::uint32_t value){
    for(unsigned i=0;i<4;++i)
      bytes[addr+i]=static_cast<std::uint8_t>(value>>(8*i));
  }
  bool get16(std::uint32_t addr,std::uint16_t& value)const {
    std::uint32_t n=0;
    for(unsigned i=0;i<2;++i){
      const auto it=bytes.find(addr+i);
      if(it==bytes.end())return false;
      n|=std::uint32_t(it->second)<<(8*i);
    }
    value=static_cast<std::uint16_t>(n);return true;
  }
  bool get32(std::uint32_t addr,std::uint32_t& value)const {
    value=0;
    for(unsigned i=0;i<4;++i){
      const auto it=bytes.find(addr+i);
      if(it==bytes.end())return false;
      value|=std::uint32_t(it->second)<<(8*i);
    }
    return true;
  }
  std::uint16_t u16(std::uint32_t a)const {std::uint16_t x=0;assert(get16(a,x));return x;}
  std::uint32_t u32(std::uint32_t a)const {std::uint32_t x=0;assert(get32(a,x));return x;}
  bool read_u16(std::uint32_t a,std::uint16_t& result)override {
    events.emplace_back("R16:"+std::to_string(a));return get16(a,result);
  }
  bool read_u32(std::uint32_t a,std::uint32_t& result)override {
    events.emplace_back("R32:"+std::to_string(a));
    if(fail_read_record_dword && a==kRecord+4)return false;
    return get32(a,result);
  }
  bool write_u16(std::uint32_t a,std::uint16_t value)override {
    events.emplace_back("W16:"+std::to_string(a));set16(a,value);return true;
  }
  bool write_u32(std::uint32_t a,std::uint32_t value)override {
    events.emplace_back("W32:"+std::to_string(a));
    if(fail_write_record_dword && a==kSlots+3*16+12)return false;
    set32(a,value);return true;
  }
  bool enumerate_22280fb0(std::uint32_t table,std::uint32_t record,
         std::uint32_t& first,std::uint32_t& second)override {
    events.emplace_back("ENUM");assert(table==kTable && first==record);
    enumerated_record=record;
    if(reenter_enum && active_register)
      nested=active_register->register_record(kTable,kRecord,0).code;
    if(throw_in_enum)throw std::runtime_error("enumerator fault");
    if(reject_enum)return false;
    if(!do_not_rewrite){first=0x4321u;second=1u;}
    return true;
  }
  bool link_slot_22281f30(std::uint32_t table,std::uint16_t index,
       std::uint32_t first,std::uint32_t second)override {
    events.emplace_back("LINK");assert(table==kTable);
    seen_new=index;linked_first=first;linked_second=second;
    if(reject_link)return false;
    if(relocate_on_link)set32(kTable+4,0x5000u);
    return true;
  }
  bool get_free_slot_next_22280d40(std::uint32_t table,
       std::uint16_t free_head,std::uint16_t& next)override {
    events.emplace_back("FREE");assert(table==kTable);
    seen_next=free_head;
    if(reject_get)return false;
    return get16(u32(table+4)+std::uint32_t(free_head)*16u+2u,next);
  }
  bool grow_slot_array_22280dd0(std::uint32_t owner,std::uint32_t increment)override {
    events.emplace_back("GROW");assert(owner==kTable+4 && increment==1);
    if(reenter_grow && active_acquire)
      nested=active_acquire->acquire(kTable).code;
    if(reject_grow)return false;
    set32(kTable+8,u32(kTable+8)+1u);
    if(relocate_on_grow)set32(kTable+4,0x4000u);
    return true;
  }
  // Phase638 source-forward adapter with an explicitly fabricated source
  // record; NOT a claim of real retail stack/pointer object layout.
  bool lookup_original_tree_2228f7a0(std::uint32_t tree,
       std::uint32_t& scratch,std::uint32_t sym,
       std::uint16_t& token)override {
    events.emplace_back("TREE");tree_seen=tree;source_seen=sym;
    assert(tree==kTable+0x1c && scratch==sym);
    token=0x55u;return true;
  }
  bool allocate_original_slot_22282270(std::uint32_t table,
      std::uint16_t& token,std::uint16_t& out)override {
    events.emplace_back("ALLOC-ADAPTER");assert(table==kTable);
    assert(active_register!=nullptr);
    set16(kRecord,token);set32(kRecord+4,0x76543210u);
    last_registered=active_register->register_record(table,kRecord,0u);
    if(last_registered.code!=p::Code::Completed)return false;
    out=last_registered.handle;
    token=static_cast<std::uint16_t>(token+1u);
    return true;
  }
  void reset(){
    bytes.clear();events.clear();
    reject_enum=reject_grow=reject_link=reject_get=false;
    fail_write_record_dword=fail_read_record_dword=false;
    relocate_on_grow=relocate_on_link=throw_in_enum=false;
    reenter_enum=reenter_grow=do_not_rewrite=false;
    active_acquire=nullptr;active_register=nullptr;nested=p::Code::Completed;
    seen_new=seen_next=0xffffu;linked_first=linked_second=0;
    enumerated_record=tree_seen=source_seen=0;last_registered={};
    set32(kContext,kTable);
    set16(kTable+0x12u,0u);set16(kTable+0x14u,0xffffu);
    set16(kTable+0x16u,3u);set32(kTable+8u,8u);
    set32(kTable+4u,kSlots);set32(kTable+0x18u,kSlots);
    set16(kRecord,0xbeefu);set32(kRecord+4u,0x44556677u);
  }
};
void test_new_slot(){
  Host h;h.reset();p::AcquireSlot a(h);h.active_acquire=&a;
  auto r=a.acquire(kTable);
  assert(r.code==p::Code::Completed && r.handle==3u);
  assert(!r.grew_slots && !r.used_free_list && r.writes_committed==3u);
  assert(h.u16(kTable+0x16u)==4u && h.u16(kSlots+3*16+8)==0xffffu);
  assert(h.u32(kTable+0x18u)==kSlots);
  assert((h.events==std::vector<std::string>{
    "R16:8212", "R16:8214", "R32:8200", "R16:8214",
    "W16:8214", "R32:8196", "W16:12344", "R32:8196",
    "W32:8216"}));
}
void test_grow_and_free_list(){
  Host h;h.reset();p::AcquireSlot a(h);h.active_acquire=&a;
  h.set32(kTable+8,3);h.relocate_on_grow=true;
  auto r=a.acquire(kTable);
  assert(r.code==p::Code::Completed && r.grew_slots && r.handle==3u);
  assert(h.u32(kTable+4)==0x4000u && h.u32(kTable+8)==4u);
  assert(h.u16(0x4000u+3*16+8)==0xffffu);
  assert(h.u32(kTable+0x18)==0x4000u);
  h.reset();h.set16(kTable+0x14u,5u);
  h.set16(kSlots+5*16+2,7u);
  r=a.acquire(kTable);
  assert(r.code==p::Code::Completed && r.used_free_list && r.handle==5u);
  assert(h.seen_next==5u && h.u16(kTable+0x14u)==7u);
  assert(h.u16(kTable+0x16u)==3u);
  assert(h.u16(kSlots+5*16+8)==0xffffu);
  h.reset();h.set16(kTable+0x14u,0xffffu);
  h.set16(kTable+0x16u,0xffffu);
  h.set32(kTable+8u,0xfffeu);
  // Unlike an invented failure, native next-index increments as a WORD.
  r=a.acquire(kTable);
  assert(r.code==p::Code::Completed && r.handle==0xffffu);
  assert(h.u16(kTable+0x16u)==0u);
  assert(h.u16(kSlots+0xffffu*16u+8u)==0xffffu);
}
void test_acquire_partial_failures(){
  Host h;h.reset();p::AcquireSlot a(h);h.active_acquire=&a;
  h.set32(kTable+8u,3u);h.reject_grow=true;
  auto r=a.acquire(kTable);
  assert(r.code==p::Code::HostRejected && r.stage==p::Stage::GrowSlotArray80DD0);
  assert(h.u16(kTable+0x16u)==3u && r.writes_committed==0u);
  h.reset();h.set16(kTable+0x14u,4u);h.reject_get=true;
  r=a.acquire(kTable);
  assert(r.code==p::Code::HostRejected && r.stage==p::Stage::QueryFreeNode80D40);
  assert(h.u16(kTable+0x14u)==4u);
  h.reset();h.set32(kTable+4u,0u);
  r=a.acquire(kTable);
  assert(r.code==p::Code::NullSlotsHazard && r.writes_committed==1u);
  assert(h.u16(kTable+0x16u)==4u); // original commits prior increment
  h.reset();h.set32(kTable+4u,0xfffffff0u);
  r=a.acquire(kTable);
  assert(r.code==p::Code::AddressOverflow && r.writes_committed==1u);
  h.reset();h.active_acquire=&a;h.reenter_grow=true;h.set32(kTable+8,3u);
  r=a.acquire(kTable);
  assert(r.code==p::Code::Completed && h.nested==p::Code::Reentrant);
  r=a.acquire(0u);assert(r.code==p::Code::NullTableHazard);
}
void test_full_registration(){
 Host h;h.reset();p::AcquireSlot a(h);p::RegisterSlot reg(h,a);
 h.active_acquire=&a;h.active_register=&reg;
 auto r=reg.register_record(kTable,kRecord,0u);
 assert(r.code==p::Code::Completed && r.enumerated && r.acquired && r.linked);
 assert(r.copied_record && r.active_count_incremented && r.handle==3u);
 assert(r.writes_committed==6u);
 assert(h.enumerated_record==kRecord && h.linked_first==0x4321u);
 assert(h.linked_second==1u && h.seen_new==3u);
 assert(h.u16(kTable+0x12)==1u);
 assert(h.u16(kSlots+3*16+8u)==0xbeefu);
 assert(h.u32(kSlots+3*16+12u)==0x44556677u);
 assert(h.events.front()=="ENUM" && h.events.back()=="W32:12348");
 h.reset();h.active_acquire=&a;h.active_register=&reg;
 h.relocate_on_link=true;
 r=reg.register_record(kTable,kRecord,0u);
 assert(r.code==p::Code::Completed && r.slot_va==0x5000u+3*16u+8u);
 assert(h.u16(r.slot_va)==0xbeefu);
 assert(h.u16(kSlots+3*16+8u)==0xffffu); // earlier acquire wrote old array
 h.reset();h.active_acquire=&a;h.active_register=&reg;
 h.fail_read_record_dword=true;
 r=reg.register_record(kTable,kRecord,0u);
 assert(r.code==p::Code::HostRejected && r.active_count_incremented);
 assert(r.stage==p::Stage::ReadRecordDword4 && !r.copied_record);
 assert(h.u16(kSlots+3*16+8u)==0xbeefu); // no rollback
 assert(h.u16(kTable+0x12u)==1u);
 h.reset();h.active_acquire=&a;h.active_register=&reg;
 h.reject_link=true;
 r=reg.register_record(kTable,kRecord,0u);
 assert(r.code==p::Code::HostRejected && r.acquired && !r.linked);
 assert(h.u16(kTable+0x16u)==4u);
 assert(h.u16(kTable+0x12u)==0u);
 h.reset();h.active_acquire=&a;h.active_register=&reg;
 h.throw_in_enum=true;
 r=reg.register_record(kTable,kRecord,0u);
 assert(r.code==p::Code::HostRejected && !r.enumerated);
 h.reset();h.active_acquire=&a;h.active_register=&reg;
 h.reenter_enum=true;
 r=reg.register_record(kTable,kRecord,0u);
 assert(r.code==p::Code::Completed && h.nested==p::Code::Reentrant);
 r=reg.register_record(kTable,0u,0u);
 assert(r.code==p::Code::NullRecordHazard);
}
void test_phase638_original_source_composition(){
 Host h;h.reset();p::AcquireSlot a(h);p::RegisterSlot reg(h,a);
 f::Forwarder upstream(h);h.active_acquire=&a;h.active_register=&reg;
 const auto first=upstream.write(kContext,0x123456u,42);
 assert(first.code==f::Code::Written && first.slot_handle==3u);
 assert(h.last_registered.code==p::Code::Completed);
 assert(h.tree_seen==kTable+0x1c && h.source_seen==0x123456u);
 assert(h.u16(kSlots+3*16+8u)==0x55u);
 // Original 22282370 writes mapped index AFTER Phase639 slot copy.
 assert(h.u32(kSlots+3*16+12u)==42u);
 assert(h.u16(kTable+0x12u)==1u);
 assert(h.events.back()=="W32:12348");
}
}
int main(){
 test_new_slot();test_grow_and_free_list();test_acquire_partial_failures();
 test_full_registration();test_phase638_original_source_composition();
 return 0;
}
