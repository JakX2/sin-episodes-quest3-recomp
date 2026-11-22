#pragma once
// Phase623: independent host-controlled MODEL of original 2006
// CNPC_EliteGrunt::Spawn, verified from retail server.dll @ 0x22355820.
// Not a Source CBaseNPC, an engine factory, or a runnable native vtable.
// Host methods stand for original-engine effects; no guessed Source ABI.
#include <cstdint>
#include <string_view>

namespace sin_compat::original_elite_grunt_spawn_phase623 {
inline constexpr const char* kOriginalClassname="npc_druglab_grunt_elite";
inline constexpr const char* kOriginalWeapon="weapon_chaingun";
inline constexpr std::uint32_t kPrimaryFlags=0x00100000u;
inline constexpr std::uint32_t kSecondaryFlags=0x00200000u;
inline constexpr std::uint32_t kInvalidHandle=0xffffffffu;
enum class DirtyPath:std::uint8_t {
  NoMarker,NativeField200,EdictStateBit0
};
enum class Stage:std::uint8_t {
  None,WeaponSymbol,Virtual68,Virtual1CThen6C,InheritedSpawn,
  ClearFD9,Virtual798,ReadScalar9C,UpdateField9C,ReadScalar98,
  UpdateField98,FlagsPrimary,FlagsSecondary,ResolveActiveChaingun,
  WriteHandleFDC
};
enum class Code:std::uint8_t {
  Applied,UnsupportedClass,HostRejected,UnsupportedOriginalX87,Reentrant
};
struct Snapshot {
  std::string_view original_classname;
  std::string_view current_weapon_symbol; // null game symbol => empty
  std::int32_t original_field_9c=0;
  std::int32_t original_field_98=0;
  std::uint32_t original_flags_b4=0;
  bool original_flag_1f0=false; // original +0x200 netvar dirty path
  bool original_edict_16c_present=false;
};
struct Result {
  Code code=Code::HostRejected;
  Stage failed_stage=Stage::None;
  std::uint32_t flags_after=0;
  std::uint32_t final_handle_fdc=kInvalidHandle;
  bool wrote_handle=false;
  bool ok()const noexcept{return code==Code::Applied;}
};
class IHost {
public:
  virtual ~IHost()=default;
  virtual bool assign_original_weapon_symbol(std::string_view name)=0;
  virtual bool native_virtual_68()=0;
  // Original virtual +0x1C produces a string, then +0x6C consumes it.
  virtual bool native_virtual_1c_then_6c()=0;
  virtual bool inherited_npc_spawn_2235F2A0()=0;
  virtual bool clear_byte_fd9()=0;
  virtual bool native_virtual_798_with_zero()=0;
  // Read original ConVar at global +0x2259C208, pointer +0x28.
  // Retail code reads this scalar TWICE, once for each numeric field.
  // Host must not assume both reads have the same value.
  virtual bool read_original_convar_float_28(float& out)=0;
  // Both original native virtual notifications precede field stores.
  virtual bool virtual_1b4_then_write_field_9c(std::int32_t value)=0;
  virtual bool virtual_1b0_then_write_field_98(std::int32_t value)=0;
  // Original sets flag bits in order; updates write +0xB4 and optionally
  // dirty marker +0x200 or edict state bit0. Host owns actual memory.
  virtual bool commit_flags_b4(std::uint32_t flags,DirtyPath marker)=0;
  // Original: active weapon handle via 0x222AA640, RTTI downcast against
  // CWeaponChaingun, virtual +0x08, then first dword of returned pointer.
  // Return 1 = accepted typed chaingun, raw encoded value in out;
  //        0 = absent / wrong type; -1 = host failure. No engine pointers.
  virtual int resolve_chaingun_encoded_handle(std::uint32_t& out)=0;
  virtual bool write_field_fdc(std::uint32_t encoded_handle)=0;
};
class SpawnReference final {
public:
  explicit SpawnReference(IHost& host)noexcept:host_(host){}
  SpawnReference(const SpawnReference&)=delete;
  SpawnReference& operator=(const SpawnReference&)=delete;
  Result apply(const Snapshot& current) noexcept;
private:
  IHost& host_;
  bool busy_=false;
};
} // namespace sin_compat::original_elite_grunt_spawn_phase623
