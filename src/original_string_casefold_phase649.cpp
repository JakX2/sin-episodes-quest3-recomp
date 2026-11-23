#include "sin_compat/original_string_casefold_phase649.hpp"
#include <limits>
namespace sin_compat::original_string_casefold_phase649 {
namespace{
struct Guard{bool& busy;explicit Guard(bool& b)noexcept:busy(b){b=true;}
 ~Guard()noexcept{busy=false;}};
constexpr bool add(std::uint32_t a,std::uint32_t b,std::uint32_t& out)noexcept{
 if(a==0u||a>std::numeric_limits<std::uint32_t>::max()-b)return false;
 out=a+b;return true;
}
constexpr std::uint8_t lower_ascii(std::uint8_t c)noexcept{
 return (c>=65u&&c<=90u)?static_cast<std::uint8_t>(c+32u):c;
}
constexpr std::int32_t signed_from_bits(std::uint32_t raw)noexcept{
 if(raw<=0x7fffffffu)return static_cast<std::int32_t>(raw);
 // Handle wrapping original SUB EBX,ECX without signed overflow.
 const auto magnitude=~raw;
 return -static_cast<std::int32_t>(magnitude)-1;
}
}
TransformResult Casefold::transform(std::uint32_t value)noexcept{
 if(transforming_)return {Code::Reentrant,Stage::None};
 Guard guard(transforming_);
 TransformResult r{};r.input=value;r.output=value;
 try{
  std::uint32_t locale=0;
  r.stage=Stage::ReadLocaleMode;
  if(!host_.read_u32(kModeGlobal,locale))return r;
  if(locale==0u){
   // Original 0x223A739B uses signed 32-bit comparisons against A and Z.
   if(value>=65u&&value<=90u)r.output=value+32u;
   r.fast_ascii=true;r.code=Code::Completed;return r;
  }
  std::uint32_t fast=0u;
  r.stage=Stage::ReadCTypeFast;
  if(!host_.read_u32(kCTypeFastGlobal,fast))return r;
  if(fast!=0u&&value<=0x7fu){
   if(value>=65u&&value<=90u)r.output=value+32u;
   r.fast_ascii=true;r.code=Code::Completed;return r;
  }
  std::uint32_t table=0u;
  if(value<0x100u){
   std::uint32_t mode=0u;
   r.stage=Stage::ReadCTypeMode;
   if(!host_.read_u32(kCTypeModeGlobal,mode))return r;
   std::uint32_t classified=0u;
   if(mode>1u&&mode<0x80000000u){
    r.stage=Stage::CallCTypeHelper;r.used_ctype_helper=true;
    if(!host_.classify_original_223a898a(value,1u,classified))return r;
   }else{
    r.stage=Stage::ReadCTypeTable;
    if(!host_.read_u32(kCTypeTableGlobal,table))return r;
    if(table==0u){r.code=Code::NullPointerHazard;return r;}
    std::uint32_t ctype_va=0u;
    if(!add(table,value*2u,ctype_va)){
     r.code=Code::AddressOverflow;return r;
    }
    std::uint8_t ctype=0;
    r.stage=Stage::ReadCTypeMask;
    if(!host_.read_u8(ctype_va,ctype))return r;
    classified=static_cast<std::uint32_t>(ctype&1u);
   }
   if(classified==0u){r.code=Code::Completed;return r;}
  }
  // Original uses the high byte's lead-bit at table[(value>>8)&0xff],
  // including the case where value >= 256 bypassed the 0x100 bound.
  if(table==0u){
   r.stage=Stage::ReadCTypeTable;
   if(!host_.read_u32(kCTypeTableGlobal,table))return r;
   if(table==0u){r.code=Code::NullPointerHazard;return r;}
  }
  const auto high_byte=static_cast<std::uint32_t>((value>>8u)&0xffu);
  std::uint32_t lead_va=0u;
  if(!add(table,high_byte*2u+1u,lead_va)){
   r.code=Code::AddressOverflow;return r;
  }
  std::uint8_t lead=0;
  r.stage=Stage::ReadHighLeadMask;
  if(!host_.read_u8(lead_va,lead))return r;
  MapRequest request{};
  request.mode=locale;
  if((lead&0x80u)!=0u){
   request.first=static_cast<std::uint8_t>((value>>8u)&0xffu);
   request.second=static_cast<std::uint8_t>(value&0xffu);
   request.input_count=2u;
  }else request.first=static_cast<std::uint8_t>(value&0xffu);
  r.stage=Stage::ReadCodepage;
  if(!host_.read_u32(kLocaleCodepageGlobal,request.codepage))return r;
  MapAnswer answer{};
  r.stage=Stage::CallLocaleMap;r.used_locale_map=true;
  if(!host_.locale_map_original_223ac9e7(request,answer))return r;
  if(answer.count==1u)r.output=answer.first;
  else if(answer.count!=0u)r.output=(static_cast<std::uint32_t>(answer.first)<<8u)|answer.second;
  r.code=Code::Completed;
 }catch(...){r.code=Code::HostRejected;}
 return r;
}
Result Casefold::compare_ascii(std::uint32_t left,std::uint32_t right)noexcept{
 Result r{};r.left_va=left;r.right_va=right;r.ascii_fast=true;
 try{
  for(std::uint32_t i=0;i<65536u;++i){
   std::uint32_t a=0,b=0;
   if(!add(left,i,a)||!add(right,i,b)){
    r.code=Code::AddressOverflow;return r;
   }
   // In retail 0x223A8410 right (ESI) is read before left (EDI).
   std::uint8_t bv=0,av=0;
   r.stage=Stage::ReadFirstRight;
   if(!host_.read_u8(b,bv))return r;
   r.stage=Stage::ReadFirstLeft;
   if(!host_.read_u8(a,av))return r;
   ++r.characters;
   if(av==bv){
    if(bv==0u){r.code=Code::Completed;return r;}
    continue;
   }
   const auto af=lower_ascii(av),bf=lower_ascii(bv);
   if(af==bf)continue;
   r.comparison=af<bf?-1:1;
   r.code=Code::Completed;return r;
  }
  r.code=Code::CycleGuard;
 }catch(...){r.code=Code::HostRejected;}
 return r;
}
Result Casefold::compare_locale(std::uint32_t left,std::uint32_t right)noexcept{
 Result r{};r.left_va=left;r.right_va=right;r.locale_mode=true;
 try{
  for(std::uint32_t i=0u;i<65536u;++i){
   std::uint32_t a=0u,b=0u;
   if(!add(left,i,a)||!add(right,i,b)){
    r.code=Code::AddressOverflow;return r;
   }
   std::uint8_t av=0,bv=0;
   r.stage=Stage::ReadFirstLeft;
   if(!host_.read_u8(a,av))return r;
   r.stage=Stage::TransformLeft;
   const auto at=transform(av);
   if(at.code!=Code::Completed){r.code=at.code;r.stage=at.stage;return r;}
   r.stage=Stage::ReadFirstRight;
   if(!host_.read_u8(b,bv))return r;
   r.stage=Stage::TransformRight;
   const auto bt=transform(bv);
   if(bt.code!=Code::Completed){r.code=bt.code;r.stage=bt.stage;return r;}
   r.transforms+=2u;
   r.locale_mappings+=static_cast<std::uint32_t>(at.used_locale_map)+
     static_cast<std::uint32_t>(bt.used_locale_map);
   ++r.characters;
   if(at.output==0u||at.output!=bt.output){
    r.comparison=signed_from_bits(at.output-bt.output);
    r.code=Code::Completed;return r;
   }
  }
  r.code=Code::CycleGuard;
 }catch(...){r.code=Code::HostRejected;}
 return r;
}
Result Casefold::compare(std::uint32_t left,std::uint32_t right)noexcept{
 if(comparing_)return {Code::Reentrant,Stage::None};
 Guard guard(comparing_);
 Result r{};r.left_va=left;r.right_va=right;
 if(left==0u||right==0u){r.code=Code::NullPointerHazard;return r;}
 try{
  std::uint32_t mode=0u;
  r.stage=Stage::ReadCompareMode;
  if(!host_.read_u32(kModeGlobal,mode))return r;
  return mode==0u?compare_ascii(left,right):compare_locale(left,right);
 }catch(...){r.code=Code::HostRejected;}
 return r;
}
} // namespace sin_compat::original_string_casefold_phase649
