// Phase636 authentic 2006 node constructor/linker/owner registration.
// Synthetic host memory, no retail module loaded or native engine ABI.
// Test setup -> registration -> phase635 real range-conversion model.
#include "sin_compat/original_elite_mapping_init_phase636.hpp"
#include "sin_compat/original_elite_id_mapping_phase635.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
namespace i=sin_compat::original_elite_mapping_init_phase636;
namespace m=sin_compat::original_elite_id_mapping_phase635;
namespace {
struct Host final:i::IGuestMemory,m::IHost {
  std::map<std::uint32_t,std::uint32_t> words;
  std::vector<std::string> actions;
  std::uint32_t fail_read=0u,fail_write=0u;
  bool throw_read=false,throw_write=false;
  i::NodeLinker* reenter=nullptr;
  i::Code nested=i::Code::Completed;
  bool read_u32(std::uint32_t address,std::uint32_t& value)override {
    actions.push_back("R"+std::to_string(address));
    if(reenter) nested=reenter->link(0x1000u,0x2000u,0u).code;
    if(throw_read)throw std::runtime_error("synthetic read exception");
    if(address==fail_read)return false;
    const auto found=words.find(address);
    if(found==words.end())return false;
    value=found->second;return true;
  }
  bool write_u32(std::uint32_t address,std::uint32_t value)override {
    actions.push_back("W"+std::to_string(address));
    if(throw_write)throw std::runtime_error("synthetic write exception");
    if(address==fail_write)return false;
    words[address]=value;return true;
  }
  bool read_original_mapping_node(
      std::uint64_t address,m::RangeNode& range)override {
    if(address>0xffffffffull)return false;
    const auto base=static_cast<std::uint32_t>(address);
    std::uint32_t value[4]{};
    for(unsigned j=0;j<4u;++j) {
      const std::uint32_t offset=j==0?0u:j==1?4u:j==2?8u:0x10u;
      if(base>0xffffffffu-offset ||
         !read_u32(base+offset,value[j]))return false;
    }
    range.mapped_start=static_cast<std::int32_t>(value[0]);
    range.source_start=static_cast<std::int32_t>(value[1]);
    range.source_end=static_cast<std::int32_t>(value[2]);
    range.next=value[3];return true;
  }
  std::uint32_t get(std::uint32_t address)const {
    const auto found=words.find(address);
    assert(found!=words.end());return found->second;
  }
};
void test_constructor_both_mode_values_and_byte_truncation() {
  Host h;i::NodeInitializer init(h);
  const std::uint32_t node=0x1000u;
  const auto zero=init.initialize(node,0u);
  assert(zero.code==i::Code::Completed&&zero.writes_committed==6u);
  assert(h.get(node+0u)==0xffffffffu);
  assert(h.get(node+4u)==9999u);
  assert(h.get(node+8u)==0xffffffffu);
  assert(h.get(node+12u)==0xffffffffu);
  assert(h.get(node+16u)==0u&&h.get(node+20u)==0u);
  const auto positive=init.initialize(node+0x20u,1u);
  assert(positive.code==i::Code::Completed&&positive.writes_committed==6u);
  assert(h.get(node+0x20u)==0u&&h.get(node+0x24u)==0u);
  assert(h.get(node+0x28u)==0xffffffffu);
  assert(h.get(node+0x2cu)==0xffffffffu);
  assert(h.get(node+0x30u)==0u&&h.get(node+0x34u)==0u);
  assert(init.initialize(node+0x40u,255u).code==i::Code::Completed);
  assert(h.get(node+0x40u)==0u&&h.get(node+0x44u)==0u);
  assert(init.initialize(0u,0u).code==i::Code::OriginalNullPointerHazard);
  const auto over=init.initialize(0xfffffffeu,0u);
  assert(over.code==i::Code::OriginalAddressOverflow);
  // Original constructor writes node+0 before attempting +4. The
  // checked reference preserves that first successful guest write.
  assert(over.writes_committed==1u);
  assert(h.get(0xfffffffeu)==0xffffffffu);
}
void test_conditional_link_source_order_and_sentinel() {
  Host h;i::NodeLinker link(h);
  const std::uint32_t node=0x1000u,source=0x2000u,next=0x3000u;
  h.words[node+0x0cu]=0u;
  h.words[source+4u]=0xface00cdu;
  const auto linked=link.link(node,source,next);
  assert(linked.code==i::Code::Completed&&linked.writes_committed==6u);
  assert(h.get(node+0u)==0xface00cdu);
  assert(h.get(node+4u)==9999u);
  assert(h.get(node+8u)==0xffffffffu&&h.get(node+12u)==0xffffffffu);
  assert(h.get(node+0x10u)==next&&h.get(node+0x14u)==source);
  assert(h.actions==std::vector<std::string>({
    "R4108","W4100","W4104","W4108",
    "W4112","W4116","R8196","W4096"}));
  // For original node [+0xC]=-1, conditional +4/+8/+C mutation
  // MUST be skipped. +4=9999 remains a skip sentinel.
  h.actions.clear();h.words[node+0x0c]=0xffffffffu;
  h.words[node+4u]=9999u;h.words[source+4u]=1234u;
  const auto held=link.link(node,source,0u);
  assert(held.code==i::Code::Completed&&held.writes_committed==3u);
  assert(h.get(node+4u)==9999u&&h.get(node+0u)==1234u);
  assert(h.get(node+0x10u)==0u);
  assert(h.actions==std::vector<std::string>({
    "R4108","W4112","W4116","R8196","W4096"}));
  // Without a sentinel, next==0 resets source low to zero.
  h.words[node+0x0c]=0u;h.actions.clear();
  assert(link.link(node,source,0u).code==i::Code::Completed);
  assert(h.get(node+4u)==0u);
}
void test_original_null_read_hazard_and_partial_writes() {
  Host h;i::NodeLinker linker(h);
  h.words[0x100cu]=0xffffffffu;
  const auto null_source=linker.link(0x1000u,0u,0x3000u);
  assert(null_source.code==i::Code::OriginalNullPointerHazard);
  assert(null_source.stage==i::Stage::ReadSourcePlus4);
  assert(null_source.writes_committed==2u);
  assert(h.get(0x1010u)==0x3000u&&h.get(0x1014u)==0u);
  h.words[0x2004u]=33u;h.fail_write=0x1000u;
  const auto refused=linker.link(0x1000u,0x2000u,0u);
  assert(refused.code==i::Code::HostRejected);
  assert(refused.stage==i::Stage::WriteDestination0);
  assert(refused.writes_committed==2u);
  h.fail_write=0u;h.throw_read=true;
  const auto thrown=linker.link(0x1000u,0x2000u,0u);
  assert(thrown.code==i::Code::HostRejected);
  assert(thrown.stage==i::Stage::ReadExistingC);
  h.throw_read=false;h.reenter=&linker;
  assert(linker.link(0x1000u,0x2000u,0u).code==i::Code::Completed);
  assert(h.nested==i::Code::Reentrant);
}
void initialize_original_owner_nodes(Host& h,std::uint32_t base) {
  i::NodeInitializer init(h);
  for(const auto offset:{
      i::kFirstOwnerNode,i::kSecondOwnerNode,i::kThirdOwnerNode})
    assert(init.initialize(base+offset,0u).code==i::Code::Completed);
}
void test_three_nested_owner_registration_and_phase635() {
  Host h;
  constexpr std::uint32_t owner=0x10000u;
  constexpr std::uint32_t other=0x18000u;
  constexpr std::uint32_t original_name=0x31000u;
  constexpr std::uint32_t source=0x21000u;
  initialize_original_owner_nodes(h,owner);
  initialize_original_owner_nodes(h,other);
  // Invented source-record DWORD values, NOT the original retail
  // memory behind 0x22547624, which is uninitialized virtual .data.
  h.words[source+4u]=1000000003u;
  h.words[source+12u]=1000000004u;
  h.words[source+20u]=1000000033u;
  i::OwnerRegistration registration(h);
  const auto r=registration.register_owner(owner,original_name,source,other);
  assert(r.code==i::Code::Completed);
  assert(r.links_completed==3u&&r.writes_committed==10u);
  assert(h.get(owner)==original_name);
  for(unsigned n=0;n<3u;++n) {
    const std::uint32_t offset=4u+24u*n;
    const std::uint32_t source_offset=8u*n;
    assert(h.get(owner+offset+0x10u)==other+offset);
    assert(h.get(owner+offset+0x14u)==source+source_offset);
    assert(h.get(owner+offset)==h.get(source+source_offset+4u));
    assert(h.get(owner+offset+4u)==9999u);
    assert(h.get(owner+offset+0xcu)==0xffffffffu);
  }
  // Source-backed Phase635 converter sees ONLY sentinel nodes after
  // original constructor/registration. The original runtime must
  // populate range bounds later, not at this registration call.
  m::Reference converter(h);
  const auto no_range=converter.translate(owner+0x34u,33);
  assert(no_range.code==m::Code::NotFound&&no_range.output==-1);
  assert(no_range.visited==2u);
  // This one mutation is deliberately synthetic, NOT an implementation
  // of a still-unrecovered original range-population function.
  h.words[owner+0x34u+4u]=33u;
  h.words[owner+0x34u+8u]=33u;
  const auto filled=converter.translate(owner+0x34u,33);
  assert(filled.code==m::Code::Mapped);
  assert(filled.output==1000000033);
}
void test_owner_null_linked_source_and_failure_provenance() {
  Host h;constexpr std::uint32_t owner=0x10000u,source=0x21000u;
  initialize_original_owner_nodes(h,owner);
  h.words[source+4u]=1u;h.words[source+12u]=2u;
  h.words[source+20u]=3u;
  i::OwnerRegistration reg(h);
  const auto no_child=reg.register_owner(owner,0x3333u,source,0u);
  assert(no_child.code==i::Code::Completed&&no_child.links_completed==3u);
  assert(h.get(owner+4u+0x10u)==0u);
  assert(h.get(owner+0x1cu+0x10u)==0u);
  assert(h.get(owner+0x34u+0x10u)==0u);
  h.fail_write=owner+0x1cu+0x14u;
  const auto mid=reg.register_owner(owner,0x3333u,source,0u);
  assert(mid.code==i::Code::HostRejected);
  assert(mid.stage==i::Stage::LinkSecond);
  assert(mid.links_completed==1u);
  assert(mid.last_node==owner+0x1cu);
  h.fail_write=0u;
  // Native registration stores the classname, then its first original
  // linker writes successor/source fields before reading source+4.
  // A null source must leave those partial side effects observable.
  const auto null_source=reg.register_owner(owner,0x3333u,0u,0u);
  assert(null_source.code==i::Code::OriginalNullPointerHazard);
  assert(null_source.stage==i::Stage::LinkFirst);
  assert(null_source.links_completed==0u);
  assert(null_source.writes_committed==3u);
  assert(h.get(owner+4u+0x10u)==0u);
  assert(h.get(owner+4u+0x14u)==0u);
  const auto overflow=reg.register_owner(0xfffffff0u,0x3333u,source,0u);
  assert(overflow.code==i::Code::OriginalAddressOverflow);
  assert(overflow.links_completed==0u);
}
} // namespace
int main() {
  test_constructor_both_mode_values_and_byte_truncation();
  test_conditional_link_source_order_and_sentinel();
  test_original_null_read_hazard_and_partial_writes();
  test_three_nested_owner_registration_and_phase635();
  test_owner_null_linked_source_and_failure_provenance();
  assert(i::kNodeInitVA==0x22073610u&&i::kNodeInitBytes==58u);
  assert(i::kNodeLinkVA==0x22073650u&&i::kNodeLinkBytes==55u);
  assert(i::kOwnerRegisterVA==0x22088f10u&&i::kOwnerRegisterBytes==119u);
  assert(i::kOriginalEliteOwnerVA+i::kThirdOwnerNode==
         i::kOriginalEliteFirstRangeVA);
  std::puts("PASS Phase636 original 2006 node constructor, conditional linker, three-node owner registration, Phase635 converter initialization boundary");
}
