#pragma once
// Recovered complete 2006 SiN Episodes server.dll methods: native array
// 0x2228EAB0 (90B), nested map 0x2228F360 (81B), release 0x2228EB10 (41B).
// Guest memory and original allocator wrappers are caller-host supplied.
#include <cstdint>
namespace sin_compat::original_nested_map_phase647 {
inline constexpr std::uint32_t kArrayConstructorVA=0x2228eab0u;
inline constexpr std::uint32_t kMapConstructorVA=0x2228f360u;
inline constexpr std::uint32_t kArrayReleaseVA=0x2228eb10u;
inline constexpr std::uint32_t kArrayBytes=90u;
inline constexpr std::uint32_t kMapBytes=81u;
inline constexpr std::uint32_t kReleaseBytes=41u;
inline constexpr std::uint32_t kRecordStride=12u;
inline constexpr std::uint32_t kBytewiseLessComparatorVA=0x2228e910u;
inline constexpr std::uint32_t kStringComparatorVA=0x2228e9b0u;
inline constexpr std::uint16_t kNullIndex=0xffffu;
inline constexpr std::uint32_t kDisabledStep=0xffffffffu;
enum class Code:std::uint8_t {Completed,HostRejected,NullObjectHazard,
  AddressOverflow,Reentrant};
enum class Stage:std::uint8_t {None,WriteCallback0,WriteArray4Null,
  WriteCapacity8,WriteStepC,AllocateOriginal2226e390,WriteArray4Allocated,
  WriteRoot10,WriteFreeHead14,ReadArray4ForCache,WriteCache18,
  WriteCount12,WriteNext16,NestedArrayConstructor,WriteMap1C,
  WriteMap20,WriteMap24,WriteMap28,WriteMap2C,
  ReadCallback0ForSelector,WriteDefaultComparator0,
  ReadStepCForRelease,ReadArray4ForRelease,FreeOriginal2226e3e0,
  ClearArray4,ClearCapacity8};
struct Result {
  Code code=Code::HostRejected;
  Stage stage=Stage::None;
  std::uint32_t object_va=0;
  std::uint32_t writes_committed=0;
  std::uint32_t initial_capacity=0;
  std::uint32_t allocation_bytes=0;
  std::uint32_t allocated_pointer=0;
  std::uint32_t selected_comparator=0;
  bool allocation_called=false;
  bool release_called=false;
  bool skipped_disabled=false;
  bool nested_array_completed=false;
};
class IHost {
public:
  virtual ~IHost()=default;
  virtual bool read_u32(std::uint32_t va,std::uint32_t& out)=0;
  virtual bool write_u32(std::uint32_t va,std::uint32_t value)=0;
  virtual bool write_u16(std::uint32_t va,std::uint16_t value)=0;
  // These reference original Phase645 wrappers. The real memory manager
  // remains external, never replaced by a fictitious engine allocator.
  virtual bool allocate_original_2226e390(std::uint32_t bytes,
      std::uint32_t& out)=0;
  virtual bool free_original_2226e3e0(std::uint32_t pointer)=0;
};
class ArrayConstructor final {
public:
  explicit ArrayConstructor(IHost& host)noexcept:host_(host){}
  // __thiscall ECX=array, three DWORDs (growth_step, capacity, callback).
  Result construct(std::uint32_t object,std::uint32_t growth_step,
      std::uint32_t capacity,std::uint32_t callback_pointer)noexcept;
private:
  IHost& host_;
  bool busy_=false;
};
class MapConstructor final {
public:
  MapConstructor(IHost& host,ArrayConstructor& array)noexcept
      :host_(host),array_(array){}
  // __thiscall ECX=map, (growth_step,initial_capacity,selector).
  // Only LOW BYTE of selector matters. The nested array is initialized
  // with callback=0, then the appropriate original comparator installed.
  Result construct(std::uint32_t object,std::uint32_t growth_step,
      std::uint32_t initial_capacity,std::uint32_t selector)noexcept;
private:
  IHost& host_;
  ArrayConstructor& array_;
  bool busy_=false;
};
class ArrayRelease final {
public:
  explicit ArrayRelease(IHost& host)noexcept:host_(host){}
  Result release(std::uint32_t object)noexcept;
private:
  IHost& host_;
  bool busy_=false;
};
} // namespace sin_compat::original_nested_map_phase647
