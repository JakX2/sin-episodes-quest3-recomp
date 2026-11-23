#pragma once
// 2006 SiN Episodes retail server.dll symbol table construction/comparator.
// Full original method bodies: 0x22280B50 (19 B), 0x22282130 (63 B),
// 0x22282210 (90 B). This is a portable source model, not an ABI shim.
#include <cstdint>
namespace sin_compat::original_symbol_comparator_phase646 {
inline constexpr std::uint32_t kCompareVA=0x22280b50u;
inline constexpr std::uint32_t kCompareLength=19u;
inline constexpr std::uint32_t kTableConstructVA=0x22282130u;
inline constexpr std::uint32_t kTableConstructLength=63u;
inline constexpr std::uint32_t kFactoryConstructVA=0x22282210u;
inline constexpr std::uint32_t kFactoryConstructLength=90u;
inline constexpr std::uint32_t kFactoryAllocateVA=0x2226e790u;
inline constexpr std::uint32_t kNestedInitializeVA=0x2228f360u;
inline constexpr std::uint32_t kTableSize=0x50u;
inline constexpr std::uint16_t kInvalid=0xffffu;
enum class Code:std::uint8_t {
  Completed, HostRejected, NullPointerHazard, AddressOverflow, Reentrant
};
enum class Stage:std::uint8_t {
  None, ReadKeyWord, ReadExistingWord, AllocateTable80,
  WriteComparator0, WriteSlots4, WriteCapacity8, WriteGrowthStepC,
  WriteRoot10, WriteFreeHead14, ReadSlotsForCache4,
  WriteSlotCache18, WriteCount12, WriteNextIndex16,
  InitializeNestedF360, StoreFactoryOwner0
};
struct CompareResult {
  Code code=Code::HostRejected;
  Stage stage=Stage::None;
  std::uint32_t key_va=0;
  std::uint32_t existing_va=0;
  std::uint16_t key=0,existing=0;
  std::uint32_t eax=0;
};
struct ConstructResult {
  Code code=Code::HostRejected;
  Stage stage=Stage::None;
  std::uint32_t owner_va=0;
  std::uint32_t table_va=0;
  std::uint32_t writes_committed=0;
  bool nested_init_invoked=false;
  bool alloc_attempted=false;
  bool null_allocation=false;
  bool owner_pointer_written=false;
};
class IHost {
public:
  virtual ~IHost()=default;
  virtual bool read_u16(std::uint32_t addr,std::uint16_t& value)=0;
  virtual bool read_u32(std::uint32_t addr,std::uint32_t& value)=0;
  virtual bool write_u16(std::uint32_t addr,std::uint16_t value)=0;
  virtual bool write_u32(std::uint32_t addr,std::uint32_t value)=0;
  // Guest allocator 0x2226E790 is a distinct original wrapper address.
  // Its real memory-management backend is host supplied (Phase645 ABI).
  virtual bool allocate_original_2226e790(std::uint32_t bytes,
      std::uint32_t& guest_pointer)=0;
  // Original nested 0x2228F360 (subobject at table+0x1c); its internal
  // string map/hash setup is not implied to be reconstructed here.
  virtual bool initialize_original_2228f360(std::uint32_t subobject_va,
      std::uint32_t arg0,std::uint32_t arg1,std::uint32_t arg2)=0;
};
class CompareKey final {
public:
  explicit CompareKey(IHost& host)noexcept:host_(host){}
  // cdecl: DWORD key*, DWORD existing*; return EAX=0 or 1.
  CompareResult compare(std::uint32_t key_va,
                        std::uint32_t existing_va)noexcept;
private:
  IHost& host_;
  bool busy_=false;
};
class InitializeTable final {
public:
  explicit InitializeTable(IHost& host)noexcept:host_(host){}
  // Native constructor 0x22282130: ECX=table, return EAX=table.
  ConstructResult construct(std::uint32_t table_va)noexcept;
  // Native owner factory 0x22282210: ECX=DWORD* owner; allocate 0x50
  // bytes via 0x2226E790, run inlined table constructor, return owner.
  ConstructResult factory(std::uint32_t owner_va)noexcept;
private:
  bool initialize_fields(std::uint32_t table_va,ConstructResult& result)noexcept;
  IHost& host_;
  bool busy_=false;
};
} // namespace sin_compat::original_symbol_comparator_phase646
