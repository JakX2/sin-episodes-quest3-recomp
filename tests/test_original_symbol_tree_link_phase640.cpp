#include "sin_compat/original_symbol_tree_link_phase640.hpp"
#include "sin_compat/original_symbol_slot_allocation_phase639.hpp"
#include "sin_compat/original_symbol_forward_phase638.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <algorithm>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
namespace l=sin_compat::original_symbol_tree_link_phase640;
namespace p=sin_compat::original_symbol_slot_allocation_phase639;
namespace f=sin_compat::original_symbol_forward_phase638;
namespace {
constexpr std::uint32_t kContext=0x1000u,kTable=0x2000u,kSlots=0x3000u,kRec=0x6000u;
class Host final:public l::IHost,public p::IHost,public f::IHost {
public:
 std::map<std::uint32_t,std::uint8_t> mem;
 std::vector<std::string> trace;
 unsigned writes=0u,fail_write=0u;
 bool deny_rebalance=false,throw_rebalance=false,relocate=false,reenter=false;
 bool deny_enumerate=false,deny_link=false,poison_array_after_initialize=false;
 l::LinkNode* active_link=nullptr;
 p::RegisterSlot* active_reg=nullptr;
 p::AcquireSlot* active_acquire=nullptr;
 l::Code nested=l::Code::Linked;
 std::uint32_t rebalanced_index=0u;
 std::uint32_t rebalanced_table=0u;
 p::Result last_register{};
 void set16(std::uint32_t a,std::uint16_t n) {mem[a]=static_cast<std::uint8_t>(n);mem[a+1]=static_cast<std::uint8_t>(n>>8);}
 void set32(std::uint32_t a,std::uint32_t n) {for(unsigned i=0;i<4u;i++)mem[a+i]=static_cast<std::uint8_t>(n>>(8*i));}
 bool get16(std::uint32_t a,std::uint16_t& n)const {n=0;for(unsigned i=0;i<2u;i++){auto it=mem.find(a+i);if(it==mem.end())return false;n|=static_cast<std::uint16_t>(it->second<<(8*i));}return true;}
 bool get32(std::uint32_t a,std::uint32_t& n)const {n=0;for(unsigned i=0;i<4u;i++){auto it=mem.find(a+i);if(it==mem.end())return false;n|=static_cast<std::uint32_t>(it->second)<<(8*i);}return true;}
 std::uint16_t u16(std::uint32_t a)const {std::uint16_t n=0;assert(get16(a,n));return n;}
 std::uint32_t u32(std::uint32_t a)const {std::uint32_t n=0;assert(get32(a,n));return n;}
 void reset(){mem.clear();trace.clear();writes=fail_write=0u;deny_rebalance=throw_rebalance=relocate=reenter=false;deny_enumerate=deny_link=poison_array_after_initialize=false;active_link=nullptr;active_reg=nullptr;active_acquire=nullptr;nested=l::Code::Linked;rebalanced_index=rebalanced_table=0u;last_register={};set32(kContext,kTable);set32(kTable+4u,kSlots);set16(kTable+0x10u,0xffffu);set16(kTable+0x12u,0u);set16(kTable+0x14u,0xffffu);set16(kTable+0x16u,3u);set32(kTable+8u,8u);set32(kTable+0x18u,kSlots);set16(kRec,0x52u);set32(kRec+4u,0x88997766u);}
 bool read_u16(std::uint32_t a,std::uint16_t& n)override {trace.emplace_back("r16:"+std::to_string(a));return get16(a,n);}
 bool read_u32(std::uint32_t a,std::uint32_t& n)override {trace.emplace_back("r32:"+std::to_string(a));return get32(a,n);}
 bool write_u16(std::uint32_t a,std::uint16_t n)override {
   trace.emplace_back("w16:"+std::to_string(a));++writes;
   if(fail_write==writes)return false;
   set16(a,n);
   if(relocate && a==kSlots+3*16+6) set32(kTable+4u,0x5000u);
   if(poison_array_after_initialize && a==kSlots+3*16+6) set32(kTable+4u,0u);
   return true;
 }
 bool write_u32(std::uint32_t a,std::uint32_t n)override {trace.emplace_back("w32:"+std::to_string(a));set32(a,n);return true;}
 bool rebalance_original_22281840(std::uint32_t table,std::uint32_t full_idx)override {
   trace.emplace_back("rebalance");rebalanced_table=table;rebalanced_index=full_idx;
   if(reenter && active_link)nested=active_link->link(kTable,4u,0xffffu,0u).code;
   if(throw_rebalance)throw std::runtime_error("original tree callback refused");
   return !deny_rebalance;
 }
 bool enumerate_22280fb0(std::uint32_t table,std::uint32_t record,
                          std::uint32_t& first,std::uint32_t& second)override {
   trace.emplace_back("enumerate");assert(table==kTable&&record==kRec&&first==kRec);
   if(deny_enumerate)return false;
   first=0xffffu;second=0u;return true; // fabricated root insertion
 }
 bool link_slot_22281f30(std::uint32_t table,std::uint16_t index,
                         std::uint32_t parent,std::uint32_t side)override {
   trace.emplace_back("link-adapter");if(deny_link || !active_link)return false;
   return active_link->link(table,index,parent,side).code==l::Code::Linked;
 }
 bool get_free_slot_next_22280d40(std::uint32_t table,
           std::uint16_t free_index,std::uint16_t& next)override {
   trace.emplace_back("free");assert(table==kTable);
   return get16(u32(kTable+4u)+16u*free_index+2u,next);
 }
 bool grow_slot_array_22280dd0(std::uint32_t owner,std::uint32_t amount)override {
   trace.emplace_back("grow");assert(owner==kTable+4u && amount==1u);
   set32(kTable+8u,u32(kTable+8u)+1u);return true;
 }
 bool lookup_original_tree_2228f7a0(std::uint32_t tree,std::uint32_t& scratch,
        std::uint32_t symbol,std::uint16_t& token)override {
   trace.emplace_back("tree");assert(tree==kTable+0x1cu && scratch==symbol);
   token=0x52u;return true;
 }
 bool allocate_original_slot_22282270(std::uint32_t table,
    std::uint16_t& token,std::uint16_t& slot)override {
   trace.emplace_back("alloc-adapter");assert(table==kTable&&active_reg);
   set16(kRec,token);last_register=active_reg->register_record(table,kRec,0u);
   if(last_register.code!=p::Code::Completed)return false;
   slot=last_register.handle;return true;
 }
};
void test_root_and_full_argument(){
 Host h;h.reset();l::LinkNode link(h);h.active_link=&link;
 auto r=link.link(kTable,0xbeef0003u,0xcafeffffu,0xffffffffu);
 assert(r.code==l::Code::Linked && r.writes_committed==5u && r.linked_as_root);
 assert(r.child_index==3u && r.parent_index==0xffffu && r.rebalance_called);
 assert(h.u16(kTable+0x10u)==3u && h.u16(kSlots+3*16u+0u)==0xffffu);
 assert(h.u16(kSlots+3*16u+2u)==0xffffu);
 assert(h.u16(kSlots+3*16u+4u)==0xffffu && h.u16(kSlots+3*16u+6u)==0u);
 assert(h.rebalanced_table==kTable&&h.rebalanced_index==0xbeef0003u);
 assert((h.trace==std::vector<std::string>{"r32:8196","w16:12340","w16:12338",
   "w16:12336","w16:12342","w16:8208","rebalance"}));
}
void test_left_right_and_refresh(){
 Host h;h.reset();l::LinkNode link(h);h.active_link=&link;
 auto r=link.link(kTable,3u,1u,0x100u); // low BYTE is zero -> right
 assert(r.code==l::Code::Linked&&r.linked_as_right&&!r.linked_as_left);
 assert(h.u16(kSlots+3*16u+4u)==1u&&h.u16(kSlots+1*16u+2u)==3u);
 assert(h.trace[5]=="r32:8196" && h.trace.back()=="rebalance");
 h.reset();h.active_link=&link;
 r=link.link(kTable,3u,1u,0x101u); // low BYTE nonzero -> left
 assert(r.code==l::Code::Linked&&r.linked_as_left&&!r.linked_as_right);
 assert(h.u16(kSlots+1*16u+0u)==3u);
 h.reset();h.active_link=&link;h.relocate=true;
 r=link.link(kTable,3u,1u,1u);
 assert(r.code==l::Code::Linked && r.slot_va==kSlots+48u);
 assert(r.parent_slot_va==0x5000u+16u);
 assert(h.u16(kSlots+3*16u+4u)==1u);
 assert(h.u16(0x5000u+16u)==3u);
}
void test_unsafe_paths_and_partial_writes(){
 Host h;h.reset();l::LinkNode link(h);h.active_link=&link;
 auto r=link.link(0u,3u,0xffffu,0u);
 assert(r.code==l::Code::NullTableHazard && h.trace.empty());
 h.reset();h.set32(kTable+4u,0u);
 r=link.link(kTable,3u,0xffffu,0u);
 assert(r.code==l::Code::NullArrayHazard && r.writes_committed==0u);
 h.reset();h.set32(kTable+4u,0xfffffff0u);
 r=link.link(kTable,3u,0xffffu,0u);
 assert(r.code==l::Code::AddressOverflow && r.writes_committed==0u);
 for(unsigned nth=1;nth<=5;++nth){
   h.reset();h.fail_write=nth;
   r=link.link(kTable,3u,0xffffu,0u);
   assert(r.code==l::Code::HostRejected && r.writes_committed==nth-1u);
   assert(h.trace.back().substr(0,3)=="w16");
 }
 h.reset();h.deny_rebalance=true;
 r=link.link(kTable,3u,0xffffu,0u);
 assert(r.code==l::Code::HostRejected && r.writes_committed==5u);
 assert(r.rebalance_called&&h.u16(kTable+0x10u)==3u);
 h.reset();h.throw_rebalance=true;
 r=link.link(kTable,3u,0xffffu,0u);
 assert(r.code==l::Code::HostRejected&&r.writes_committed==5u);
 h.reset();h.active_link=&link;h.reenter=true;
 r=link.link(kTable,3u,0xffffu,0u);
 assert(r.code==l::Code::Linked && h.nested==l::Code::Reentrant);
 h.reset();h.active_link=&link;h.relocate=true;
 r=link.link(kTable,3u,0xfffeu,0u);
 assert(r.code==l::Code::Linked && h.u16(0x5000u+0xfffeu*16u+2u)==3u);
 // If second array read is made invalid after four writes, they survive.
 h.reset();h.poison_array_after_initialize=true;
 r=link.link(kTable,3u,1u,1u);
 assert(r.code==l::Code::NullArrayHazard && r.writes_committed==4u);
 assert(r.stage==l::Stage::RefreshSlotArray4);
 assert(h.u16(kSlots+3*16u+4u)==1u);
 assert(h.rebalanced_index==0u);
}
void test_direct_slot_accessor(){
 Host h;h.reset();l::DirectSlotAccessor accessor(h);
 auto r=accessor.resolve(kTable,0xdead0005u);
 assert(r.code==l::Code::Linked&&r.slot_va==kSlots+80u);
 r=accessor.resolve(kTable,0xffffu); // unlike helper 0x22280D40, no sentinel
 assert(r.code==l::Code::Linked && r.slot_va==kSlots+0xffffu*16u);
 h.set32(kTable+4u,0xfffffff0u);
 r=accessor.resolve(kTable,2u);
 assert(r.code==l::Code::AddressOverflow);
 r=accessor.resolve(0u,2u);assert(r.code==l::Code::NullTableHazard);
}
void test_phase638_639_640_composed(){
 Host h;h.reset();l::LinkNode link(h);p::AcquireSlot acquire(h);
 p::RegisterSlot register_slot(h,acquire);f::Forwarder forward(h);
 h.active_link=&link;h.active_reg=&register_slot;h.active_acquire=&acquire;
 auto r=forward.write(kContext,0x12340000u,42);
 assert(r.code==f::Code::Written && r.slot_handle==3u);
 assert(h.last_register.code==p::Code::Completed);
 assert(h.u16(kTable+0x10u)==3u);
 assert(h.u16(kSlots+3*16u+4u)==0xffffu);
 assert(h.u16(kSlots+3*16u+8u)==0x52u);
 assert(h.u32(kSlots+3*16u+12u)==42u); // Phase638 after Phase639 record
 assert(h.u16(kTable+0x12u)==1u);
 auto it=std::find(h.trace.begin(),h.trace.end(),"rebalance");
 assert(it!=h.trace.end() && std::find(it+1,h.trace.end(),"w32:12348")!=h.trace.end());
}
}
int main(){test_root_and_full_argument();test_left_right_and_refresh();
  test_unsafe_paths_and_partial_writes();test_direct_slot_accessor();
  test_phase638_639_640_composed();return 0;}
