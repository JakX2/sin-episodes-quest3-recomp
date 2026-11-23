#include "sin_compat/original_symbol_enumeration_phase643.hpp"
#include "sin_compat/original_symbol_slot_allocation_phase639.hpp"
#include "sin_compat/original_symbol_tree_link_phase640.hpp"
#include "sin_compat/original_symbol_rotations_phase641.hpp"
#include "sin_compat/original_symbol_rebalance_phase642.hpp"
#include "sin_compat/original_symbol_forward_phase638.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
namespace e=sin_compat::original_symbol_enumeration_phase643;
namespace p=sin_compat::original_symbol_slot_allocation_phase639;
namespace l=sin_compat::original_symbol_tree_link_phase640;
namespace r=sin_compat::original_symbol_rotations_phase641;
namespace b=sin_compat::original_symbol_rebalance_phase642;
namespace f=sin_compat::original_symbol_forward_phase638;
namespace {
constexpr std::uint32_t kTable=0x2000u,kContext=0x1000u;
constexpr std::uint32_t kArray=0x3000u,kRecord=0x6000u;
constexpr std::uint32_t kCallback=0x99999999u;
class Host final:public e::IHost,public p::IHost,public l::IHost,
                 public r::IHost,public f::IHost {
public:
 std::map<std::uint32_t,std::uint8_t> bytes;
 std::vector<std::string> events;
 bool reject_compare=false,throw_compare=false,reenter_compare=false;
 bool relocate_compare=false,first_compared=false,fail_child_read=false;
 bool injected_write_failure=false;
 unsigned fail_at_write=0u,writes=0u;
 std::uint32_t last_callback=0,compare_count=0,branch_left=0,branch_right=0;
 std::uint32_t last_record_ptr=0,last_table=0,last_new_node=0;
 e::Code nested=e::Code::Completed;
 e::Enumerate* enumerator=nullptr;
 p::RegisterSlot* registrar=nullptr;
 l::LinkNode* linker=nullptr;
 b::Rebalance* rebalancer=nullptr;
 std::vector<std::uint16_t> expected_tokens;
 e::Result last_enum{};
 p::Result last_reg{};
 b::Result last_balance{};
 void set16(std::uint32_t a,std::uint16_t v){for(unsigned i=0;i<2u;i++)bytes[a+i]=static_cast<std::uint8_t>(v>>(8*i));}
 void set32(std::uint32_t a,std::uint32_t v){for(unsigned i=0;i<4u;i++)bytes[a+i]=static_cast<std::uint8_t>(v>>(8*i));}
 template<typename T> bool get(std::uint32_t a,T& v)const {
   v=0;for(unsigned i=0;i<sizeof(T);++i){auto it=bytes.find(a+i);
     if(it==bytes.end())return false;
     v|=static_cast<T>(it->second)<<(8*i);
   }
   return true;
 }
 std::uint16_t u16(std::uint32_t a)const {std::uint16_t v=0;assert(get(a,v));return v;}
 std::uint32_t u32(std::uint32_t a)const {std::uint32_t v=0;assert(get(a,v));return v;}
 std::uint32_t slot(std::uint16_t idx)const {return u32(kTable+4u)+16u*idx;}
 void reset(){bytes.clear();events.clear();reject_compare=throw_compare=reenter_compare=false;
   relocate_compare=first_compared=fail_child_read=injected_write_failure=false;
   fail_at_write=writes=last_callback=compare_count=branch_left=branch_right=0u;
   last_record_ptr=last_table=last_new_node=0u;expected_tokens.clear();
   nested=e::Code::Completed;last_enum={};last_reg={};last_balance={};
   set32(kContext,kTable);set32(kTable,kCallback);set32(kTable+4u,kArray);
   set32(kTable+8u,4096u);set16(kTable+0x10u,0xffffu);
   set16(kTable+0x12u,0u);set16(kTable+0x14u,0xffffu);
   set16(kTable+0x16u,0u);set32(kTable+0x18u,kArray);
   set32(e::kSentinelFlags,0x00000300u);
 }
 bool read_u8(std::uint32_t a,std::uint8_t& v)override {
   events.push_back("r8:"+std::to_string(a));return get(a,v);}
 bool read_u16(std::uint32_t a,std::uint16_t& v)override {
   events.push_back("r16:"+std::to_string(a));
   if(fail_child_read && a>=kArray && a<kArray+4096u && ((a-kArray)%16u)<4u)
     return false;
   return get(a,v);
 }
 bool read_u32(std::uint32_t a,std::uint32_t& v)override {
   events.push_back("r32:"+std::to_string(a));return get(a,v);}
 bool write_u16(std::uint32_t a,std::uint16_t v)override {
   events.push_back("w16:"+std::to_string(a));++writes;
   if(fail_at_write==writes)return false;
   set16(a,v);return true;
 }
 bool write_u32(std::uint32_t a,std::uint32_t v)override {
   events.push_back("w32:"+std::to_string(a));++writes;
   if(fail_at_write==writes)return false;
   set32(a,v);return true;
 }
 bool compare_original_22280fb0(std::uint32_t tab,std::uint32_t callback,
       std::uint32_t key,std::uint32_t slot_record,std::uint8_t& al)override {
   events.emplace_back("COMPARE");last_table=tab;last_callback=callback;
   last_record_ptr=slot_record;++compare_count;
   if(reenter_compare && enumerator){std::uint32_t par=0,side=0;
     nested=enumerator->search(kTable,key,par,side).code;}
   if(throw_compare)throw std::runtime_error("injected comparator fault");
   if(reject_compare)return false;
   const auto wanted=u16(key),existing=u16(slot_record);
   al=(wanted<existing)?0x80u:0u; // nonzero AL selects LEFT
   if(al)++branch_left;else ++branch_right;
   if(relocate_compare && !first_compared){
     first_compared=true;const auto saved=bytes;
     for(const auto& [a,v]:saved)
       if(a>=kArray && a<kArray+65536u)bytes[0x5000u+a-kArray]=v;
     set32(kTable+4u,0x5000u);
   }
   return true;
 }
 bool enumerate_22280fb0(std::uint32_t table,std::uint32_t record,
       std::uint32_t& parent_arg,std::uint32_t& side_arg)override {
   assert(enumerator);last_enum=enumerator->search(table,record,parent_arg,side_arg);
   return last_enum.code==e::Code::Completed;
 }
 bool link_slot_22281f30(std::uint32_t table,std::uint16_t index,
       std::uint32_t parent,std::uint32_t side)override {
   assert(linker);last_new_node=index;
   return linker->link(table,index,parent,side).code==l::Code::Linked;
 }
 bool get_free_slot_next_22280d40(std::uint32_t table,std::uint16_t index,
       std::uint16_t& next)override {
   assert(table==kTable);return get(slot(index)+2u,next);
 }
 bool grow_slot_array_22280dd0(std::uint32_t owner,std::uint32_t inc)override {
   assert(owner==kTable+4u && inc==1u);set32(kTable+8u,u32(kTable+8u)+1u);return true;
 }
 bool rebalance_original_22281840(std::uint32_t table,std::uint32_t index)override {
   assert(rebalancer);last_balance=rebalancer->insert(table,index);
   return last_balance.code==b::Code::Completed;
 }
 bool lookup_original_tree_2228f7a0(std::uint32_t tree,
       std::uint32_t& scratch,std::uint32_t key,
       std::uint16_t& token)override {
   assert(tree==kTable+0x1cu && scratch==key);
   token=static_cast<std::uint16_t>(key);
   return true;
 }
 bool allocate_original_slot_22282270(std::uint32_t table,
       std::uint16_t& token,std::uint16_t& handle)override {
   assert(registrar);set16(kRecord,token);
   set32(kRecord+4u,0x12345678u);
   last_reg=registrar->register_record(table,kRecord,0u);
   if(last_reg.code!=p::Code::Completed)return false;
   handle=last_reg.handle;return true;
 }
 std::uint16_t nodefield(std::uint16_t idx,std::uint32_t off)const {
   if(idx==0xffffu)return off==6u?1u:0xffffu;
   return u16(slot(idx)+off);
 }
 int verifytree(std::uint16_t idx,std::uint16_t parent,int lo,int hi,
                std::set<std::uint16_t>& seen)const {
   if(idx==0xffffu)return 1;
   assert(seen.insert(idx).second);
   assert(idx<expected_tokens.size());
   assert(nodefield(idx,4u)==parent);
   const auto key=static_cast<int>(nodefield(idx,8u));
   assert(key>lo&&key<hi);
   const auto col=nodefield(idx,6u);
   assert(col==0u||col==1u);
   const auto left=nodefield(idx,0u),right=nodefield(idx,2u);
   if(col==0u){assert(nodefield(left,6u)==1u);assert(nodefield(right,6u)==1u);}
   const int lh=verifytree(left,idx,lo,key,seen);
   const int rh=verifytree(right,idx,key,hi,seen);
   assert(lh==rh);return lh+(col==1u);
 }
 void verifyall()const {
   const auto root=u16(kTable+0x10u);
   if(expected_tokens.empty()){assert(root==0xffffu);return;}
   assert(nodefield(root,6u)==1u);
   std::set<std::uint16_t> visited;
   verifytree(root,0xffffu,-1,65536,visited);
   assert(visited.size()==expected_tokens.size());
   for(std::uint16_t i=0;i<expected_tokens.size();++i){
     assert(nodefield(i,8u)==expected_tokens[i]);
     assert(u32(slot(i)+12u)==1000u+expected_tokens[i]);
   }
 }
};
void test_empty_partial_width_and_callback_paths(){
 Host h;h.reset();e::Enumerate search(h);h.enumerator=&search;
 std::uint32_t parent=0x12345678u,side=0xaabbccdd;
 auto out=search.search(kTable,kRecord,parent,side);
 assert(out.code==e::Code::Completed&&out.empty_tree);
 assert(parent==0x1234ffffu && side==0xaabbcc00u);
 assert(out.comparator_calls==0u&&out.sentinel_writes_committed==0u);
 assert(h.u32(e::kSentinelFlags)==0x300u);
 h.reset();h.set16(kTable+0x10u,1u);
 h.set16(h.slot(1)+0u,0xffffu);h.set16(h.slot(1)+2u,0xffffu);
 h.set16(h.slot(1)+8u,200u);h.set16(kRecord,100u);
 parent=0xf00d1234u;side=0xaabbcc55u;
 out=search.search(kTable,kRecord,parent,side);
 assert(out.code==e::Code::Completed&&!out.empty_tree);
 assert(out.comparator_calls==1u && out.visited_nodes==1u);
 assert(parent==0xf00d0001u && side==0xaabbcc01u);
 assert(out.initialized_sentinel && out.sentinel_writes_committed==5u);
 assert(h.u32(e::kSentinelFlags)==0x301u);
 assert(h.u16(e::kSentinelNode)==0xffffu);
 assert(h.u16(e::kSentinelNode+2u)==0xffffu);
 assert(h.u16(e::kSentinelNode+4u)==0xffffu);
 assert(h.u16(e::kSentinelNode+6u)==1u);
 h.set16(kRecord,300u);
 parent=0x98760001u;side=0x123456ffu;
 out=search.search(kTable,kRecord,parent,side);
 assert(out.code==e::Code::Completed && out.last_side==0u);
 assert(parent==0x98760001u && side==0x12345600u);
 assert(out.sentinel_writes_committed==0u && !out.initialized_sentinel);
 assert(h.branch_left==1u && h.branch_right==1u);
}
void test_multinode_and_dynamic_reread(){
 Host h;h.reset();e::Enumerate s(h);h.enumerator=&s;
 // Root=1(50); left=2(20); right=3(80); left.right=4(30)
 h.set16(kTable+0x10u,1u);
 for(auto i:{1u,2u,3u,4u}){
   h.set16(h.slot(static_cast<std::uint16_t>(i)),0xffffu);
   h.set16(h.slot(static_cast<std::uint16_t>(i))+2u,0xffffu);
 }
 h.set16(h.slot(1u)+8u,50u);h.set16(h.slot(2u)+8u,20u);
 h.set16(h.slot(3u)+8u,80u);h.set16(h.slot(4u)+8u,30u);
 h.set16(h.slot(1u),2u);h.set16(h.slot(1u)+2u,3u);
 h.set16(h.slot(2u)+2u,4u);
 h.set16(kRecord,40u);
 std::uint32_t parent=0xdeadbaadu,side=0xcafebabeu;
 auto out=s.search(kTable,kRecord,parent,side);
 assert(out.code==e::Code::Completed && out.visited_nodes==3u);
 assert(out.last_parent==4u && out.last_side==0u);
 assert(parent==0xdead0004u && side==0xcafeba00u);
 assert(out.comparator_calls==3u && h.branch_left==1u&&h.branch_right==2u);
 // Comparator changes storage pointer after the compare. A stale array
 // would use old slot and fail; original explicitly refreshes [table+4].
 h.reset();h.set16(kTable+0x10u,1u);
 h.set16(h.slot(1u),0xffffu);h.set16(h.slot(1u)+2u,0xffffu);
 h.set16(h.slot(1u)+8u,50u);h.set16(kRecord,40u);
 h.relocate_compare=true;
 parent=0x12340000u;side=0x43210000u;
 out=s.search(kTable,kRecord,parent,side);
 assert(out.code==e::Code::Completed&&out.last_parent==1u);
 assert(h.u32(kTable+4u)==0x5000u);
 assert(out.initialized_sentinel&&h.last_record_ptr==kArray+16u+8u);
 assert(parent==0x12340001u && side==0x43210001u);
}
void test_failure_provenance(){
 Host h;h.reset();e::Enumerate s(h);h.enumerator=&s;
 h.set16(kTable+0x10u,1u);h.set16(h.slot(1u),0xffffu);
 h.set16(h.slot(1u)+2u,0xffffu);h.set16(h.slot(1u)+8u,42u);
 h.set16(kRecord,2u);
 std::uint32_t parent=0x12345678u,side=0xffeeddccu;
 h.reject_compare=true;
 auto out=s.search(kTable,kRecord,parent,side);
 assert(out.code==e::Code::HostRejected && out.stage==e::Stage::CompareOriginalCallback);
 assert(parent==0x12340001u && side==0xffeedd00u);
 assert(h.u32(e::kSentinelFlags)==0x300u);
 h.reject_compare=false;h.throw_compare=true;
 out=s.search(kTable,kRecord,parent,side);
 assert(out.code==e::Code::HostRejected && !out.initialized_sentinel);
 h.throw_compare=false;h.reenter_compare=true;
 out=s.search(kTable,kRecord,parent,side);
 assert(out.code==e::Code::Completed && h.nested==e::Code::Reentrant);
 h.reenter_compare=false;h.fail_at_write=3u;
 h.reset();h.set16(kTable+0x10u,1u);h.set16(h.slot(1u),0xffffu);
 h.set16(h.slot(1u)+2u,0xffffu);h.set16(h.slot(1u)+8u,42u);
 h.set16(kRecord,2u);h.fail_at_write=3u;
 out=s.search(kTable,kRecord,parent,side);
 assert(out.code==e::Code::HostRejected);
 assert(out.stage==e::Stage::WriteSentinelWord2);
 assert(out.sentinel_writes_committed==2u);
 assert(h.u32(e::kSentinelFlags)==0x301u);
 assert(h.u16(e::kSentinelNode)==0xffffu);
 assert(h.bytes.count(e::kSentinelNode+2u)==0u);
 h.reset();h.set16(kTable+0x10u,1u);
 h.set32(kTable+4u,0u);
 out=s.search(kTable,kRecord,parent,side);
 assert(out.code==e::Code::NullArrayHazard && out.visited_nodes==1u);
 assert(out.caller_parent_initialized && out.caller_side_initialized);
 h.reset();h.set16(kTable+0x10u,1u);h.set32(kTable,0u);
 out=s.search(kTable,kRecord,parent,side);
 assert(out.code==e::Code::NullComparatorHazard);
 h.reset();h.set16(kTable+0x10u,1u);h.set32(kTable+4u,0xfffffff0u);
 out=s.search(kTable,kRecord,parent,side);
 assert(out.code==e::Code::AddressOverflow);
 out=s.search(0u,kRecord,parent,side);
 assert(out.code==e::Code::NullTableHazard);
}
void test_full_original_registration_chain(){
 Host h;h.reset();
 e::Enumerate search(h);p::AcquireSlot acquire(h);
 p::RegisterSlot registerslot(h,acquire);l::LinkNode linker(h);
 r::SentinelSlotAccessor sentinel(h);r::RotateNode rotator(h,sentinel);
 b::Rebalance balance(h,sentinel,rotator);f::Forwarder forward(h);
 h.enumerator=&search;h.registrar=&registerslot;h.linker=&linker;
 h.rebalancer=&balance;
 const std::vector<std::uint16_t> keys={60u,30u,90u,20u,40u,80u,
   100u,35u,45u,25u,75u,85u,95u,105u,15u,50u,55u,65u};
 bool saw_left=false,saw_right=false,saw_double=false,saw_recolor=false;
 for(std::uint16_t key:keys){
   auto out=forward.write(kContext,key,1000+key);
   assert(out.code==f::Code::Written);
   assert(out.slot_handle==h.expected_tokens.size());
   assert(h.last_reg.code==p::Code::Completed);
   assert(h.last_balance.code==b::Code::Completed);
   assert(h.last_reg.enumerated && h.last_reg.linked);
   h.expected_tokens.push_back(key);
   h.verifyall();
   saw_left|=h.branch_left!=0u;
   saw_right|=h.branch_right!=0u;
   saw_double|=(h.last_balance.left_rotations!=0u&&h.last_balance.right_rotations!=0u);
   saw_recolor|=h.last_balance.recolor_cases!=0u;
 }
 assert(saw_left && saw_right && saw_recolor);
 assert(h.u16(kTable+0x12u)==keys.size());
 assert(h.compare_count>keys.size());
 // Dedicated three-node chain forces mirrored double rotation to verify
 // adapter ABI and original 16-bit/8-bit argument writes end-to-end.
 h.reset();
 for(std::uint16_t key:{10u,20u,15u}){
   const auto out=forward.write(kContext,key,1000+key);
   assert(out.code==f::Code::Written);
   h.expected_tokens.push_back(key);h.verifyall();
 }
 assert(h.last_balance.left_rotations==1u&&h.last_balance.right_rotations==1u);
 (void)saw_double;
}
}
int main(){test_empty_partial_width_and_callback_paths();
 test_multinode_and_dynamic_reread();test_failure_provenance();
 test_full_original_registration_chain();return 0;}
