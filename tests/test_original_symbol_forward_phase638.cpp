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
namespace f=sin_compat::original_symbol_forward_phase638;
namespace r=sin_compat::original_elite_range_registration_phase637;
namespace {
constexpr std::uint32_t context=0x1000u,table=0x2000u,slots=0x3000u;
class Host final:public f::IHost,public r::IHost {
public:
 std::map<std::uint32_t,std::uint32_t> mem;
 std::vector<std::string> trace;
 std::uint32_t blocked_read=0u,blocked_write=0u;
 bool deny_lookup=false,deny_alloc=false,throw_lookup=false,relocate=false;
 bool reenter=false,deny_map=false;
 f::Forwarder* current=nullptr;
 f::Code nested=f::Code::Written;
 std::uint32_t seen_tree=0u,seen_symbol=0u,seen_category=0u;
 std::uint16_t seen_token=0u;
 std::int32_t mapped_seen=0;
 bool read_u32(std::uint32_t a,std::uint32_t& value)override {
   trace.push_back("read:"+std::to_string(a));
   if(a==blocked_read)return false;
   auto it=mem.find(a);if(it==mem.end())return false;
   value=it->second;return true;
 }
 bool write_u32(std::uint32_t a,std::uint32_t value)override {
   trace.push_back("write:"+std::to_string(a));
   if(a==blocked_write)return false;
   mem[a]=value;return true;
 }
 bool lookup_original_tree_2228f7a0(std::uint32_t a,
   std::uint32_t& scratch,std::uint32_t symbol,
   std::uint16_t& token)override {
   trace.emplace_back("lookup");
   seen_tree=a;seen_symbol=symbol;
   assert(scratch==symbol);
   scratch=0xdeadc0deu;
   if(reenter && current)nested=current->write(context,0x80u,55).code;
   if(throw_lookup)throw std::runtime_error("host tree fault");
   if(deny_lookup)return false;
   token=0x50f0u;return true;
 }
 bool allocate_original_slot_22282270(std::uint32_t a,
   std::uint16_t& token,std::uint16_t& slot)override {
   trace.emplace_back("allocate");
   assert(a==table);seen_token=token;
   if(deny_alloc)return false;
   token=0x13u;
   if(relocate)mem[table+4u]=0x4000u;
   slot=3u;return true;
 }
 bool report_original_diagnostic(r::Diagnostic,
   std::uint32_t category,std::uint32_t)override {
   seen_category=category; trace.emplace_back("diagnostic");return true;
 }
 bool convert_original_id_22073700(std::uint32_t,
   std::int32_t id,std::int32_t& mapped)override {
   trace.emplace_back("convert");if(deny_map)return false;
   mapped=id+100;return true; // fabricated mapping, not retail IDs
 }
 bool forward_original_symbol_22282370(std::uint32_t c,
   std::uint32_t symbol,std::int32_t mapped)override {
   trace.emplace_back("forward");mapped_seen=mapped;
   if(!current)return false;
   return current->write(c,symbol,mapped).code==f::Code::Written;
 }
 void reset() {
   mem.clear();trace.clear();blocked_read=blocked_write=0u;
   deny_lookup=deny_alloc=throw_lookup=relocate=reenter=deny_map=false;
   current=nullptr;seen_tree=seen_symbol=seen_category=0u;
   seen_token=0u;mapped_seen=0;nested=f::Code::Written;
   mem[context]=table;mem[table+4u]=slots;
 }
};
void forwarding_tests(){
 Host h;h.reset();f::Forwarder forwarder(h);h.current=&forwarder;
 auto v=forwarder.write(context,0x5151u,-7);
 assert(v.code==f::Code::Written && v.mapped_index_written);
 assert(v.tree_lookup_completed && v.allocation_completed);
 assert(v.tree_token==0x50f0u && v.slot_handle==3u);
 assert(v.table_va==table && v.slot_address==slots+3u*16u+12u);
 assert(h.mem[v.slot_address]==0xfffffff9u);
 assert(h.seen_tree==table+0x1cu && h.seen_symbol==0x5151u);
 assert(h.seen_token==0x50f0u);
 assert((h.trace==std::vector<std::string>{"read:4096","lookup",
   "allocate","read:8196","write:12348"}));
 h.reset();h.current=&forwarder;h.relocate=true;
 v=forwarder.write(context,33u,42);
 assert(v.code==f::Code::Written && v.slot_address==0x4000u+60u);
 assert(h.mem[v.slot_address]==42u);
 assert(h.mem.count(12348u)==0u);
 h.reset();h.current=&forwarder;h.deny_lookup=true;
 v=forwarder.write(context,33u,1);
 assert(v.code==f::Code::HostRejected && !v.allocation_completed);
 assert(h.trace.back()=="lookup");
 h.reset();h.current=&forwarder;h.deny_alloc=true;
 v=forwarder.write(context,33u,1);
 assert(v.code==f::Code::HostRejected && v.tree_lookup_completed);
 assert(!v.allocation_completed && h.trace.back()=="allocate");
 h.reset();h.current=&forwarder;h.blocked_write=12348u;
 v=forwarder.write(context,33u,1);
 assert(v.code==f::Code::HostRejected && v.allocation_completed);
 assert(!v.mapped_index_written && h.mem.count(12348u)==0);
 h.reset();h.current=&forwarder;h.throw_lookup=true;
 v=forwarder.write(context,33u,1);
 assert(v.code==f::Code::HostRejected && !v.tree_lookup_completed);
 h.reset();h.current=&forwarder;h.reenter=true;
 v=forwarder.write(context,33u,1);
 assert(v.code==f::Code::Written && h.nested==f::Code::Reentrant);
 h.reset();h.current=&forwarder;
 v=forwarder.write(0,33u,1);
 assert(v.code==f::Code::OriginalNullContextHazard && h.trace.empty());
 h.mem[context]=0u;v=forwarder.write(context,33u,1);
 assert(v.code==f::Code::OriginalNullTableHazard);
 h.reset();h.current=&forwarder;h.mem[table+4u]=0u;
 v=forwarder.write(context,33u,1);
 assert(v.code==f::Code::OriginalNullSlotArrayHazard && v.allocation_completed);
 h.reset();h.current=&forwarder;h.mem[context]=0xfffffff0u;
 v=forwarder.write(context,33u,1);
 assert(v.code==f::Code::OriginalAddressOverflow);
 h.reset();h.current=&forwarder;h.mem[table+4u]=0xfffffff0u;
 v=forwarder.write(context,33u,1);
 assert(v.code==f::Code::OriginalAddressOverflow && v.allocation_completed);
 h.reset();h.current=&forwarder;h.blocked_read=context;
 v=forwarder.write(context,33u,1);
 assert(v.code==f::Code::HostRejected && !v.tree_lookup_completed);
}
void condition_wrapper_tests(){
 Host h;h.reset();f::Forwarder fw(h);h.current=&fw;
 r::RegisterRange registrar(h);f::ConditionWrapper wrapper(registrar);
 constexpr std::uint32_t owner=0x7000u,node=owner+0x34u,rec=0x8000u;
 h.mem[node+0x14u]=rec;h.mem[node+4u]=9999u;
 h.mem[node+8u]=0xffffffffu;h.mem[node+0x10u]=0u;
 h.mem[node]=40u;h.mem[rec]=context;h.mem[rec+4u]=0u;
 // Actual compiled Phase637 registration; fabricated mapping callback.
 auto v=wrapper.add_condition(owner,0x5050u,3,0x9090u);
 assert(v.code==r::Code::Registered && v.node_va==node);
 assert(h.mem[node+4u]==3u && h.mem[node+8u]==3u);
 assert(h.mem[node+12u]==40u);
 assert(h.mapped_seen==103 && h.mem[slots+60u]==103u);
 assert(h.mem[rec+4u]==104u);
 assert(h.seen_category==0u);
 v=wrapper.add_condition(owner,0x5050u,2,0x9090u);
 assert(v.code==r::Code::OriginalBelowRangeStart);
 assert(h.seen_category==f::kOriginalConditionCategoryVA);
 v=wrapper.add_condition(0u,0x5050u,3,0x9090u);
 assert(v.code==r::Code::OriginalNullNodeHazard);
 v=wrapper.add_condition(0xfffffff0u,0x5050u,3,0x9090u);
 assert(v.code==r::Code::OriginalAddressOverflow);
}
}
int main(){forwarding_tests();condition_wrapper_tests();return 0;}
