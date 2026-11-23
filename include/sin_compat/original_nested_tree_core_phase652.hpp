#pragma once
// Source reference of complete original 2006 SiN Episodes native methods.
// 0x2228E470(261B), 0x2228E620(86B), 0x2228F460(109B).
// No original game binary bytes are included. Rebalance remains host-supplied.
#include <cstdint>
namespace sin_compat::original_nested_tree_core_phase652 {
inline constexpr std::uint32_t kFindParentVA=0x2228e470u;
inline constexpr std::uint32_t kAcquireSlotVA=0x2228e620u;
inline constexpr std::uint32_t kLinkNodeVA=0x2228f460u;
inline constexpr std::uint32_t kGrowSlotVA=0x2228e200u;
inline constexpr std::uint32_t kFreeSlotResolveVA=0x2228e100u;
inline constexpr std::uint32_t kRebalanceVA=0x2228edf0u;
inline constexpr std::uint32_t kSentinelVA=0x2259012cu;
inline constexpr std::uint32_t kSentinelFlagVA=0x22590134u;
inline constexpr std::uint16_t kInvalid=0xffffu;
inline constexpr std::uint32_t kStride=12u;
enum class Code:std::uint8_t{Completed,HostRejected,NullPointerHazard,AddressOverflow,
 TraversalCycle,Reentrant};
enum class Stage:std::uint8_t{None,ReadRoot10,WriteOutputParent,WriteOutputBranch,
 ReadArray4,CompareCallback,InitializeSentinel,ReadLeft0,ReadRight2,
 ReadFreeHead14,ReadNextIndex16,ReadCapacity8,GrowSlots200,
 WriteArrayCache18,WriteNextIndex16,ResolveFreeSlot100,ReadFreeNext2,
 WriteFreeHead14,ReadNodeArray4,WriteParent4,WriteRight2,WriteLeft0,
 WriteColor6,WriteRoot10,WriteParentLeft0,WriteParentRight2,RebalanceEDF0};
struct Result {
 Code code=Code::HostRejected;
 Stage stage=Stage::None;
 std::uint32_t owner_va=0;
 std::uint32_t writes_committed=0,visited=0,comparisons=0;
 std::uint16_t handle=kInvalid,parent=kInvalid;
 std::uint8_t branch=0;
 bool sentinel_initialized=false,used_free_list=false,grow_called=false;
 bool rebalance_called=false,linked=false;
};
class IHost {
public:
 virtual ~IHost()=default;
 virtual bool read_u8(std::uint32_t va,std::uint8_t& out)=0;
 virtual bool read_u16(std::uint32_t va,std::uint16_t& out)=0;
 virtual bool read_u32(std::uint32_t va,std::uint32_t& out)=0;
 virtual bool write_u8(std::uint32_t va,std::uint8_t value)=0;
 virtual bool write_u16(std::uint32_t va,std::uint16_t value)=0;
 virtual bool write_u32(std::uint32_t va,std::uint32_t value)=0;
 // This is the original indirect cdecl comparator at [map+0],
 // (key record, node+8 record, comparator context), AL indicates left.
 virtual bool compare_original_indirect(std::uint32_t comparator_va,
     std::uint32_t key_va,std::uint32_t node_value_va,
     std::uint32_t context_va,std::uint8_t& al)=0;
 // Entire methods 0x2228E200, 0x2228E100, 0x2228EDF0 are NOT claimed here.
 virtual bool grow_original_2228e200(std::uint32_t slot_owner_va,
     std::uint32_t additional)=0;
 virtual bool resolve_original_2228e100(std::uint32_t map_va,
     std::uint16_t slot,std::uint32_t& node_va)=0;
 virtual bool rebalance_original_2228edf0(std::uint32_t map_va,
     std::uint16_t handle)=0;
};
class FindParent final {
public:
 explicit FindParent(IHost& host)noexcept:host_(host){}
 // Original __thiscall ECX=map, (key*, WORD* parent, BYTE* branch, context*).
 Result find(std::uint32_t map,std::uint32_t key,
             std::uint32_t out_parent,std::uint32_t out_branch,
             std::uint32_t context)noexcept;
private:IHost& host_;bool busy_=false;
};
class AcquireSlot final {
public:
 explicit AcquireSlot(IHost& host)noexcept:host_(host){}
 // Original __thiscall ECX=map, return AX=slot index.
 Result acquire(std::uint32_t map)noexcept;
private:IHost& host_;bool busy_=false;
};
class LinkNode final {
public:
 explicit LinkNode(IHost& host)noexcept:host_(host){}
 // Original __thiscall ECX=map, (WORD slot, WORD parent, BYTE left).
 // Node is marked red before original rebalance callback.
 Result link(std::uint32_t map,std::uint16_t handle,
             std::uint16_t parent,std::uint8_t left)noexcept;
private:IHost& host_;bool busy_=false;
};
} // namespace sin_compat::original_nested_tree_core_phase652
