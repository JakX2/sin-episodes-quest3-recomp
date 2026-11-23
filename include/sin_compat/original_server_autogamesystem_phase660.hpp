#pragma once
// Behavioral reconstruction of SiN Episodes (2006) original server.dll
// AutoGameSystem pending-registration and initializer lifecycle.
// Functions 0x221F5820 (70B) and 0x221F5700 (277B).
// The original engine's allocator, late-array insert, debug output and
// per-system virtual initializer remain typed external dependencies.
#include <cstdint>

namespace sin_compat::original_server_autogamesystem_phase660 {
inline constexpr std::uint32_t kRegisterVA=0x221f5820u;
inline constexpr std::uint32_t kRegisterBytes=70u;
inline constexpr std::uint32_t kInitAllVA=0x221f5700u;
inline constexpr std::uint32_t kInitAllBytes=277u;
inline constexpr std::uint32_t kPendingHeadVA=0x22572cc0u;
inline constexpr std::uint32_t kInitFlagVA=0x22572cb8u;
inline constexpr std::uint32_t kArrayVA=0x22510a58u;
inline constexpr std::uint32_t kCapacityVA=0x22510a5cu;
inline constexpr std::uint32_t kCountVA=0x22510a64u;
inline constexpr std::uint32_t kArrayCacheVA=0x22510a68u;
inline constexpr std::uint32_t kAutoSystemVtableVA=0x22443e74u;
inline constexpr std::uint32_t kNativeGrowVA=0x221f51a0u;
inline constexpr std::uint32_t kNativeInsertVA=0x221f54f0u;
inline constexpr std::uint32_t kNativeInitCallSiteVA=0x221f57fau;

enum class Code : std::uint8_t {
  Completed, VirtualInitFailed, HostRejected, InvalidObject,
  MissingArray, AddressOverflow, CycleGuard, Reentrant
};
enum class Stage : std::uint8_t {
  None, RegisterWriteVtable, RegisterReadFlag, RegisterReadPending,
  RegisterWriteNext, RegisterWriteHead, RegisterLateNativeInsert,
  ReadPendingHead, ReadCount, ReadArray, FindExisting, DuplicateLog,
  ReadCapacity, NativeGrow, AppendIncrementCount, AppendCache,
  AppendWriteSlot, ReadPendingNext, ClearPendingHead,
  MarkInitialized, VirtualInitSlotZero
};
struct Result {
  Code code=Code::HostRejected;
  Stage stage=Stage::None;
  std::uint32_t object_va=0u;
  std::uint32_t writes_committed=0u;
  std::uint32_t processed_pending=0u;
  std::uint32_t appended=0u;
  std::uint32_t duplicates=0u;
  std::uint32_t virtual_init_calls=0u;
  std::uint32_t failed_system_va=0u;
  bool native_grow_called=false;
  bool native_late_insert_called=false;
  bool initialized_flag_set=false;
};
class IHost {
public:
  virtual ~IHost()=default;
  virtual bool read_u8(std::uint32_t va,std::uint8_t& value)=0;
  virtual bool read_u32(std::uint32_t va,std::uint32_t& value)=0;
  virtual bool write_u8(std::uint32_t va,std::uint8_t value)=0;
  virtual bool write_u32(std::uint32_t va,std::uint32_t value)=0;
  // Original 0x221F51A0 called on ECX=0x22510A58, stack increment.
  virtual bool grow_original_221f51a0(std::uint32_t array_owner,
                                      std::uint32_t increment)=0;
  // Original 0x221F54F0 takes the array owner, insert index and a pointer
  // to a DWORD containing the system object. The source passes the value.
  virtual bool insert_original_221f54f0(std::uint32_t array_owner,
       std::uint32_t index,std::uint32_t system_object)=0;
  // Original indirectly calls logger with exact duplicate-warning text.
  virtual bool warn_duplicate_original(std::uint32_t system_object)=0;
  // Original dispatches each system's virtual vtable slot zero, returning AL.
  virtual bool virtual_init_slot_zero(std::uint32_t object,bool& al)=0;
};
class AutoGameSystems final {
public:
  explicit AutoGameSystems(IHost& host)noexcept:host_(host){}
  // Native __thiscall ECX=system; C3 return of same object EAX.
  Result register_system(std::uint32_t object_va)noexcept;
  // Native __cdecl 0x221F5700 with no parameters, returns AL bool.
  Result initialize_all()noexcept;
private:
  IHost& host_;
  bool busy_=false;
};
} // namespace sin_compat::original_server_autogamesystem_phase660
