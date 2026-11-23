#include "sin_compat/original_symbol_comparator_phase646.hpp"
#include <limits>
namespace sin_compat::original_symbol_comparator_phase646 {
namespace {
struct Guard { bool& flag;explicit Guard(bool& f)noexcept:flag(f){f=true;}
  ~Guard()noexcept{flag=false;} };
constexpr bool address(std::uint32_t base,std::uint32_t offset,
                       std::uint32_t& output)noexcept{
  if(base==0u||base>std::numeric_limits<std::uint32_t>::max()-offset)
    return false;
  output=base+offset;return true;
}
}
CompareResult CompareKey::compare(std::uint32_t key_va,
                                 std::uint32_t existing_va)noexcept{
  if(busy_)return {Code::Reentrant,Stage::None};
  Guard guard(busy_);
  CompareResult r{};r.key_va=key_va;r.existing_va=existing_va;
  if(key_va==0u||existing_va==0u){r.code=Code::NullPointerHazard;return r;}
  try{
    r.stage=Stage::ReadKeyWord;
    if(!host_.read_u16(key_va,r.key))return r;
    r.stage=Stage::ReadExistingWord;
    if(!host_.read_u16(existing_va,r.existing))return r;
    // Native: cmp cx,[edx]; sbb eax,eax; neg eax.
    // CF from 16-bit unsigned subtraction (no signed comparison).
    r.eax=r.key<r.existing?1u:0u;
    r.code=Code::Completed;
  }catch(...){r.code=Code::HostRejected;}
  return r;
}
bool InitializeTable::initialize_fields(std::uint32_t table,
                                         ConstructResult& r)noexcept{
  auto write32=[&](std::uint32_t offset,std::uint32_t value,
                   Stage stage)->bool{
    r.stage=stage;
    std::uint32_t at=0u;
    if(!address(table,offset,at)){
      r.code=Code::AddressOverflow;return false;
    }
    if(!host_.write_u32(at,value))return false;
    ++r.writes_committed;return true;
  };
  auto write16=[&](std::uint32_t offset,std::uint16_t value,
                   Stage stage)->bool{
    r.stage=stage;
    std::uint32_t at=0u;
    if(!address(table,offset,at)){
      r.code=Code::AddressOverflow;return false;
    }
    if(!host_.write_u16(at,value))return false;
    ++r.writes_committed;return true;
  };
  // Both original 0x22282130 and 0x22282210 inline the identical field
  // setup. Preserve original guest write ordering, including the fresh
  // array-pointer read before setting +0x18 cache.
  if(!write32(0x00u,kCompareVA,Stage::WriteComparator0))return false;
  if(!write32(0x04u,0u,Stage::WriteSlots4))return false;
  if(!write32(0x08u,0u,Stage::WriteCapacity8))return false;
  if(!write32(0x0cu,0u,Stage::WriteGrowthStepC))return false;
  if(!write16(0x10u,kInvalid,Stage::WriteRoot10))return false;
  if(!write16(0x14u,kInvalid,Stage::WriteFreeHead14))return false;
  std::uint32_t cache=0u,slot_field=0u;
  r.stage=Stage::ReadSlotsForCache4;
  if(!address(table,0x04u,slot_field)){
    r.code=Code::AddressOverflow;return false;
  }
  if(!host_.read_u32(slot_field,cache))return false;
  if(!write32(0x18u,cache,Stage::WriteSlotCache18))return false;
  if(!write16(0x12u,0u,Stage::WriteCount12))return false;
  if(!write16(0x16u,0u,Stage::WriteNextIndex16))return false;
  std::uint32_t nested=0u;
  r.stage=Stage::InitializeNestedF360;
  if(!address(table,0x1cu,nested)){
    r.code=Code::AddressOverflow;return false;
  }
  r.nested_init_invoked=true;
  // Original [ESP] pushes: 1, 0, 0; callee reads (0,0,1).
  return host_.initialize_original_2228f360(nested,0u,0u,1u);
}
ConstructResult InitializeTable::construct(std::uint32_t table)noexcept{
  if(busy_)return {Code::Reentrant,Stage::None};
  Guard guard(busy_);
  ConstructResult r{};r.table_va=table;
  if(table==0u){r.code=Code::NullPointerHazard;return r;}
  try{
    if(!initialize_fields(table,r))return r;
    r.code=Code::Completed;
  }catch(...){r.code=Code::HostRejected;}
  return r;
}
ConstructResult InitializeTable::factory(std::uint32_t owner)noexcept{
  if(busy_)return {Code::Reentrant,Stage::None};
  Guard guard(busy_);
  ConstructResult r{};r.owner_va=owner;
  if(owner==0u){r.code=Code::NullPointerHazard;return r;}
  try{
    r.stage=Stage::AllocateTable80;
    r.alloc_attempted=true;
    if(!host_.allocate_original_2226e790(kTableSize,r.table_va))return r;
    if(r.table_va!=0u){
      if(!initialize_fields(r.table_va,r))return r;
    }else r.null_allocation=true;
    // The factory writes the owner pointer last, both after nested init
    // succeeds and in the native malloc-failure branch (owner=0).
    r.stage=Stage::StoreFactoryOwner0;
    if(!host_.write_u32(owner,r.table_va))return r;
    ++r.writes_committed;
    r.owner_pointer_written=true;
    r.code=Code::Completed;
  }catch(...){r.code=Code::HostRejected;}
  return r;
}
} // namespace sin_compat::original_symbol_comparator_phase646
