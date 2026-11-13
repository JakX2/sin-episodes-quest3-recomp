#pragma once
// Phase212: original client.dll 0x241996F0 (2006 SiN) type routing only.
// Complete 1036-byte original x86 body is SHA-pinned in the research audit.
// This independent C++17 reference does NOT implement any original
// record serializer helper, native memory write, or executable Source ABI.
#include <array>
#include <cstdint>

namespace sin_compat::original_client_snapshot_record_router {

inline constexpr std::uint32_t kOriginalRecordWriterVA=0x241996f0u;
inline constexpr std::uint32_t kNativeBodyBytes=1036u;
inline constexpr std::uint32_t kNativeCaseJumpTableVA=0x24199afcu;
inline constexpr std::uint32_t kNativeTypeSelectorVA=0x24199b30u;
inline constexpr std::uint32_t kNativeNextFunctionVA=0x24199b50u;
inline constexpr std::uint32_t kNativeWarningIATVA=0x242761e0u;
inline constexpr std::uint32_t kNativeWarningStringVA=0x242a41e0u;
inline constexpr std::uint32_t kNativeTypeLimit=26u;
inline constexpr std::uint32_t kNativeCaseCount=13u;
inline constexpr std::uint32_t kNativeTypeCount=27u;
inline constexpr std::uint32_t kOriginalRecordBytes=0x34u;

// Exact original 27 BYTE values at original .text+0x199B30.
// Index 0 is skip, 12 is warning, 10 invokes the nested-record handler.
inline constexpr std::array<std::uint8_t,kNativeTypeCount> kTypeCases{{
     0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10, 0, 0,11, 0, 0,
     0, 0, 0, 0,12, 0,12,12,12,12, 0
}};
// Original 13 x86 absolute little-endian DWORDs at 0x24199AFC.
// Branches are within the 1036-byte original function body.
inline constexpr std::array<std::uint32_t,kNativeCaseCount> kCaseDestinations{{
    0x24199ad5u,0x24199847u,0x2419987fu,0x241998adu,
    0x241998e7u,0x241999b7u,0x2419997du,0x241999efu,
    0x24199a29u,0x24199921u,0x241997ceu,0x24199a8eu,
    0x24199ac7u
}};

// Native record type/flags from record+0x00 and signed WORD +0x12.
// Input 'mode' comes from the native state struct DWORD +0x00.
// 'already_seen' captures the original comparison of record+0x2C
// to the current process token. The x86 instructions first test it
// and can skip before reaching the type-filter/switch.
struct RouteInput {
    std::uint32_t type{};
    std::uint16_t flags_12{};
    std::uint32_t mode{};
    bool already_seen{};
};

// These three conditions are EXACT x86 tests after type != 0x0A:
//   test DH,0x02   => flags & 0x0200 -> skip
//   mode==1 & test DH,0x01 => flags & 0x0100 -> skip
//   mode==2 & !(test DH,0x01) => flags & 0x0100 absent -> skip.
// Type 10 skips the flag filtering but not the earlier already-seen test.
inline constexpr bool filter_out(const RouteInput& input) noexcept {
    if(input.already_seen) return true;
    if(input.type==10u) return false;
    if((input.flags_12 & 0x0200u)!=0u) return true;
    if(input.mode==1u && (input.flags_12 & 0x0100u)!=0u) return true;
    if(input.mode==2u && (input.flags_12 & 0x0100u)==0u) return true;
    return false;
}

enum class RouteKind:std::uint8_t {
    filtered, no_action, specialist, nested_records, warning
};
struct Route {
    RouteKind kind;
    std::uint8_t case_index;
    std::uint32_t original_branch_va;
};

inline constexpr Route classify(const RouteInput& input) noexcept {
    if(filter_out(input))return {RouteKind::filtered,0u,0u};
    const auto type=input.type;
    if(type>kNativeTypeLimit)
        return {RouteKind::warning,12u,kCaseDestinations[12]};
    const std::uint8_t selector=kTypeCases[type];
    const RouteKind kind=selector==0u?RouteKind::no_action:
                         selector==10u?RouteKind::nested_records:
                         selector==12u?RouteKind::warning:
                         RouteKind::specialist;
    return {kind,selector,kCaseDestinations[selector]};
}

// The original x86 code indexes 27 byte selectors, then jumps through 13
// original pointer table entries. This host callback deliberately reports
// the selected case only, rather than pretending to execute 36 native
// specialist helper implementations.
template<class Host>
void dispatch_case(const RouteInput& input,Host& host) {
    const auto route=classify(input);
    switch(route.kind) {
    case RouteKind::filtered:host.on_filtered(input);break;
    case RouteKind::no_action:host.on_no_action(input);break;
    case RouteKind::specialist:
        host.on_opaque_specialist(input,route.case_index,route.original_branch_va);
        break;
    case RouteKind::nested_records:
        host.on_opaque_nested_record(input,route.original_branch_va);
        break;
    case RouteKind::warning:
        host.on_native_warning(input,route.original_branch_va);
        break;
    }
}
static_assert(kNativeTypeSelectorVA==kNativeCaseJumpTableVA+13u*4u);
static_assert(kNativeNextFunctionVA==kNativeTypeSelectorVA+27u+5u);
static_assert(kCaseDestinations[10]==0x241997ceu);
static_assert(kTypeCases[13]==11u && kTypeCases[20]==12u);

} // namespace sin_compat::original_client_snapshot_record_router
