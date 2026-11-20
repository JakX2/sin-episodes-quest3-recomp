#pragma once
// Original SiN Episodes 2006 client.dll stat ID -> value-type map,
// privately audited SHA-256-authenticated retail binary 2026-11-20.
//
// CStatSystem's original helper 0x241FD220 looks up [0x2431E660 +
// id*12] as the stat-value factory discriminator. Adjacent helpers
// 0x241FD230 and 0x241FD240 access the following two 32-bit values.
// All 66 records are compiled into the retail DLL's initial .data
// image. Their raw 792-byte SHA256:
// 63c1ee30f9b2a021115150b8b98a298530f4f2e0ebd621fca3ff7132d7b80ffb
//
// The last two columns are UNINTERPRETED original 32-bit metadata.
// Do not assign semantic names or use them for state mutations until
// their callers are independently reconstructed and tested.
//
// No original code, game assets or live proprietary pointers here.
#include "client_stats_value_factory_contract.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace sin_independent::stats_original_types {
inline constexpr unsigned kStatCount=66;
inline constexpr std::uint32_t kOriginalMapVA=0x2431e660u;
inline constexpr std::uint32_t kOriginalTypeHelperVA=0x241fd220u;
inline constexpr std::size_t kOriginalRecordBytes=12;

struct Entry final {
    std::uint8_t factory_type;
    std::uint8_t raw_meta_1;
    std::uint8_t raw_meta_2;
};
inline constexpr std::array<Entry,kStatCount> kByStatId{{
    { 7, 1, 1}, // 0
    { 7, 1, 2}, // 1
    { 1, 1, 1}, // 2
    { 7, 1, 1}, // 3
    { 6, 1, 1}, // 4
    { 0, 2, 1}, // 5
    { 0, 2, 1}, // 6
    { 0, 2, 1}, // 7
    { 0, 2, 1}, // 8
    { 5, 3, 1}, // 9
    { 5, 3, 1}, // 10
    { 0, 2, 1}, // 11
    { 0, 2, 1}, // 12
    { 0, 2, 1}, // 13
    { 0, 2, 1}, // 14
    { 0, 2, 1}, // 15
    { 0, 2, 1}, // 16
    { 0, 2, 1}, // 17
    {11, 3, 1}, // 18
    {11, 3, 1}, // 19
    { 9, 0, 2}, // 20
    {10, 0, 2}, // 21
    { 0, 2, 2}, // 22
    { 0, 2, 2}, // 23
    { 7, 1, 1}, // 24
    { 7, 1, 1}, // 25
    { 0, 2, 1}, // 26
    { 9, 0, 1}, // 27
    { 9, 0, 1}, // 28
    { 8, 0, 1}, // 29
    { 8, 0, 1}, // 30
    { 8, 0, 1}, // 31
    { 0, 2, 1}, // 32
    { 0, 2, 1}, // 33
    { 0, 2, 1}, // 34
    { 0, 2, 1}, // 35
    { 0, 2, 1}, // 36
    { 0, 2, 1}, // 37
    { 0, 0, 1}, // 38
    { 0, 2, 1}, // 39
    { 0, 2, 1}, // 40
    { 0, 2, 1}, // 41
    { 0, 2, 1}, // 42
    { 0, 2, 1}, // 43
    { 0, 2, 1}, // 44
    {14, 2, 2}, // 45
    { 2, 3, 1}, // 46
    { 2, 2, 1}, // 47
    { 1, 3, 1}, // 48
    { 1, 3, 1}, // 49
    {14, 2, 1}, // 50
    { 0, 3, 1}, // 51
    { 0, 2, 1}, // 52
    { 0, 2, 1}, // 53
    { 0, 2, 2}, // 54
    { 5, 3, 1}, // 55
    { 0, 2, 1}, // 56
    { 0, 2, 1}, // 57
    { 0, 2, 1}, // 58
    { 5, 3, 1}, // 59
    { 0, 2, 1}, // 60
    { 0, 2, 1}, // 61
    { 0, 2, 1}, // 62
    { 0, 2, 2}, // 63
    { 2, 3, 1}, // 64
    {11, 0, 1} // 65
}};
constexpr const Entry* TryGet(unsigned statId) noexcept {
    return statId<kStatCount?&kByStatId[statId]:nullptr;
}
constexpr int FactoryTypeForStatId(unsigned statId) noexcept {
    return statId<kStatCount?
        static_cast<int>(kByStatId[statId].factory_type):-1;
}
constexpr bool VerifyKnownValues() noexcept {
    if(kByStatId.size()!=66)return false;
    for(unsigned i=0;i<kStatCount;++i) {
        if(!stats_value_types::IsKnown(kByStatId[i].factory_type) ||
           kByStatId[i].raw_meta_1>3 ||
           kByStatId[i].raw_meta_2<1 ||
           kByStatId[i].raw_meta_2>2)return false;
    }
    return FactoryTypeForStatId(0)==7 &&
           FactoryTypeForStatId(20)==9 &&
           FactoryTypeForStatId(21)==10 &&
           FactoryTypeForStatId(45)==14 &&
           FactoryTypeForStatId(55)==5 &&
           FactoryTypeForStatId(65)==11;
}
static_assert(VerifyKnownValues(),
              "original SHA-authenticated 66-stat value-type registry");
} // namespace sin_independent::stats_original_types
