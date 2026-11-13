#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#include "sin_compat/original_server_allocator_dispatch.hpp"
namespace a=sin_compat::original_server_allocator_dispatch;
struct Host {
 a::NativeLazyContexts32& state;
 std::vector<std::string> log{};
 bool recurse_alloc=false,recurse_realloc=false,enabled=true;
 unsigned checks=0,creations=0;
 bool metadata_enabled() {
  log.emplace_back("check");++checks;
  if(recurse_alloc) {
   recurse_alloc=false;
   assert(a::allocate(state,*this,3u)==0x1003u);
  }
  if(recurse_realloc) {
   recurse_realloc=false;
   assert(a::reallocate(state,*this,0x2000u,5u)==0x2005u);
  }
  return enabled;
 }
 std::uint32_t make_metadata(){log.emplace_back("metadata");++creations;return 0xBEEFu;}
 std::uint32_t allocate_basic(std::uint32_t n){log.emplace_back("basic-a");return 0x1000u+n;}
 std::uint32_t allocate_tagged(std::uint32_t n,std::uint32_t c,std::uint32_t z){
  assert(c==0xBEEFu && z==0);log.emplace_back("tagged-a");return 0x3000u+n;
 }
 std::uint32_t reallocate_basic(std::uint32_t p,std::uint32_t n){
  assert(p==0x2000u);log.emplace_back("basic-r");return p+n;
 }
 std::uint32_t reallocate_tagged(std::uint32_t p,std::uint32_t n,std::uint32_t c,std::uint32_t z){
  assert(p==0x2000u && c==0xBEEFu && z==0);log.emplace_back("tagged-r");return 0x4000u+n;
 }
 void deallocate_direct(std::uint32_t p){assert(p==0xAABBCCDDu);log.emplace_back("free");}
};
int main(){
 static_assert(sizeof(a::NativeLazyContexts32)==16u);
 {
  a::NativeLazyContexts32 state{};Host h{state};
  h.recurse_alloc=true;
  assert(a::allocate(state,h,7u)==0x3007u);
  assert((h.log==std::vector<std::string>{"check","basic-a","metadata","tagged-a"}));
  assert(h.checks==1 && h.creations==1);
  assert(state.alloc_flags_04==1u && state.alloc_context_00==0xBEEFu);
  assert(a::allocate(state,h,9u)==0x3009u && h.checks==1);
 }
 {
  a::NativeLazyContexts32 state{};Host h{state};
  h.recurse_realloc=true;
  assert(a::reallocate(state,h,0x2000u,7u)==0x4007u);
  assert((h.log==std::vector<std::string>{"check","basic-r","metadata","tagged-r"}));
  assert(h.checks==1 && h.creations==1);
  assert(state.realloc_flags_0c==1u && state.realloc_context_08==0xBEEFu);
  assert(state.alloc_flags_04==0u);
 }
 {
  a::NativeLazyContexts32 state{};
  state.alloc_flags_04=0x80u;state.realloc_flags_0c=0x40u;
  Host h{state};h.enabled=false;h.recurse_alloc=true;
  assert(a::allocate(state,h,6u)==0x1006u);
  assert(state.alloc_flags_04==0x81u && state.alloc_context_00==0u);
  assert(h.checks==1 && h.creations==0);
  h.enabled=true;
  assert(a::allocate(state,h,8u)==0x1008u && h.checks==1);
  h.recurse_realloc=true;
  assert(a::reallocate(state,h,0x2000u,8u)==0x4008u);
  assert(state.realloc_flags_0c==0x41u);
  assert(h.checks==2 && h.creations==1);
  a::deallocate(h,0xAABBCCDDu);
  assert(h.log.back()=="free");
 }
}
