#pragma once
// Phase622: SHA-authenticated original 2006 CNPC_EliteGrunt factory
// CONTROL-FLOW REFERENCE, never a live Source engine ABI or vtable.
// It is a distinct registered type from the four CNPC_MobGrunt aliases.
// All offsets below are specific to the 2006 server.dll.
// Native engine host MUST own actual CBaseNPC allocation, real entity/edict
// construction, and source-compatible vtables for its selected runtime.
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace sin_compat::original_elite_grunt_factory_phase622 {
inline constexpr const char* kOriginalClassname="npc_druglab_grunt_elite";
inline constexpr std::size_t kOriginalAllocationBytes=0xFE0u;
inline constexpr std::size_t kFactoryReturnedSubobjectOffset=0x160u;
inline constexpr std::uint32_t kPrimaryVptrOffset=0u;
inline constexpr std::uint32_t kSecondVptrOffset=0x790u;
inline constexpr std::uint32_t kThirdVptrOffset=0xC14u;
inline constexpr std::uint32_t kExtraSentinelFieldOffset=0xFDCu;
inline constexpr std::uint32_t kExtraSentinel=0xffffffffu;

enum class VptrRole :std::uint8_t {Primary,Secondary790,TertiaryC14};
enum class Stage :std::uint8_t {
  None,Allocate,BaseConstructor,PrimaryVptr,SecondaryVptr,TertiaryVptr,
  StaticInitialization,ReadEpochs,EpochRefresh,PublishEpoch,ExtraSentinel,
  NativeVirtual70
};
enum class Code :std::uint8_t {
  Created,UnsupportedClass,Reentrant,AllocationFailed,PointerOverflow,
  HostRejected,RollbackFailed
};
struct Result {
  Code code=Code::UnsupportedClass;
  Stage failed_stage=Stage::None;
  std::uintptr_t original_object=0;
  std::uintptr_t original_networkable_view=0;
  bool ok()const noexcept{return code==Code::Created;}
};
struct Epochs {
  // Read from original-equivalent host-managed static state. Source's
  // cached word is 0x22534694, epoch input is 0x2254A3C0.
  std::uint32_t cached=0,current=0;
};
class IHost {
public:
  virtual ~IHost()=default;
  virtual std::uintptr_t allocate_native_npc(std::size_t bytes)=0;
  virtual bool base_npc_constructor_22360D80(std::uintptr_t object)=0;
  virtual bool write_host_native_vptr(
      std::uintptr_t object,std::uint32_t offset,VptrRole role)=0;
  // Original call 0x22360BC0 follows all three elite vptr assignments.
  virtual bool static_initialize_22360BC0()=0;
  virtual bool read_epoch_words(Epochs& out)=0;
  // Only if cached != current: 0x22355A20 then the two global stores.
  virtual bool refresh_static_epoch_22355A20()=0;
  virtual bool commit_static_epoch(std::uint32_t current)=0;
  // Original ctor writes 0xffffffff to new object's +0xFDC.
  virtual bool write_extra_sentinel(
      std::uintptr_t object,std::uint32_t offset,std::uint32_t value)=0;
  virtual bool native_virtual_70(std::uintptr_t object,
                                 std::uintptr_t factory_argument)=0;
  // Independent fail-closed recovery; NOT the original's null
  // allocation/error behavior. Does not restore global side effects.
  virtual bool reclaim_unpublished_npc(std::uintptr_t object)=0;
  virtual bool delegate_original_virtual_18(std::uintptr_t entity_view)=0;
};

class FactoryReference final {
public:
  explicit FactoryReference(IHost& h)noexcept:host_(h){}
  FactoryReference(const FactoryReference&)=delete;
  FactoryReference& operator=(const FactoryReference&)=delete;
  static bool supports(std::string_view name)noexcept;
  Result create(std::string_view name,
                std::uintptr_t original_factory_argument)noexcept;
  bool delegate_release(std::uintptr_t networkable_view)noexcept;
private:
  struct Guard {
    bool& busy;
    bool entered;
    explicit Guard(bool& b)noexcept:busy(b),entered(!b){if(entered)busy=true;}
    ~Guard()noexcept{if(entered)busy=false;}
    explicit operator bool()const noexcept{return entered;}
  };
  IHost& host_;
  bool busy_=false;
};
} // namespace sin_compat::original_elite_grunt_factory_phase622
