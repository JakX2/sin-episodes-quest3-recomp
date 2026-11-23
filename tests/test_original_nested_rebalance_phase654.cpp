#include "sin_compat/original_nested_rebalance_phase654.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <numeric>
#include <vector>

namespace n = sin_compat::original_nested_rebalance_phase654;
namespace {
constexpr std::uint32_t kMap = 0x2000u, kArray = 0x5000u;
struct Guest final : n::IHost {
  std::array<std::uint8_t, 0x10000> mem{};
  std::array<std::uint8_t, 16> statics{};
  bool fail = false;
  std::uint8_t& byte(std::uint32_t a) {
    if (a >= n::kSentinelVA && a < n::kSentinelVA + statics.size())
      return statics.at(static_cast<std::size_t>(a - n::kSentinelVA));
    return mem.at(static_cast<std::size_t>(a));
  }
  bool read_u8(std::uint32_t a, std::uint8_t& v) override {
    if (fail) return false;
    v = byte(a); return true;
  }
  bool read_u16(std::uint32_t a, std::uint16_t& v) override {
    if (fail) return false;
    v = static_cast<std::uint16_t>(byte(a) | (byte(a+1u)<<8)); return true;
  }
  bool read_u32(std::uint32_t a, std::uint32_t& v) override {
    if (fail) return false;
    v = static_cast<std::uint32_t>(byte(a)) |
        (static_cast<std::uint32_t>(byte(a+1u)) << 8) |
        (static_cast<std::uint32_t>(byte(a+2u)) << 16) |
        (static_cast<std::uint32_t>(byte(a+3u)) << 24);
    return true;
  }
  bool write_u16(std::uint32_t a, std::uint16_t v) override {
    if (fail) return false;
    byte(a) = static_cast<std::uint8_t>(v);
    byte(a+1u) = static_cast<std::uint8_t>(v >> 8);
    return true;
  }
  bool write_u32(std::uint32_t a, std::uint32_t v) override {
    if (fail) return false;
    for (unsigned i=0;i<4u;++i) byte(a+i) = static_cast<std::uint8_t>(v >> (i*8u));
    return true;
  }
  std::uint16_t get16(std::uint32_t a) { std::uint16_t v=0;assert(read_u16(a,v));return v; }
  std::uint32_t get32(std::uint32_t a) { std::uint32_t v=0;assert(read_u32(a,v));return v; }
  void set16(std::uint32_t a, std::uint16_t v) { assert(write_u16(a,v)); }
  void set32(std::uint32_t a, std::uint32_t v) { assert(write_u32(a,v)); }
  std::uint32_t node(std::uint16_t id) { return kArray + static_cast<std::uint32_t>(id)*12u; }
  std::uint16_t at(std::uint16_t id, unsigned offset) { return get16(node(id)+offset); }
  void put(std::uint16_t id, unsigned offset, std::uint16_t value) { set16(node(id)+offset,value); }
  Guest() { set32(kMap+4u,kArray);set16(kMap+0x10u,n::kNil);set32(n::kSentinelFlagVA,0xDEADBEEEu); }
};
struct Tree {
  Guest g;
  n::Rebalance runner{g};
  std::uint16_t inserted = 0;
  unsigned left_rot = 0, right_rot = 0;
  void add(unsigned key) {
    const auto id = inserted++;
    g.put(id,0u,n::kNil);g.put(id,2u,n::kNil);
    g.put(id,4u,n::kNil);g.put(id,6u,n::kRed);
    g.set32(g.node(id)+8u,key);
    auto parent=n::kNil;
    auto cur=g.get16(kMap+0x10u);
    while(cur!=n::kNil) {
      parent=cur;
      cur=g.at(cur,g.get32(g.node(cur)+8u)>key?0u:2u);
    }
    g.put(id,4u,parent);
    if(parent==n::kNil)g.set16(kMap+0x10u,id);
    else g.put(parent,g.get32(g.node(parent)+8u)>key?0u:2u,id);
    const auto r=runner.fixup(kMap,id);
    assert(r.code==n::Code::Completed && r.root_blacked);
    left_rot+=r.left_rotations;right_rot+=r.right_rotations;
    check();
  }
  unsigned check_node(std::uint16_t id,std::uint16_t parent,
                      std::uint32_t lower,std::uint32_t upper,
                      std::vector<bool>& seen) {
    if(id==n::kNil)return 1u;
    assert(id < inserted);
    assert(!seen.at(id));seen[id]=true;
    assert(g.at(id,4u)==parent);
    const auto key=g.get32(g.node(id)+8u);
    assert(lower<key && key<upper);
    const auto color=g.at(id,6u);
    assert(color==n::kRed||color==n::kBlack);
    const auto l=g.at(id,0u),r=g.at(id,2u);
    if(color==n::kRed) {
      if(l!=n::kNil) assert(g.at(l,6u)==n::kBlack);
      if(r!=n::kNil) assert(g.at(r,6u)==n::kBlack);
    }
    const unsigned lb=check_node(l,id,lower,key,seen);
    const unsigned rb=check_node(r,id,key,upper,seen);
    assert(lb==rb);
    return lb + (color==n::kBlack ? 1u:0u);
  }
  void check() {
    const auto root=g.get16(kMap+0x10u);
    assert(root!=n::kNil && g.at(root,4u)==n::kNil && g.at(root,6u)==n::kBlack);
    std::vector<bool> seen(inserted,false);
    assert(check_node(root,n::kNil,0u,100000u,seen)>=2u);
    assert(std::all_of(seen.begin(),seen.end(),[](bool b){return b;}));
    // Native insertion of the first/root node does not touch the sentinel.
    if(inserted==1u) {
      assert(g.get32(n::kSentinelFlagVA)==0xDEADBEEEu);
      assert(g.get16(n::kSentinelVA+6u)==0u);
    }
    // Sentinel is lazily initialized on non-root operations.
    if(inserted>=2u) {
      assert((g.get32(n::kSentinelFlagVA)&1u)==1u);
      assert(g.get16(n::kSentinelVA+0u)==n::kNil);
      assert(g.get16(n::kSentinelVA+2u)==n::kNil);
      assert(g.get16(n::kSentinelVA+4u)==n::kNil);
      assert(g.get16(n::kSentinelVA+6u)==n::kBlack);
      assert(g.get32(n::kSentinelFlagVA)==0xDEADBEEFu);
    }
  }
};
void insertion_cases() {
  const std::array<std::array<unsigned,3>,4> cases{{{{30u,20u,10u}},{{10u,20u,30u}},
                                                 {{30u,10u,20u}},{{10u,30u,20u}}}};
  for (const auto& values:cases) {
    Tree tree;
    for(unsigned v:values)tree.add(v);
    assert(tree.left_rot+tree.right_rot>=1u);
  }
}
void all_permutations() {
  std::array<unsigned,7> values{{1u,2u,3u,4u,5u,6u,7u}};
  unsigned tested=0,uncles=0;
  do {
    Tree tree;
    for(unsigned v:values)tree.add(v);
    uncles += tree.left_rot+tree.right_rot;
    ++tested;
  }while(std::next_permutation(values.begin(),values.end()));
  assert(tested==5040u && uncles>1000u);
}
void native_rotations() {
  Guest g;
  // x=0 root; y=1 right; beta=2 is y.left, initially black.
  for(std::uint16_t i=0;i<3u;++i) {
    for(unsigned off : {0u,2u,4u})g.put(i,off,n::kNil);
    g.put(i,6u,n::kBlack);
  }
  g.set16(kMap+0x10u,0u);
  g.put(0u,2u,1u);g.put(1u,4u,0u);
  g.put(1u,0u,2u);g.put(2u,4u,1u);
  n::Rebalance model(g);
  auto r=model.rotate_left(kMap,0u);
  assert(r.code==n::Code::Completed&&r.left_rotations==1u);
  assert(g.get16(kMap+0x10u)==1u);
  assert(g.at(1u,0u)==0u && g.at(0u,2u)==2u && g.at(2u,4u)==0u);
  r=model.rotate_right(kMap,1u);
  assert(r.code==n::Code::Completed&&r.right_rotations==1u);
  assert(g.get16(kMap+0x10u)==0u);
  assert(g.at(0u,2u)==1u && g.at(1u,0u)==2u && g.at(2u,4u)==1u);
  assert(model.rotate_right(kMap,0u).code==n::Code::InvalidRotation);
}
void invalid_inputs() {
  Guest g;n::Rebalance model(g);
  assert(model.fixup(0u,0u).code==n::Code::InvalidMap);
  assert(model.fixup(kMap,n::kNil).code==n::Code::InvalidNode);
  assert(model.fixup(kMap,0u).code==n::Code::InvalidNode);
  g.fail=true;
  assert(model.fixup(kMap,0u).code==n::Code::HostRejected);
}
}
int main() { insertion_cases();all_permutations();native_rotations();invalid_inputs(); }
