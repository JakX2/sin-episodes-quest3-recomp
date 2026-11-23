#include "sin_compat/original_nested_map_phase647.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
namespace n=sin_compat::original_nested_map_phase647;
namespace {
constexpr std::uint32_t kObject=0x5000u;
struct Host final:n::IHost {
 std::map<std::uint32_t,std::uint8_t> memory;
 std::vector<std::string> events;
 std::uint32_t alloc_count=0,free_count=0,last_alloc_bytes=0,last_freed=0;
 std::uint32_t alloc_return=0x9000u;
 unsigned fail_write=0,writes=0;
 bool fail_alloc=false,fail_free=false,throw_alloc=false,throw_free=false;
 bool reenter_alloc=false,reenter_free=false;
 n::ArrayConstructor* array=nullptr;
 n::ArrayRelease* release=nullptr;
 n::Code nested_alloc=n::Code::Completed,nested_free=n::Code::Completed;
 void set16(std::uint32_t a,std::uint16_t v){for(unsigned i=0;i<2u;++i)memory[a+i]=static_cast<std::uint8_t>(v>>(8*i));}
 void set32(std::uint32_t a,std::uint32_t v){for(unsigned i=0;i<4u;++i)memory[a+i]=static_cast<std::uint8_t>(v>>(8*i));}
 std::uint32_t get32(std::uint32_t a)const {
   std::uint32_t v=0;for(unsigned i=0;i<4u;++i){auto it=memory.find(a+i);assert(it!=memory.end());v|=static_cast<std::uint32_t>(it->second)<<(i*8);}return v;
 }
 std::uint16_t get16(std::uint32_t a)const {
   std::uint16_t v=0;for(unsigned i=0;i<2u;++i){auto it=memory.find(a+i);assert(it!=memory.end());v|=static_cast<std::uint16_t>(it->second)<<(i*8);}return v;
 }
 bool read_u32(std::uint32_t a,std::uint32_t& v)override {
   events.push_back("r32:"+std::to_string(a));
   for(unsigned i=0;i<4u;++i)if(memory.count(a+i)==0u)return false;
   v=get32(a);return true;
 }
 bool write_u32(std::uint32_t a,std::uint32_t v)override {
   events.push_back("w32:"+std::to_string(a));++writes;
   if(writes==fail_write)return false;
   set32(a,v);return true;
 }
 bool write_u16(std::uint32_t a,std::uint16_t v)override {
   events.push_back("w16:"+std::to_string(a));++writes;
   if(writes==fail_write)return false;
   set16(a,v);return true;
 }
 bool allocate_original_2226e390(std::uint32_t size,std::uint32_t& ret)override {
   events.push_back("ALLOC:"+std::to_string(size));
   ++alloc_count;last_alloc_bytes=size;
   if(reenter_alloc && array)nested_alloc=array->construct(kObject,0u,0u,0u).code;
   if(throw_alloc)throw std::runtime_error("injected failure");
   if(fail_alloc)return false;
   ret=alloc_return;return true;
 }
 bool free_original_2226e3e0(std::uint32_t pointer)override {
   events.push_back("FREE:"+std::to_string(pointer));
   ++free_count;last_freed=pointer;
   if(reenter_free && release)nested_free=release->release(kObject).code;
   if(throw_free)throw std::runtime_error("injected failure");
   return !fail_free;
 }
};
void test_array_base_and_nonzero_capacity(){
 Host h;n::ArrayConstructor c(h);h.array=&c;
 auto r=c.construct(kObject,0x44444444u,0u,0x12345678u);
 assert(r.code==n::Code::Completed && r.writes_committed==9u);
 assert(h.get32(kObject)==0x12345678u);
 assert(h.get32(kObject+4u)==0u && h.get32(kObject+8u)==0u);
 assert(h.get32(kObject+0xcu)==0x44444444u);
 assert(h.get16(kObject+0x10u)==0xffffu && h.get16(kObject+0x14u)==0xffffu);
 assert(h.get16(kObject+0x12u)==0u && h.get16(kObject+0x16u)==0u);
 assert(h.get32(kObject+0x18u)==0u && h.alloc_count==0u);
 r=c.construct(kObject,3u,7u,0xfedcba98u);
 assert(r.code==n::Code::Completed&&r.writes_committed==10u);
 assert(r.allocation_called && r.allocation_bytes==84u);
 assert(h.alloc_count==1u && h.last_alloc_bytes==84u);
 assert(h.get32(kObject)==0xfedcba98u);
 assert(h.get32(kObject+4u)==0x9000u && h.get32(kObject+0x18u)==0x9000u);
 assert(h.get32(kObject+8u)==7u && h.get32(kObject+0xcu)==3u);
 assert(h.get16(kObject+0x10u)==0xffffu && h.get16(kObject+0x14u)==0xffffu);
 r=c.construct(kObject,1u,0x40000000u,0u);
 assert(r.code==n::Code::Completed && r.allocation_bytes==0u);
 assert(h.last_alloc_bytes==0u); // original 32-bit LEA/ADD arithmetic wraps
 h.alloc_return=0u;
 r=c.construct(kObject,0u,2u,0u);
 assert(r.code==n::Code::Completed && r.allocation_called);
 assert(h.get32(kObject+4u)==0u && h.get32(kObject+0x18u)==0u);
}
void test_map_selector_and_defaults(){
 Host h;n::ArrayConstructor a(h);n::MapConstructor c(h,a);
 auto r=c.construct(kObject,0u,0u,1u);
 assert(r.code==n::Code::Completed && r.nested_array_completed);
 assert(r.writes_committed==15u);
 assert(r.selected_comparator==n::kStringComparatorVA);
 assert(h.get32(kObject)==n::kStringComparatorVA);
 assert(h.get32(kObject+0x1cu)==0u && h.get32(kObject+0x20u)==0u);
 assert(h.get32(kObject+0x24u)==8u);
 assert(h.get32(kObject+0x28u)==0u && h.get32(kObject+0x2cu)==0u);
 r=c.construct(kObject,0u,0u,0u);
 assert(r.code==n::Code::Completed && r.selected_comparator==n::kBytewiseLessComparatorVA);
 assert(h.get32(kObject)==n::kBytewiseLessComparatorVA);
 r=c.construct(kObject,0u,0u,0x100u); // only the low selector BYTE is used
 assert(r.code==n::Code::Completed && r.selected_comparator==n::kBytewiseLessComparatorVA);
 r=c.construct(kObject,2u,3u,0x101u);
 assert(r.code==n::Code::Completed && r.allocation_called);
 assert(r.allocation_bytes==36u && r.writes_committed==16u);
 assert(r.selected_comparator==n::kStringComparatorVA);
 assert(h.get32(kObject+0x18u)==h.get32(kObject+4u));
}
void test_array_release_and_guards(){
 Host h;n::ArrayConstructor a(h);n::ArrayRelease release(h);
 h.array=&a;h.release=&release;
 h.set32(kObject+0xcu,0u);h.set32(kObject+4u,0u);
 h.set32(kObject+8u,7u);
 auto r=release.release(kObject);
 assert(r.code==n::Code::Completed && !r.release_called && r.writes_committed==1u);
 assert(h.get32(kObject+8u)==0u && h.free_count==0u);
 h.set32(kObject+4u,0x9abcu);h.set32(kObject+8u,33u);
 h.set32(kObject+0x18u,0xfeedbeefu);
 r=release.release(kObject);
 assert(r.code==n::Code::Completed&&r.release_called);
 assert(r.writes_committed==2u&&h.last_freed==0x9abcu);
 assert(h.get32(kObject+4u)==0u && h.get32(kObject+8u)==0u);
 assert(h.get32(kObject+0x18u)==0xfeedbeefu); // release does not touch cached pointer
 h.set32(kObject+0xcu,0xffffffffu);h.set32(kObject+4u,0x9abcu);
 h.set32(kObject+8u,99u);
 r=release.release(kObject);
 assert(r.code==n::Code::Completed && r.skipped_disabled);
 assert(h.get32(kObject+4u)==0x9abcu && h.get32(kObject+8u)==99u);
 assert(release.release(0u).code==n::Code::NullObjectHazard);
 assert(release.release(0xfffffffbu).code==n::Code::AddressOverflow);
}
void test_failure_order_and_reentry(){
 Host h;n::ArrayConstructor a(h);n::MapConstructor map(h,a);n::ArrayRelease release(h);
 h.array=&a;h.release=&release;
 h.fail_write=3u;
 auto r=a.construct(kObject,1u,2u,0u);
 assert(r.code==n::Code::HostRejected && r.writes_committed==2u);
 assert(h.alloc_count==0u && h.get32(kObject+4u)==0u);
 h.fail_write=0u;h.writes=0u;h.fail_alloc=true;
 r=a.construct(kObject,1u,2u,0u);
 assert(r.code==n::Code::HostRejected && r.writes_committed==4u);
 assert(h.get32(kObject+8u)==2u && h.get32(kObject+0xcu)==1u);
 h.fail_alloc=false;h.throw_alloc=true;
 r=a.construct(kObject,1u,2u,0u);
 assert(r.code==n::Code::HostRejected && r.allocation_called);
 h.throw_alloc=false;h.reenter_alloc=true;
 r=a.construct(kObject,0u,1u,0u);
 assert(r.code==n::Code::Completed && h.nested_alloc==n::Code::Reentrant);
 h.reenter_alloc=false;h.fail_free=true;
 r=release.release(kObject);
 assert(r.code==n::Code::HostRejected && r.release_called);
 assert(h.get32(kObject+4u)==0x9000u); // no rollback; free rejected before clearing
 h.fail_free=false;h.reenter_free=true;
 r=release.release(kObject);
 assert(r.code==n::Code::Completed && h.nested_free==n::Code::Reentrant);
 h.reenter_free=false;h.writes=0u;h.fail_write=15u;
 r=map.construct(kObject,0u,0u,1u);
 assert(r.code==n::Code::HostRejected && r.writes_committed==14u);
 assert(r.stage==n::Stage::WriteDefaultComparator0);
 assert(h.get32(kObject+0x24u)==8u);
 assert(map.construct(0u,0u,0u,1u).code==n::Code::NullObjectHazard);
 assert(a.construct(0u,0u,0u,0u).code==n::Code::NullObjectHazard);
 assert(a.construct(0xfffffff8u,0u,0u,0u).code==n::Code::AddressOverflow);
}
}
int main(){test_array_base_and_nonzero_capacity();test_map_selector_and_defaults();
  test_array_release_and_guards();test_failure_order_and_reentry();return 0;}
