#pragma once
// Behavioral reconstruction: SiN Episodes (2006) original client.dll
// 16-byte linked-list registration/connection/disconnection of client
// startup app systems. The actual factory and cleanup callbacks are host-typed.
// Original x86 functions: 0x241640C0 / 0x241640F0 / 0x24164120.
#include <cstdint>

namespace sin_compat::original_client_appsystem_phase659 {
inline constexpr std::uint32_t kRegisterVA=0x241640c0u;
inline constexpr std::uint32_t kRegisterBytes=39u;
inline constexpr std::uint32_t kDisconnectVA=0x241640f0u;
inline constexpr std::uint32_t kDisconnectBytes=48u;
inline constexpr std::uint32_t kConnectVA=0x24164120u;
inline constexpr std::uint32_t kConnectBytes=54u;
inline constexpr std::uint32_t kHeadVA=0x24369374u;
inline constexpr std::uint32_t kEntryBytes=16u;

enum class Code : std::uint8_t {
  Completed, MissingRequiredAppSystem, HostRejected, InvalidArgument,
  NullDestinationHazard, CycleGuard, Reentrant
};
enum class Stage : std::uint8_t {
  None, ReadHead, WriteEntryTarget, WriteEntryResolve, WriteEntryRelease,
  WriteEntryNext, WriteHead, ReadEntryTarget, ReadEntryResolve,
  CallResolver, StoreResolvedPointer, ReadEntryNext,
  ReadEntryRelease, ReadTargetPointer, CallRelease, ClearTarget,
  DisconnectAfterFailure
};
struct Result {
  Code code=Code::HostRejected;
  Stage stage=Stage::None;
  std::uint32_t entries_visited=0;
  std::uint32_t resolver_calls=0;
  std::uint32_t release_calls=0;
  std::uint32_t writes_committed=0;
  std::uint32_t failed_entry=0;
  std::uint32_t failed_callback=0;
  bool cleanup_invoked=false;
};
class IHost {
public:
  virtual ~IHost()=default;
  virtual bool read_u32(std::uint32_t guest_va,std::uint32_t& value)=0;
  virtual bool write_u32(std::uint32_t guest_va,std::uint32_t value)=0;
  // Zero-argument original callback pointer stored in entry[+4], returns EAX.
  virtual bool resolve_original_callback(std::uint32_t callback_va,
      std::uint32_t& returned_interface)=0;
  // Original entry[+8] cleanup callback invoked with *destination on stack.
  virtual bool release_original_callback(std::uint32_t callback_va,
      std::uint32_t interface_pointer)=0;
};
class AppSystemChain final {
public:
  explicit AppSystemChain(IHost& host)noexcept:host_(host){}
  // Original __thiscall ECX=entry, stack DWORDs (target*,resolveFn,releaseFn),
  // RET 0xC, prepend entry to module linked list kHeadVA.
  Result register_entry(std::uint32_t entry_va,std::uint32_t target_storage,
                        std::uint32_t resolver_callback,
                        std::uint32_t release_callback)noexcept;
  // Original 0x24164120: resolve each entry; failure of a non-optional
  // destination invokes complete original 0x241640F0 cleanup.
  Result connect_all()noexcept;
  // Original 0x241640F0: cleanup *every* entry with nonzero destination,
  // even if its value was already zero. Does NOT delete list entries.
  Result disconnect_all()noexcept;
private:
  IHost& host_;
  bool busy_=false;
  Result disconnect_internal()noexcept;
};
} // namespace sin_compat::original_client_appsystem_phase659
