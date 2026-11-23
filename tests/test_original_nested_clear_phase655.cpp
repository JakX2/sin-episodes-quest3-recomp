#include "sin_compat/original_nested_clear_phase655.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

namespace n=sin_compat::original_nested_clear_phase655;
namespace {
constexpr std::uint32_t kMap=0x4000u,kArray=0x9000u,kBlocks=0xb000u;
constexpr std::uint32_t kOutput=0x6100u,kScratch=0x6200u;
struct Host final:n::IHost {
 std::map<std::uint32_t,std::uint8_t> bytes;
 std::vector<std::pair<std::uint32_t,std::uint16_t>> writes16;
 std::vector<std::uint32_t> freed;
 std::uint32_t queried_map=0,queried_key=0,queried_context=0;
 std::uint32_t free_calls=0,lookup_calls=0,read_calls=0,write_calls=0;
 unsigned fail_after=0;
 bool fail_lookup=false,fail_free=false;
 n::ClearSlots* clear=nullptr;
 bool reentrant=false,was_reentrant=false,throw_free=false;
 void put8(std::uint32_t a,std::uint8_t v){bytes[a]=v;}
 void put16(std::uint32_t a,std::uint16_t v){
  put8(a,static_cast<std::uint8_t>(v));
  put8(a+1u,static_cast<std::uint8_t>(v>>8));
 }
 void put32(std::uint32_t a,std::uint32_t v){
  for(unsigned i=0;i<4u;++i)put8(a+i,static_cast<std::uint8_t>(v>>(8u*i)));
 }
 std::uint8_t byte(std::uint32_t a)const{
  const auto it=bytes.find(a);assert(it!=bytes.end());return it->second;
 }
 std::uint16_t word(std::uint32_t a)const{
  return static_cast<std::uint16_t>(byte(a)|(static_cast<std::uint16_t>(byte(a+1u))<<8u));
 }
 std::uint32_t dword(std::uint32_t a)const{
  std::uint32_t v=0;
  for(unsigned i=0;i<4u;++i)v|=static_cast<std::uint32_t>(byte(a+i))<<(8u*i);
  return v;
 }
 bool permit(bool writing){
  if(writing){++write_calls;if(fail_after&&write_calls>=fail_after)return false;}
  else ++read_calls;
  return true;
 }
 bool read_u8(std::uint32_t a,std::uint8_t& v)override{
  if(!permit(false)||!bytes.count(a))return false;
  v=byte(a);return true;
 }
 bool read_u16(std::uint32_t a,std::uint16_t& v)override{
  if(!permit(false)||!bytes.count(a)||!bytes.count(a+1u))return false;
  v=word(a);return true;
 }
 bool read_u32(std::uint32_t a,std::uint32_t& v)override{
  if(!permit(false))return false;
  for(unsigned i=0;i<4u;++i)if(!bytes.count(a+i))return false;
  v=dword(a);return true;
 }
 bool write_u16(std::uint32_t a,std::uint16_t v)override{
  if(!permit(true))return false;
  writes16.push_back({a,v});put16(a,v);return true;
 }
 bool write_u32(std::uint32_t a,std::uint32_t v)override{
  if(!permit(true))return false;
  put32(a,v);return true;
 }
 bool free_original_2226e3e0(std::uint32_t block)override{
  ++free_calls;if(throw_free)throw std::runtime_error("host free");
  if(fail_free)return false;
  freed.push_back(block);return true;
 }
 bool find_original_2228eb40(std::uint32_t map,std::uint32_t key,
                             std::uint32_t context,std::uint16_t& handle)override{
  ++lookup_calls;queried_map=map;queried_key=key;queried_context=context;
  assert(word(key)==n::kInvalid&&word(key+2u)==n::kInvalid);
  if(fail_lookup)return false;
  handle=3u;return true;
 }
 void reset(std::uint16_t next=0u){
  bytes.clear();writes16.clear();freed.clear();write_calls=read_calls=0u;
  fail_after=0u;fail_free=fail_lookup=false;throw_free=false;
  free_calls=lookup_calls=0u;
  put32(kMap+4u,kArray);put16(kMap+0x10u,2u);
  put16(kMap+0x12u,7u);put16(kMap+0x14u,n::kInvalid);
  put16(kMap+0x16u,next);put32(kMap+0x1cu,kBlocks);
  put32(kMap+0x28u,0u);put32(kMap+0x30u,0x77777777u);
  put32(n::kSentinelFlagVA,0xAABBCC00u);
  put16(kOutput,0u);put16(kScratch,0u);put16(kScratch+2u,0u);
  for(unsigned i=0;i<next;++i){
   const auto a=kArray+i*12u;
   put16(a,0xaaaau);put16(a+2u,0xbbbbu);
   put16(a+4u,0x1234u);put16(a+6u,0u);
   put32(a+8u,0xabc00000u+i);
  }
 }
};

void empty_rebuild(){
 Host h;h.reset(0u);n::ClearSlots clear(h);
 const auto r=clear.run(kMap);
 assert(r.code==n::Code::Completed&&r.slots_recycled==0u);
 assert(!r.sentinel_initialized&&r.free_head==n::kInvalid);
 assert(h.word(kMap+0x10u)==n::kInvalid&&h.word(kMap+0x12u)==0u);
 assert(h.word(kMap+0x14u)==n::kInvalid&&h.word(kMap+0x16u)==0u);
 assert(h.dword(n::kSentinelFlagVA)==0xAABBCC00u);
 assert(h.writes16.size()==3u);
}
void rebuild_order_and_content(){
 Host h;h.reset(4u);n::ClearSlots clear(h);
 const auto r=clear.run(kMap);
 assert(r.code==n::Code::Completed&&r.slots_recycled==4u);
 assert(r.free_head==0u&&r.sentinel_initialized);
 assert(h.word(kMap+0x10u)==n::kInvalid&&h.word(kMap+0x12u)==0u);
 assert(h.word(kMap+0x14u)==0u&&h.word(kMap+0x16u)==4u);
 assert(h.dword(n::kSentinelFlagVA)==0xAABBCC01u);
 for(unsigned i=0;i<4u;++i){
  const auto node=kArray+i*12u;
  assert(h.word(node)==i);
  assert(h.word(node+2u)==(i==3u?n::kInvalid:i+1u));
  assert(h.word(node+4u)==0x1234u&&h.word(node+6u)==0u);
  assert(h.dword(node+8u)==0xabc00000u+i);
 }
 // Four sentinel WORDs, then each node in descending order: right, left.
 assert(h.writes16.size()==4u+8u+3u);
 for(unsigned i=0;i<4u;++i){
  const auto node=kArray+(3u-i)*12u;
  assert(h.writes16[4u+2u*i].first==node+2u);
  assert(h.writes16[4u+2u*i+1u].first==node);
 }
 const auto repeat=clear.run(kMap);
 assert(repeat.code==n::Code::Completed&&!repeat.sentinel_initialized);
 assert(repeat.slots_recycled==4u&&h.word(kMap+0x14u)==0u);
}
void block_release_and_failure(){
 Host h;h.reset(3u);h.put32(kMap+0x28u,3u);
 h.put32(kBlocks,0xf100u);h.put32(kBlocks+4u,0u);
 h.put32(kBlocks+8u,0xf300u);
 n::ClearSlots clear(h);n::ReleaseBlocks release(h,clear);
 const auto r=release.run(kMap);
 assert(r.code==n::Code::Completed&&r.slots_recycled==3u);
 assert(r.block_free_calls==3u&&h.free_calls==3u);
 assert((h.freed==std::vector<std::uint32_t>{0xf100u,0u,0xf300u}));
 assert(h.dword(kMap+0x28u)==0u&&h.dword(kMap+0x1cu)==kBlocks);
 assert(h.dword(kBlocks)==0xf100u);
 const auto second=release.run(kMap);
 assert(second.code==n::Code::Completed&&second.block_free_calls==0u);
 assert(h.free_calls==3u);
 h.reset(2u);h.put32(kMap+0x28u,2u);
 h.put32(kBlocks,0xf100u);h.put32(kBlocks+4u,0xf200u);
 h.fail_free=true;
 const auto failed=release.run(kMap);
 assert(failed.code==n::Code::HostRejected);
 assert(failed.block_free_calls==1u);
 assert(h.word(kMap+0x10u)==n::kInvalid);
 assert(h.dword(kMap+0x28u)==2u);
 h.fail_free=false;h.throw_free=true;
 const auto throwing=release.run(kMap);
 assert(throwing.code==n::Code::HostRejected&&throwing.block_free_calls==1u);
}
void name_view(){
 Host h;h.reset();n::FindNameView view(h);
 const auto absent=view.run(0u,kOutput,0u,0u);
 assert(absent.code==n::Code::Completed&&absent.null_name);
 assert(absent.return_eax==kOutput &&h.word(kOutput)==n::kInvalid);
 assert(h.lookup_calls==0u);
 const auto found=view.run(kMap,kOutput,0xe000u,kScratch);
 assert(found.code==n::Code::Completed&&found.lookup_called);
 assert(found.return_eax==kOutput&&found.handle==3u);
 assert(h.dword(kMap+0x30u)==0xe000u&&h.word(kOutput)==3u);
 assert(h.word(kScratch)==n::kInvalid&&h.word(kScratch+2u)==n::kInvalid);
 assert(h.lookup_calls==1u && h.queried_map==kMap);
 assert(h.queried_key==kScratch&&h.queried_context==kMap);
 h.fail_lookup=true;
 const auto rejected=view.run(kMap,kOutput,0xe100u,kScratch);
 assert(rejected.code==n::Code::HostRejected&&rejected.lookup_called);
 assert(h.dword(kMap+0x30u)==0xe100u);
 assert(h.word(kOutput)==3u);
}
void invalid_and_partial(){
 Host h;h.reset(1u);n::ClearSlots clear(h);
 assert(clear.run(0u).code==n::Code::NullOwnerHazard);
 h.fail_after=2u;
 const auto fail=clear.run(kMap);
 assert(fail.code==n::Code::HostRejected);
 assert(fail.writes_committed==1u);
 assert(h.dword(n::kSentinelFlagVA)==0xAABBCC01u);
 // The native initializes the flag *before* its sentinel WORDs.
 assert(h.word(kMap+0x10u)==2u);
 h.reset(1u);h.put32(kMap+4u,0u);
 const auto no_array=clear.run(kMap);
 assert(no_array.code==n::Code::NullArrayHazard);
 h.reset();h.put32(kMap+0x28u,0x80000001u);
 n::ReleaseBlocks rel(h,clear);
 const auto negative=rel.run(kMap);
 assert(negative.code==n::Code::Completed && negative.block_free_calls==0u);
 assert(h.dword(kMap+0x28u)==0u);
}
}
int main(){
 empty_rebuild();rebuild_order_and_content();block_release_and_failure();
 name_view();invalid_and_partial();
}
