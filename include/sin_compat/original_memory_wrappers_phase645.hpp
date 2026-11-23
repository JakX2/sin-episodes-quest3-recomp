#pragma once
// Retail SiN Episodes: Emergence 2006 server.dll allocation dispatch.
// Complete original methods 0x2226E290(90), 0x2226E390(79),
// 0x2226E400(90), 0x2226E3E0(18), total 277 original bytes.
// Memory-manager and imported Windows API operations are host-supplied.
#include <array>
#include <cstdint>
namespace sin_compat::original_memory_wrappers_phase645 {
inline constexpr std::uint32_t kTagDiscoveryVA=0x2226e290u;
inline constexpr std::uint32_t kAllocateVA=0x2226e390u;
inline constexpr std::uint32_t kReallocateVA=0x2226e400u;
inline constexpr std::uint32_t kFreeVA=0x2226e3e0u;
inline constexpr std::uint32_t kMemoryManagerGlobal=0x223d2124u;
inline constexpr std::uint32_t kAllocateTag=0x225885dcu;
inline constexpr std::uint32_t kAllocateFlag=0x225885e0u;
inline constexpr std::uint32_t kReallocateTag=0x225885e4u;
inline constexpr std::uint32_t kReallocateFlag=0x225885e8u;
inline constexpr std::uint32_t kTagProbeAddress=0x225885d8u;
inline constexpr std::uint32_t kModuleNameBytes=260u;
inline constexpr std::uint32_t kVirtualQueryBytes=28u;
inline constexpr std::uint32_t kSlotTaggedAllocate=0x00u;
inline constexpr std::uint32_t kSlotUntaggedAllocate=0x04u;
inline constexpr std::uint32_t kSlotTaggedReallocate=0x08u;
inline constexpr std::uint32_t kSlotUntaggedReallocate=0x0cu;
inline constexpr std::uint32_t kSlotFree=0x14u;
inline constexpr std::uint32_t kSlotTagProbe=0x5cu;
enum class Code : std::uint8_t {
  Completed,HostRejected,NullManagerHazard,AddressOverflow,Reentrant
};
enum class Stage : std::uint8_t {
  None,ReadCacheFlagByte,ReadCacheFlagDword,WriteCacheFlagDword,
  ReadCachedTag,ProbeManagerTag,GetProcessHeap,HeapAlloc260,
  VirtualQueryModuleBase,GetModuleFileNameA260,WriteCachedTag,
  ReadManagerGlobal,ReadManagerInstance,ReadManagerVtable,ReadVtableMethod,
  DispatchUntaggedAllocate,DispatchTaggedAllocate,
  DispatchUntaggedReallocate,DispatchTaggedReallocate,DispatchFree
};
struct Result {
  Code code=Code::HostRejected;
  Stage stage=Stage::None;
  std::uint32_t result_pointer=0;
  std::uint32_t tag_pointer=0;
  std::uint32_t dispatch_slot=0;
  std::uint32_t dispatch_target=0;
  std::uint32_t writes_committed=0;
  std::uint32_t tag_discovery_calls=0;
  bool cache_was_initially_set=false;
  bool tag_enabled=false;
};
class IHost {
public:
  virtual ~IHost()=default;
  virtual bool read_u8(std::uint32_t guest_va,std::uint8_t& value)=0;
  virtual bool read_u32(std::uint32_t guest_va,std::uint32_t& value)=0;
  virtual bool write_u32(std::uint32_t guest_va,std::uint32_t value)=0;
  // Dispatches [*memory_manager.vtable + slot]. The actual pointer is read
  // from guest memory first and passed in; these are NOT invented algorithms.
  virtual bool call_memory_manager(std::uint32_t instance,std::uint32_t method_va,
      std::uint32_t slot,const std::array<std::uint32_t,4>& args,
      std::uint32_t count,std::uint32_t& eax_result)=0;
  // Import-boundary callbacks exactly corresponding to retail Win32 APIs.
  virtual bool win_get_process_heap(std::uint32_t& heap)=0;
  virtual bool win_heap_alloc(std::uint32_t heap,std::uint32_t flags,
      std::uint32_t count,std::uint32_t& guest_buffer)=0;
  // Result's allocation_base is the first DWORD of 28-byte
  // MEMORY_BASIC_INFORMATION originally written to the x86 stack.
  virtual bool win_virtual_query(std::uint32_t address,std::uint32_t length,
      std::uint32_t& allocation_base)=0;
  virtual bool win_get_module_file_name_a(std::uint32_t module,
      std::uint32_t guest_buffer,std::uint32_t count,
      std::uint32_t& return_length)=0;
};
class AllocatorWrappers final {
public:
  explicit AllocatorWrappers(IHost& host)noexcept:host_(host){}
  Result allocate(std::uint32_t bytes)noexcept;
  Result reallocate(std::uint32_t prior,std::uint32_t bytes)noexcept;
  Result free(std::uint32_t prior)noexcept;
  // Original 0x2226E290 standalone tag discovery, returns guest char* or 0.
  Result discover_tag()noexcept;
private:
  bool resolve_method(std::uint32_t slot,Result& r,
      std::uint32_t& instance,std::uint32_t& method)noexcept;
  bool load_cached_tag(bool realloc,Result& r)noexcept;
  IHost& host_;
  bool busy_=false;
  bool discovery_busy_=false;
};
} // namespace sin_compat::original_memory_wrappers_phase645
