#include "sin_compat/original_symbol_rotations_phase641.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
namespace p=sin_compat::original_symbol_rotations_phase641;
namespace {
constexpr std::uint32_t table=0x2000u,slots=0x3000u;
struct Host final:p::IHost {
 std::map<std::uint32_t,std::uint8_t> mem;
 std::vector<std::string> trace;
 unsigned write_number=0,failed_write=0;
 bool relocation=false,fail_reads=false,throw_on_write=false;
 p::SentinelSlotAccessor* accessor=nullptr;
 p::RotateNode* rotator=nullptr;
 p::Code nested=p::Code::Completed;
 bool reenter=false;
 std::uint32_t relocation_trap=0;
 void set8(std::uint32_t address,std::uint8_t v){mem[address]=v;}
 void set16(std::uint32_t address,std::uint16_t v){for(unsigned k=0;k<2;k++)mem[address+k]=static_cast<std::uint8_t>(v>>(k*8));}
 void set32(std::uint32_t address,std::uint32_t v){for(unsigned k=0;k<4;k++)mem[address+k]=static_cast<std::uint8_t>(v>>(k*8));}
 template<class T> bool get(std::uint32_t a,T& v)const {v=0;for(unsigned k=0;k<sizeof(T);k++){auto it=mem.find(a+k);if(it==mem.end())return false;v|=static_cast<T>(it->second)<<(k*8);}return true;}
 std::uint8_t b(std::uint32_t a)const {std::uint8_t n=0;assert(get(a,n));return n;}
 std::uint16_t w(std::uint32_t a)const {std::uint16_t n=0;assert(get(a,n));return n;}
 std::uint32_t d(std::uint32_t a)const {std::uint32_t n=0;assert(get(a,n));return n;}
 std::uint32_t node(std::uint16_t index,std::uint32_t base=slots)const{return base+16u*index;}
 void initialize(){mem.clear();trace.clear();write_number=failed_write=0;fail_reads=throw_on_write=relocation=reenter=false;relocation_trap=0;set32(table+4,slots);set16(table+0x10,0xffffu);set32(p::kSentinelInitFlags,0x200u);}
 void nset(std::uint16_t i,std::uint16_t left,std::uint16_t right,std::uint16_t parent){set16(node(i),left);set16(node(i)+2,right);set16(node(i)+4,parent);set16(node(i)+6,0);}
 bool read_u8(std::uint32_t a,std::uint8_t& v)override {trace.emplace_back("r8:"+std::to_string(a));return !fail_reads && get(a,v);}
 bool read_u16(std::uint32_t a,std::uint16_t& v)override {trace.emplace_back("r16:"+std::to_string(a));return !fail_reads && get(a,v);}
 bool read_u32(std::uint32_t a,std::uint32_t& v)override {trace.emplace_back("r32:"+std::to_string(a));return !fail_reads && get(a,v);}
 bool write_u16(std::uint32_t a,std::uint16_t v)override {
  trace.emplace_back("w16:"+std::to_string(a));++write_number;
  if(reenter && rotator && a==p::kSentinelNode)nested=rotator->left(table,0u).code;
  if(throw_on_write)throw std::runtime_error("host fault");
  if(write_number==failed_write)return false;
  set16(a,v);
  if(relocation && a==relocation_trap){
    // Source reads the slot array again; copy all old byte-entries into
    // relocated memory, except the root/table fields.
    for(const auto& [addr,byte]:mem)
      if(addr>=slots && addr<slots+4096u)set8(0x5000u+addr-slots,byte);
    set32(table+4,0x5000u);
  }
  return true;
 }
 bool write_u32(std::uint32_t a,std::uint32_t v)override {
  trace.emplace_back("w32:"+std::to_string(a));++write_number;
  if(write_number==failed_write)return false;
  if(throw_on_write)throw std::runtime_error("host fault");
  set32(a,v);return true;
 }
};
void sentinel_behavior(){
 Host h;h.initialize();p::SentinelSlotAccessor accessor(h);h.accessor=&accessor;
 auto a=accessor.resolve(table,0xffff1234u);
 assert(a.code==p::Code::Completed && a.initialized_sentinel && a.writes_committed==5);
 assert(a.slot_va==slots+16u*0x1234u);
 assert(h.d(p::kSentinelInitFlags)==0x201u);
 for(unsigned off=0;off<6;off+=2)assert(h.w(p::kSentinelNode+off)==0xffffu);
 assert(h.w(p::kSentinelNode+6)==1u);
 const auto before=h.write_number;
 a=accessor.resolve(table,0x1234ffffu);
 assert(a.code==p::Code::Completed && !a.initialized_sentinel && a.writes_committed==0);
 assert(a.slot_va==p::kSentinelNode && before==h.write_number);
 h.initialize();h.failed_write=3;
 a=accessor.resolve(table,1u);
 assert(a.code==p::Code::HostRejected && a.writes_committed==2u);
 assert(h.d(p::kSentinelInitFlags)==0x201u);
 assert(h.w(p::kSentinelNode)==0xffffu);
 h.initialize();h.set32(p::kSentinelInitFlags,1u);
 a=accessor.resolve(table,0xffffffffu);
 assert(a.code==p::Code::Completed && a.writes_committed==0);
 h.initialize();h.set32(table+4,0xfffffff0u);
 a=accessor.resolve(table,3u);
 assert(a.code==p::Code::AddressOverflow && a.writes_committed==5u);
 h.initialize();h.set32(table+4,0u);
 a=accessor.resolve(table,1u);
 assert(a.code==p::Code::NullArrayHazard && a.writes_committed==5u);
}
void root_left(){
 Host h;h.initialize();h.nset(1,0xffffu,2,0xffffu);h.nset(2,3,0xffffu,1);h.nset(3,0xffffu,0xffffu,2);h.set16(table+0x10,1u);
 p::SentinelSlotAccessor a(h);p::RotateNode rot(h,a);h.accessor=&a;h.rotator=&rot;
 auto r=rot.left(table,0xabc00001u);
 assert(r.code==p::Code::Completed && r.index==1 && r.pivot==2 && r.transferred==3);
 assert(r.root_updated && !r.parent_updated && r.sentinel_initializations==1);
 assert(r.accessor_calls==5); // x, y, y, y, x (root branch)
 assert(r.writes_committed==11); // 5 singleton + 6 node/root writes
 assert(h.w(table+0x10)==2);
 assert(h.w(h.node(1)+2)==3 && h.w(h.node(1)+4)==2);
 assert(h.w(h.node(2)+0)==1 && h.w(h.node(2)+4)==0xffffu);
 assert(h.w(h.node(3)+4)==1);
 assert(h.w(p::kSentinelNode+0)==0xffffu);
}
void root_right(){
 Host h;h.initialize();h.nset(2,1,0xffffu,0xffffu);h.nset(1,0xffffu,3,2);h.nset(3,0xffffu,0xffffu,1);h.set16(table+0x10,2);
 p::SentinelSlotAccessor a(h);p::RotateNode rot(h,a);
 auto r=rot.right(table,2);
 assert(r.code==p::Code::Completed && r.root_updated && r.pivot==1 && r.transferred==3);
 assert(h.w(table+0x10)==1);
 assert(h.w(h.node(2)+0)==3 && h.w(h.node(2)+4)==1);
 assert(h.w(h.node(1)+2)==2 && h.w(h.node(1)+4)==0xffffu);
 assert(h.w(h.node(3)+4)==2);
}
void rotations_without_transferred_child(){
 Host h;h.initialize();h.nset(1,0xffffu,2,0xffffu);h.nset(2,0xffffu,0xffffu,1);h.set16(table+0x10,1);
 p::SentinelSlotAccessor a(h);p::RotateNode r(h,a);
 auto out=r.left(table,1);
 assert(out.code==p::Code::Completed && out.transferred==0xffffu);
 assert(out.accessor_calls==4 && out.writes_committed==10); // init5 + five node/root writes
 assert(h.w(h.node(1)+2)==0xffffu && h.w(table+0x10)==2);
 h.initialize();h.nset(2,1,0xffffu,0xffffu);h.nset(1,0xffffu,0xffffu,2);h.set16(table+0x10,2);
 out=r.right(table,2);
 assert(out.code==p::Code::Completed && out.transferred==0xffffu);
 assert(out.accessor_calls==4 && h.w(table+0x10)==1);
}
void nonroot_branching(){
 Host h;h.initialize();p::SentinelSlotAccessor a(h);p::RotateNode r(h,a);
 // x is left child of p, right child of x is pivot.
 h.nset(0,1,0xffffu,0xffffu);h.nset(1,0xffffu,2,0);h.nset(2,0xffffu,0xffffu,1);h.set16(table+0x10,0);
 auto out=r.left(table,1);
 assert(out.code==p::Code::Completed && out.parent_updated&&!out.root_updated);
 assert(h.w(h.node(0))==2 && h.w(h.node(2)+4)==0);
 assert(h.w(h.node(2))==1 && h.w(h.node(1)+4)==2);
 // x is right child of p; left rotation must choose right field.
 h.initialize();h.nset(0,0xffffu,1,0xffffu);h.nset(1,0xffffu,2,0);h.nset(2,0xffffu,0xffffu,1);h.set16(table+0x10,0);
 out=r.left(table,1);assert(out.code==p::Code::Completed);
 assert(h.w(h.node(0)+2)==2);
 // mirrored: x right child of p, rotating right.
 h.initialize();h.nset(0,0xffffu,2,0xffffu);h.nset(2,1,0xffffu,0);h.nset(1,0xffffu,0xffffu,2);h.set16(table+0x10,0);
 out=r.right(table,2);assert(out.code==p::Code::Completed);
 assert(h.w(h.node(0)+2)==1);
 // mirrored: x left child of p, rotating right.
 h.initialize();h.nset(0,2,0xffffu,0xffffu);h.nset(2,1,0xffffu,0);h.nset(1,0xffffu,0xffffu,2);h.set16(table+0x10,0);
 out=r.right(table,2);assert(out.code==p::Code::Completed);
 assert(h.w(h.node(0))==1);
}
void failures_and_relations(){
 Host h;h.initialize();p::SentinelSlotAccessor a(h);p::RotateNode r(h,a);h.rotator=&r;
 auto out=r.left(0,1);assert(out.code==p::Code::NullTableHazard);
 h.set32(table+4,0xfffffff0u);out=r.left(table,1);
 assert(out.code==p::Code::AddressOverflow);
 h.initialize();h.nset(1,0xffffu,2,0xffffu);h.nset(2,0xffffu,0xffffu,1);h.set16(table+0x10,1);
 h.failed_write=6u;out=r.left(table,1); // after 5 initial sentinel writes, fail x.right
 assert(out.code==p::Code::HostRejected && out.writes_committed==5u);
 assert(h.w(h.node(1)+2)==2);
 h.initialize();h.nset(1,0xffffu,2,0xffffu);h.nset(2,0xffffu,0xffffu,1);h.set16(table+0x10,1);
 h.failed_write=7;out=r.left(table,1);
 assert(out.code==p::Code::HostRejected && out.writes_committed==6u);
 assert(h.w(h.node(1)+2)==0xffffu && h.w(h.node(2)+4)==1);
 h.initialize();h.nset(1,0xffffu,2,0xffffu);h.nset(2,0xffffu,0xffffu,1);h.set16(table+0x10,1);
 h.reenter=true;out=r.left(table,1);
 assert(out.code==p::Code::Completed && h.nested==p::Code::Reentrant);
 h.initialize();h.nset(1,0xffffu,2,0xffffu);h.nset(2,0xffffu,0xffffu,1);h.set16(table+0x10,1);
 h.throw_on_write=true;out=r.left(table,1);
 assert(out.code==p::Code::HostRejected && out.writes_committed==0u);
 // A source array can move on a host-write callback; future direct accesses
 // must reload [table+4] rather than caching initial array.
 h.initialize();h.nset(1,0xffffu,2,0xffffu);h.nset(2,0xffffu,0xffffu,1);h.set16(table+0x10,1);
 h.relocation=true;h.relocation_trap=h.node(1)+2;
 out=r.left(table,1);
 assert(out.code==p::Code::Completed);
 assert(h.d(table+4)==0x5000u);
 assert(h.w(0x5000u+16u*2u)==1u);
}
}
int main(){sentinel_behavior();root_left();root_right();rotations_without_transferred_child();nonroot_branching();failures_and_relations();}
