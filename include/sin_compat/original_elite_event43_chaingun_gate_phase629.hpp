#pragma once
// Phase629: independently authored 2006 SiN x86 Elite event43 gate on
// a weapon-linked RESOLVED entity. Spawn obtains its handle through
// CWeaponChaingun RTTI, but the returned handle target class is unproven.
// Verified from original server.dll:
// 0x22315320 (82 bytes) and paired update 0x22315390 (173 bytes).
// Called by original Elite event override 0x22355480 via 0x22355503.
// Does NOT construct game weapons, claim a callable Source vtable, or
// simulate an engine clock. All engine side effects stay host-controlled.
#include <cstdint>

namespace sin_compat::original_elite_event43_chaingun_gate_phase629 {
inline constexpr std::uint32_t kNativeGateVA=0x22315320u;
inline constexpr std::uint32_t kNativeUpdateVA=0x22315390u;
inline constexpr std::uint32_t kNativeEvent43CallerVA=0x22355503u;
inline constexpr float kNativePhaseSeconds=1.0f;
struct State {
  std::uint32_t original_phase_594=0u;
  float original_time_590=0.f;
  float original_time_58c=0.f;
};
enum class Stage:std::uint8_t {
  None,GateVirtual3C4Command13,GateTime590,GateTime58C,
  UpdateReadTime,UpdateTime590,UpdateTime58C,
  UpdateVirtual3C8Command13,UpdateVirtual3C4Command14,
  WritePhase594,WriteTime590,WriteTime58C
};
enum class Code:std::uint8_t {
  Blocked,Allowed,Advanced,Unchanged,RejectedNullTarget,
  HostRejected,Reentrant
};
struct Result {
  Code code=Code::HostRejected;
  Stage failed_stage=Stage::None;
  bool event_permitted()const noexcept{return code==Code::Allowed;}
  bool native_call_accepted()const noexcept{
    return code==Code::Blocked || code==Code::Allowed ||
           code==Code::Advanced || code==Code::Unchanged;
  }
};
class IHost {
public:
  virtual ~IHost()=default;
  // Original slot +0x3C4 with stack arguments (13,0) at gate phase 0;
  // and (14,0) at update phase 3. Never guess the name of the engine
  // operation; the call target is dynamic in the owner's actual vtable.
  virtual bool native_virtual_3c4(std::uint32_t command,
                                  std::uint32_t zero)=0;
  // Original slot +0x3C8 with argument 13 in update phase 3.
  virtual bool native_virtual_3c8(std::uint32_t command)=0;
  // Original global pointer at 0x22572190 and float field +0x0C.
  // Every call is an independent source load; do not coalesce reads.
  virtual bool read_original_engine_time(float& value)=0;
  // Phase630: optional engine-host native field publication callbacks.
  // Default local reference models keep the in-memory State only; an
  // engine adapter MUST override all three for source-order native writes.
  virtual bool write_original_phase_594(std::uint32_t) {return true;}
  virtual bool write_original_time_590(float) {return true;}
  virtual bool write_original_time_58c(float) {return true;}
};
class Reference final {
public:
  explicit Reference(IHost& host)noexcept:host_(host){}
  Result gate(State& state,bool valid_native_target)noexcept;
  Result update(State& state,bool valid_native_target)noexcept;
  Reference(const Reference&)=delete;
  Reference& operator=(const Reference&)=delete;
private:
  IHost& host_;
  bool busy_=false;
};
} // namespace sin_compat::original_elite_event43_chaingun_gate_phase629
