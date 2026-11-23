#pragma once
// 2006 SiN original retail server.dll CRT-like string comparison dependency.
// Method bodies: 0x223A5574 (67B), 0x223A8410 (78B), 0x223A72DA (213B).
// Host-supplied locale/ctype callees are not fabricated native implementations.
#include <cstdint>
namespace sin_compat::original_string_casefold_phase649 {
inline constexpr std::uint32_t kCompareVA=0x223a5574u;
inline constexpr std::uint32_t kAsciiVA=0x223a8410u;
inline constexpr std::uint32_t kTransformVA=0x223a72dau;
inline constexpr std::uint32_t kModeGlobal=0x2259f500u;
inline constexpr std::uint32_t kLocaleCodepageGlobal=0x2259f510u;
inline constexpr std::uint32_t kCTypeFastGlobal=0x2253d1e0u;
inline constexpr std::uint32_t kCTypeModeGlobal=0x2253d208u;
inline constexpr std::uint32_t kCTypeTableGlobal=0x2253d200u;
inline constexpr std::uint32_t kCTypeHelperVA=0x223a898au;
inline constexpr std::uint32_t kLocaleMapVA=0x223ac9e7u;
enum class Code:std::uint8_t{Completed,HostRejected,NullPointerHazard,AddressOverflow,CycleGuard,Reentrant};
enum class Stage:std::uint8_t{None,ReadCompareMode,ReadFirstLeft,ReadFirstRight,ReadLocaleMode,
  ReadCTypeFast,ReadCTypeMode,ReadCTypeTable,ReadCTypeMask,CallCTypeHelper,
  ReadHighLeadMask,ReadCodepage,CallLocaleMap,TransformLeft,TransformRight};
struct MapRequest{
 std::uint32_t mode=0u,options=0x100u;
 std::uint8_t first=0u,second=0u;
 std::uint32_t input_count=1u,output_capacity=3u,codepage=0u,flag=1u;
};
struct MapAnswer{std::uint32_t count=0u;std::uint8_t first=0u,second=0u;};
struct Result{Code code=Code::HostRejected;Stage stage=Stage::None;
 std::uint32_t left_va=0u,right_va=0u;
 std::uint32_t characters=0u,transforms=0u,locale_mappings=0u;
 std::int32_t comparison=0;
 bool locale_mode=false,ascii_fast=false;};
struct TransformResult{Code code=Code::HostRejected;Stage stage=Stage::None;
 std::uint32_t input=0u,output=0u;
 bool used_locale_map=false,used_ctype_helper=false,fast_ascii=false;};
class IHost{
public:
 virtual ~IHost()=default;
 virtual bool read_u8(std::uint32_t va,std::uint8_t& out)=0;
 virtual bool read_u32(std::uint32_t va,std::uint32_t& out)=0;
 // Actual native 0x223A898A(c,1) is external; classified bit returns in EAX.
 virtual bool classify_original_223a898a(std::uint32_t input,
    std::uint32_t mask,std::uint32_t& result)=0;
 // Native 0x223AC9E7 has eight stack parameters including local buffers.
 // Struct models their scalar values and output, not the original stack VA.
 virtual bool locale_map_original_223ac9e7(const MapRequest& request,
    MapAnswer& answer)=0;
};
class Casefold final{
public:
 explicit Casefold(IHost& host)noexcept:host_(host){}
 Result compare(std::uint32_t left,std::uint32_t right)noexcept;
 TransformResult transform(std::uint32_t character)noexcept;
private:
 Result compare_ascii(std::uint32_t left,std::uint32_t right)noexcept;
 Result compare_locale(std::uint32_t left,std::uint32_t right)noexcept;
 IHost& host_;
 bool comparing_=false,transforming_=false;
};
} // namespace sin_compat::original_string_casefold_phase649
