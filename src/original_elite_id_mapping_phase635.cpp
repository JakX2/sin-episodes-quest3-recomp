#include "sin_compat/original_elite_id_mapping_phase635.hpp"
#include <array>
namespace sin_compat::original_elite_id_mapping_phase635 {
namespace {
struct Guard final {
  bool& busy;
  explicit Guard(bool& b)noexcept:busy(b){busy=true;}
  ~Guard()noexcept{busy=false;}
};
std::int32_t original_wrap_add_sub(
    std::int32_t mapped_start,std::int32_t source_start,
    std::int32_t input)noexcept {
  // Native x86 EAX arithmetic: MOV mapped_start, SUB source_start, ADD
  // EDX input. All operations wrap mod 2^32 without signed C++ UB.
  const std::int64_t wide=static_cast<std::int64_t>(mapped_start)-
      static_cast<std::int64_t>(source_start)+
      static_cast<std::int64_t>(input);
  const auto bits=static_cast<std::uint32_t>(wide);
  const std::int64_t signed_value=bits<=0x7fffffffu?
      static_cast<std::int64_t>(bits):
      static_cast<std::int64_t>(bits)-0x100000000LL;
  return static_cast<std::int32_t>(signed_value);
}
} // namespace
Result Reference::translate(
    std::uint64_t head_context_node,std::int32_t input_id)noexcept {
  if(busy_)return {Code::Reentrant,Stage::None};
  Guard guard(busy_);
  Result out{};
  // Original opcode 0x22073704: first compare EDX to -1; if equal,
  // jump directly to mov EAX,-1 / ret4, without reading any node.
  if(input_id==-1) {
    out.code=Code::NotFound;
    out.input_minus_one_shortcut=true;
    return out;
  }
  if(head_context_node==0u) {
    // Native [ECX+4] at the first node would fault.
    out.code=Code::OriginalNullHeadHazard;
    out.stage=Stage::Head;
    return out;
  }
  std::array<std::uint64_t,kMaxSafeOriginalNodes> visited{};
  std::uint64_t address=head_context_node;
  Stage stage=Stage::Head;
  try {
    while(address!=0u) {
      if(out.visited==kMaxSafeOriginalNodes) {
        out.code=Code::OriginalMappingLimit;
        out.stage=Stage::NextNode;
        out.last_node=address;
        return out;
      }
      for(unsigned i=0;i<out.visited;++i) {
        if(visited[i]==address) {
          out.code=Code::OriginalMappingCycle;
          out.stage=Stage::NextNode;
          out.last_node=address;
          return out;
        }
      }
      visited[out.visited++]=address;
      out.last_node=address;
      RangeNode node{};
      stage=Stage::ReadNode;
      if(!host_.read_original_mapping_node(address,node)) {
        out.code=Code::HostRejected;
        out.stage=stage;
        return out;
      }
      // In native code, node +4 = 0x270F is a *special skip marker*.
      // Otherwise compare original signed input against inclusive
      // source range [node+4, node+8]. First matching node wins.
      stage=Stage::CompareSourceStart;
      if(node.source_start!=static_cast<std::int32_t>(
          kOriginalSentinelStart) && input_id>=node.source_start) {
        stage=Stage::CompareSourceEnd;
        if(input_id<=node.source_end) {
          stage=Stage::ComputeDestination;
          out.output=original_wrap_add_sub(
              node.mapped_start,node.source_start,input_id);
          out.code=Code::Mapped;
          out.stage=Stage::None;
          out.matched_node=address;
          return out;
        }
      }
      stage=Stage::NextNode;
      address=node.next; // exact original +0x10 load on skip
    }
    // No node matched. Original x86 OR EAX,0xFFFFFFFF then RET 4.
    out.code=Code::NotFound;
    out.stage=Stage::None;
    out.output=-1;
    return out;
  } catch(...) {
    out.code=Code::HostRejected;
    out.stage=stage;
    return out;
  }
}
} // namespace sin_compat::original_elite_id_mapping_phase635
