#include "sin_compat/original_server_system_init_phase661.hpp"
#include <limits>

namespace sin_compat::original_server_system_init_phase661 {
namespace {
constexpr NameSpec kOriginalNames[5]={
 {"phonemes",kPhonemesLiteralVA},
 {"phonemes_weak",kPhonemesWeakLiteralVA},
 {"phonemes_strong",kPhonemesStrongLiteralVA},
 {"random",kRandomLiteralVA},
 {"randomAlert",kRandomAlertLiteralVA}
};
struct Guard {
 bool& flag;
 explicit Guard(bool& f)noexcept:flag(f){flag=true;}
 ~Guard()noexcept{flag=false;}
};
bool add32(std::uint32_t a,std::uint32_t b,std::uint32_t& out)noexcept{
 if(a>std::numeric_limits<std::uint32_t>::max()-b)return false;
 out=a+b;
 return true;
}
}
const NameSpec* original_name_specs(std::size_t& count)noexcept{
 count=5;
 return kOriginalNames;
}
Result Initializers::run(std::uint32_t function,std::uint32_t this_va)noexcept{
 Result result{};
 result.original_function_va=function;
 result.original_this_va=this_va;
 if(busy_){result.code=Code::Reentrant;return result;}
 if(function!=kSubobjectDelegationVA && function!=kNamedRegistrationVA &&
    function!=kDecalScriptVA && function!=kSpatialPartitionVA){
   result.code=Code::Unsupported;return result;
 }
 // 0x221FF7D0 explicitly has an ECX==NULL branch. The others do not.
 if(!this_va&&function!=kSpatialPartitionVA){
   result.code=Code::InvalidObject;return result;
 }
 Guard guard(busy_);
 try {
   if(function==kSubobjectDelegationVA){
     // Native 0x2209CE50: read [ECX-630h], invoke the enclosing
     // object's vtable[6] with ECX=ECX-630h, then hand returned
     // EAX to 0x2209CDD0. Both results are ignored by final MOV AL,1.
     if(this_va<kNestedOwnerOffset){result.code=Code::InvalidPointer;return result;}
     const auto owner=this_va-kNestedOwnerOffset;
     result.final_object_va=owner;
     result.stage=Stage::ReadOwnerVtable;
     std::uint32_t vt=0u;
     if(!host_.read_u32(owner,vt)){result.code=Code::GuestReadFailure;return result;}
     if(!vt){result.code=Code::InvalidPointer;return result;}
     std::uint32_t slot_va=0u;
     if(!add32(vt,kPrimaryVirtualOffset,slot_va)){
       result.code=Code::InvalidPointer;return result;
     }
     result.stage=Stage::ReadPrimarySlotSix;
     std::uint32_t func=0u;
     if(!host_.read_u32(slot_va,func)){result.code=Code::GuestReadFailure;return result;}
     if(!func){result.code=Code::InvalidPointer;return result;}
     result.stage=Stage::CallPrimarySlotSix;
     std::uint32_t eax=0u;
     ++result.external_calls;
     if(!host_.invoke_primary_slot_six(func,owner,eax)){
       result.code=Code::ExternalCallbackFailure;return result;
     }
     result.stage=Stage::CallNestedHelper;
     ++result.external_calls;
     if(!host_.invoke_nested_helper(owner,eax)){
       result.code=Code::ExternalCallbackFailure;return result;
     }
   }else if(function==kNamedRegistrationVA){
     // Exactly five original calls to 0x220FB9B0, no loop condition.
     // First call uses caller's ECX; later calls use saved ESI=ECX.
     result.final_object_va=this_va;
     for(const auto& spec:kOriginalNames){
       result.stage=Stage::NamedRegistration;
       ++result.external_calls;
       if(!host_.invoke_named_helper(this_va,0u,spec)){
         result.code=Code::ExternalCallbackFailure;return result;
       }
     }
   }else if(function==kDecalScriptVA){
     // Original 0x22122540 pushes a single literal pointer and
     // executes ADD ECX,-4 before its direct CALL to 0x22122280.
     if(this_va<kDecalOwnerOffset){
       result.code=Code::InvalidPointer;return result;
     }
     result.final_object_va=this_va-kDecalOwnerOffset;
     result.stage=Stage::LoadDecalScript;
     ++result.external_calls;
     if(!host_.invoke_decal_helper(result.final_object_va,
           NameSpec{"scripts/decals_subrect.txt",kDecalLiteralVA})){
       result.code=Code::ExternalCallbackFailure;return result;
     }
   }else{
     // Original 0x221FF7D0: IF ECX != 0 THEN argument=ECX+8
     // ELSE argument=0. Either case calls Source's spatial-partition
     // interface via vtable byte offset 0x28, then MOV AL,1; RET.
     std::uint32_t arg=0u;
     if(this_va&&!add32(this_va,kPartitionDataOffset,arg)){
       result.code=Code::InvalidPointer;return result;
     }
     result.final_object_va=this_va;
     std::uint32_t interface_va=0u;
     result.stage=Stage::ReadSpatialGlobal;
     if(!host_.read_u32(kSpatialPartitionGlobalVA,interface_va)){
       result.code=Code::GuestReadFailure;return result;
     }
     if(!interface_va){result.code=Code::InvalidPointer;return result;}
     std::uint32_t vtable=0u;
     result.stage=Stage::ReadPartitionVtable;
     if(!host_.read_u32(interface_va,vtable)){
       result.code=Code::GuestReadFailure;return result;
     }
     if(!vtable){result.code=Code::InvalidPointer;return result;}
     std::uint32_t slot=0u;
     if(!add32(vtable,kPartitionVirtualOffset,slot)){
       result.code=Code::InvalidPointer;return result;
     }
     std::uint32_t callback=0u;
     result.stage=Stage::ReadPartitionSlotTen;
     if(!host_.read_u32(slot,callback)){
       result.code=Code::GuestReadFailure;return result;
     }
     if(!callback){result.code=Code::InvalidPointer;return result;}
     result.stage=Stage::CallPartitionSlotTen;
     ++result.external_calls;
     if(!host_.invoke_partition_slot_ten(callback,interface_va,arg)){
       result.code=Code::ExternalCallbackFailure;return result;
     }
   }
   result.original_return_al=true;
   result.code=Code::Completed;
 }catch(...){
   result.code=Code::ExternalCallbackFailure;
 }
 return result;
}
} // namespace sin_compat::original_server_system_init_phase661
