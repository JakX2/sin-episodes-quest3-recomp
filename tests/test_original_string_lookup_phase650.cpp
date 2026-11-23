#include "sin_compat/original_string_lookup_phase650.hpp"
#include "sin_compat/original_nested_comparators_phase648.hpp"
#include "sin_compat/original_string_casefold_phase649.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <map>
#include <vector>
#include <string>
#include <stdexcept>
namespace q=sin_compat::original_string_lookup_phase650;
namespace c=sin_compat::original_nested_comparators_phase648;
namespace f=sin_compat::original_string_casefold_phase649;
namespace {
constexpr std::uint32_t m=0x4000u,output=0x400u,ref=0x408u;
constexpr std::uint32_t slots=0x10000u,block_pointers=0x20000u;
struct Guest final:q::IHost,c::IHost,f::IHost {
 std::map<std::uint32_t,std::uint8_t> data;
 c::Comparators* comparator=nullptr;
 f::Casefold* casefold=nullptr;
 q::LookupOrInsert* lookup=nullptr;
 q::Code reentered=q::Code::Completed;
 std::uint32_t alloc_base=0x30000u,actual_alloc_count=0u;
 std::uint32_t appends=0u,registered=0u,compare_calls=0u,casefold_calls=0u;
 std::uint32_t fail_address=0u;bool fail_alloc=false,fail_append=false,fail_insert=false;
 bool throw_compare=false,reenter_lookup=false;
 void put8(std::uint32_t a,std::uint8_t v){data[a]=v;}
 void put16(std::uint32_t a,std::uint16_t v){put8(a,static_cast<std::uint8_t>(v));put8(a+1u,static_cast<std::uint8_t>(v>>8));}
 void put32(std::uint32_t a,std::uint32_t v){for(unsigned i=0;i<4u;++i)put8(a+i,static_cast<std::uint8_t>(v>>(i*8u)));}
 std::uint8_t get8(std::uint32_t a)const {auto i=data.find(a);assert(i!=data.end());return i->second;}
 std::uint16_t get16(std::uint32_t a)const {return static_cast<std::uint16_t>(get8(a)|(get8(a+1u)<<8u));}
 std::uint32_t get32(std::uint32_t a)const {std::uint32_t v=0;for(unsigned i=0;i<4u;++i)v|=static_cast<std::uint32_t>(get8(a+i))<<(8u*i);return v;}
 bool read_u8(std::uint32_t va,std::uint8_t& v)override{
  if(fail_address==va)return false;
  auto i=data.find(va);if(i==data.end())return false;
  v=i->second;return true;
 }
 bool read_u16(std::uint32_t va,std::uint16_t& v)override{
  std::uint8_t a=0,b=0;if(!read_u8(va,a)||!read_u8(va+1u,b))return false;
  v=static_cast<std::uint16_t>(a|(static_cast<std::uint16_t>(b)<<8u));return true;
 }
 bool read_u32(std::uint32_t va,std::uint32_t& v)override{
  v=0;for(unsigned i=0;i<4u;++i){std::uint8_t b=0;if(!read_u8(va+i,b))return false;v|=static_cast<std::uint32_t>(b)<<(8u*i);}return true;
 }
 bool write_u8(std::uint32_t va,std::uint8_t v)override{
  if(fail_address==va)return false;
  put8(va,v);
  return true;
 }
 bool write_u16(std::uint32_t va,std::uint16_t v)override{
  if(fail_address==va)return false;
  put16(va,v);
  return true;
 }
 bool write_u32(std::uint32_t va,std::uint32_t v)override{
  if(fail_address==va)return false;
  put32(va,v);
  return true;
 }
 bool compare_original_223a5574(std::uint32_t left,std::uint32_t right,std::int32_t& eax)override{
  ++casefold_calls;assert(casefold);
  const auto r=casefold->compare(left,right);
  if(r.code!=f::Code::Completed)return false;
  eax=r.comparison;return true;
 }
 bool classify_original_223a898a(std::uint32_t,std::uint32_t,std::uint32_t& value)override{value=0u;return true;}
 bool locale_map_original_223ac9e7(const f::MapRequest&,f::MapAnswer&)override{return false;}
 bool compare_original_nested(std::uint32_t function,std::uint32_t key,
    std::uint32_t node,std::uint32_t context,bool& less)override{
  assert(function==c::kDelegatingVA && context==m && comparator);
  ++compare_calls;
  if(throw_compare)throw std::runtime_error("comparator exception");
  if(reenter_lookup&&lookup){
   reentered=lookup->run(m,output,0x5000u,ref).code;
  }
  const auto r=comparator->delegated(key,node,context);
  if(r.code!=c::Code::Completed)return false;
  less=r.eax!=0u;return true;
 }
 bool allocate_original_2226e390(std::uint32_t size,std::uint32_t& ptr)override{
  if(fail_alloc)return false;
  assert(size>=q::kDefaultBlockCapacity+11u);
  ++actual_alloc_count;ptr=alloc_base;alloc_base+=0x10000u;
  return true;
 }
 bool append_original_2228ed80(std::uint32_t owner,std::uint32_t block,
    std::uint32_t& index)override{
  assert(owner==m+0x1cu);
  if(fail_append)return false;
  index=get32(m+0x28u);
  const std::uint32_t old=index;
  assert(index<32u);
  put32(block_pointers+index*4u,block);
  put32(m+0x1cu,block_pointers);
  put32(m+0x28u,old+1u);
  ++appends;return true;
 }
 bool register_original_2228f710(std::uint32_t map,std::uint32_t string_ref,
    std::uint16_t& handle)override{
  assert(map==m&&string_ref==ref);
  if(fail_insert)return false;
  const std::uint16_t index=static_cast<std::uint16_t>(registered);
  const std::uint32_t slot=slots+static_cast<std::uint32_t>(index)*12u;
  put16(slot,kInvalid());put16(slot+2u,kInvalid());put16(slot+4u,kInvalid());
  put16(slot+6u,1u);
  put16(slot+8u,get16(ref));put16(slot+10u,get16(ref+2u));
  if(registered==0u){put16(m+0x10u,index);}else{
   std::uint16_t cursor=get16(m+0x10u);
   for(unsigned depth=0;depth<40u;++depth){
    const std::uint32_t current=slots+static_cast<std::uint32_t>(cursor)*12u;
    bool less=false;
    if(!compare_original_nested(c::kDelegatingVA,ref,current+8u,m,less))return false;
    const std::uint32_t child_va=current+(less?0u:2u);
    const auto child=get16(child_va);
    if(child==q::kInvalid){put16(child_va,index);put16(slot+4u,cursor);break;}
    cursor=child;
    assert(depth<39u);
   }
  }
  ++registered;put16(m+0x16u,static_cast<std::uint16_t>(registered));
  handle=index;return true;
 }
 static constexpr std::uint16_t kInvalid(){return q::kInvalid;}
 void string(std::uint32_t addr,const std::string& s){for(std::size_t i=0;i<s.size();++i)put8(addr+static_cast<std::uint32_t>(i),static_cast<std::uint8_t>(s[i]));put8(addr+static_cast<std::uint32_t>(s.size()),0u);}
 void init(){
  put32(m,c::kDelegatingVA);put32(m+4u,slots);put32(m+8u,32u);put32(m+0xcu,0u);
  put16(m+0x10u,q::kInvalid);put16(m+0x14u,q::kInvalid);
  put16(m+0x12u,0u);put16(m+0x16u,0u);
  put32(m+0x1cu,0u);put32(m+0x20u,0u);put32(m+0x24u,8u);put32(m+0x28u,0u);
  put32(m+0x30u,0xaaaaaaaau);put32(f::kModeGlobal,0u);
  put32(q::kSentinelStateVA,0u);put16(output,0x1234u);
 }
};
void test_complete_original_nested_registration_and_search(){
 Guest h;h.init();q::TreeSearch search(h);q::BlockFit fit(h);q::LookupOrInsert lookup(h,search,fit);
 c::Comparators cmp(h);f::Casefold casefold(h);h.comparator=&cmp;h.casefold=&casefold;h.lookup=&lookup;
 auto r=lookup.run(m,output,0u,ref);
 assert(r.code==q::Code::Completed&&r.original_null_name&&r.handle==q::kInvalid);
 assert(h.get16(output)==q::kInvalid && h.get32(m+0x30u)==0xaaaaaaaau);
 assert(h.registered==0u&&h.appends==0u);
 h.string(0x5000u,"Alpha");
 r=lookup.run(m,output,0x5000u,ref);
 assert(r.code==q::Code::Completed&&r.inserted&&r.handle==0u);
 assert(r.allocated_block&&r.bytes_required==6u&&r.bytes_copied==6u);
 assert(r.block_index==0u&&h.actual_alloc_count==1u&&h.appends==1u);
 assert(h.get16(output)==0u&&h.get16(m+0x10u)==0u);
 assert(h.get32(m+0x30u)==0x5000u);
 assert(h.get32(h.get32(block_pointers))==0x800u);
 assert(h.get32(h.get32(block_pointers)+4u)==6u);
 assert(h.get8(h.get32(block_pointers)+8u)=='A');
 assert(h.get16(slots+8u)==0u&&h.get16(slots+10u)==0u);
 assert(h.get32(q::kSentinelStateVA)==0u); // first empty lookup has no traversal
 h.string(0x5100u,"ALPHA");
 r=lookup.run(m,output,0x5100u,ref);
 assert(r.code==q::Code::Completed&&r.found_existing&&r.handle==0u);
 assert(!r.inserted&&r.bytes_copied==0u&&h.registered==1u&&h.appends==1u);
 assert(h.get32(m+0x30u)==0x5100u);
 assert(h.get32(h.get32(block_pointers)+4u)==6u);
 assert(h.get32(q::kSentinelStateVA)==0u); // equality does NOT initialize sentinel
 h.string(0x5200u,"Zulu");
 r=lookup.run(m,output,0x5200u,ref);
 assert(r.code==q::Code::Completed&&r.handle==1u&&r.inserted);
 assert(!r.allocated_block&&r.block_used_before==6u);
 assert(h.get16(slots+2u)==1u && h.get16(slots+12u+4u)==0u);
 assert(h.get32(q::kSentinelStateVA)==1u);
 assert(h.get16(q::kSentinelWordsVA)==q::kInvalid);
 assert(h.get16(q::kSentinelWordsVA+6u)==1u);
 h.string(0x5300u,"aardvark");
 r=lookup.run(m,output,0x5300u,ref);
 assert(r.code==q::Code::Completed&&r.handle==2u&&r.inserted);
 assert(h.get16(slots)==2u);
 assert(h.registered==3u&&h.appends==1u);
 h.string(0x5400u,"zulu");
 r=lookup.run(m,output,0x5400u,ref);
 assert(r.code==q::Code::Completed&&r.found_existing&&r.handle==1u);
 assert(h.registered==3u);
 // Deliberately fill current block; the next string goes to a new block.
 const auto first=h.get32(block_pointers);h.put32(first+4u,0x800u-2u);
 h.string(0x5500u,"Echo");
 r=lookup.run(m,output,0x5500u,ref);
 assert(r.code==q::Code::Completed&&r.inserted&&r.allocated_block);
 assert(r.block_index==1u&&r.handle==3u&&h.appends==2u);
 assert(h.get16(slots+3u*12u+8u)==1u);
 assert(h.get16(slots+3u*12u+10u)==0u);
 h.string(0x5600u,"ECHO");
 r=lookup.run(m,output,0x5600u,ref);
 assert(r.code==q::Code::Completed&&r.found_existing&&r.handle==3u);
 assert(h.registered==4u && h.compare_calls>0u && h.casefold_calls>0u);
}
void test_block_fit_and_error_edges(){
 Guest h;h.init();q::TreeSearch search(h);q::BlockFit fit(h);q::LookupOrInsert lookup(h,search,fit);
 c::Comparators cmp(h);f::Casefold casefold(h);h.comparator=&cmp;h.casefold=&casefold;h.lookup=&lookup;
 auto r=fit.find(m,6u);assert(r.code==q::Code::Completed&&r.block_index==0xffffffffu);
 assert(search.find(m,ref,m).code==q::Code::Completed);
 assert(lookup.run(0u,output,0x5000u,ref).code==q::Code::NullPointerHazard);
 assert(lookup.run(m,0u,0x5000u,ref).code==q::Code::NullPointerHazard);
 h.fail_alloc=true;h.string(0x5000u,"abc");
 r=lookup.run(m,output,0x5000u,ref);
 assert(r.code==q::Code::HostRejected&&r.stage==q::Stage::AllocateBlock);
 assert(h.get32(m+0x30u)==0x5000u); // committed before allocation fails
 assert(h.get16(ref)==q::kInvalid&&h.get16(ref+2u)==q::kInvalid);
 h.fail_alloc=false;h.fail_append=true;
 r=lookup.run(m,output,0x5000u,ref);
 assert(r.code==q::Code::HostRejected&&r.allocated_block);
 assert(r.stage==q::Stage::AppendBlockPointer);
 h.fail_append=false;
 r=lookup.run(m,output,0x5000u,ref);
 assert(r.code==q::Code::Completed&&r.inserted&&r.handle==0u);
 // When callback throws, no synthetic success is claimed.
 h.throw_compare=true;h.string(0x5100u,"beta");
 r=lookup.run(m,output,0x5100u,ref);
 assert(r.code==q::Code::HostRejected&&r.stage==q::Stage::CompareKeyNode);
 h.throw_compare=false;
 h.reenter_lookup=true;
 r=lookup.run(m,output,0x5100u,ref);
 assert(r.code==q::Code::Completed&&h.reentered==q::Code::Reentrant);
 h.reenter_lookup=false;
 h.fail_address=m+0x30u;
 r=lookup.run(m,output,0x5100u,ref);
 assert(r.code==q::Code::HostRejected&&r.stage==q::Stage::WriteFallback30);
 h.fail_address=0u;
 // Original BlockFit is signed; upper-bit count is non-positive.
 h.put32(m+0x28u,0xffffffffu);
 r=fit.find(m,1u);assert(r.code==q::Code::Completed&&r.block_index==0xffffffffu);
 h.put32(m+0x28u,1u);
 const auto block=h.get32(block_pointers);
 h.put32(block,0xffffffffu);h.put32(block+4u,0u);
 r=fit.find(m,5u);assert(r.code==q::Code::Completed&&r.block_index==0xffffffffu);
 h.put32(block,0x800u);h.put32(block+4u,0u);
 r=fit.find(m,5u);assert(r.code==q::Code::Completed&&r.block_index==0u);
 h.put16(m+0x10u,0u);h.put16(slots,0u);h.put16(slots+2u,0u);
 // Force a cycle, with key always greater than the sole node.
 h.string(0x5200u,"zzzz");
 h.put32(m+0x30u,0x5200u);
 h.put16(ref,q::kInvalid);h.put16(ref+2u,q::kInvalid);
 h.put16(slots+2u,0u);
 r=search.find(m,ref,m);
 assert(r.code==q::Code::CycleGuard && r.comparisons==131072u);
}
} // namespace
int main(){test_complete_original_nested_registration_and_search();
 test_block_fit_and_error_edges();return 0;}
