#include "sin_compat/original_nested_comparators_phase648.hpp"
#include <limits>
namespace sin_compat::original_nested_comparators_phase648 {
namespace {
struct Guard {bool& busy;explicit Guard(bool& b)noexcept:busy(b){busy=true;}
  ~Guard()noexcept{busy=false;}};
constexpr bool address(std::uint32_t a,std::uint32_t b,std::uint32_t& out)noexcept {
 if(a==0u||a>std::numeric_limits<std::uint32_t>::max()-b)return false;
 out=a+b;return true;
}
}
bool Comparators::resolve(std::uint32_t rec,std::uint32_t map,bool first,
                          Result& r,std::uint32_t& out)noexcept {
 std::uint16_t block=0,offset=0;
 r.stage=first?Stage::ReadLeftId0:Stage::ReadRightId0;
 if(!host_.read_u16(rec,block))return false;
 std::uint32_t offset_va=0;
 if(!address(rec,2u,offset_va)){r.code=Code::AddressOverflow;return false;}
 r.stage=first?Stage::ReadLeftOffset2:Stage::ReadRightOffset2;
 if(!host_.read_u16(offset_va,offset))return false;
 if(block==kSentinelWord&&offset==kSentinelWord){
  std::uint32_t fallback_va=0;
  if(!address(map,kFallbackStringOffset,fallback_va)){
   r.code=Code::AddressOverflow;return false;}
  r.stage=first?Stage::ReadLeftFallback30:Stage::ReadRightFallback30;
  if(!host_.read_u32(fallback_va,out))return false;
  if(first)r.left_uses_fallback=true;else r.right_uses_fallback=true;
 }else{
  std::uint32_t table_field=0,blocks=0,block_va=0,base=0;
  if(!address(map,kStringBlocksOffset,table_field)){
   r.code=Code::AddressOverflow;return false;}
  r.stage=first?Stage::ReadLeftBlocks1C:Stage::ReadRightBlocks1C;
  if(!host_.read_u32(table_field,blocks))return false;
  if(blocks==0u){r.code=Code::NullPointerHazard;return false;}
  if(!address(blocks,static_cast<std::uint32_t>(block)*4u,block_va)){
   r.code=Code::AddressOverflow;return false;}
  r.stage=first?Stage::ReadLeftBlockPointer:Stage::ReadRightBlockPointer;
  if(!host_.read_u32(block_va,base))return false;
  if(!address(base,static_cast<std::uint32_t>(offset)+kBlockStringDataOffset,out)){
   r.code=Code::AddressOverflow;return false;}
 }
 if(out==0u){r.code=Code::NullPointerHazard;return false;}
 return true;
}
Result Comparators::bytewise(std::uint32_t left,std::uint32_t right,
                             std::uint32_t map)noexcept {
 if(busy_)return {Code::Reentrant,Stage::None};
 Guard guard(busy_);
 Result r{};r.left_record_va=left;r.right_record_va=right;r.map_va=map;
 if(left==0u||right==0u||map==0u){r.code=Code::NullPointerHazard;return r;}
 try {
  if(!resolve(left,map,true,r,r.left_string_va))return r;
  if(!resolve(right,map,false,r,r.right_string_va))return r;
  // Original x86 loops through up to two bytes per iteration, comparing
  // unsigned BYTEs, returning 1 exactly when the first differs low.
  // It checks the terminator before accessing the next byte in a pair.
  for(std::uint32_t i=0;i<65536u;++i){
   for(std::uint32_t j=0;j<2u;++j){
    const std::uint32_t offset=i*2u+j;
    std::uint32_t a=0,b=0;
    if(!address(r.left_string_va,offset,a)||
       !address(r.right_string_va,offset,b)){
      r.code=Code::AddressOverflow;return r;
    }
    std::uint8_t x=0,y=0;
    r.stage=j==0u?Stage::ReadLeftByte0:Stage::ReadLeftByte1;
    if(!host_.read_u8(a,x))return r;
    r.stage=j==0u?Stage::ReadRightByte0:Stage::ReadRightByte1;
    if(!host_.read_u8(b,y))return r;
    ++r.bytes_compared;
    if(x!=y){r.eax=x<y?1u:0u;r.code=Code::Completed;return r;}
    if(x==0u){r.code=Code::Completed;return r;}
   }
  }
  r.code=Code::CycleGuard;
 }catch(...){r.code=Code::HostRejected;}
 return r;
}
Result Comparators::delegated(std::uint32_t left,std::uint32_t right,
                              std::uint32_t map)noexcept {
 if(busy_)return {Code::Reentrant,Stage::None};
 Guard guard(busy_);
 Result r{};r.left_record_va=left;r.right_record_va=right;r.map_va=map;
 if(left==0u||right==0u||map==0u){r.code=Code::NullPointerHazard;return r;}
 try {
  if(!resolve(left,map,true,r,r.left_string_va))return r;
  if(!resolve(right,map,false,r,r.right_string_va))return r;
  r.stage=Stage::DelegateOriginal223a5574;
  r.delegated=true;
  if(!host_.compare_original_223a5574(r.left_string_va,r.right_string_va,
                                      r.delegated_result))return r;
  // The retail function executes TEST EAX,EAX; SETL CL; MOV AL,CL.
  r.eax=r.delegated_result<0?1u:0u;
  r.code=Code::Completed;
 }catch(...){r.code=Code::HostRejected;}
 return r;
}
} // namespace sin_compat::original_nested_comparators_phase648
