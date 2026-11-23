#include "sin_compat/original_elite_mapping_init_phase636.hpp"
#include <limits>
namespace sin_compat::original_elite_mapping_init_phase636 {
namespace {
struct Guard final {
  bool& busy;
  explicit Guard(bool& b)noexcept:busy(b){busy=true;}
  ~Guard()noexcept{busy=false;}
};
bool at(std::uint32_t base,std::uint32_t offset,
        std::uint32_t& target)noexcept {
  if(base==0u||base>std::numeric_limits<std::uint32_t>::max()-offset)
    return false;
  target=base+offset;
  return true;
}
} // namespace
Result NodeInitializer::initialize(
    std::uint32_t node_va,std::uint8_t mode)noexcept {
  if(busy_)return {Code::Reentrant,Stage::None};
  Guard guard(busy_);
  Result r{};
  r.last_node=node_va;
  r.stage=Stage::ValidateNode;
  // Retail 0x22073610 consumes exactly the low BYTE argument. The
  // original NEG/SBB/NEG sequence gives:
  // mode 0: node[0]=-1, node[4]=9999;
  // nonzero: node[0]=0, node[4]=0.
  const bool nonzero=mode!=0u;
  const std::uint32_t values[6]{
    nonzero?0u:kUnspecifiedDword,
    nonzero?0u:kSentinelSourceStart,
    kUnspecifiedDword,kUnspecifiedDword,0u,0u
  };
  const Stage stages[6]{
    Stage::WriteInit0,Stage::WriteInit4,Stage::WriteInit8,
    Stage::WriteInitC,Stage::WriteInit10,Stage::WriteInit14
  };
  try {
    for(unsigned i=0;i<6u;++i) {
      std::uint32_t address=0u;
      r.stage=stages[i];
      if(!at(node_va,i*4u,address)) {
        r.code=(node_va==0u)?Code::OriginalNullPointerHazard:
                             Code::OriginalAddressOverflow;
        return r;
      }
      if(!host_.write_u32(address,values[i]))return r;
      ++r.writes_committed;
    }
    r.code=Code::Completed;r.stage=Stage::None;
    return r;
  } catch(...) {
    r.code=Code::HostRejected;
    return r;
  }
}
Result NodeLinker::link(
    std::uint32_t node_va,std::uint32_t source_va,
    std::uint32_t successor_va)noexcept {
  if(busy_)return {Code::Reentrant,Stage::None};
  Guard guard(busy_);
  Result r{};
  r.last_node=node_va;
  r.stage=Stage::ReadExistingC;
  std::uint32_t address=0u,marker=0u;
  try {
    if(!at(node_va,0xCu,address)) {
      r.code=node_va==0u?Code::OriginalNullPointerHazard:
                           Code::OriginalAddressOverflow;
      return r;
    }
    if(!host_.read_u32(address,marker))return r;
    if(marker!=kUnspecifiedDword) {
      // 2006 code conditionally rewrites this node's source-low:
      // successor != null => sentinel 9999; successor null => zero.
      // This occurs BEFORE linking and copying the source value.
      const Stage stages[3]{
        Stage::ConditionalWrite4,Stage::ConditionalWrite8,
        Stage::ConditionalWriteC
      };
      const std::uint32_t offsets[3]{4u,8u,12u};
      const std::uint32_t values[3]{
        successor_va!=0u?kSentinelSourceStart:0u,
        kUnspecifiedDword,kUnspecifiedDword
      };
      for(unsigned i=0;i<3u;++i) {
        r.stage=stages[i];
        if(!at(node_va,offsets[i],address)) {
          r.code=Code::OriginalAddressOverflow;return r;
        }
        if(!host_.write_u32(address,values[i]))return r;
        ++r.writes_committed;
      }
    }
    // The retail method ALWAYS writes +0x10 then +0x14 and only then
    // reads [source+4] before overwriting node[0]. If source is null
    // (or unreadable), prior link writes remain performed.
    r.stage=Stage::WriteNext10;
    if(!at(node_va,0x10u,address)) {
      r.code=Code::OriginalAddressOverflow;return r;
    }
    if(!host_.write_u32(address,successor_va))return r;
    ++r.writes_committed;
    r.stage=Stage::WriteSource14;
    if(!at(node_va,0x14u,address)) {
      r.code=Code::OriginalAddressOverflow;return r;
    }
    if(!host_.write_u32(address,source_va))return r;
    ++r.writes_committed;
    r.stage=Stage::ReadSourcePlus4;
    if(!at(source_va,4u,address)) {
      r.code=source_va==0u?Code::OriginalNullPointerHazard:
                             Code::OriginalAddressOverflow;
      return r;
    }
    std::uint32_t mapped_start=0u;
    if(!host_.read_u32(address,mapped_start))return r;
    r.stage=Stage::WriteDestination0;
    if(!at(node_va,0u,address)) {
      r.code=Code::OriginalAddressOverflow;return r;
    }
    if(!host_.write_u32(address,mapped_start))return r;
    ++r.writes_committed;
    r.code=Code::Completed;r.stage=Stage::None;
    return r;
  } catch(...) {
    r.code=Code::HostRejected;
    return r;
  }
}
Result OwnerRegistration::register_owner(
    std::uint32_t owner_va,std::uint32_t name_va,
    std::uint32_t source_va,
    std::uint32_t linked_owner_va)noexcept {
  if(busy_)return {Code::Reentrant,Stage::None};
  Guard guard(busy_);
  Result r{};
  r.stage=Stage::WriteOwnerName0;
  try {
    if(owner_va==0u) {
      r.code=Code::OriginalNullPointerHazard;return r;
    }
    if(!host_.write_u32(owner_va,name_va))return r;
    ++r.writes_committed;
    // The three original calls are at 0x22088F33, 0x22088F4F,
    // 0x22088F6B. Arguments are not interchangeable:
    // node offsets +4,+0x1C,+0x34;
    // source-record offsets +0,+8,+0x10;
    // linked owner offsets +4,+0x1C,+0x34 (or 0 if absent).
    const std::uint32_t node_offsets[3]{
      kFirstOwnerNode,kSecondOwnerNode,kThirdOwnerNode
    };
    const std::uint32_t source_offsets[3]{
      0u,kSourceRecordStride,2u*kSourceRecordStride
    };
    const Stage stages[3]{
      Stage::LinkFirst,Stage::LinkSecond,Stage::LinkThird
    };
    for(unsigned i=0;i<3u;++i) {
      r.stage=stages[i];
      std::uint32_t node=0u,source=0u,next=0u;
      // A missing source pointer is NOT rejected by the original
      // registration function before it invokes 0x22073650. Pass the
      // first null source through so the source helper can perform its
      // original preceding node writes, then refuse the unsafe load.
      if(!at(owner_va,node_offsets[i],node) ||
         (source_va!=0u &&
          !at(source_va,source_offsets[i],source)) ||
         (linked_owner_va!=0u &&
          !at(linked_owner_va,node_offsets[i],next))) {
        r.code=Code::OriginalAddressOverflow;
        return r;
      }
      if(source_va==0u && i==0u)source=0u;
      const Result linked=linker_.link(node,source,next);
      r.last_node=node;
      r.writes_committed+=linked.writes_committed;
      if(linked.code!=Code::Completed) {
        r.code=linked.code; // preserve failing native primitive
        return r;
      }
      ++r.links_completed;
    }
    r.code=Code::Completed;r.stage=Stage::None;
    return r;
  } catch(...) {
    r.code=Code::HostRejected;
    return r;
  }
}
} // namespace sin_compat::original_elite_mapping_init_phase636
