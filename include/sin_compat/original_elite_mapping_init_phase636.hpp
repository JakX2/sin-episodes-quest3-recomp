#pragma once
// Gameplay Phase636. Three COMPLETE source-authenticated original 2006
// SiN server.dll routines in Elite's runtime event-ID mapping setup:
// 0x22073610 58-byte 24-byte node constructor (input BYTE mode),
// 0x22073650 55-byte node link/copy (two native pointer args),
// 0x22088F10 119-byte owner 3-node registration (three pointer args).
// Native original game memory is NOT accessed. A host supplies checked
// 32-bit reads/writes; changed host state is never silently rolled back.
#include <cstdint>

namespace sin_compat::original_elite_mapping_init_phase636 {
inline constexpr std::uint32_t kNodeInitVA=0x22073610u;
inline constexpr std::uint32_t kNodeInitBytes=58u;
inline constexpr std::uint32_t kNodeLinkVA=0x22073650u;
inline constexpr std::uint32_t kNodeLinkBytes=55u;
inline constexpr std::uint32_t kOwnerRegisterVA=0x22088f10u;
inline constexpr std::uint32_t kOwnerRegisterBytes=119u;
inline constexpr std::uint32_t kOriginalEliteOwnerVA=0x2259c2f8u;
inline constexpr std::uint32_t kOriginalEliteFirstRangeVA=0x2259c32cu;
inline constexpr std::uint32_t kSentinelSourceStart=9999u;
inline constexpr std::uint32_t kUnspecifiedDword=0xffffffffu;
inline constexpr std::uint32_t kNodeSize=0x18u;
inline constexpr std::uint32_t kFirstOwnerNode=0x04u;
inline constexpr std::uint32_t kSecondOwnerNode=0x1cu;
inline constexpr std::uint32_t kThirdOwnerNode=0x34u;
inline constexpr std::uint32_t kSourceRecordStride=8u;

enum class Code:std::uint8_t {
  Completed,HostRejected,OriginalNullPointerHazard,
  OriginalAddressOverflow,Reentrant
};
enum class Stage:std::uint8_t {
  None,ValidateNode,ReadExistingC,ConditionalWrite4,
  ConditionalWrite8,ConditionalWriteC,
  WriteNext10,WriteSource14,ReadSourcePlus4,WriteDestination0,
  WriteInit0,WriteInit4,WriteInit8,WriteInitC,
  WriteInit10,WriteInit14,
  WriteOwnerName0,LinkFirst,LinkSecond,LinkThird
};
struct Result {
  Code code=Code::HostRejected;
  Stage stage=Stage::None;
  unsigned writes_committed=0;
  unsigned links_completed=0;
  std::uint32_t last_node=0u;
};
class IGuestMemory {
public:
  virtual ~IGuestMemory()=default;
  // An address is an ORIGINAL x86 guest VA, not a process-local host ptr.
  virtual bool read_u32(std::uint32_t guest_va,std::uint32_t& result)=0;
  virtual bool write_u32(std::uint32_t guest_va,std::uint32_t value)=0;
};
class NodeInitializer final {
public:
  explicit NodeInitializer(IGuestMemory& host)noexcept:host_(host){}
  Result initialize(std::uint32_t node_va,std::uint8_t mode)noexcept;
private:
  IGuestMemory& host_;
  bool busy_=false;
};
class NodeLinker final {
public:
  explicit NodeLinker(IGuestMemory& host)noexcept:host_(host){}
  Result link(std::uint32_t node_va,std::uint32_t source_va,
              std::uint32_t successor_va)noexcept;
private:
  IGuestMemory& host_;
  bool busy_=false;
};
class OwnerRegistration final {
public:
  explicit OwnerRegistration(IGuestMemory& host)noexcept:
    host_(host),linker_(host){}
  // Original __thiscall this=owner, [esp+4]=name/classname,
  // [esp+8]=source-record array, [esp+0xC]=linked owner or zero.
  // Entire register method calls original 0x22073650 three times,
  // against owner+4, owner+0x1C, owner+0x34.
  Result register_owner(std::uint32_t owner_va,std::uint32_t name_va,
                        std::uint32_t source_va,
                        std::uint32_t linked_owner_va)noexcept;
private:
  IGuestMemory& host_;
  NodeLinker linker_;
  bool busy_=false;
};
} // namespace sin_compat::original_elite_mapping_init_phase636
