#pragma once
// SiN Episodes: Emergence 2006 server.dll reference-only original routines.
// Whole original native bodies: 0x2228ED80 (100B), 0x2228F710 (94B),
// 0x2228E280 (112B). Engine allocation/memmove and nested tree callees
// remain mandatory host operations. Never bundle retail DLL binary bytes.
#include <cstdint>
namespace sin_compat::original_nested_insertion_phase651 {
inline constexpr std::uint32_t kAppendVA=0x2228ed80u;
inline constexpr std::uint32_t kAppendBytes=100u;
inline constexpr std::uint32_t kTreeInsertVA=0x2228f710u;
inline constexpr std::uint32_t kTreeInsertBytes=94u;
inline constexpr std::uint32_t kGrowVA=0x2228e280u;
inline constexpr std::uint32_t kGrowBytes=112u;
inline constexpr std::uint32_t kMoveVA=0x223a5d10u;
inline constexpr std::uint32_t kAcquireVA=0x2228e620u;
inline constexpr std::uint32_t kFindParentVA=0x2228e470u;
inline constexpr std::uint32_t kLinkVA=0x2228f460u;
inline constexpr std::uint32_t kDefaultCapacity=8u;
inline constexpr std::uint32_t kPointerStride=4u;
inline constexpr std::uint32_t kTreeStride=12u;
inline constexpr std::uint16_t kInvalid=0xffffu;
inline constexpr std::uint32_t kDisabledStep=0xffffffffu;
enum class Code:std::uint8_t{Completed,HostRejected,NullOwnerHazard,AddressOverflow,
  NullArrayHazard,GrowthLoopGuard,Reentrant};
enum class Stage:std::uint8_t{
 None,ReadGrowthStep8,ReadCapacity4,WriteCapacity4,ReadArray0,
 AllocateOriginal390,ReallocateOriginal400,WriteArray0,
 ReadAppendCountC,ReadAppendCapacity4,GrowArray280,IncrementCountC,
 ReadAppendArray0,WriteAppendCache10,MoveOriginal5D10,ReadSourceWord,
 WriteInsertedPointer,FindParent470,AcquireSlot620,LinkNode460,
 ReadTreeCount12,WriteTreeCount12,ReadTreeArray4,ReadReferenceWord,
 WriteTreeReference8
};
struct Result{
 Code code=Code::HostRejected;
 Stage stage=Stage::None;
 std::uint32_t owner_va=0;
 std::uint32_t returned_index=0;
 std::uint32_t old_capacity=0,new_capacity=0,alloc_bytes=0;
 std::uint32_t writes_committed=0,growth_rounds=0;
 std::uint32_t moved_bytes=0,allocated_pointer=0;
 std::uint16_t handle=kInvalid,parent=kInvalid;
 std::uint8_t branch=0;
 bool growth_called=false,allocation_called=false,reallocation_called=false;
 bool move_called=false,found_parent=false,acquired_node=false,linked_node=false;
 bool disabled_growth=false,inserted_record=false;
};
class IHost{
public:
 virtual ~IHost()=default;
 virtual bool read_u16(std::uint32_t va,std::uint16_t& out)=0;
 virtual bool read_u32(std::uint32_t va,std::uint32_t& out)=0;
 virtual bool write_u16(std::uint32_t va,std::uint16_t value)=0;
 virtual bool write_u32(std::uint32_t va,std::uint32_t value)=0;
 // Original memory-manager ABI dispatch, previously recovered in Phase645.
 virtual bool allocate_original_2226e390(std::uint32_t bytes,std::uint32_t& out)=0;
 virtual bool reallocate_original_2226e400(std::uint32_t prior,std::uint32_t bytes,
     std::uint32_t& out)=0;
 // Original memmove wrapper (overlapping source/destination are permitted).
 virtual bool move_original_223a5d10(std::uint32_t destination,
     std::uint32_t source,std::uint32_t bytes)=0;
 // Actual 0x2228E470 has four stack arguments and writes parent and branch
 // through caller stack pointers. Only this side-effect is abstracted here.
 virtual bool find_parent_original_2228e470(std::uint32_t map,std::uint32_t key_ref,
     std::uint32_t context,std::uint16_t& parent,std::uint8_t& branch)=0;
 virtual bool acquire_slot_original_2228e620(std::uint32_t map,
     std::uint16_t& handle)=0;
 // Original 0x2228F460 also calls 0x2228EDF0 for red-black rebalancing.
 // Host supplies this entire original link-and-rebalance behavior.
 virtual bool link_original_2228f460(std::uint32_t map,std::uint16_t handle,
     std::uint16_t parent,std::uint8_t branch)=0;
};
class ArrayGrowth final{
public:
 explicit ArrayGrowth(IHost& h)noexcept:host_(h){}
 // ECX=owner (pointer+0,capacity+4,step+8), arg1 increment.
 Result grow(std::uint32_t owner,std::uint32_t increment)noexcept;
private:IHost& host_;bool busy_=false;
};
class BlockArrayInsert final{
public:
 BlockArrayInsert(IHost& h,ArrayGrowth& g)noexcept:host_(h),growth_(g){}
 // Original __thiscall ECX=owner, stack (index, pointer_to_DWORD_value).
 // Inserts at the specified index, with right-shift of existing DWORDs.
 Result insert(std::uint32_t owner,std::uint32_t index,std::uint32_t value_word_va)noexcept;
private:IHost& host_;ArrayGrowth& growth_;bool busy_=false;
};
class TreeInsert final{
public:
 explicit TreeInsert(IHost& h)noexcept:host_(h){}
 // ECX=map, two stack args (pointer-to-4B stringref, comparator context).
 // Native returns newly acquired WORD slot handle in AX.
 Result insert(std::uint32_t map,std::uint32_t key_ref,
     std::uint32_t comparator_context)noexcept;
private:IHost& host_;bool busy_=false;
};
} // namespace sin_compat::original_nested_insertion_phase651
