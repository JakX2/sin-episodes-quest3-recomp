#pragma once
// SiN Episodes: Emergence 2006 server.dll original nested string registration.
// Native function body provenance: 0x2228EB40 (268B) search, 0x2228EA30
// (47B) block-fit, and 0x2228F7A0 (261B) lookup-or-insert. Original
// allocation/append/tree-registration callees are mandatory host operations.
#include <cstdint>
namespace sin_compat::original_string_lookup_phase650 {
inline constexpr std::uint32_t kTreeSearchVA=0x2228eb40u;
inline constexpr std::uint32_t kTreeSearchBytes=268u;
inline constexpr std::uint32_t kBlockFitVA=0x2228ea30u;
inline constexpr std::uint32_t kBlockFitBytes=47u;
inline constexpr std::uint32_t kLookupVA=0x2228f7a0u;
inline constexpr std::uint32_t kLookupBytes=261u;
inline constexpr std::uint32_t kAllocatorVA=0x2226e390u;
inline constexpr std::uint32_t kAppendBlockVA=0x2228ed80u;
inline constexpr std::uint32_t kInsertTreeVA=0x2228f710u;
inline constexpr std::uint32_t kSentinelStateVA=0x22590134u;
inline constexpr std::uint32_t kSentinelWordsVA=0x2259012cu;
inline constexpr std::uint16_t kInvalid=0xffffu;
inline constexpr std::uint32_t kSlotStride=12u;
inline constexpr std::uint32_t kDefaultBlockCapacity=0x800u;
inline constexpr std::uint32_t kBlockHeaderBytes=8u;
inline constexpr std::uint32_t kBlockAllocationExtra=11u;
enum class Code:std::uint8_t {Completed,HostRejected,NullPointerHazard,
 AddressOverflow,CycleGuard,Reentrant};
enum class Stage:std::uint8_t {None,ReadRoot10,ReadSlotArray4,
 CompareKeyNode,CompareNodeKey,InitializeSentinel,ReadChild,
 WriteFallback30,WriteSentinelRef,SearchExisting,ReadName,
 ReadBlockCount28,ReadBlockPointers1C,ReadBlockPointer,ReadBlockCapacity,
 ReadBlockUsed,AllocateBlock,InitializeBlock,AppendBlockPointer,
 CopyString,AdvanceBlockUsed,WriteInsertionRef,RegisterRef,
 WriteOutputHandle};
struct Result {
 Code code=Code::HostRejected;
 Stage stage=Stage::None;
 std::uint32_t map_va=0u,output_va=0u,search_key_va=0u;
 std::uint32_t return_eax=0u,bytes_copied=0u,bytes_required=0u;
 std::uint32_t block_index=0xffffffffu,block_capacity=0u,block_used_before=0u;
 std::uint16_t handle=kInvalid;
 std::uint32_t comparisons=0u,writes_committed=0u;
 bool found_existing=false,inserted=false,allocated_block=false;
 bool sentinel_initialized=false,original_null_name=false;
};
class IHost {
public:
 virtual ~IHost()=default;
 virtual bool read_u8(std::uint32_t va,std::uint8_t& value)=0;
 virtual bool read_u16(std::uint32_t va,std::uint16_t& value)=0;
 virtual bool read_u32(std::uint32_t va,std::uint32_t& value)=0;
 virtual bool write_u8(std::uint32_t va,std::uint8_t value)=0;
 virtual bool write_u16(std::uint32_t va,std::uint16_t value)=0;
 virtual bool write_u32(std::uint32_t va,std::uint32_t value)=0;
 // Original [map] cdecl (key record*, node record*, context map*) -> AL.
 // The real Phase648/649 comparator implementations can be composed here.
 virtual bool compare_original_nested(std::uint32_t function_va,
      std::uint32_t key_record,std::uint32_t node_record,
      std::uint32_t context_map,bool& less)=0;
 // Original native 0x2226E390 allocation, Phase645 wrapper dispatch.
 virtual bool allocate_original_2226e390(std::uint32_t bytes,
      std::uint32_t& guest_pointer)=0;
 // Original 0x2228ED80 append takes map+0x1c array owner and pointer to
 // a DWORD containing the new block pointer; returns appended index.
 virtual bool append_original_2228ed80(std::uint32_t array_owner_va,
      std::uint32_t new_block_ptr,std::uint32_t& appended_index)=0;
 // Original 0x2228F710 tree record insertion (not reconstructed here).
 virtual bool register_original_2228f710(std::uint32_t map_va,
      std::uint32_t string_ref_va,std::uint16_t& handle)=0;
};
class TreeSearch final {
public:
 explicit TreeSearch(IHost& host)noexcept:host_(host){}
 // ECX=map, arg1=key-ref*, arg2=comparator context*.
 Result find(std::uint32_t map,std::uint32_t key_ref,
     std::uint32_t context)noexcept;
private:
 bool initialize_sentinel(Result& result)noexcept;
 IHost& host_;
 bool busy_=false;
};
class BlockFit final {
public:
 explicit BlockFit(IHost& host)noexcept:host_(host){}
 // Original ECX=map, arg1=bytes required; signed capacity tests.
 Result find(std::uint32_t map,std::uint32_t required)noexcept;
private:
 IHost& host_;
 bool busy_=false;
};
class LookupOrInsert final {
public:
 LookupOrInsert(IHost& host,TreeSearch& search,BlockFit& fit)noexcept
  :host_(host),search_(search),fit_(fit){}
 // ECX=map, arg1=WORD output*, arg2=char*; scratch_ref is a guest
 // 4-byte writable stand-in for the original mutable x86 arg2 stack word.
 Result run(std::uint32_t map,std::uint32_t output_word,
     std::uint32_t name,std::uint32_t scratch_ref)noexcept;
private:
 IHost& host_;TreeSearch& search_;BlockFit& fit_;
 bool busy_=false;
};
} // namespace sin_compat::original_string_lookup_phase650
