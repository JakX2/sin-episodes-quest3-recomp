#pragma once
// Gameplay Phase633: three COMPLETE and separately SHA256-authenticated
// 2006 SiN server.dll methods reached by original Elite event 0x47.
//   0x220326D0 (30 bytes): clock >= Elite field +0x9DC predicate
//   0x222AA640 (46 bytes): object serial handle at Elite +0x78C
//   0x2203F8E0 (55 bytes): target DWORD +0x4EC setter + dirty branches
// No Source C++ class/vtable guesses, native engine calls, or game memory
// allocations. Each original side effect is explicitly host-owned.
#include <cstdint>
namespace sin_compat::original_elite_event47_helpers_phase633 {
inline constexpr std::uint32_t kOriginalCooldownVA=0x220326d0u;
inline constexpr std::uint32_t kOriginalHandleLookupVA=0x222aa640u;
inline constexpr std::uint32_t kOriginalSetterVA=0x2203f8e0u;
inline constexpr std::uint32_t kOriginalGlobalTimePointerVA=0x22572190u;
inline constexpr std::uint32_t kOriginalGlobalSerialTableVA=0x224f5620u;
inline constexpr std::uint32_t kInvalidSerialHandle=0xffffffffu;
enum class Code:std::uint8_t {
  Allowed,Blocked,TargetFound,TargetAbsent,Applied,Unchanged,
  HostRejected,Reentrant
};
enum class Stage:std::uint8_t {
  None,
  ClockRead,EliteDeadline9DC,
  EliteEncodedHandle78C,OpenSerialTable,SerialSlotCheck,
  Field4ECRead,OriginalChangeFlag1F0,WriteDirtyByte200,
  DirtyOwner16CRead,MarkDirtyBit0,WriteField4EC
};
struct Result {
  Code code=Code::HostRejected;
  Stage stage=Stage::None;
  std::uint32_t encoded_handle=kInvalidSerialHandle;
  std::uint32_t slot_index=0u;
  std::uint32_t serial=0u;
  std::uint64_t target=0u;
  bool changed=false;
  bool dirty_byte_committed=false;
  bool edict_dirty_committed=false;
  bool field_committed=false;
};
class IClockHost {
public:
  virtual ~IClockHost()=default;
  // Original global *0x22572190 +0x0C; independent direct float read.
  virtual bool read_original_engine_time(float& out)=0;
  // Original Elite offset +0x9DC, compared with x87 FCOMP.
  virtual bool read_original_elite_deadline_9dc(float& out)=0;
};
class ISerialHost {
public:
  virtual ~ISerialHost()=default;
  // Re-read actor +0x78C on EVERY distinct helper invocation.
  virtual bool read_original_elite_serial_78c(std::uint32_t& out)=0;
  // Native source obtains global *0x224F5620 only after non-INVALID.
  virtual bool open_original_serial_table()=0;
  // Native checks 16-byte slot (+4 serial, +0 pointer), after:
  // index=handle&0xFFF, serial=handle>>12.
  // 1=matching serial (object may be 0), 0=mismatch, -1=host error.
  virtual int read_original_serial_slot(std::uint32_t index,
      std::uint32_t expected_serial,std::uint64_t& out)=0;
};
class IFieldHost {
public:
  virtual ~IFieldHost()=default;
  // Given an already resolved target from 0x222AA640, source passes
  // ECX=target+0x4EC to native 0x2203F8E0.
  virtual bool read_native_dword_4ec(std::uint32_t& out)=0;
  // Original flag target+0x1F0 decides which dirty path is taken.
  virtual bool read_native_flag_1f0(std::uint8_t& out)=0;
  // If flag is nonzero: set target+0x200=1, then write +0x4EC.
  virtual bool write_native_dirty_byte_200(std::uint8_t value)=0;
  // Otherwise read native pointer at target+0x16C, possibly null.
  virtual bool read_native_dirty_owner_16c(std::uint64_t& out)=0;
  // If owner pointer is nonzero: OR original DWORD *owner with bit 0.
  virtual bool or_native_dirty_owner_bit0(std::uint64_t owner)=0;
  // Finally publish DWORD target+0x4EC only if old != new.
  virtual bool write_native_dword_4ec(std::uint32_t new_value)=0;
};
class Cooldown final {
public:
  explicit Cooldown(IClockHost& host)noexcept:host_(host){}
  Result check()noexcept;
private:
  IClockHost& host_;
  bool busy_=false;
};
class SerialLookup final {
public:
  explicit SerialLookup(ISerialHost& host)noexcept:host_(host){}
  Result resolve()noexcept;
private:
  ISerialHost& host_;
  bool busy_=false;
};
class Field4ECSetter final {
public:
  explicit Field4ECSetter(IFieldHost& host)noexcept:host_(host){}
  Result set(std::uint32_t new_value)noexcept;
private:
  IFieldHost& host_;
  bool busy_=false;
};
} // namespace sin_compat::original_elite_event47_helpers_phase633
