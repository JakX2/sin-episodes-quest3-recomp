#include "sin_compat/original_nested_comparators_phase648.hpp"
#include "sin_compat/original_nested_map_phase647.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
namespace c=sin_compat::original_nested_comparators_phase648;
namespace n=sin_compat::original_nested_map_phase647;
namespace {
constexpr std::uint32_t map_va=0x4000u,reference_left=0x1000u,reference_right=0x1100u;
constexpr std::uint32_t block_table=0x7000u,block_zero=0x8000u,block_one=0x9000u;
constexpr std::uint32_t fallback=0xa000u;
struct Host final:c::IHost,n::IHost {
 std::map<std::uint32_t,std::uint8_t> mem;
 c::Comparators* comparators=nullptr;
 unsigned calls=0u,reads=0u,allocation_calls=0u,free_calls=0u;
 bool compare_fail=false,compare_throw=false,compare_reenter=false;
 bool force_compare=false;std::int32_t forced=0;
 c::Code reentrant=c::Code::Completed;
 std::uint32_t last_left=0u,last_right=0u,last_size=0u;
 void put8(std::uint32_t a,std::uint8_t v){mem[a]=v;}
 void put16(std::uint32_t a,std::uint16_t v){put8(a,static_cast<std::uint8_t>(v));put8(a+1u,static_cast<std::uint8_t>(v>>8));}
 void put32(std::uint32_t a,std::uint32_t v){for(unsigned i=0u;i<4u;++i)put8(a+i,static_cast<std::uint8_t>(v>>(8*i)));}
 bool read_u8(std::uint32_t a,std::uint8_t& v)override{++reads;auto x=mem.find(a);if(x==mem.end())return false;v=x->second;return true;}
 bool read_u16(std::uint32_t a,std::uint16_t& v)override{
  std::uint8_t lo=0,hi=0;if(!read_u8(a,lo)||!read_u8(a+1u,hi))return false;
  v=static_cast<std::uint16_t>(lo | (static_cast<std::uint16_t>(hi)<<8));return true;}
 bool read_u32(std::uint32_t a,std::uint32_t& v)override{
  v=0u;for(unsigned i=0;i<4u;++i){std::uint8_t byte=0;if(!read_u8(a+i,byte))return false;v|=static_cast<std::uint32_t>(byte)<<(8*i);}return true;}
 bool write_u16(std::uint32_t a,std::uint16_t v)override{put16(a,v);return true;}
 bool write_u32(std::uint32_t a,std::uint32_t v)override{put32(a,v);return true;}
 bool allocate_original_2226e390(std::uint32_t size,std::uint32_t& out)override{++allocation_calls;last_size=size;out=0xb000u;return true;}
 bool free_original_2226e3e0(std::uint32_t)override{++free_calls;return true;}
 bool compare_original_223a5574(std::uint32_t left,std::uint32_t right,std::int32_t& out)override{
  ++calls;last_left=left;last_right=right;
  if(compare_reenter&&comparators)reentrant=comparators->bytewise(reference_left,reference_right,map_va).code;
  if(compare_throw)throw std::runtime_error("injected compare error");
  if(compare_fail)return false;
  if(force_compare){out=forced;return true;}
  // Invented test callback: case-insensitive ordering only to check that
  // the original delegated path does NOT silently use bytewise comparison.
  for(std::uint32_t i=0;i<1024u;++i){
   std::uint8_t l=0,r=0;
   if(!read_u8(left+i,l)||!read_u8(right+i,r))return false;
   if(l>=65u&&l<=90u)l=static_cast<std::uint8_t>(l+32u);
   if(r>=65u&&r<=90u)r=static_cast<std::uint8_t>(r+32u);
   if(l!=r){out=l<r?-1:1;return true;}
   if(l==0u){out=0;return true;}
  }
  return false;
 }
 void string(std::uint32_t va,const std::string& s){
  for(std::size_t i=0;i<s.size();++i)put8(va+static_cast<std::uint32_t>(i),static_cast<std::uint8_t>(s[i]));
  put8(va+static_cast<std::uint32_t>(s.size()),0u);
 }
 void init(){
  put32(map_va+0x1cu,block_table);put32(map_va+0x30u,fallback);
  put32(block_table,block_zero);put32(block_table+4u,block_one);
  put16(reference_left,0u);put16(reference_left+2u,0u);
  put16(reference_right,1u);put16(reference_right+2u,0u);
 }
};
void test_unsigned_bytewise_and_two_byte_loop(){
 Host h;h.init();c::Comparators cmp(h);h.comparators=&cmp;
 h.string(block_zero+8u,"Abyss");h.string(block_one+8u,"Abzss");
 auto r=cmp.bytewise(reference_left,reference_right,map_va);
 assert(r.code==c::Code::Completed&&r.eax==1u);
 assert(r.left_string_va==block_zero+8u&&r.right_string_va==block_one+8u);
 assert(r.bytes_compared==3u&&!r.delegated);
 h.string(block_one+8u,"Abyss");
 r=cmp.bytewise(reference_left,reference_right,map_va);
 assert(r.code==c::Code::Completed&&r.eax==0u&&r.bytes_compared==6u);
 h.string(block_one+8u,"Aby");
 r=cmp.bytewise(reference_left,reference_right,map_va);
 assert(r.code==c::Code::Completed&&r.eax==0u&&r.bytes_compared==4u);
 h.string(block_zero+8u,"Aby");h.string(block_one+8u,"Abyss");
 r=cmp.bytewise(reference_left,reference_right,map_va);
 assert(r.code==c::Code::Completed&&r.eax==1u&&r.bytes_compared==4u);
 h.put8(block_zero+8u,0xffu);h.put8(block_zero+9u,0u);
 h.put8(block_one+8u,0x01u);h.put8(block_one+9u,0u);
 r=cmp.bytewise(reference_left,reference_right,map_va);
 assert(r.code==c::Code::Completed&&r.eax==0u&&r.bytes_compared==1u);
 h.put8(block_zero+8u,0u);h.put8(block_one+8u,0u);
 r=cmp.bytewise(reference_left,reference_right,map_va);
 assert(r.code==c::Code::Completed&&r.eax==0u&&r.bytes_compared==1u);
}
void test_fallback_and_offset_resolution(){
 Host h;h.init();c::Comparators cmp(h);
 h.put16(reference_left,0xffffu);h.put16(reference_left+2u,0xffffu);
 h.string(fallback,"Delta");h.string(block_one+8u+3u,"Echo");
 h.put16(reference_right+2u,3u);
 auto r=cmp.bytewise(reference_left,reference_right,map_va);
 assert(r.code==c::Code::Completed&&r.eax==1u&&r.left_uses_fallback);
 assert(r.left_string_va==fallback&&r.right_string_va==block_one+11u);
 h.put16(reference_right,0xffffu);h.put16(reference_right+2u,0xffffu);
 r=cmp.bytewise(reference_left,reference_right,map_va);
 assert(r.code==c::Code::Completed&&r.eax==0u&&r.right_uses_fallback);
 // Sentinel requires BOTH 16-bit words to be FFFF. Lone FFFF is
 // treated as a normal block-table index, not as an empty string.
 h.put16(reference_right+2u,0u);
 r=cmp.bytewise(reference_left,reference_right,map_va);
 assert(r.code==c::Code::HostRejected||r.code==c::Code::AddressOverflow);
}
void test_delegated_signed_comparison(){
 Host h;h.init();c::Comparators cmp(h);h.comparators=&cmp;
 h.string(block_zero+8u,"Alpha");h.string(block_one+8u,"alpha");
 auto r=cmp.delegated(reference_left,reference_right,map_va);
 assert(r.code==c::Code::Completed&&r.eax==0u&&r.delegated&&h.calls==1u);
 assert(r.left_string_va==h.last_left&&r.right_string_va==h.last_right);
 // The native delegation is semantic, not a direct-bytewise shortcut.
 const auto direct=cmp.bytewise(reference_left,reference_right,map_va);
 assert(direct.code==c::Code::Completed&&direct.eax==1u);
 h.force_compare=true;
 for(const auto value:{std::int32_t{-2147483647-1},std::int32_t{-7},std::int32_t{-1},std::int32_t{0},std::int32_t{1},std::int32_t{100}}){
  h.forced=value;r=cmp.delegated(reference_left,reference_right,map_va);
  assert(r.code==c::Code::Completed);
  assert(r.delegated_result==value&&r.eax==(value<0?1u:0u));
 }
 h.compare_fail=true;
 r=cmp.delegated(reference_left,reference_right,map_va);
 assert(r.code==c::Code::HostRejected&&r.stage==c::Stage::DelegateOriginal223a5574);
 h.compare_fail=false;h.compare_throw=true;
 r=cmp.delegated(reference_left,reference_right,map_va);
 assert(r.code==c::Code::HostRejected);
 h.compare_throw=false;h.compare_reenter=true;
 r=cmp.delegated(reference_left,reference_right,map_va);
 assert(r.code==c::Code::Completed&&h.reentrant==c::Code::Reentrant);
}
void test_nested_constructor_and_both_selected_methods(){
 Host h;c::Comparators cmp(h);n::ArrayConstructor array(h);n::MapConstructor nested(h,array);
 auto out=nested.construct(map_va,0u,0u,1u);
 assert(out.code==n::Code::Completed&&out.selected_comparator==c::kDelegatingVA);
 assert(h.allocation_calls==0u);
 h.init();h.string(block_zero+8u,"one");h.string(block_one+8u,"two");
 auto result=cmp.delegated(reference_left,reference_right,map_va);
 assert(result.code==c::Code::Completed&&result.eax==1u);
 out=nested.construct(map_va,0u,0u,0u);
 assert(out.code==n::Code::Completed&&out.selected_comparator==c::kDirectVA);
 h.init();result=cmp.bytewise(reference_left,reference_right,map_va);
 assert(result.code==c::Code::Completed&&result.eax==1u);
}
void test_hazard_handling(){
 Host h;h.init();c::Comparators cmp(h);
 assert(cmp.bytewise(0u,reference_right,map_va).code==c::Code::NullPointerHazard);
 assert(cmp.delegated(reference_left,reference_right,0u).code==c::Code::NullPointerHazard);
 assert(cmp.bytewise(0xfffffffdu,reference_right,map_va).code==c::Code::HostRejected ||
        cmp.bytewise(0xfffffffdu,reference_right,map_va).code==c::Code::AddressOverflow);
 h.put32(map_va+0x1cu,0u);
 assert(cmp.bytewise(reference_left,reference_right,map_va).code==c::Code::NullPointerHazard);
 h.put32(map_va+0x1cu,block_table);
 h.put32(block_table,0u);
 assert(cmp.bytewise(reference_left,reference_right,map_va).code==c::Code::AddressOverflow);
}
}
int main(){test_unsigned_bytewise_and_two_byte_loop();test_fallback_and_offset_resolution();
 test_delegated_signed_comparison();test_nested_constructor_and_both_selected_methods();
 test_hazard_handling();return 0;}
