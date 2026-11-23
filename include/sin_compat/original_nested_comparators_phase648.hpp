#pragma once
// SiN Episodes: Emergence original 2006 server.dll, complete native comparator
// routines 0x2228E910 (157 bytes) and 0x2228E9B0 (113 bytes).
// Portable guest-state model, not a replacement for the retail PE32 functions.
#include <cstdint>
namespace sin_compat::original_nested_comparators_phase648 {
inline constexpr std::uint32_t kDirectVA=0x2228e910u;
inline constexpr std::uint32_t kDirectBytes=157u;
inline constexpr std::uint32_t kDelegatingVA=0x2228e9b0u;
inline constexpr std::uint32_t kDelegatingBytes=113u;
inline constexpr std::uint32_t kNativeStringCompareVA=0x223a5574u;
inline constexpr std::uint16_t kSentinelWord=0xffffu;
inline constexpr std::uint32_t kStringBlocksOffset=0x1cu;
inline constexpr std::uint32_t kFallbackStringOffset=0x30u;
inline constexpr std::uint32_t kBlockStringDataOffset=8u;
enum class Code:std::uint8_t {Completed,HostRejected,NullPointerHazard,AddressOverflow,CycleGuard,Reentrant};
enum class Stage:std::uint8_t {None,ReadLeftId0,ReadLeftOffset2,ReadLeftFallback30,ReadLeftBlocks1C,ReadLeftBlockPointer,
  ReadRightId0,ReadRightOffset2,ReadRightFallback30,ReadRightBlocks1C,ReadRightBlockPointer,
  ReadLeftByte0,ReadRightByte0,ReadLeftByte1,ReadRightByte1,DelegateOriginal223a5574};
struct Result {
 Code code=Code::HostRejected;
 Stage stage=Stage::None;
 std::uint32_t left_record_va=0,right_record_va=0,map_va=0;
 std::uint32_t left_string_va=0,right_string_va=0;
 std::uint32_t bytes_compared=0;
 std::uint32_t eax=0;
 std::int32_t delegated_result=0;
 bool left_uses_fallback=false,right_uses_fallback=false;
 bool delegated=false;
};
class IHost {
public:
 virtual ~IHost()=default;
 virtual bool read_u8(std::uint32_t va,std::uint8_t& out)=0;
 virtual bool read_u16(std::uint32_t va,std::uint16_t& out)=0;
 virtual bool read_u32(std::uint32_t va,std::uint32_t& out)=0;
 // Original call at 0x2228EA0E, cdecl(left_string,right_string),
 // which returns signed 32-bit comparison in EAX. Its native internals
 // and global mode switch are not silently approximated here.
 virtual bool compare_original_223a5574(std::uint32_t left_string_va,
     std::uint32_t right_string_va,std::int32_t& eax)=0;
};
class Comparators final {
public:
 explicit Comparators(IHost& host)noexcept:host_(host){}
 // Both original routines: cdecl(record*,record*,map*), plain RET.
 Result bytewise(std::uint32_t left,std::uint32_t right,std::uint32_t map)noexcept;
 Result delegated(std::uint32_t left,std::uint32_t right,std::uint32_t map)noexcept;
private:
 bool resolve(std::uint32_t record,std::uint32_t map,bool first,Result& r,
     std::uint32_t& out)noexcept;
 IHost& host_;
 bool busy_=false;
};
} // namespace sin_compat::original_nested_comparators_phase648
