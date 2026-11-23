#include "sin_compat/original_server_object_init_phase662.hpp"
#include <limits>
namespace sin_compat::original_server_object_init_phase662 {
namespace {
constexpr ObjectSpec kOriginalObjects[kObjectCount]={
  {0x22223496u,0x222234bdu,0x22225ba0u,0x2244b1e4u,0x0cu,0x22223600u,0x22223620u},
  {0x222234d1u,0x222234f8u,0x22225f10u,0x2244b1d0u,0x08u,0x22223630u,0x22223650u},
  {0x2222350cu,0x22223533u,0x22226280u,0x2244b1bcu,0x10u,0x22223660u,0x22223680u},
  {0x22223547u,0x2222356eu,0x222265f0u,0x2244b1a8u,0x14u,0x22223690u,0x222236b0u},
  {0x22223582u,0x222235a9u,0x22226960u,0x2244b194u,0x18u,0x222236c0u,0x222236e0u},
  {0x222235bdu,0x222235e4u,0x22226cd0u,0x2244b180u,0x1cu,0x222236f0u,0x22223710u}
};
struct BusyGuard{
 bool& busy;
 explicit BusyGuard(bool& flag)noexcept:busy(flag){busy=true;}
 ~BusyGuard()noexcept{busy=false;}
};
bool add(std::uint32_t lhs,std::uint32_t rhs,std::uint32_t& result)noexcept{
 if(lhs>std::numeric_limits<std::uint32_t>::max()-rhs)return false;
 result=lhs+rhs;return true;
}
}
const ObjectSpec* original_objects(std::size_t& count)noexcept{
 count=kObjectCount;return kOriginalObjects;
}
Result InitAuxiliaryObjects::run(std::uint32_t object)noexcept{
 Result r{};r.owner_va=object;
 if(busy_){r.code=Code::Reentrant;return r;}
 if(!object){r.code=Code::InvalidOwner;return r;}
 BusyGuard guard(busy_);
 try{
  for(std::size_t i=0u;i<kObjectCount;++i){
   const auto& spec=kOriginalObjects[i];
   r.failed_index=i;
   std::uint32_t owner_field=0u;
   if(!add(object,spec.owner_field_offset,owner_field)){
    r.code=Code::InvalidAddress;return r;
   }
   // Original calls malloc even when *owner_field already contains an
   // object. It does NOT skip allocation based on the destination.
   std::uint32_t ptr=0u;
   r.stage=Stage::Allocate;
   ++r.allocation_calls;
   if(!host_.allocate_original_2226e790(kObjectBytes,ptr)){
    r.code=Code::ExternalCallbackFailure;return r;
   }
   if(ptr!=0u){
    ++r.successful_allocations;
    r.stage=Stage::WriteVtable;
    if(!host_.write_u32(ptr,spec.original_vtable_va)){
      r.code=Code::WriteFailure;return r;
    }
    std::uint32_t derived_this=0u;
    if(!add(ptr,kCtorThisOffset,derived_this)){
      r.code=Code::InvalidAddress;return r;
    }
    r.stage=Stage::Construct;
    ++r.constructor_calls;
    if(!host_.construct_original(spec,derived_this,kCtorElementSize,0u,0u)){
      r.code=Code::ExternalCallbackFailure;return r;
    }
   }else{
    // Original MOV ESI,0 branch after malloc failure, then
    // conditionally stores zero if the destination is currently zero.
    ++r.missing_allocations;
   }
   std::uint32_t current=0u;
   r.stage=Stage::ReadOwnerField;
   if(!host_.read_u32(owner_field,current)){
    r.code=Code::ReadFailure;return r;
   }
   if(current==0u){
    r.stage=Stage::StoreOwnerField;
    if(!host_.write_u32(owner_field,ptr)){
      r.code=Code::WriteFailure;return r;
    }
    ++r.destination_writes;
   }else{
    ++r.preexisting_destinations;
   }
   ++r.objects_processed;
  }
  // Every original path after six attempted allocations executes
  // MOV AL,1; POP EDI; POP ESI; RET, irrespective of malloc failures.
  r.code=Code::Completed;
  r.failed_index=kObjectCount;
  r.original_return_al=true;
 }catch(...){r.code=Code::ExternalCallbackFailure;}
 return r;
}
} // namespace sin_compat::original_server_object_init_phase662
