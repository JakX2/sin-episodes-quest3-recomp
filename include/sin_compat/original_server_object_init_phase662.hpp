#pragma once
// Original 2006 server.dll 0x22223490, 363 bytes. Six 40-byte
// game-system auxiliary object allocations + constructors. Engine allocator
// and original constructors remain typed guest-host dependencies.
#include <cstddef>
#include <cstdint>
namespace sin_compat::original_server_object_init_phase662 {
inline constexpr std::uint32_t kOriginalVA=0x22223490u;
inline constexpr std::uint32_t kOriginalBytes=363u;
inline constexpr std::uint32_t kOriginalRegisteredVtableVA=0x2244aeccu;
inline constexpr std::uint32_t kAllocatorVA=0x2226e790u;
inline constexpr std::uint32_t kObjectBytes=40u;
inline constexpr std::uint32_t kCtorThisOffset=4u;
inline constexpr std::uint32_t kCtorElementSize=64u;
inline constexpr std::size_t kObjectCount=6u;
struct ObjectSpec {
  std::uint32_t original_allocator_call_va;
  std::uint32_t original_constructor_call_va;
  std::uint32_t original_constructor_function_va;
  std::uint32_t original_vtable_va;
  std::uint32_t owner_field_offset;
  std::uint32_t comparator_function_va;
  std::uint32_t hash_function_va;
};
const ObjectSpec* original_objects(std::size_t& count) noexcept;
enum class Code : std::uint8_t {
  Completed, InvalidOwner, InvalidAddress, ReadFailure,
  WriteFailure, ExternalCallbackFailure, Reentrant
};
enum class Stage : std::uint8_t {
  None, Allocate, WriteVtable, Construct, ReadOwnerField, StoreOwnerField
};
struct Result {
  Code code=Code::ExternalCallbackFailure;
  Stage stage=Stage::None;
  std::uint32_t original_function_va=kOriginalVA;
  std::uint32_t owner_va=0u;
  std::size_t objects_processed=0u;
  std::uint32_t allocation_calls=0u;
  std::uint32_t constructor_calls=0u;
  std::uint32_t successful_allocations=0u;
  std::uint32_t missing_allocations=0u;
  std::uint32_t destination_writes=0u;
  std::uint32_t preexisting_destinations=0u;
  std::size_t failed_index=0u;
  bool original_return_al=false;
};
class IHost {
public:
 virtual ~IHost()=default;
 // Original external allocator 0x2226E790, one DWORD size arg, returns EAX.
 // NULL is an ordinary original allocation failure, not transport failure.
 virtual bool allocate_original_2226e790(std::uint32_t bytes,
       std::uint32_t& allocation_va)=0;
 virtual bool read_u32(std::uint32_t guest_va,std::uint32_t& value)=0;
 virtual bool write_u32(std::uint32_t guest_va,std::uint32_t value)=0;
 // Real constructor receives ECX=allocation+4 and stack args
 // (element_size=0x40, 0, 0, comparator_va, hash_va).
 virtual bool construct_original(const ObjectSpec& spec,
       std::uint32_t object_this_va,
       std::uint32_t element_size,std::uint32_t zero1,
       std::uint32_t zero2)=0;
};
class InitAuxiliaryObjects final {
public:
 explicit InitAuxiliaryObjects(IHost& host)noexcept:host_(host){}
 Result run(std::uint32_t original_this_va) noexcept;
private:
 IHost& host_;
 bool busy_=false;
};
} // namespace sin_compat::original_server_object_init_phase662
