// Phase635: source-based 58-byte original Elite 2006 range-ID translator.
// Whole compiled composition: original Phase632 event -> Phase633 gameplay
// helpers -> Phase634 flags/state -> Phase635 original range converter.
// All nodes/actors are invented synthetic host fixtures, not retail data.
#include "sin_compat/original_elite_id_mapping_phase635.hpp"
#include "sin_compat/original_elite_event47_terminal_phase634.hpp"
#include "sin_compat/original_elite_event47_helpers_phase633.hpp"
#include "sin_compat/original_elite_secondary_event_phase632.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
namespace m=sin_compat::original_elite_id_mapping_phase635;
namespace t=sin_compat::original_elite_event47_terminal_phase634;
namespace h=sin_compat::original_elite_event47_helpers_phase633;
namespace e=sin_compat::original_elite_secondary_event_phase632;
namespace {
constexpr std::uint64_t kSyntheticMapperOwner=0x71000000ull;
constexpr std::uint64_t kSyntheticMapperHead=
    kSyntheticMapperOwner+m::kVirtualContextMemberOffset;
constexpr std::uint64_t kSyntheticMapperSecond=0x71000084ull;
class TableHost :public m::IHost {
public:
  std::map<std::uint64_t,m::RangeNode> nodes;
  std::vector<std::uint64_t> reads;
  bool refuse=false,throw_on_read=false;
  m::Reference* reenter=nullptr;
  m::Code reentered_code=m::Code::Mapped;
  bool read_original_mapping_node(
      std::uint64_t address,m::RangeNode& result)override {
    reads.push_back(address);
    if(reenter)
      reentered_code=reenter->translate(address,4).code;
    if(throw_on_read)throw std::runtime_error("synthetic read fault");
    if(refuse)return false;
    const auto it=nodes.find(address);
    if(it==nodes.end())return false;
    result=it->second;return true;
  }
};
void test_exact_inclusive_range_and_first_match() {
  TableHost host;
  host.nodes.emplace(0x1000ull,m::RangeNode{1000000003,3,5,0x2000ull});
  host.nodes.emplace(0x2000ull,m::RangeNode{987,4,8,0u});
  m::Reference ref(host);
  for(const std::int32_t n:{3,4,5}) {
    const auto r=ref.translate(0x1000ull,n);
    assert(r.code==m::Code::Mapped);
    assert(r.output==1000000000+n);
    assert(r.matched_node==0x1000ull && r.visited==1u);
  }
  const auto next=ref.translate(0x1000ull,6);
  assert(next.code==m::Code::Mapped);
  assert(next.output==989&&next.visited==2u);
  assert(next.matched_node==0x2000ull);
  const auto end=ref.translate(0x1000ull,8);
  assert(end.code==m::Code::Mapped&&end.output==991);
  const auto none=ref.translate(0x1000ull,9);
  assert(none.code==m::Code::NotFound&&none.output==-1);
  assert(none.visited==2u);
}
void test_skip_sentinel_and_input_minus_one() {
  TableHost host;
  host.nodes.emplace(0x1000ull,m::RangeNode{
      2222,9999,20000,0x2000ull});
  host.nodes.emplace(0x2000ull,m::RangeNode{
      1000010000,10000,10000,0u});
  m::Reference ref(host);
  const auto valid=ref.translate(0x1000ull,10000);
  assert(valid.code==m::Code::Mapped);
  assert(valid.output==1000010000&&valid.visited==2u);
  assert(host.reads==std::vector<std::uint64_t>({0x1000ull,0x2000ull}));
  host.reads.clear();
  const auto absent=ref.translate(0x1000ull,9999);
  assert(absent.code==m::Code::NotFound&&absent.visited==2u);
  host.reads.clear();
  const auto minus_one=ref.translate(0u,-1);
  assert(minus_one.code==m::Code::NotFound);
  assert(minus_one.input_minus_one_shortcut);
  assert(minus_one.visited==0u&&host.reads.empty());
  const auto null=ref.translate(0u,0);
  assert(null.code==m::Code::OriginalNullHeadHazard);
  assert(null.stage==m::Stage::Head);
}
void test_signed_comparisons_and_32bit_wraparound() {
  TableHost host;
  host.nodes.emplace(0x1000ull,m::RangeNode{
      1000000000,-4,-2,0u});
  m::Reference ref(host);
  const auto negative=ref.translate(0x1000ull,-2);
  assert(negative.code==m::Code::Mapped);
  assert(negative.output==1000000002);
  assert(ref.translate(0x1000ull,-5).code==m::Code::NotFound);
  host.nodes[0x1000ull]=m::RangeNode{
      std::numeric_limits<std::int32_t>::max(),0,3,0u};
  const auto wrapped=ref.translate(0x1000ull,1);
  assert(wrapped.code==m::Code::Mapped);
  assert(wrapped.output==std::numeric_limits<std::int32_t>::min());
  host.nodes[0x1000ull]=m::RangeNode{-1,0,2,0u};
  const auto mapped_minus_one=ref.translate(0x1000ull,0);
  assert(mapped_minus_one.code==m::Code::Mapped);
  assert(mapped_minus_one.output==-1); // matched source can equal sentinel
  host.nodes[0x1000ull]=m::RangeNode{1,9,8,0u};
  assert(ref.translate(0x1000ull,9).code==m::Code::NotFound);
}
void test_cycles_bounded_chains_and_host_failures() {
  TableHost host;
  host.nodes.emplace(0x1000ull,m::RangeNode{1,100,101,0x2000ull});
  host.nodes.emplace(0x2000ull,m::RangeNode{2,100,101,0x1000ull});
  m::Reference ref(host);
  const auto cycle=ref.translate(0x1000ull,3);
  assert(cycle.code==m::Code::OriginalMappingCycle);
  assert(cycle.visited==2u&&cycle.last_node==0x1000ull);
  host.reads.clear();host.nodes[0x2000ull].next=0x3000ull;
  const auto missing=ref.translate(0x1000ull,3);
  assert(missing.code==m::Code::HostRejected);
  assert(missing.stage==m::Stage::ReadNode&&missing.visited==3u);
  host.refuse=true;
  const auto refused=ref.translate(0x1000ull,3);
  assert(refused.code==m::Code::HostRejected&&refused.visited==1u);
  host.refuse=false;host.throw_on_read=true;
  assert(ref.translate(0x1000ull,3).code==m::Code::HostRejected);
  host.throw_on_read=false;host.reenter=&ref;
  const auto nested=ref.translate(0x1000ull,3);
  assert(nested.code==m::Code::HostRejected);
  assert(host.reentered_code==m::Code::Reentrant);
  host.reenter=nullptr;host.nodes.clear();host.reads.clear();
  for(unsigned i=0u;i<=m::kMaxSafeOriginalNodes;++i) {
    const auto addr=0x4000ull+0x20ull*i;
    const auto next=0x4000ull+0x20ull*(i+1u);
    host.nodes[addr]=m::RangeNode{0,100,101,next};
  }
  const auto limit=ref.translate(0x4000ull,3);
  assert(limit.code==m::Code::OriginalMappingLimit);
  assert(limit.visited==m::kMaxSafeOriginalNodes);
  assert(host.reads.size()==m::kMaxSafeOriginalNodes);
}
struct Integrated final:TableHost,t::IHost,e::IHost,
                        h::IClockHost,h::ISerialHost,h::IFieldHost {
  std::vector<std::string> calls;
  std::array<std::uint32_t,4> flags{{0xffffffffu,0u,0u,0u}};
  std::uint32_t state7a4=0u,field4ec=7u,owner_dirty=0u;
  unsigned mapping_invocations=0u,maintenance=0u,flag_clears=0u;
  unsigned field_writes=0u,source_reads=0u,serial_opens=0u,
           slot_reads=0u,commands=0u;
  std::uint64_t targets[3]{0x101u,0x202u,0x303u};
  std::uint32_t handles[3]{0x1001u,0x2002u,0x3003u};
  float now=10.0f,deadline=9.0f;
  bool gate_allowed=false;
  m::Reference translator;
  t::ClearFlag flag_clear;
  t::StateZero state_zero;
  h::Cooldown cooldown;
  h::SerialLookup serial;
  h::Field4ECSetter field_setter;
  Integrated():translator(*this),flag_clear(*this),state_zero(*this),
     cooldown(*this),serial(*this),field_setter(*this) {
    // Invented synthetic owner table, NOT the uninspected retail mapping.
    nodes.emplace(kSyntheticMapperHead,
      m::RangeNode{1000000003,3,5,kSyntheticMapperSecond});
    nodes.emplace(kSyntheticMapperSecond,
      m::RangeNode{1000000033,33,33,0u});
  }
  // Phase635 native mapping table node reader is TableHost::read_node.
  // Phase634 original source obtains virtual+0x760 object and passes its
  // embedded node at +0x34 to original 0x22073700.
  bool original_virtual_760_context(std::uint64_t& context)override {
    calls.emplace_back("virtual760");
    context=kSyntheticMapperOwner;return true;
  }
  bool original_id_map_22073700(
      std::uint64_t raw_context,std::int32_t input,
      std::int32_t& mapped)override {
    calls.emplace_back("map73700");
    ++mapping_invocations;
    assert(raw_context==kSyntheticMapperOwner);
    const auto r=translator.translate(
        raw_context+m::kVirtualContextMemberOffset,input);
    mapped=r.output;
    return r.code==m::Code::Mapped||r.code==m::Code::NotFound;
  }
  bool clear_original_elite_flag_bit(
      std::uint32_t word,std::uint32_t mask)override {
    calls.emplace_back("clear-bit");
    if(word>=flags.size())return false;
    flags[word]&=~mask;++flag_clears;return true;
  }
  bool read_original_elite_flag_bit(
      std::uint32_t word,std::uint32_t mask,bool& set)override {
    calls.emplace_back("test-bit");
    if(word>=flags.size())return false;
    set=(flags[word]&mask)!=0u;return true;
  }
  bool original_maintenance_2204E420()override {
    calls.emplace_back("maintenance");++maintenance;return true;
  }
  bool write_original_elite_state_7a4(std::uint32_t n)override {
    calls.emplace_back("state4");
    if(n!=4u)return false;
    state7a4=n;return true;
  }
  bool read_original_engine_time(float& out)override {
    calls.emplace_back("engine-time");out=now;return true;
  }
  bool read_original_elite_deadline_9dc(float& out)override {
    calls.emplace_back("elite-deadline");out=deadline;return true;
  }
  bool read_original_elite_serial_78c(std::uint32_t& out)override {
    calls.emplace_back("serial78c");
    if(source_reads>=3u)return false;
    out=handles[source_reads++];return true;
  }
  bool open_original_serial_table()override {
    calls.emplace_back("serial-table");++serial_opens;return true;
  }
  int read_original_serial_slot(
      std::uint32_t index,std::uint32_t generation,
      std::uint64_t& out)override {
    calls.emplace_back("serial-slot");
    if(slot_reads>=3u)return -1;
    const unsigned i=slot_reads++;
    assert(index==(handles[i]&0xfffu));
    assert(generation==(handles[i]>>12u));
    out=targets[i];return 1;
  }
  bool read_native_dword_4ec(std::uint32_t& out)override {
    calls.emplace_back("read4ec");out=field4ec;return true;
  }
  bool read_native_flag_1f0(std::uint8_t& out)override {
    calls.emplace_back("flag1f0");out=0u;return true;
  }
  bool write_native_dirty_byte_200(std::uint8_t)override{
    calls.emplace_back("dirty200");return true;
  }
  bool read_native_dirty_owner_16c(std::uint64_t& out)override {
    calls.emplace_back("owner16c");out=0x777u;return true;
  }
  bool or_native_dirty_owner_bit0(std::uint64_t owner)override {
    calls.emplace_back("mark-dirty");
    if(owner!=0x777u)return false;
    owner_dirty|=1u;return true;
  }
  bool write_native_dword_4ec(std::uint32_t n)override {
    calls.emplace_back("write4ec");
    field4ec=n;++field_writes;return true;
  }
  bool read_elite_predicate_fd9(std::uint8_t& out)override {
    calls.emplace_back("fd9");out=0u;return true;
  }
  bool read_elite_serial_handle_fdc(std::uint32_t& out)override {
    calls.emplace_back("fdc");out=0x4001u;return true;
  }
  int resolve_original_serial(
      std::uint32_t handle,std::uint64_t& out)override {
    calls.emplace_back("serial-fdc");
    if(handle!=0x4001u)return -1;
    out=0x505u;return 1;
  }
  bool original_gate43(std::uint64_t target,bool& allowed)override {
    calls.emplace_back("gate43");
    if(target!=0x505u)return false;
    allowed=gate_allowed;return true;
  }
  bool inherited_event_2235CCE0(std::uint32_t)override{
    calls.emplace_back("inherited");return true;
  }
  bool predicate_220326D0(bool& out)override {
    const auto r=cooldown.check();
    out=r.code==h::Code::Allowed;
    return r.code==h::Code::Allowed||r.code==h::Code::Blocked;
  }
  bool fetch_222AA640(std::uint64_t& out)override {
    const auto r=serial.resolve();
    out=r.target;
    return r.code==h::Code::TargetFound||r.code==h::Code::TargetAbsent;
  }
  bool object_virtual_404(
      std::uint64_t object,std::uint32_t& out)override {
    calls.emplace_back("virtual404");
    if(object!=targets[1])return false;
    out=0x123456u;return true;
  }
  bool component_4EC_call_2203F8E0(
      std::uint64_t object,std::uint32_t value)override {
    if(object!=targets[2])return false;
    const auto r=field_setter.set(value);
    return r.code==h::Code::Applied||r.code==h::Code::Unchanged;
  }
  bool call_220398A0(unsigned n)override {
    if(n!=commands+3u)return false;
    ++commands;
    const auto r=flag_clear.run(static_cast<std::int32_t>(n));
    return r.code==t::Code::Cleared||r.code==t::Code::Skipped;
  }
  bool action_zero_2203D2D0()override {
    const auto r=state_zero.run(0u);
    return r.code==t::Code::State4Written||
           r.code==t::Code::StatePreserved;
  }
};
void test_full_original_elite_event47_with_real_mapping_algorithm() {
  Integrated host;
  e::Reference secondary(host);
  const auto result=secondary.dispatch(e::kEvent47);
  assert(result.code==e::Code::ActionZero);
  assert(result.object_lookups==3u);
  assert(result.numbered_callbacks==3u);
  assert(host.source_reads==3u&&host.slot_reads==3u&&
         host.serial_opens==3u);
  assert(host.field4ec==0x123456u&&host.field_writes==1u);
  assert(host.owner_dirty==1u);
  assert(host.mapping_invocations==4u);
  assert(host.reads.size()==5u); // 3 times first node, 0x21 walks 2
  assert(host.reads[0]==kSyntheticMapperHead &&
         host.reads[4]==kSyntheticMapperSecond);
  assert(host.flag_clears==3u&&host.commands==3u);
  assert(host.flags[0]==(
      0xffffffffu&~(1u<<3)&~(1u<<4)&~(1u<<5)));
  assert(host.flags[1]==0u);
  assert(host.maintenance==1u&&host.state7a4==4u);
  // The two original methods are separately recovered, and Event43
  // reaches the SAME mapped 0x21 test via the original source gate.
  Integrated other;
  other.gate_allowed=true;other.flags[1]=(1u<<1);
  e::Reference secondary43(other);
  const auto blocked=secondary43.dispatch(e::kEvent43);
  assert(blocked.code==e::Code::ActionZero);
  assert(other.mapping_invocations==1u);
  assert(other.reads.size()==2u);
  assert(other.maintenance==1u&&other.state7a4==0u);
  other.gate_allowed=false;
  other.maintenance=0u;
  const auto denied=secondary43.dispatch(e::kEvent43);
  assert(denied.code==e::Code::Skipped&&other.maintenance==0u);
}
void test_no_invented_retail_mapping_data() {
  Integrated host;
  host.nodes.clear();
  e::Reference ref(host);
  const auto r=ref.dispatch(e::kEvent47);
  assert(r.code==e::Code::HostRejected);
  assert(r.stage==e::Stage::NumberedCallback3);
  assert(host.mapping_invocations==1u);
  assert(host.flag_clears==0u);
  assert(host.field_writes==1u); // original side effect already published
  assert(host.maintenance==0u);
}
}
int main(){
  test_exact_inclusive_range_and_first_match();
  test_skip_sentinel_and_input_minus_one();
  test_signed_comparisons_and_32bit_wraparound();
  test_cycles_bounded_chains_and_host_failures();
  test_full_original_elite_event47_with_real_mapping_algorithm();
  test_no_invented_retail_mapping_data();
  assert(m::kOriginalVA==0x22073700u);
  assert(m::kCompleteOriginalByteCount==58u);
  assert(m::kOriginalEliteVtableBaseVA+
         m::kOriginalEliteVirtual760Offset==0x2248e664u);
  assert(m::kOriginalVirtual760ThunkVA==0x22355d40u);
  assert(m::kOriginalVirtual760ThunkBytes==6u);
  assert(m::kOriginalMappingOwnerVA==0x2259c2f8u);
  assert(m::kOriginalMappingOwnerVA+
         m::kVirtualContextMemberOffset==m::kOriginalMappingFirstNodeVA);
  assert(m::kOriginalMappingFirstNodeVA==0x2259c32cu);
  std::puts("PASS Phase635 original 58-byte range converter, sentinel, signed bounds, cycles, and full 632-635 Elite event chain");
}
