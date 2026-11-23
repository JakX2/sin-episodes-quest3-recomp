#include "sin_compat/original_symbol_rebalance_phase642.hpp"
#include <limits>
namespace sin_compat::original_symbol_rebalance_phase642 {
namespace {
struct Guard {
  bool& busy;
  explicit Guard(bool& flag)noexcept:busy(flag){busy=true;}
  ~Guard()noexcept{busy=false;}
};
constexpr bool add(std::uint32_t base,std::uint32_t offset,
                   std::uint32_t& result)noexcept {
  if(base==0u || base>std::numeric_limits<std::uint32_t>::max()-offset)
    return false;
  result=base+offset;return true;
}
}
Result Rebalance::insert(std::uint32_t table,std::uint32_t index32)noexcept {
  if(busy_)return {Code::Reentrant,Stage::None};
  Guard guard(busy_);
  Result out{};
  out.table_va=table;out.input_index32=index32;
  std::uint16_t x=static_cast<std::uint16_t>(index32);
  out.final_index=x;
  if(table==0u){out.code=Code::NullTableHazard;return out;}
  try {
    std::uint32_t root_field=0;
    if(!add(table,0x10u,root_field)){
      out.code=Code::AddressOverflow;return out;
    }
    auto read_root=[&](std::uint16_t& value,Stage stage)->bool {
      out.stage=stage;
      return host_.read_u16(root_field,value);
    };
    auto read_node=[&](std::uint16_t index,std::uint32_t offset,
                       std::uint16_t& value,Stage stage)->bool {
      out.stage=stage;
      const auto a=accessor_.resolve(table,index);
      out.sentinel_writes_committed+=a.writes_committed;
      if(a.code!=original_symbol_rotations_phase641::Code::Completed){
        if(a.code==original_symbol_rotations_phase641::Code::AddressOverflow)
          out.code=Code::AddressOverflow;
        else if(a.code==original_symbol_rotations_phase641::Code::NullArrayHazard)
          out.code=Code::NullArrayHazard;
        else out.code=Code::HostRejected;
        return false;
      }
      std::uint32_t at=0;
      if(!add(a.slot_va,offset,at)){
        out.code=Code::AddressOverflow;return false;
      }
      return host_.read_u16(at,value);
    };
    // Original color writes use a fresh *direct* table+4 slot address,
    // unlike its repeated inline sentinel-aware READ expressions.
    // A 0xFFFF index in a direct native color write is invalid guest memory
    // and is guarded here rather than writing unrelated memory.
    auto write_color=[&](std::uint16_t index,std::uint16_t value,
                         Stage stage)->bool {
      out.stage=stage;
      if(index==kNullIndex){out.code=Code::SentinelWriteHazard;return false;}
      std::uint32_t array_field=0,array=0,target=0;
      if(!add(table,4u,array_field)){
        out.code=Code::AddressOverflow;return false;
      }
      if(!host_.read_u32(array_field,array))return false;
      if(array==0u){out.code=Code::NullArrayHazard;return false;}
      if(!add(array,static_cast<std::uint32_t>(index)*16u+6u,target)){
        out.code=Code::AddressOverflow;return false;
      }
      if(!host_.write_u16(target,value))return false;
      ++out.direct_writes_committed;
      return true;
    };
    auto rotation=[&](std::uint16_t index,bool left)->bool {
      out.stage=left?Stage::RotateLeft:Stage::RotateRight;
      const auto res=left?rotate_.left(table,index):rotate_.right(table,index);
      out.rotation_writes_committed+=res.writes_committed;
      if(res.code!=original_symbol_rotations_phase641::Code::Completed){
        if(res.code==original_symbol_rotations_phase641::Code::AddressOverflow)
          out.code=Code::AddressOverflow;
        else if(res.code==original_symbol_rotations_phase641::Code::NullArrayHazard)
          out.code=Code::NullArrayHazard;
        else out.code=Code::HostRejected;
        return false;
      }
      if(left)++out.left_rotations;else ++out.right_rotations;
      return true;
    };
    std::uint16_t root=kNullIndex;
    if(!read_root(root,Stage::ReadRoot))return out;
    out.reached_root_initially=(x==root);
    // Original 0x22281848 short-circuits immediately if inserted index is
    // root, skipping all global sentinel-init checks and fixup branching.
    // Termination bound is a portable guard for corrupted synthetic trees.
    while(x!=root){
      if(out.loop_iterations++>=65536u){
        out.code=Code::NonTerminatingTree;return out;
      }
      std::uint16_t parent=kNullIndex,pc=kBlack;
      if(!read_node(x,4u,parent,Stage::ReadParent))return out;
      if(!read_node(parent,6u,pc,Stage::ReadParentColor))return out;
      if(pc!=kRed)break;
      std::uint16_t grand=kNullIndex,grandleft=kNullIndex;
      if(!read_node(parent,4u,grand,Stage::ReadGrandparent))return out;
      if(!read_node(grand,0u,grandleft,Stage::ReadGrandparentLeft))return out;
      const bool parent_is_left=(parent==grandleft);
      const std::uint32_t uncle_field=parent_is_left?2u:0u;
      std::uint16_t uncle=kNullIndex,uncle_color=kBlack;
      if(!read_node(grand,uncle_field,uncle,Stage::ReadUncle))return out;
      if(!read_node(uncle,6u,uncle_color,Stage::ReadUncleColor))return out;
      if(uncle_color==kRed){
        // Original red-uncle path, both mirrored variants:
        // parent black -> uncle black -> grandparent red -> move x up.
        if(!write_color(parent,kBlack,Stage::RecolorParentBlack))return out;
        if(!write_color(uncle,kBlack,Stage::RecolorUncleBlack))return out;
        if(!write_color(grand,kRed,Stage::RecolorGrandparentRed))return out;
        x=grand;out.final_index=x;++out.recolor_cases;
      }else if(parent_is_left){
        std::uint16_t right_child=kNullIndex;
        if(!read_node(parent,2u,right_child,
                     Stage::ReadParentChildForZigzag))return out;
        if(x==right_child){
          // Original 0x22281B40: x=old parent, rotate left(old parent).
          x=parent;out.final_index=x;
          if(!rotation(x,true))return out;
          if(!read_node(x,4u,parent,
                        Stage::RefreshParentAfterRotation))return out;
          if(!read_node(parent,4u,grand,
                        Stage::RefreshGrandparentAfterRotation))return out;
        }
        if(!write_color(parent,kBlack,Stage::RecolorParentBlack))return out;
        if(!write_color(grand,kRed,Stage::RecolorGrandparentRed))return out;
        if(!rotation(grand,false))return out;
      }else{
        std::uint16_t left_child=kNullIndex;
        if(!read_node(parent,0u,left_child,
                     Stage::ReadParentChildForMirrorZigzag))return out;
        if(x==left_child){
          // Original 0x22281D1C: x=old parent, rotate right(old parent).
          x=parent;out.final_index=x;
          if(!rotation(x,false))return out;
          if(!read_node(x,4u,parent,
                        Stage::RefreshParentAfterRotation))return out;
          if(!read_node(parent,4u,grand,
                        Stage::RefreshGrandparentAfterRotation))return out;
        }
        if(!write_color(parent,kBlack,Stage::RecolorParentBlack))return out;
        if(!write_color(grand,kRed,Stage::RecolorGrandparentRed))return out;
        if(!rotation(grand,true))return out;
      }
      // Native 0x22281DE0 tests its stored 16-bit index against fresh
      // root, then cycles to 0x22281860 until parent is no longer red.
      if(!read_root(root,Stage::ReadRoot))return out;
    }
    // Original 0x22281DF6 always blacks the *current* root by direct array
    // reference and preserves the native 16-bit color width.
    std::uint16_t final_root=kNullIndex;
    if(!read_root(final_root,Stage::FinalReadRoot))return out;
    if(!write_color(final_root,kBlack,Stage::FinalBlackenRoot))return out;
    out.root_blackened=true;
    out.final_index=x;
    out.code=Code::Completed;
  }catch(...){out.code=Code::HostRejected;}
  return out;
}
} // namespace sin_compat::original_symbol_rebalance_phase642
