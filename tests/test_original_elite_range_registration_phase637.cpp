#include "sin_compat/original_elite_range_registration_phase637.hpp"
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
#include <utility>
#include <vector>
namespace r=sin_compat::original_elite_range_registration_phase637;
namespace i=sin_compat::original_elite_mapping_init_phase636;
namespace m=sin_compat::original_elite_id_mapping_phase635;
namespace {
constexpr std::uint32_t kOwner=0x10000u;
constexpr std::uint32_t kNode=kOwner+0x1cu; // exact native TASK wrapper
constexpr std::uint32_t kRecord=0x21000u;
constexpr std::uint32_t kResource=0x7a000u;
constexpr std::uint32_t kLocalBase=1000000003u;
class Host final:public r::IHost,public m::IHost {
public:
  std::map<std::uint32_t,std::uint32_t> memory;
  std::vector<std::string> trace;
  std::vector<std::pair<std::uint32_t,std::int32_t>> forwarded;
  unsigned maps=0u;
  unsigned diagnostics=0u;
  r::Diagnostic last_diagnostic=r::Diagnostic::UninitializedTable;
  std::uint32_t last_category=0u,last_name=0u;
  std::uint32_t bad_read=0u,bad_write=0u;
  bool bad_map=false,bad_forward=false,bad_log=false;
  bool throws=false,forward_changes_count=false;
  r::RegisterRange* reenter=nullptr;
  r::Code nested=r::Code::Registered;
  m::Reference map_model;
  Host():map_model(*this){}
  bool read_u32(std::uint32_t addr,std::uint32_t& out)override {
    trace.emplace_back("R"+std::to_string(addr));
    if(throws)throw std::runtime_error("read fault");
    if(addr==bad_read)return false;
    const auto it=memory.find(addr);
    if(it==memory.end())return false;
    out=it->second;return true;
  }
  bool write_u32(std::uint32_t addr,std::uint32_t value)override {
    trace.emplace_back("W"+std::to_string(addr));
    if(throws)throw std::runtime_error("write fault");
    if(addr==bad_write)return false;
    memory[addr]=value;return true;
  }
  bool read_original_mapping_node(
      std::uint64_t addr,m::RangeNode& out)override {
    if(addr>0xffffffffULL)return false;
    const auto base=static_cast<std::uint32_t>(addr);
    std::uint32_t word[4]{};
    const std::uint32_t offsets[4]{0u,4u,8u,0x10u};
    for(unsigned j=0;j<4u;++j)
      if(base>0xffffffffu-offsets[j] ||
         !read_u32(base+offsets[j],word[j]))return false;
    out.mapped_start=static_cast<std::int32_t>(word[0]);
    out.source_start=static_cast<std::int32_t>(word[1]);
    out.source_end=static_cast<std::int32_t>(word[2]);
    out.next=word[3];return true;
  }
  bool convert_original_id_22073700(
      std::uint32_t node,std::int32_t source_id,
      std::int32_t& mapped)override {
    trace.emplace_back("MAP");
    ++maps;
    if(bad_map)return false;
    const auto result=map_model.translate(node,source_id);
    mapped=result.output;
    return result.code==m::Code::Mapped||
           result.code==m::Code::NotFound;
  }
  bool forward_original_symbol_22282370(
      std::uint32_t context,std::uint32_t source_symbol,
      std::int32_t mapped)override {
    trace.emplace_back("FORWARD");
    if(reenter)nested=reenter->add(
      kNode,0x404u,8,0x303u,0x505u).code;
    if(bad_forward)return false;
    assert(context==kResource);
    forwarded.emplace_back(source_symbol,mapped);
    if(forward_changes_count)memory[kRecord+4u]=0x7fffffff;
    return true;
  }
  bool report_original_diagnostic(
      r::Diagnostic kind,std::uint32_t category,
      std::uint32_t name)override {
    trace.emplace_back("ERROR");
    ++diagnostics;
    last_diagnostic=kind;
    last_category=category;last_name=name;
    return !bad_log;
  }
  std::uint32_t get(std::uint32_t addr)const {
    const auto it=memory.find(addr);
    assert(it!=memory.end());return it->second;
  }
};
void configure(Host& h,std::uint32_t node=kNode,
               std::uint32_t record=kRecord) {
  i::NodeInitializer init(h);
  assert(init.initialize(node,0u).code==i::Code::Completed);
  h.memory[record]=kResource;
  h.memory[record+4u]=kLocalBase;
  i::NodeLinker linker(h);
  assert(linker.link(node,record,0u).code==i::Code::Completed);
  assert(h.get(node)==kLocalBase);
  assert(h.get(node+4u)==9999u);
  assert(h.get(node+8u)==0xffffffffu);
  assert(h.get(node+12u)==0xffffffffu);
  h.trace.clear();
}
void test_complete_constructor_link_registration_converter() {
  Host h;configure(h);
  r::TaskWrapper wrapper(h);
  const auto first=wrapper.add_task(kOwner,0x1234u,3,0x9999u);
  assert(first.code==r::Code::Registered&&first.node_va==kNode);
  assert(first.low_initialized&&first.range_extended);
  assert(first.mapping_called&&first.mapping_found&&first.forwarded);
  assert(first.writes_committed==4u);
  assert(first.resource_count_written);
  assert(first.mapped_id==static_cast<std::int32_t>(kLocalBase));
  assert(h.get(kNode+4u)==3u&&h.get(kNode+8u)==3u);
  assert(h.get(kNode+12u)==kLocalBase);
  assert(h.get(kRecord+4u)==kLocalBase+1u);
  assert(h.forwarded.size()==1u);
  assert(h.forwarded[0]==std::make_pair(
      0x1234u,static_cast<std::int32_t>(kLocalBase)));
  assert(h.diagnostics==0u&&h.maps==1u);
  // Later ID extends source upper bound and mapped upper field. The
  // original resource record keeps the growing last-mapped+1 count.
  const auto second=wrapper.add_task(kOwner,0x5678u,4,0x9999u);
  assert(second.code==r::Code::Registered);
  assert(second.range_extended&&!second.low_initialized);
  assert(second.mapping_found&&second.forwarded);
  assert(h.get(kNode+8u)==4u&&h.get(kNode+12u)==kLocalBase+1u);
  assert(h.get(kRecord+4u)==kLocalBase+2u);
  assert(h.forwarded.back()==std::make_pair(
      0x5678u,static_cast<std::int32_t>(kLocalBase+1u)));
  const auto existing=wrapper.add_task(kOwner,0x6789u,3,0x9999u);
  assert(existing.code==r::Code::Registered);
  assert(!existing.range_extended&&!existing.low_initialized);
  assert(!existing.resource_count_written);
  assert(h.get(kRecord+4u)==kLocalBase+2u);
  assert(h.forwarded.size()==3u);
  // Record +4 is read after the forwarding callback, not from an
  // earlier snapshot. A callback-adjusted limit must win.
  h.forward_changes_count=true;
  const auto reentrant_count=wrapper.add_task(
      kOwner,0x9998u,4,0x9999u);
  assert(reentrant_count.code==r::Code::Registered);
  assert(reentrant_count.forwarded);
  assert(!reentrant_count.resource_count_written);
  assert(h.get(kRecord+4u)==0x7fffffffu);
}
void test_missing_resource_original_diagnostic_no_write() {
  Host h;configure(h);
  h.memory[kNode+0x14u]=0u;h.trace.clear();
  r::RegisterRange reference(h);
  const auto fail=reference.add(kNode,0x123u,3,0x111u,0x222u);
  assert(fail.code==r::Code::OriginalUninitializedTable);
  assert(fail.stage==r::Stage::ReportUninitialized);
  assert(fail.diagnostic_emitted&&fail.writes_committed==0u);
  assert(h.get(kNode+4u)==9999u);
  assert(h.last_diagnostic==r::Diagnostic::UninitializedTable);
  // Native source logs only display-name argument in this branch,
  // not category; no guessed string argument is forwarded.
  assert(h.last_category==0u&&h.last_name==0x222u);
  assert(h.forwarded.empty()&&h.maps==0u);
}
void test_ordering_error_preserves_first_low_write() {
  Host h;configure(h);
  constexpr std::uint32_t successor=0x22000u;
  h.memory[kNode+0x10u]=successor;
  h.memory[successor+4u]=5u;
  r::RegisterRange reference(h);
  const auto fail=reference.add(kNode,0x444u,5,0xabcdefu,0x456u);
  assert(fail.code==r::Code::OriginalBadOrdering);
  assert(fail.stage==r::Stage::ReportOrdering);
  assert(fail.low_initialized&&fail.writes_committed==1u);
  assert(h.get(kNode+4u)==5u); // no rollback of original partial write
  assert(h.get(kNode+8u)==0xffffffffu);
  assert(h.last_category==0xabcdefu&&h.last_name==0x456u);
  assert(h.maps==0u);
  // Next node sentinel skips the ordering comparison entirely.
  Host ignore;configure(ignore);
  ignore.memory[kNode+0x10u]=successor;
  ignore.memory[successor+4u]=9999u;
  r::RegisterRange succeeds(ignore);
  const auto ok=succeeds.add(kNode,0x333u,5,0xabcdefu,0x456u);
  assert(ok.code==r::Code::Registered);
  assert(ignore.get(kNode+8u)==5u&&ignore.diagnostics==0u);
}
void test_below_start_and_unmapped_returns_success() {
  Host h;configure(h);
  h.memory[kNode+4u]=9u;
  r::RegisterRange reference(h);
  const auto low=reference.add(kNode,0x111u,8,0x121u,0x131u);
  assert(low.code==r::Code::OriginalBelowRangeStart);
  assert(low.stage==r::Stage::ReportBelowRange);
  assert(low.writes_committed==0u&&h.maps==0u);
  assert(h.last_diagnostic==r::Diagnostic::BelowRangeStart);
  // Unknown source ID -1 is a no-match in Phase635 and native code
  // returns success without forwarding or resource count changes.
  h.memory[kNode+4u]=9999u;
  const auto none=reference.add(kNode,0x111u,-1,0x121u,0x131u);
  assert(none.code==r::Code::Registered);
  assert(none.low_initialized&&none.range_extended);
  assert(none.mapping_called&&!none.mapping_found);
  assert(!none.forwarded&&!none.resource_count_written);
  assert(h.maps==1u);
}
void test_range_extension_publishes_source_order_and_wrap() {
  Host h;configure(h);
  h.memory[kNode+4u]=10u;
  h.memory[kNode+8u]=11u;
  h.memory[kNode+12u]=kLocalBase+1u;
  r::RegisterRange reference(h);
  const auto result=reference.add(kNode,0x101u,13,0x201u,0x301u);
  assert(result.code==r::Code::Registered);
  assert(result.range_extended&&!result.low_initialized);
  assert(h.get(kNode+8u)==13u);
  assert(h.get(kNode+12u)==kLocalBase+3u);
  assert(result.mapped_id==static_cast<std::int32_t>(kLocalBase+3u));
  // New high must be visible to original mapper, so it can map 13.
  assert(result.mapping_found&&result.forwarded);
  Host wrap;configure(wrap);
  wrap.memory[kNode]=0x7ffffffeu;
  wrap.memory[kNode+4u]=1u;
  wrap.memory[kNode+8u]=2u;
  wrap.memory[kNode+12u]=0x7fffffffu;
  r::RegisterRange wrap_ref(wrap);
  const auto wrapped=wrap_ref.add(kNode,0x1u,3,0x2u,0x3u);
  assert(wrapped.code==r::Code::Registered);
  assert(wrap.get(kNode+12u)==0x80000000u);
  assert(wrapped.mapped_id==static_cast<std::int32_t>(0x80000000u));
  assert(wrap.get(kRecord+4u)==kLocalBase); // signed limit comparison
}
void test_source_order_partial_faults_and_callbacks() {
  Host h;configure(h);
  r::RegisterRange reference(h);
  h.bad_write=kNode+4u;
  const auto first=reference.add(kNode,1u,3,2u,3u);
  assert(first.code==r::Code::HostRejected);
  assert(first.stage==r::Stage::WriteInitialLow4);
  assert(first.writes_committed==0u);
  h.bad_write=0u;h.bad_read=kRecord;
  const auto later=reference.add(kNode,1u,3,2u,3u);
  assert(later.code==r::Code::HostRejected);
  assert(later.stage==r::Stage::ReadResourceContext0);
  assert(later.low_initialized&&later.range_extended);
  assert(later.writes_committed==3u);
  assert(h.get(kNode+4u)==3u&&h.get(kNode+8u)==3u);
  assert(h.forwarded.empty());
  h.bad_read=0u;h.bad_forward=true;
  const auto cannot=reference.add(kNode,1u,3,2u,3u);
  assert(cannot.code==r::Code::HostRejected);
  assert(cannot.stage==r::Stage::ForwardOriginalSymbol);
  assert(cannot.mapping_found&&!cannot.forwarded);
  h.bad_forward=false;h.bad_write=kRecord+4u;
  const auto partial=reference.add(kNode,1u,3,2u,3u);
  assert(partial.code==r::Code::HostRejected);
  assert(partial.stage==r::Stage::WriteResourceMax4);
  assert(partial.forwarded);
  assert(h.forwarded.size()==1u); // irreversible native callback
  h.bad_write=0u;h.reenter=&reference;
  const auto nested=reference.add(kNode,1u,3,2u,3u);
  assert(nested.code==r::Code::Registered);
  assert(h.nested==r::Code::Reentrant);
  h.reenter=nullptr;h.throws=true;
  const auto exception=reference.add(kNode,1u,3,2u,3u);
  assert(exception.code==r::Code::HostRejected);
  assert(exception.stage==r::Stage::ReadSourceRecord14);
}
void test_original_elite_schedule_direct_callsite_owner_plus_four() {
  Host h;
  constexpr std::uint32_t schedule_node=kOwner+r::kOriginalScheduleNodeOffset;
  constexpr std::uint32_t schedule_record=kRecord+0x100u;
  configure(h,schedule_node,schedule_record);
  r::RegisterRange source_registrar(h);
  // Original 0x22355B2D..3A passes category "schedule" and
  // ECX=0x2259C2FC (owner+4); this is not the TASK wrapper+0x1C
  // and not the Elite animation/flag node+0x34.
  const auto original_schedule=source_registrar.add(
      schedule_node,0x2020u,3,r::kOriginalScheduleLabelVA,0x4040u);
  assert(original_schedule.code==r::Code::Registered);
  assert(original_schedule.node_va==schedule_node);
  assert(original_schedule.mapping_found&&original_schedule.forwarded);
  assert(h.get(schedule_node+4u)==3u);
  assert(h.get(schedule_node+8u)==3u);
  assert(h.get(schedule_record+4u)==kLocalBase+1u);
  assert(h.forwarded.size()==1u);
  assert(h.forwarded[0]==std::make_pair(
      0x2020u,static_cast<std::int32_t>(kLocalBase)));
  assert(schedule_node!=kNode);
  assert(schedule_node!=kOwner+r::kOriginalAnimationNodeOffset);
  assert(r::kOriginalEliteScheduleCallVA==0x22355b3au);
}
void test_null_target_and_wrapper_node_checks() {
  Host h;configure(h);
  h.memory[kRecord]=0u;
  r::RegisterRange reference(h);
  const auto hazard=reference.add(kNode,1u,3,2u,3u);
  assert(hazard.code==r::Code::OriginalNullTargetHazard);
  assert(hazard.stage==r::Stage::ReadResourceContext0);
  assert(hazard.mapping_found&&!hazard.forwarded);
  r::TaskWrapper wrapper(h);
  assert(wrapper.add_task(0u,1u,3,2u).code==
         r::Code::OriginalNullNodeHazard);
  assert(wrapper.add_task(0xfffffff0u,1u,3,2u).code==
         r::Code::OriginalAddressOverflow);
}
} // namespace
int main() {
  test_complete_constructor_link_registration_converter();
  test_missing_resource_original_diagnostic_no_write();
  test_ordering_error_preserves_first_low_write();
  test_below_start_and_unmapped_returns_success();
  test_range_extension_publishes_source_order_and_wrap();
  test_source_order_partial_faults_and_callbacks();
  test_original_elite_schedule_direct_callsite_owner_plus_four();
  test_null_target_and_wrapper_node_checks();
  assert(r::kOriginalRegisterVA==0x22073760u);
  assert(r::kOriginalRegisterBytes==217u);
  assert(r::kOriginalTaskWrapperVA==0x22088fa0u);
  assert(r::kOriginalTaskNodeOffset==0x1cu);
  assert(r::kOriginalAnimationNodeOffset==0x34u);
  std::puts("PASS Phase637: original 217-byte LOCALID range registrar, task wrapper, Phase635 converter, Phase636 initializer, native-order partial writes");
}
