#pragma once
// Original 2006 SiN Episodes server.dll: full 0x2228E100 (84B) free-node
// resolver and 0x2228E200 (115B) 12-byte slot-array growth reference.
// Host callbacks expose guest memory and already recovered allocator wrappers.
#include <cstdint>
namespace sin_compat::original_nested_slot_growth_phase653 {
inline constexpr std::uint32_t kResolveVA=0x2228e100u;
inline constexpr std::uint32_t kResolveBytes=84u;
inline constexpr std::uint32_t kGrowVA=0x2228e200u;
inline constexpr std::uint32_t kGrowBytes=115u;
inline constexpr std::uint32_t kSentinelVA=0x2259012cu;
inline constexpr std::uint32_t kSentinelFlagVA=0x22590134u;
inline constexpr std::uint32_t kStride=12u;
inline constexpr std::uint32_t kInitialCapacity=3u;
inline constexpr std::uint16_t kInvalid=0xffffu;
inline constexpr std::uint32_t kDisabled=0xffffffffu;
enum class Code:std::uint8_t{Completed,HostRejected,NullOwnerHazard,
  NullArrayHazard,AddressOverflow,GrowthLoopGuard,Reentrant};
enum class Stage:std::uint8_t{
 None,ReadSentinelFlag,ReadSentinelFlagDWORD,SetSentinelFlag,
  WriteSentinelLeft,WriteSentinelRight,WriteSentinelParent,
  WriteSentinelBlack,ReadArray4,ReadStep8,ReadCapacity4,
  WriteCapacity4,Allocate390,Reallocate400,WriteArray0};
struct Result{
 Code code=Code::HostRejected;
 Stage stage=Stage::None;
 std::uint32_t owner_va=0;
 std::uint16_t index=kInvalid;
 std::uint32_t result_pointer=0,alloc_bytes=0;
 std::uint32_t old_capacity=0,new_capacity=0,growth_rounds=0;
 std::uint32_t writes_committed=0;
 bool sentinel_initialized=false,sentinel_returned=false;
 bool growth_disabled=false,allocation_called=false,reallocation_called=false;
};
class IHost{
public:
 virtual ~IHost()=default;
 virtual bool read_u8(std::uint32_t va,std::uint8_t& value)=0;
 virtual bool read_u32(std::uint32_t va,std::uint32_t& value)=0;
 virtual bool write_u16(std::uint32_t va,std::uint16_t value)=0;
 virtual bool write_u32(std::uint32_t va,std::uint32_t value)=0;
 // Calls exactly original allocator wrappers 0x2226E390 and 0x2226E400.
 // Actual engine vtable memory allocator remains host-supplied.
 virtual bool allocate_original_2226e390(std::uint32_t bytes,
     std::uint32_t& out)=0;
 virtual bool reallocate_original_2226e400(std::uint32_t prior,
     std::uint32_t bytes,std::uint32_t& out)=0;
};
class ResolveSlot final{
public:
 explicit ResolveSlot(IHost& host)noexcept:host_(host){}
 // Original __thiscall ECX=table/map, arg1 WORD slot (stack DWORD), RET 4.
 Result resolve(std::uint32_t map,std::uint16_t slot)noexcept;
private:IHost& host_;bool busy_=false;
};
class GrowSlots final{
public:
 explicit GrowSlots(IHost& host)noexcept:host_(host){}
 // Original __thiscall ECX=slot array owner (normally map+4), arg1
 // additional DWORD slot count, RET 4. Original stride is 12 bytes.
 Result grow(std::uint32_t slot_owner,std::uint32_t additional)noexcept;
private:IHost& host_;bool busy_=false;
};
} // namespace sin_compat::original_nested_slot_growth_phase653
