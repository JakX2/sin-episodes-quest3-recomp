#pragma once
// Authentic original 2006 client.dll 16-case stat-definition factory,
// 14 distinct MSVC RTTI identities. Metadata only: original VAs are
// never executable function pointers in the independently built client.
#include <array>
#include <cstddef>
#include <cstdint>
namespace sin_independent::stats_definition {
struct NativeCase {
    const char* original_class;          // TypeDescriptor undecorated name
    std::uint8_t native_bytes;           // original malloc request
    std::uint32_t case_entry_va;         // jump table target in 2006 DLL
    std::uint32_t native_constructor_va; // 0 means inline constructor
    std::uint32_t original_vtable_va;    // private game image metadata
    std::uint32_t initialization_float_bits; // 0 if not specialized
};
inline constexpr std::array<NativeCase,16> kCases{{
    {"CIntStat",16,0x24205C00u,0,0x242AF384u,0},
    {"CFloatStat",16,0x24205BD9u,0,0x242AF3DCu,0},
    {"CIntOutOfStat",20,0x24205C47u,0x242054E0u,0x242AF434u,0},
    {"CFloatOutOfStat",20,0x24205C27u,0x24205590u,0x242AF48Cu,0},
    {"CPercentageStat",20,0x24205C67u,0x24205620u,0x242AF4E4u,0},
    {"CAccuracyStat",20,0x24205C87u,0x24205690u,0x242AF53Cu,0},
    {"CAverageStat",20,0x24205CA7u,0x242056C0u,0x242AF594u,0},
    {"CTimeStat",20,0x24205CC7u,0x24205720u,0x242AF5ECu,0},
    {"CPerTimeStat",28,0x24205CE7u,0x242057A0u,0x242AF644u,0x3F800000u},
    {"CPerTimeStat",28,0x24205D0Cu,0x242057A0u,0x242AF644u,0x3C888889u},
    {"CPerTimeStat",28,0x24205D31u,0x242057A0u,0x242AF644u,0x3991A2B4u},
    {"CAveragePerTimeStat",32,0x24205D56u,0x24205840u,0x242AF69Cu,0x3F800000u},
    {"CMaxStat",16,0x24205D97u,0x242058D0u,0x242AF6F4u,0},
    {"CMinStat",16,0x24205D7Bu,0x24205980u,0x242AF74Cu,0},
    {"CMaxAccumulatorStat",20,0x24205DCFu,0x24205A30u,0x242AF7A4u,0},
    {"CMinAccumulatorStat",20,0x24205DB3u,0x24205AE0u,0x242AF7FCu,0}
}};
inline constexpr const NativeCase* Describe(std::size_t index) noexcept {
    return index<kCases.size()?&kCases[index]:nullptr;
}
inline constexpr std::size_t kDistinctTypes=14;
} // namespace sin_independent::stats_definition
