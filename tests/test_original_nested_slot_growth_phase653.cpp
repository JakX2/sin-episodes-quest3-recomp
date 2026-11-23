#include "sin_compat/original_nested_slot_growth_phase653.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <vector>
namespace n=sin_compat::original_nested_slot_growth_phase653;
namespace {
constexpr std::uint32_t kMap=0x4000u,kOwner=kMap+4u,kSlots=0x9000u;
struct Host final:n::IHost {
 std::map<std::uint32_t,std::uint8_t> memory;
 n::GrowSlots* grow_model=nullptr;
 n::ResolveSlot* resolver_model=nullptr;
 n::Code nested=n::Code::Completed;
 std::uint32_t allocs=0,reallocs=0,bytes_requested=0;
 std::uint32_t next_alloc=kSlots,returned_pointer=0;
 std::uint32_t writes=0,fail_write=0;
 bool fail_alloc=false,fail_realloc=false,return_null=false;
 bool throw_alloc=false,reenter=false,reenter_on_flag=false;
 void put8(std::uint32_t va,std::uint8_t v){memory[va]=v;}
 void put16(std::uint32_t va,std::uint16_t v){put8(va,static_cast<std::uint8_t>(v));put8(va+1u,static_cast<std::uint8_t>(v>>8u));}
 void put32(std::uint32_t va,std::uint32_t v){for(unsigned i=0;i<4u;++i)put8(va+i,static_cast<std::uint8_t>(v>>(8u*i)));}
 std::uint8_t byte(std::uint32_t va)const{auto it=memory.find(va);assert(it!=memory.end());return it->second;}
 std::uint16_t word(std::uint32_t va)const{return static_cast<std::uint16_t>(byte(va)|(static_cast<std::uint16_t>(byte(va+1u))<<8u));}
 std::uint32_t dword(std::uint32_t va)const{std::uint32_t v=0;for(unsigned i=0;i<4u;++i)v|=static_cast<std::uint32_t>(byte(va+i))<<(8u*i);return v;}
 bool read_u8(std::uint32_t va,std::uint8_t& out)override{auto it=memory.find(va);if(it==memory.end())return false;out=it->second;return true;}
 bool read_u32(std::uint32_t va,std::uint32_t& out)override{for(unsigned i=0;i<4u;++i)if(!memory.count(va+i))return false;out=dword(va);return true;}
 bool write_u16(std::uint32_t va,std::uint16_t v)override{++writes;if(writes==fail_write)return false;put16(va,v);return true;}
 bool write_u32(std::uint32_t va,std::uint32_t v)override{
  ++writes;
  if(reenter_on_flag&&va==n::kSentinelFlagVA&&resolver_model)nested=resolver_model->resolve(kMap,n::kInvalid).code;
  if(writes==fail_write)return false;
  put32(va,v);return true;
 }
 bool allocate_original_2226e390(std::uint32_t size,std::uint32_t& out)override{
  ++allocs;bytes_requested=size;
  if(reenter&&grow_model)nested=grow_model->grow(kOwner,0u).code;
  if(throw_alloc)throw std::runtime_error("test injected allocator throw");
  if(fail_alloc)return false;
  if(return_null){out=0u;return true;}
  out=next_alloc;next_alloc+=0x1000u;returned_pointer=out;
  for(std::uint32_t i=0;i<size&&i<4096u;++i)put8(out+i,0u);
  return true;
 }
 bool reallocate_original_2226e400(std::uint32_t prior,std::uint32_t size,std::uint32_t& out)override{
  ++reallocs;bytes_requested=size;
  if(fail_realloc)return false;
  if(return_null){out=0u;return true;}
  out=next_alloc;next_alloc+=0x1000u;returned_pointer=out;
  for(std::uint32_t i=0;i<size&&i<4096u;++i){auto it=memory.find(prior+i);put8(out+i,it==memory.end()?0u:it->second);}
  return true;
 }
 void reset(){memory.clear();writes=allocs=reallocs=bytes_requested=0;
  fail_write=0;fail_alloc=fail_realloc=return_null=throw_alloc=reenter=reenter_on_flag=false;
  nested=n::Code::Completed;next_alloc=kSlots;
  put32(kOwner,0u);put32(kOwner+4u,0u);put32(kOwner+8u,0u);
  put32(n::kSentinelFlagVA,0u);
 }
};
void test_slot_resolver_original_sentinel(){
 Host h;h.reset();n::ResolveSlot resolve(h);h.resolver_model=&resolve;
 auto a=resolve.resolve(kMap,n::kInvalid);
 assert(a.code==n::Code::Completed&&a.sentinel_returned&&a.sentinel_initialized);
 assert(a.writes_committed==5u&&a.result_pointer==n::kSentinelVA);
 for(unsigned i=0;i<3u;++i)assert(h.word(n::kSentinelVA+2u*i)==n::kInvalid);
 assert(h.word(n::kSentinelVA+6u)==1u&&h.dword(n::kSentinelFlagVA)==1u);
 h.put32(kOwner,kSlots);
 a=resolve.resolve(kMap,4u);
 assert(a.code==n::Code::Completed&&a.result_pointer==kSlots+48u);
 assert(!a.sentinel_initialized&&a.writes_committed==0u);
 h.put32(n::kSentinelFlagVA,0xabcd1234u); // bit 0 not yet set, preserve higher bits
 a=resolve.resolve(kMap,1u);
 assert(a.code==n::Code::Completed&&a.sentinel_initialized);
 assert(h.dword(n::kSentinelFlagVA)==0xabcd1235u);
 h.put32(kOwner,0u);
 a=resolve.resolve(kMap,1u);
 assert(a.code==n::Code::NullArrayHazard);
 assert(resolve.resolve(0u,1u).code==n::Code::NullOwnerHazard);
 assert(resolve.resolve(0xfffffffdu,1u).code==n::Code::AddressOverflow);
 h.reset();h.reenter_on_flag=true;
 a=resolve.resolve(kMap,n::kInvalid);
 assert(a.code==n::Code::Completed&&h.nested==n::Code::Reentrant);
 h.reset();h.fail_write=2u;
 a=resolve.resolve(kMap,n::kInvalid);
 assert(a.code==n::Code::HostRejected&&a.stage==n::Stage::WriteSentinelLeft);
 assert(h.dword(n::kSentinelFlagVA)==1u); // original flag set before other fields
}
void test_growth_capacity_and_allocator_paths(){
 Host h;h.reset();n::GrowSlots grow(h);h.grow_model=&grow;
 auto r=grow.grow(kOwner,1u);
 assert(r.code==n::Code::Completed&&r.old_capacity==0u&&r.new_capacity==3u);
 assert(r.growth_rounds==1u&&r.alloc_bytes==36u&&r.allocation_called);
 assert(h.allocs==1u&&h.reallocs==0u&&h.dword(kOwner+4u)==3u);
 const auto prior=h.dword(kOwner);h.put32(prior,0x913acffeu);
 r=grow.grow(kOwner,1u);
 assert(r.code==n::Code::Completed&&r.new_capacity==6u&&r.growth_rounds==1u);
 assert(r.alloc_bytes==72u&&r.reallocation_called&&h.reallocs==1u);
 assert(h.dword(h.dword(kOwner))==0x913acffeu);
 r=grow.grow(kOwner,0u);
 assert(r.code==n::Code::Completed&&r.growth_rounds==0u&&r.reallocation_called);
 assert(h.reallocs==2u); // original always reallocates when enabled
 h.put32(kOwner+4u,4u);h.put32(kOwner+8u,2u);
 r=grow.grow(kOwner,5u);
 assert(r.code==n::Code::Completed&&r.new_capacity==10u&&r.growth_rounds==3u);
 assert(r.alloc_bytes==120u&&h.dword(kOwner+4u)==10u);
 h.put32(kOwner+8u,n::kDisabled);
 const auto calls=h.allocs+h.reallocs;
 r=grow.grow(kOwner,1u);
 assert(r.code==n::Code::Completed&&r.growth_disabled);
 assert(h.allocs+h.reallocs==calls);
 h.put32(kOwner+8u,0u);h.put32(kOwner+4u,0x80000000u);
 r=grow.grow(kOwner,0u);
 assert(r.code==n::Code::Completed&&r.new_capacity==0x80000000u);
 assert(r.alloc_bytes==0u&&r.growth_rounds==0u);
 h.put32(kOwner+4u,0x7fffffffu);
 r=grow.grow(kOwner,2u); // overflowed signed goal is negative
 assert(r.code==n::Code::Completed&&r.new_capacity==0x7fffffffu);
 assert(r.growth_rounds==0u&&r.alloc_bytes==0xfffffff4u);
 h.reset();h.throw_alloc=true;
 r=grow.grow(kOwner,1u);
 assert(r.code==n::Code::HostRejected&&r.writes_committed==1u);
 assert(h.dword(kOwner+4u)==3u&&h.dword(kOwner)==0u);
 h.reset();h.fail_alloc=true;
 r=grow.grow(kOwner,1u);
 assert(r.code==n::Code::HostRejected&&r.writes_committed==1u);
 h.reset();h.reenter=true;
 r=grow.grow(kOwner,1u);
 assert(r.code==n::Code::Completed&&h.nested==n::Code::Reentrant);
 h.reset();h.return_null=true;
 r=grow.grow(kOwner,1u);
 assert(r.code==n::Code::Completed&&r.result_pointer==0u);
 assert(h.dword(kOwner)==0u&&h.dword(kOwner+4u)==3u);
 assert(grow.grow(0u,1u).code==n::Code::NullOwnerHazard);
 assert(grow.grow(0xfffffffcu,1u).code==n::Code::AddressOverflow);
}
}
int main(){test_slot_resolver_original_sentinel();
 test_growth_capacity_and_allocator_paths();return 0;}
