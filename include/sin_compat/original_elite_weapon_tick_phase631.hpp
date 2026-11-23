#pragma once
// Gameplay Phase631: independently authored reference of the COMPLETE
// 113-byte original 2006 CNPC_EliteGrunt virtual +0x424 callback at
// server.dll 0x223555F0 (SHA-pinned in original-only auditor).
// This is a native lifecycle route to the source-authenticated 0x22315390
// weapon-linked timed updater and inherited Grunt 0x2235DF10.
// No Source SDK ABI guess, entity allocations or retail code execution.
#include <cstdint>
namespace sin_compat::original_elite_weapon_tick_phase631 {
inline constexpr std::uint32_t kOriginalEliteTickVA=0x223555F0u;
inline constexpr std::uint32_t kOriginalEliteVtableVA=0x2248DF04u;
inline constexpr std::uint32_t kOriginalTickVtableOffset=0x424u;
inline constexpr std::uint32_t kOriginalWeaponUpdateVA=0x22315390u;
inline constexpr std::uint32_t kOriginalInheritedTickVA=0x2235DF10u;
inline constexpr std::uint32_t kHandleInvalid=0xffffffffu;
enum class Stage:std::uint8_t {
  None, FirstHandleRead, OpenSerialTable, FirstSerialResolve,
  SecondHandleRead, SecondSerialResolve, OriginalTargetUpdate,
  InheritedGruntTick
};
enum class Code:std::uint8_t {
  InheritedOnly, WeaponUpdatedThenInherited,
  OriginalNullTargetHazard, HostRejected, Reentrant
};
struct Result {
  Code code=Code::HostRejected;
  Stage failed_stage=Stage::None;
  std::uint32_t first_handle=kHandleInvalid;
  std::uint32_t second_handle=kHandleInvalid;
  std::uint64_t first_target=0u;
  std::uint64_t second_target=0u;
  bool update_attempted=false;
  bool inherited_called=false;
};
class IHost {
public:
  virtual ~IHost()=default;
  // Original reads Elite this+0xFDC twice, not one cached snapshot.
  virtual bool read_original_elite_fdc(std::uint32_t& value)=0;
  // Called once only after first handle is non-INVALID. The real source
  // captures global serial table base at 0x224F5620 and reuses it for
  // the second lookup, rather than acquiring a new table.
  virtual bool open_original_serial_table()=0;
  // With that same native table, 1 = serial match, output may be zero;
  // 0 = stale/mismatch, -1 = host error. Callbacks do not invent classes.
  virtual int resolve_original_serial(
      std::uint32_t encoded_handle,std::uint64_t& target)=0;
  // Source 0x22315390 is invoked with the SECOND resolved target.
  // If the second resolution goes stale, the retail code calls it with
  // ECX=0, which would dereference null. The portable model detects and
  // refuses that hazard instead of invoking an invalid native pointer.
  virtual bool invoke_original_weapon_update(std::uint64_t target)=0;
  // Source tail-jumps to inherited 0x2235DF10 for all normally safe paths.
  virtual bool inherited_grunt_tick_2235DF10()=0;
};
class Reference final {
public:
  explicit Reference(IHost& host)noexcept:host_(host){}
  Result run()noexcept;
  Reference(const Reference&)=delete;
  Reference& operator=(const Reference&)=delete;
private:
  IHost& host_;
  bool busy_=false;
};
} // namespace sin_compat::original_elite_weapon_tick_phase631
