#pragma once
// Gameplay Phase634: complete 2006 original SiN Elite event47 terminal methods.
// 0x220398A0 (91 bytes): optional virtual+0x760 and original 0x22073700
//     event-ID translation, then clear original Elite flags DWORD +0x7CC.
// 0x2203D2D0 (93 bytes): mandatory 0x2204E420 maintenance, optional
//     virtual+0x760/0x22073700 lookup for ID 0x21, then conditional
//     original Elite DWORD +0x7A4 = 4.
// The owner's exact retail server.dll complete-method SHA256s are checked
// by research/tools/audit_original_elite_event47_terminal_phase634.py.
// No guessed Source vtables or real engine callbacks are called here.
#include <cstdint>

namespace sin_compat::original_elite_event47_terminal_phase634 {
inline constexpr std::uint32_t kOriginalClearVA=0x220398A0u;
inline constexpr std::uint32_t kOriginalStateVA=0x2203D2D0u;
inline constexpr std::uint32_t kOriginalLookupVA=0x22073700u;
inline constexpr std::uint32_t kOriginalMaintenanceVA=0x2204E420u;
inline constexpr std::uint32_t kOriginalFlagArrayOffset=0x7CCu;
inline constexpr std::uint32_t kOriginalStateOffset=0x7A4u;
inline constexpr std::uint32_t kOriginalIndexBase=1000000000u;
inline constexpr std::uint32_t kOriginalTestEventId=0x21u;
inline constexpr std::uint32_t kOriginalStateValue=4u;

enum class Code:std::uint8_t {
  Cleared,Skipped,State4Written,StatePreserved,HostRejected,
  OriginalUnsafeFlagIndex,OriginalNullContextHazard,Reentrant
};
enum class Stage:std::uint8_t {
  None,MaintenanceCall,CheckStateArgument,ObtainVirtual760Context,
  ConvertSourceId,ValidateMappedId,ComputeFlagIndex,
  ClearFlagWord,TestFlagWord,WriteState4
};
struct Result {
  Code code=Code::HostRejected;
  Stage failed_stage=Stage::None;
  std::int32_t converted_id=-1;
  std::int32_t word_index=-1;
  std::uint32_t mask=0u;
  bool map_called=false;
  bool maintenance_called=false;
  bool bit_cleared=false;
  bool bit_tested=false;
  bool state_written=false;
};
class IHost {
public:
  virtual ~IHost()=default;
  // Original virtual +0x760 yields a pointer; caller then uses +0x34
  // as the context for the genuine 0x22073700 ID mapping helper.
  // Return source object identity only; host owns the actual pointer.
  virtual bool original_virtual_760_context(std::uint64_t& out)=0;
  virtual bool original_id_map_22073700(
      std::uint64_t source_context,std::int32_t input_id,
      std::int32_t& mapped_id)=0;
  // These callbacks access the original Elite +0x7CC DWORD array.
  // Index is a *signed original x86 SAR 5 result*, mask is 1<<(id&31).
  // Only nonnegative indices are forwarded; host must check real bounds.
  virtual bool clear_original_elite_flag_bit(
      std::uint32_t word_index,std::uint32_t mask)=0;
  virtual bool read_original_elite_flag_bit(
      std::uint32_t word_index,std::uint32_t mask,bool& set)=0;
  // Source call 0x2204E420 is unconditional even when state argument !=0;
  // do not pretend this side effect is a no-op.
  virtual bool original_maintenance_2204E420()=0;
  virtual bool write_original_elite_state_7a4(std::uint32_t value)=0;
};
class ClearFlag final {
public:
  explicit ClearFlag(IHost& host)noexcept:host_(host){}
  Result run(std::int32_t source_event_id)noexcept;
  ClearFlag(const ClearFlag&)=delete;
  ClearFlag& operator=(const ClearFlag&)=delete;
private:
  IHost& host_;
  bool busy_=false;
};
class StateZero final {
public:
  explicit StateZero(IHost& host)noexcept:host_(host){}
  Result run(std::uint8_t state_argument)noexcept;
  StateZero(const StateZero&)=delete;
  StateZero& operator=(const StateZero&)=delete;
private:
  IHost& host_;
  bool busy_=false;
};
} // namespace sin_compat::original_elite_event47_terminal_phase634
