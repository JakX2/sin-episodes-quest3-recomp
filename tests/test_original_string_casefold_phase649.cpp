#include "sin_compat/original_string_casefold_phase649.hpp"
#include "sin_compat/original_nested_comparators_phase648.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
namespace s=sin_compat::original_string_casefold_phase649;
namespace c=sin_compat::original_nested_comparators_phase648;
namespace {
constexpr std::uint32_t left=0x10000u,right=0x11000u,ctype=0x20000u;
class Host final:public s::IHost,public c::IHost {
public:
 std::map<std::uint32_t,std::uint8_t> memory;
 s::Casefold* casefold=nullptr;
 std::uint32_t ctype_calls=0u,map_calls=0u,last_ctype_input=0u;
 s::MapRequest last_map{};
 std::uint32_t mapped_count=1u;
 std::uint8_t mapped_first=0u,mapped_second=0u;
 bool fail_ctype=false,fail_map=false,throw_map=false,reenter=false;
 s::Code nested=s::Code::Completed;
 void set8(std::uint32_t addr,std::uint8_t val){memory[addr]=val;}
 void set16(std::uint32_t addr,std::uint16_t val){set8(addr,static_cast<std::uint8_t>(val));set8(addr+1u,static_cast<std::uint8_t>(val>>8u));}
 void set32(std::uint32_t addr,std::uint32_t val){for(unsigned i=0u;i<4u;++i)set8(addr+i,static_cast<std::uint8_t>(val>>(8u*i)));}
 void string(std::uint32_t addr,const std::string& val){for(std::size_t i=0;i<val.size();++i)set8(addr+static_cast<std::uint32_t>(i),static_cast<std::uint8_t>(val[i]));set8(addr+static_cast<std::uint32_t>(val.size()),0u);}
 void reset(){memory.clear();ctype_calls=map_calls=last_ctype_input=0u;
  mapped_count=1u;mapped_first=0u;mapped_second=0u;
  fail_ctype=fail_map=throw_map=reenter=false;nested=s::Code::Completed;last_map={};
  set32(s::kModeGlobal,0u);
  set32(s::kCTypeFastGlobal,0u);set32(s::kCTypeModeGlobal,1u);
  set32(s::kCTypeTableGlobal,ctype);set32(s::kLocaleCodepageGlobal,1250u);
  for(unsigned i=0u;i<512u;++i)set8(ctype+i,0u);
  for(std::uint32_t i=65u;i<=90u;++i)set8(ctype+i*2u,1u);
 }
 bool read_u8(std::uint32_t a,std::uint8_t& out)override{
  const auto it=memory.find(a);if(it==memory.end())return false;
  out=it->second;return true;
 }
 bool read_u16(std::uint32_t a,std::uint16_t& out)override{
  std::uint8_t l=0u,h=0u;if(!read_u8(a,l)||!read_u8(a+1u,h))return false;
  out=static_cast<std::uint16_t>(l|static_cast<std::uint16_t>(h)<<8u);return true;
 }
 bool read_u32(std::uint32_t a,std::uint32_t& out)override{
  out=0u;for(unsigned i=0u;i<4u;++i){std::uint8_t b=0u;
   if(!read_u8(a+i,b))return false;
   out|=static_cast<std::uint32_t>(b)<<(8u*i);}
  return true;
 }
 bool classify_original_223a898a(std::uint32_t character,std::uint32_t mask,std::uint32_t& out)override{
  assert(mask==1u);last_ctype_input=character;++ctype_calls;
  if(fail_ctype)return false;
  out=(character>=65u&&character<=90u)?1u:0u;
  return true;
 }
 bool locale_map_original_223ac9e7(const s::MapRequest& request,s::MapAnswer& out)override{
  ++map_calls;last_map=request;
  assert(request.options==256u && request.output_capacity==3u&&request.flag==1u);
  if(reenter&&casefold)nested=casefold->compare(left,right).code;
  if(throw_map)throw std::runtime_error("injected locale fault");
  if(fail_map)return false;
  out.count=mapped_count;
  out.first=mapped_first?mapped_first:static_cast<std::uint8_t>(request.first+32u);
  out.second=mapped_second;
  return true;
 }
 bool compare_original_223a5574(std::uint32_t a,std::uint32_t b,std::int32_t& out)override{
  assert(casefold);const auto result=casefold->compare(a,b);
  if(result.code!=s::Code::Completed)return false;
  out=result.comparison;return true;
 }
};
void test_ascii_casefold_and_normalization(){
 Host h;h.reset();s::Casefold cmp(h);h.casefold=&cmp;
 h.string(left,"Alpha");h.string(right,"alpha");
 auto r=cmp.compare(left,right);
 assert(r.code==s::Code::Completed && r.ascii_fast &&r.comparison==0);
 assert(r.characters==6u && r.transforms==0u);
 h.string(right,"AlphB");r=cmp.compare(left,right);
 assert(r.code==s::Code::Completed&&r.comparison==-1);
 h.string(right,"Alph0");r=cmp.compare(left,right);
 assert(r.code==s::Code::Completed&&r.comparison==1);
 h.string(right,"Alph");r=cmp.compare(left,right);
 assert(r.code==s::Code::Completed&&r.comparison==1);
 h.string(left,"Alph");h.string(right,"Alpha");
 r=cmp.compare(left,right);assert(r.code==s::Code::Completed&&r.comparison==-1);
 h.string(left,"");h.string(right,"");
 r=cmp.compare(left,right);assert(r.code==s::Code::Completed&&r.comparison==0&&r.characters==1u);
 h.set8(left,0xffu);h.set8(left+1u,0u);
 h.set8(right,0x01u);h.set8(right+1u,0u);
 r=cmp.compare(left,right);assert(r.code==s::Code::Completed&&r.comparison==1);
 h.set8(left,0x01u);h.set8(right,0xffu);
 r=cmp.compare(left,right);assert(r.code==s::Code::Completed&&r.comparison==-1);
 assert(h.map_calls==0u&&h.ctype_calls==0u);
 assert(cmp.compare(0u,right).code==s::Code::NullPointerHazard);
 assert(cmp.compare(left,0u).code==s::Code::NullPointerHazard);
}
void test_nonzero_mode_and_transformation_order(){
 Host h;h.reset();s::Casefold cmp(h);h.casefold=&cmp;
 h.set32(s::kModeGlobal,0x12345678u);h.set32(s::kCTypeFastGlobal,1u);
 h.string(left,"AlPHa");h.string(right,"aLpHA");
 auto r=cmp.compare(left,right);
 assert(r.code==s::Code::Completed&&r.locale_mode&&!r.ascii_fast);
 assert(r.comparison==0&&r.characters==6u&&r.transforms==12u&&r.locale_mappings==0u);
 h.string(right,"bLpHA");
 r=cmp.compare(left,right);assert(r.code==s::Code::Completed&&r.comparison==-1);
 assert(r.characters==1u&&r.transforms==2u);
 h.string(right,"A");
 r=cmp.compare(left,right);assert(r.code==s::Code::Completed&&r.comparison==108);// 'l' - zero
 h.string(left,"");h.string(right,"B");
 r=cmp.compare(left,right);assert(r.code==s::Code::Completed&&r.comparison==-98);
 assert(r.characters==1u);
}
void test_ctype_table_and_locale_mapping(){
 Host h;h.reset();s::Casefold cmp(h);h.casefold=&cmp;
 h.set32(s::kModeGlobal,0x409u);
 h.set32(s::kCTypeFastGlobal,0u);
 h.mapped_first=static_cast<std::uint8_t>('z');
 auto c=cmp.transform(65u);
 assert(c.code==s::Code::Completed&&c.output==static_cast<std::uint32_t>('z'));
 assert(c.used_locale_map&&!c.fast_ascii &&h.map_calls==1u);
 assert(h.last_map.mode==0x409u&&h.last_map.first==65u);
 assert(h.last_map.input_count==1u&&h.last_map.codepage==1250u);
 c=cmp.transform(97u);assert(c.code==s::Code::Completed&&c.output==97u);
 assert(h.map_calls==1u); // lower-case char has no bit-1 classification
 h.set32(s::kCTypeModeGlobal,2u);
 c=cmp.transform(65u);
 assert(c.code==s::Code::Completed&&c.used_ctype_helper);
 assert(h.ctype_calls==1u &&h.last_ctype_input==65u);
 h.fail_ctype=true;
 c=cmp.transform(65u);
 assert(c.code==s::Code::HostRejected&&c.stage==s::Stage::CallCTypeHelper);
 h.fail_ctype=false;h.set32(s::kCTypeModeGlobal,1u);
 // DBCS character 0x8140: high byte 0x81 is a lead byte in our invented
 // classification table. This path bypasses ctype-bit classification.
 h.set8(ctype+0x81u*2u+1u,0x80u);
 h.mapped_first=0x82u;h.mapped_second=0x40u;h.mapped_count=2u;
 c=cmp.transform(0x8140u);
 assert(c.code==s::Code::Completed&&c.output==0x8240u);
 assert(c.used_locale_map&&h.last_map.input_count==2u);
 assert(h.last_map.first==0x81u&&h.last_map.second==0x40u);
 h.mapped_count=0u;
 c=cmp.transform(65u);
 assert(c.code==s::Code::Completed&&c.output==65u);
 h.mapped_count=1u;
 h.fail_map=true;
 c=cmp.transform(65u);
 assert(c.code==s::Code::HostRejected &&c.stage==s::Stage::CallLocaleMap);
 h.fail_map=false;h.throw_map=true;
 c=cmp.transform(65u);assert(c.code==s::Code::HostRejected);
 h.throw_map=false;
}
void test_guards_and_cross_phase_648(){
 Host h;h.reset();s::Casefold fold(h);h.casefold=&fold;
 h.set32(s::kModeGlobal,1u);h.set32(s::kCTypeFastGlobal,1u);
 h.string(left,"one");h.string(right,"Two");
 h.set32(0x30000u+0x1cu,0x31000u);
 h.set32(0x31000u,0x32000u);h.set32(0x31000u+4u,0x33000u);
 h.set16(0x34000u,0u);h.set16(0x34000u+2u,0u);
 h.set16(0x35000u,1u);h.set16(0x35000u+2u,0u);
 h.string(0x32000u+8u,"one");h.string(0x33000u+8u,"Two");
 c::Comparators nested(h);
 const auto nested_result=nested.delegated(0x34000u,0x35000u,0x30000u);
 assert(nested_result.code==c::Code::Completed&&nested_result.eax==1u);
 assert(nested_result.delegated_result==-5); // 'o' - 't'
 h.reenter=true;h.set32(s::kCTypeFastGlobal,0u);
 h.string(left,"A");h.string(right,"b");
 h.mapped_first='a';
 const auto compare=fold.compare(left,right);
 assert(compare.code==s::Code::Completed&&h.nested==s::Code::Reentrant);
 h.reenter=false;
 h.memory.erase(s::kModeGlobal);
 assert(fold.compare(left,right).code==s::Code::HostRejected);
 h.set32(s::kModeGlobal,0u);
 assert(fold.compare(0xfffffffdu,right).code==s::Code::HostRejected||
        fold.compare(0xfffffffdu,right).code==s::Code::AddressOverflow);
}
}
int main(){test_ascii_casefold_and_normalization();test_nonzero_mode_and_transformation_order();
 test_ctype_table_and_locale_mapping();test_guards_and_cross_phase_648();return 0;}
