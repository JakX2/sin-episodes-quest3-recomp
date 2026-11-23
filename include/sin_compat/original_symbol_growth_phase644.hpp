#pragma once
// SiN Episodes: Emergence 2006 original server.dll symbol-array owner.
// Complete methods: 0x22280DD0 (110 B), 0x22280DA0 (39 B).
// Portable source-level execution model, not a PE32 ABI replacement.
#include <cstdint>
namespace sin_compat::original_symbol_growth_phase644 {
inline constexpr std::uint32_t kGrowVA=0x22280dd0u;
inline constexpr std::uint32_t kGrowBytes=110u;
inline constexpr std::uint32_t kReleaseVA=0x22280da0u;
inline constexpr std::uint32_t kReleaseBytes=39u;
inline constexpr std::uint32_t kOriginalAllocateVA=0x2226e390u;
inline constexpr std::uint32_t kOriginalReallocateVA=0x2226e400u;
inline constexpr std::uint32_t kOriginalFreeVA=0x2226e3e0u;
inline constexpr std::uint32_t kOwnerStride=16u;
inline constexpr std::uint32_t kDisabledGrowStep=0xffffffffu;
// Owner is [table+4], pointing at a three-DWORD array owner:
//   [owner+0] array pointer, [owner+4] capacity, [owner+8] growth step.
enum class Code:std::uint8_t {
  Completed,HostRejected,NullOwnerHazard,AddressOverflow,GrowthLoopGuard,Reentrant
};
enum class Stage:std::uint8_t {
  None,ReadGrowthStep8,ReadCapacity4,ComputeRequestedCapacity,
  ReadCapacityForLoop,WriteCapacity4,ReadArrayPointer0,
  ReadCapacityForAllocation,Allocate2226e390,Reallocate2226e400,
  StoreArrayPointer0,ReadArrayPointerForRelease0,
  Free2226e3e0,ClearArrayPointer0,ClearCapacity4
};
struct Result {
  Code code=Code::HostRejected;
  Stage stage=Stage::None;
  std::uint32_t owner_va=0;
  std::uint32_t requested_increment=0;
  std::uint32_t initial_capacity=0;
  std::uint32_t target_capacity_wrapped=0;
  std::uint32_t final_capacity=0;
  std::uint32_t old_pointer=0;
  std::uint32_t new_pointer=0;
  std::uint32_t requested_bytes_wrapped=0;
  std::uint32_t capacity_writes=0;
  std::uint32_t pointer_writes=0;
  std::uint32_t loop_iterations=0;
  bool skipped_disabled=false;
  bool performed_initial_allocate=false;
  bool performed_reallocation=false;
  bool performed_free=false;
};
class IHost {
public:
  virtual ~IHost()=default;
  virtual bool read_u32(std::uint32_t guest_va,std::uint32_t& out)=0;
  virtual bool write_u32(std::uint32_t guest_va,std::uint32_t value)=0;
  // These callbacks represent ORIGINAL allocator wrappers, whose actual
  // memory-manager selection is still not reconstructed. A null successful
  // result is preserved and written, matching the lack of native check.
  virtual bool allocate_original_2226e390(std::uint32_t bytes,
                                          std::uint32_t& guest_pointer_out)=0;
  virtual bool reallocate_original_2226e400(std::uint32_t old_guest_pointer,
      std::uint32_t bytes,std::uint32_t& guest_pointer_out)=0;
  virtual bool free_original_2226e3e0(std::uint32_t guest_pointer)=0;
};
class GrowArray final {
public:
  explicit GrowArray(IHost& host)noexcept:host_(host){}
  // Original __thiscall ECX=owner, argument uint32 increment, RET 4.
  Result grow(std::uint32_t owner_va,std::uint32_t increment)noexcept;
private:
  IHost& host_;
  bool busy_=false;
};
class ReleaseArray final {
public:
  explicit ReleaseArray(IHost& host)noexcept:host_(host){}
  // Original __thiscall ECX=owner; no stack parameters, RET.
  Result release(std::uint32_t owner_va)noexcept;
private:
  IHost& host_;
  bool busy_=false;
};
} // namespace sin_compat::original_symbol_growth_phase644
