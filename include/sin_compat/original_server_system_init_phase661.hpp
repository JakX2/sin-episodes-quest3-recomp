#pragma once
// Phase661: four complete original 2006 SiN server.dll AutoGameSystem
// virtual slot-zero startup methods. Only external helper/engine callbacks
// remain typed host dependencies; this is not a full GameDLLInit.
#include <cstdint>
#include <cstddef>
namespace sin_compat::original_server_system_init_phase661 {
inline constexpr std::uint32_t kSubobjectDelegationVA=0x2209ce50u;
inline constexpr std::uint32_t kNamedRegistrationVA=0x220fbed0u;
inline constexpr std::uint32_t kDecalScriptVA=0x22122540u;
inline constexpr std::uint32_t kSpatialPartitionVA=0x221ff7d0u;
inline constexpr std::uint32_t kSubobjectDelegationBytes=30u;
inline constexpr std::uint32_t kNamedRegistrationBytes=75u;
inline constexpr std::uint32_t kDecalScriptBytes=16u;
inline constexpr std::uint32_t kSpatialPartitionBytes=39u;
inline constexpr std::uint32_t kSpatialPartitionGlobalVA=0x225721b8u;
inline constexpr std::uint32_t kNestedOwnerOffset=0x630u;
inline constexpr std::uint32_t kDecalOwnerOffset=4u;
inline constexpr std::uint32_t kPartitionDataOffset=8u;
inline constexpr std::uint32_t kPrimaryVirtualOffset=0x18u;
inline constexpr std::uint32_t kPartitionVirtualOffset=0x28u;
inline constexpr std::uint32_t kNestedHelperVA=0x2209cdd0u;
inline constexpr std::uint32_t kNamedHelperVA=0x220fb9b0u;
inline constexpr std::uint32_t kDecalHelperVA=0x22122280u;
inline constexpr std::uint32_t kPhonemesLiteralVA=0x2240016cu;
inline constexpr std::uint32_t kPhonemesWeakLiteralVA=0x2240015cu;
inline constexpr std::uint32_t kPhonemesStrongLiteralVA=0x2240014cu;
inline constexpr std::uint32_t kRandomLiteralVA=0x223d8ef8u;
inline constexpr std::uint32_t kRandomAlertLiteralVA=0x22400140u;
inline constexpr std::uint32_t kDecalLiteralVA=0x22401d70u;

enum class Code : std::uint8_t {
  Completed, Unsupported, InvalidObject, InvalidPointer, GuestReadFailure,
  ExternalCallbackFailure, Reentrant
};
enum class Stage : std::uint8_t {
  None, ReadOwnerVtable, ReadPrimarySlotSix, CallPrimarySlotSix,
  CallNestedHelper, NamedRegistration, LoadDecalScript,
  ReadSpatialGlobal, ReadPartitionVtable, ReadPartitionSlotTen,
  CallPartitionSlotTen
};
struct NameSpec {
  const char* text;
  std::uint32_t original_literal_va;
};
const NameSpec* original_name_specs(std::size_t& count) noexcept;
struct Result {
  Code code=Code::Unsupported;
  Stage stage=Stage::None;
  std::uint32_t original_function_va=0;
  std::uint32_t original_this_va=0;
  std::uint32_t final_object_va=0;
  std::uint32_t external_calls=0;
  bool original_return_al=false;
};
class IHost {
public:
  virtual ~IHost()=default;
  virtual bool read_u32(std::uint32_t address,std::uint32_t& value)=0;
  // Original nested primary vtable[+0x18] returns EAX as a full DWORD.
  virtual bool invoke_primary_slot_six(std::uint32_t fn,
      std::uint32_t primary_this,std::uint32_t& eax)=0;
  // 0x2209CDD0 __thiscall, one DWORD argument in the original.
  virtual bool invoke_nested_helper(std::uint32_t owner,
      std::uint32_t value_from_primary)=0;
  // 0x220FB9B0 __thiscall, stack DWORDs (0,name literal pointer).
  virtual bool invoke_named_helper(std::uint32_t object,
      std::uint32_t zero_arg,const NameSpec& name)=0;
  // 0x22122280 __thiscall; object is original this-4.
  virtual bool invoke_decal_helper(std::uint32_t object,
      const NameSpec& script)=0;
  // Global 0x225721B8 vtable[+0x28] is an external Source engine service.
  virtual bool invoke_partition_slot_ten(std::uint32_t function,
      std::uint32_t interface_this,std::uint32_t argument)=0;
};
class Initializers final {
public:
  explicit Initializers(IHost& host) noexcept:host_(host){}
  Result run(std::uint32_t original_function_va,std::uint32_t this_va) noexcept;
private:
  IHost& host_;
  bool busy_=false;
};
} // namespace sin_compat::original_server_system_init_phase661
