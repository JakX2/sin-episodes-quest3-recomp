#include "sin_compat/original_server_autogamesystem_phase660.hpp"
#include <limits>

namespace sin_compat::original_server_autogamesystem_phase660 {
namespace {
struct BusyGuard {
 bool& busy;
 explicit BusyGuard(bool& f)noexcept:busy(f){busy=true;}
 ~BusyGuard()noexcept{busy=false;}
};
bool add(std::uint32_t base,std::uint32_t off,std::uint32_t& out)noexcept {
 if(!base||base>std::numeric_limits<std::uint32_t>::max()-off)return false;
 out=base+off;return true;
}
std::int64_t signed32(std::uint32_t word)noexcept{
 return word<=0x7fffffffu?static_cast<std::int64_t>(word):
                            static_cast<std::int64_t>(word)-0x100000000ll;
}
}

Result AutoGameSystems::register_system(std::uint32_t object)noexcept {
 Result r{};r.object_va=object;
 if(busy_){r.code=Code::Reentrant;return r;}
 if(!object){r.code=Code::InvalidObject;return r;}
 BusyGuard guard(busy_);
 try{
  std::uint8_t initialized=0u;
  r.stage=Stage::RegisterReadFlag;
  if(!host_.read_u8(kInitFlagVA,initialized))return r;
  r.stage=Stage::RegisterWriteVtable;
  if(!host_.write_u32(object,kAutoSystemVtableVA))return r;
  ++r.writes_committed;
  if(initialized!=0u){
   std::uint32_t count=0u;
   r.stage=Stage::ReadCount;
   if(!host_.read_u32(kCountVA,count))return r;
   r.stage=Stage::RegisterLateNativeInsert;
   r.native_late_insert_called=true;
   if(!host_.insert_original_221f54f0(kArrayVA,count,object))return r;
  }else{
   std::uint32_t old_head=0u,next_va=0u;
   r.stage=Stage::RegisterReadPending;
   if(!host_.read_u32(kPendingHeadVA,old_head))return r;
   if(!add(object,4u,next_va)){r.code=Code::AddressOverflow;return r;}
   r.stage=Stage::RegisterWriteNext;
   if(!host_.write_u32(next_va,old_head))return r;
   ++r.writes_committed;
   r.stage=Stage::RegisterWriteHead;
   if(!host_.write_u32(kPendingHeadVA,object))return r;
   ++r.writes_committed;
  }
  r.code=Code::Completed;
 }catch(...){r.code=Code::HostRejected;}
 return r;
}

Result AutoGameSystems::initialize_all()noexcept {
 Result r{};
 if(busy_){r.code=Code::Reentrant;return r;}
 BusyGuard guard(busy_);
 try{
  std::uint32_t current=0u,count=0u;
  r.stage=Stage::ReadPendingHead;
  if(!host_.read_u32(kPendingHeadVA,current))return r;
  r.stage=Stage::ReadCount;
  if(!host_.read_u32(kCountVA,count))return r;
  for(std::uint32_t visited=0u;current!=0u;++visited){
   if(visited>=65536u){r.code=Code::CycleGuard;return r;}
   r.failed_system_va=current;
   ++r.processed_pending;
   // The original searches existing live slots for this pointer and
   // emits a duplicate warning instead of appending an existing object.
   bool already=false;
   if(signed32(count)>0){
    std::uint32_t array=0u;
    r.stage=Stage::ReadArray;
    if(!host_.read_u32(kArrayVA,array))return r;
    if(!array){r.code=Code::MissingArray;return r;}
    for(std::uint32_t i=0u;i<count;++i){
     if(i>=65536u){r.code=Code::CycleGuard;return r;}
     std::uint32_t ptr=0u,slot=0u;
     if(!add(array,i*4u,slot)){r.code=Code::AddressOverflow;return r;}
     r.stage=Stage::FindExisting;
     if(!host_.read_u32(slot,ptr))return r;
     if(ptr==current){already=true;break;}
    }
   }
   if(already){
    r.stage=Stage::DuplicateLog;
    ++r.duplicates;
    if(!host_.warn_duplicate_original(current))return r;
   }else{
    std::uint32_t capacity=0u;
    r.stage=Stage::ReadCapacity;
    if(!host_.read_u32(kCapacityVA,capacity))return r;
    if(signed32(count+1u)>signed32(capacity)){
      r.stage=Stage::NativeGrow;r.native_grow_called=true;
      if(!host_.grow_original_221f51a0(kArrayVA,count+1u-capacity))return r;
      r.stage=Stage::ReadCount;
      if(!host_.read_u32(kCountVA,count))return r;
    }
    std::uint32_t array=0u,slot=0u;
    r.stage=Stage::ReadArray;
    if(!host_.read_u32(kArrayVA,array))return r;
    if(!array){r.code=Code::MissingArray;return r;}
    if(count>std::numeric_limits<std::uint32_t>::max()/4u||
       !add(array,count*4u,slot)){r.code=Code::AddressOverflow;return r;}
    r.stage=Stage::AppendIncrementCount;
    if(!host_.write_u32(kCountVA,count+1u))return r;
    ++r.writes_committed;
    r.stage=Stage::AppendCache;
    if(!host_.write_u32(kArrayCacheVA,array))return r;
    ++r.writes_committed;
    r.stage=Stage::AppendWriteSlot;
    if(!host_.write_u32(slot,current))return r;
    ++r.writes_committed;
    ++r.appended;++count;
   }
   std::uint32_t next=0u,next_va=0u;
   if(!add(current,4u,next_va)){r.code=Code::AddressOverflow;return r;}
   r.stage=Stage::ReadPendingNext;
   if(!host_.read_u32(next_va,next))return r;
   current=next;
  }
  r.stage=Stage::ClearPendingHead;
  if(!host_.write_u32(kPendingHeadVA,0u))return r;
  ++r.writes_committed;
  r.stage=Stage::MarkInitialized;
  if(!host_.write_u8(kInitFlagVA,1u))return r;
  ++r.writes_committed;r.initialized_flag_set=true;

  r.stage=Stage::ReadCount;
  if(!host_.read_u32(kCountVA,count))return r;
  if(signed32(count)>0){
    if(count>65536u){r.code=Code::CycleGuard;return r;}
    for(std::uint32_t i=0u;i<count;++i){
      std::uint32_t array=0u,slot=0u,object=0u;
      r.stage=Stage::ReadArray;
      if(!host_.read_u32(kArrayVA,array))return r;
      if(!array){r.code=Code::MissingArray;return r;}
      if(!add(array,i*4u,slot)){r.code=Code::AddressOverflow;return r;}
      if(!host_.read_u32(slot,object))return r;
      r.failed_system_va=object;
      bool success=false;
      r.stage=Stage::VirtualInitSlotZero;
      ++r.virtual_init_calls;
      if(!host_.virtual_init_slot_zero(object,success))return r;
      if(!success){r.code=Code::VirtualInitFailed;return r;}
    }
  }
  r.failed_system_va=0u;r.code=Code::Completed;
 }catch(...){r.code=Code::HostRejected;}
 return r;
}
} // namespace sin_compat::original_server_autogamesystem_phase660
