#include "sin_compat/original_symbol_rebalance_phase642.hpp"
#include "sin_compat/original_symbol_tree_link_phase640.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>
#include <algorithm>
namespace p=sin_compat::original_symbol_rebalance_phase642;
namespace rot=sin_compat::original_symbol_rotations_phase641;
namespace link=sin_compat::original_symbol_tree_link_phase640;
namespace {
constexpr std::uint32_t table=0x2000u,storage=0x3000u;
struct Host final:rot::IHost,link::IHost {
  std::map<std::uint32_t,std::uint8_t> bytes;
  p::Rebalance* fixup=nullptr;
  rot::SentinelSlotAccessor* accessor=nullptr;
  rot::RotateNode* rotations=nullptr;
  std::vector<int> values;
  unsigned fail_on_write=0,write_count=0;
  bool throw_on_write=false,reenter=false;
  p::Code nested=p::Code::Completed;
  p::Result last{};
  std::uint32_t last_index=0;
  void set8(std::uint32_t a,std::uint8_t n){bytes[a]=n;}
  void set16(std::uint32_t a,std::uint16_t n){for(unsigned i=0;i<2;i++)set8(a+i,static_cast<std::uint8_t>(n>>(i*8)));}
  void set32(std::uint32_t a,std::uint32_t n){for(unsigned i=0;i<4;i++)set8(a+i,static_cast<std::uint8_t>(n>>(i*8)));}
  template<typename T> bool fetch(std::uint32_t a,T& n)const {
    n=0;
    for(unsigned i=0;i<sizeof(T);i++){
      auto it=bytes.find(a+i);if(it==bytes.end())return false;
      n|=static_cast<T>(it->second)<<(i*8);
    }
    return true;
  }
  std::uint16_t u16(std::uint32_t a)const {std::uint16_t n=0;assert(fetch(a,n));return n;}
  std::uint32_t u32(std::uint32_t a)const {std::uint32_t n=0;assert(fetch(a,n));return n;}
  std::uint32_t node(std::uint16_t index)const{return storage+16u*index;}
  std::uint16_t field(std::uint16_t index,unsigned offset)const {
    if(index==0xffffu)return (offset==6u)?1u:0xffffu;
    return u16(node(index)+offset);
  }
  void reset(){
    bytes.clear();values.clear();write_count=fail_on_write=0;
    throw_on_write=reenter=false;nested=p::Code::Completed;last={};
    set32(table+4u,storage);set16(table+0x10u,0xffffu);
    set32(rot::kSentinelInitFlags,0u);
  }
  bool read_u8(std::uint32_t a,std::uint8_t& n)override {return fetch(a,n);}
  bool read_u16(std::uint32_t a,std::uint16_t& n)override {return fetch(a,n);}
  bool read_u32(std::uint32_t a,std::uint32_t& n)override {return fetch(a,n);}
  bool write_u16(std::uint32_t a,std::uint16_t n)override {
    ++write_count;
    if(reenter && fixup && a==node(0)+6u)
      nested=fixup->insert(table,0u).code;
    if(throw_on_write)throw std::runtime_error("test injected fault");
    if(fail_on_write && write_count==fail_on_write)return false;
    set16(a,n);return true;
  }
  bool write_u32(std::uint32_t a,std::uint32_t n)override {
    ++write_count;
    if(throw_on_write)throw std::runtime_error("test injected fault");
    if(fail_on_write && write_count==fail_on_write)return false;
    set32(a,n);return true;
  }
  bool rebalance_original_22281840(std::uint32_t ctx,std::uint32_t index)override {
    assert(ctx==table && fixup);last_index=index;
    last=fixup->insert(ctx,index);
    return last.code==p::Code::Completed;
  }
  bool insert(link::LinkNode& linker,int v){
    const std::uint16_t next=static_cast<std::uint16_t>(values.size());
    std::uint16_t cursor=u16(table+0x10),parent=0xffffu;
    std::uint32_t side=0;
    while(cursor!=0xffffu){
      parent=cursor;
      if(v<values[cursor]){cursor=field(cursor,0);side=1;}
      else if(v>values[cursor]){cursor=field(cursor,2);side=0;}
      else return false;
    }
    values.push_back(v);
    auto result=linker.link(table,next,parent,side);
    return result.code==link::Code::Linked;
  }
  int validate_subtree(std::uint16_t index,std::uint16_t parent,
                       int lower,int upper,std::set<std::uint16_t>& seen)const {
    if(index==0xffffu)return 1;
    assert(index<values.size());
    assert(seen.insert(index).second);
    assert(field(index,4)==parent);
    assert(values[index]>lower && values[index]<upper);
    const auto color=field(index,6u);
    assert(color==p::kBlack||color==p::kRed);
    const auto l=field(index,0u),r=field(index,2u);
    if(color==p::kRed){assert(field(l,6u)==p::kBlack);assert(field(r,6u)==p::kBlack);}
    const int dl=validate_subtree(l,index,lower,values[index],seen);
    const int dr=validate_subtree(r,index,values[index],upper,seen);
    assert(dl==dr);
    return dl+(color==p::kBlack?1:0);
  }
  void validate()const {
    const std::uint16_t root=u16(table+0x10u);
    if(values.empty()){assert(root==0xffffu);return;}
    assert(field(root,6u)==p::kBlack);
    std::set<std::uint16_t> seen;
    const int depth=validate_subtree(root,0xffffu,-1000000,1000000,seen);
    assert(depth>0);
    assert(seen.size()==values.size());
  }
};
void root_short_circuit(){
  Host h;h.reset();rot::SentinelSlotAccessor acc(h);rot::RotateNode rotate(h,acc);
  p::Rebalance rb(h,acc,rotate);h.fixup=&rb;
  h.set16(table+0x10u,0u);h.set16(h.node(0)+6u,0u);
  auto r=rb.insert(table,0xffff0000u);
  assert(r.code==p::Code::Completed&&r.reached_root_initially);
  assert(r.root_blackened&&r.left_rotations==0&&r.right_rotations==0);
  assert(r.sentinel_writes_committed==0u && h.u32(rot::kSentinelInitFlags)==0u);
  assert(h.field(0,6u)==1u);
}
void test_bst_insertion_and_structure(const std::vector<int>& input){
  Host h;h.reset();rot::SentinelSlotAccessor acc(h);rot::RotateNode rotate(h,acc);
  p::Rebalance rb(h,acc,rotate);link::LinkNode linknodes(h);
  h.fixup=&rb;h.accessor=&acc;h.rotations=&rotate;
  unsigned left=0,right=0,recolors=0,initial_roots=0;
  for(int v:input){
    assert(h.insert(linknodes,v));
    assert(h.last.code==p::Code::Completed&&h.last.root_blackened);
    left+=h.last.left_rotations;right+=h.last.right_rotations;
    recolors+=h.last.recolor_cases;
    initial_roots+=h.last.reached_root_initially?1u:0u;
    h.validate();
  }
  assert(initial_roots==1u);
  if(input.size()>=6u)assert(left+right+recolors>0);
}
void test_zigzag_and_recolor_counts(){
  auto run=[](std::vector<int> seq){
    Host h;h.reset();rot::SentinelSlotAccessor acc(h);rot::RotateNode rotate(h,acc);
    p::Rebalance rb(h,acc,rotate);link::LinkNode linker(h);h.fixup=&rb;
    for(int v:seq){assert(h.insert(linker,v));h.validate();}
    return h.last;
  };
  auto a=run({10,20,15});assert(a.left_rotations==1&&a.right_rotations==1);
  auto b=run({20,10,15});assert(b.left_rotations==1&&b.right_rotations==1);
  auto c=run({10,20,30});assert(c.left_rotations==1&&c.right_rotations==0);
  auto d=run({30,20,10});assert(d.right_rotations==1&&d.left_rotations==0);
  auto e=run({10,5,15,1});assert(e.recolor_cases==1&&e.left_rotations==0&&e.right_rotations==0);
  auto f=run({10,5,15,20});assert(f.recolor_cases==1&&f.left_rotations==0&&f.right_rotations==0);
}
void test_many_permutations(){
  test_bst_insertion_and_structure({2,1,3,0,4,5,8,7,6,9,11,10,13,12,14});
  test_bst_insertion_and_structure({15,14,13,12,11,10,9,8,7,6,5,4,3,2,1});
  test_bst_insertion_and_structure({1,2,3,4,5,6,7,8,9,10,11,12,13,14,15});
  for(std::uint32_t seed=1;seed<=12;seed++){
    std::vector<int> seq;
    for(int i=0;i<127;i++)seq.push_back(i*5-300);
    std::uint32_t state=seed;
    for(std::size_t i=seq.size()-1;i>0;i--){
      state^=state<<13;state^=state>>17;state^=state<<5;
      std::swap(seq[i],seq[state%(i+1)]);
    }
    test_bst_insertion_and_structure(seq);
  }
}
void test_failures_and_partial_writes(){
  Host h;h.reset();rot::SentinelSlotAccessor acc(h);rot::RotateNode rotate(h,acc);
  p::Rebalance rb(h,acc,rotate);h.fixup=&rb;
  auto r=rb.insert(0u,0u);assert(r.code==p::Code::NullTableHazard);
  h.set16(table+0x10u,0u);h.set16(h.node(0)+6u,0u);
  h.set32(table+4u,0u);
  r=rb.insert(table,0u);assert(r.code==p::Code::NullArrayHazard&&!r.root_blackened);
  h.reset();h.set16(table+0x10u,0u);h.set16(h.node(0)+6u,0u);
  h.fail_on_write=1;
  r=rb.insert(table,0u);assert(r.code==p::Code::HostRejected && !r.root_blackened);
  h.reset();h.set16(table+0x10u,0u);h.set16(h.node(0)+6u,0u);
  h.throw_on_write=true;
  r=rb.insert(table,0u);assert(r.code==p::Code::HostRejected);
  h.reset();h.set16(table+0x10u,0u);h.set16(h.node(0)+6u,0u);
  h.reenter=true;
  r=rb.insert(table,0u);
  assert(r.code==p::Code::Completed && h.nested==p::Code::Reentrant);
  // a self-parent red node intentionally creates a cyclic original tree;
  // portable fixup refuses rather than spinning forever.
  h.reset();h.set16(table+0x10u,1u);
  h.set16(h.node(0)+4u,0u);h.set16(h.node(0)+6u,0u);
  h.set16(h.node(0)+0u,0u);h.set16(h.node(0)+2u,0u);
  r=rb.insert(table,0u);
  assert(r.code!=p::Code::Completed);
}
}
int main(){root_short_circuit();test_zigzag_and_recolor_counts();test_many_permutations();test_failures_and_partial_writes();}
