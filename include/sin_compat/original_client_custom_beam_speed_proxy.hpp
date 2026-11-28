#pragma once
// Phase1715: original SiN Episodes (2006) PE32 custom DT_Beam.m_fSpeed
// RecvProxy at 0x2409ED70, complete 44 executable bytes, SHA256:
// 77c9e9200d8c3d401d77f13e979c06d0ce35e4b446a65849a92de10f5e01d8d3
// Source original x87 callback: FLD decoded CRecvProxyData+4 (binary32),
// FMUL original binary64 at 0x2427B050 (bits 0x3FB999999999999A),
// FUCOMPP with owner's binary32 at +0x508; if different, FSTP into +0x508.
// This models round-to-nearest/even x87 64-bit-significand multiplication
// followed by binary32 FSTP using ONLY PORTABLE UNSIGNED INTEGER ARITHMETIC.
// No host floating-point control word, long double, old PE32 x86 pointer,
// or original client's executable method can enter this implementation.
// Original x87 non-default rounding control and NaN exception/quieting
// semantics are unverified: NaN input is deliberately refused.
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace sin_compat::original_client_custom_beam_speed_proxy {
inline constexpr std::uint32_t kNativeSourceVA=0x2409ED70u;
inline constexpr std::uint32_t kNativeScaleConstantVA=0x2427B050u;
inline constexpr std::uint64_t kNativeScaleConstantBits=0x3FB999999999999Aull;
inline constexpr std::uint64_t kNativeScaleIntegerMantissa=0x1999999999999Aull;
inline constexpr int kNativeScaleIntegerExponent=-56;
inline constexpr std::size_t kNativeExecutableBytes=44u;
inline constexpr std::size_t kSourceTableOrdinal=128u;
inline constexpr std::size_t kSourcePropertySlot=14u;
inline constexpr std::size_t kNativeOwnerSpeedOffset=0x508u;
inline constexpr std::size_t kSourceDecodedValueOffset=4u;
inline constexpr std::size_t kSourceAssignments=1u;

struct TwoWordProduct final {
    std::uint64_t low;
    std::uint64_t high;
};
// 24-bit native binary32 mantissa times 53-bit original binary64 0.1
// significand. The exact product has at most 77 bits. Use native UINT64
// carry arithmetic instead of nonportable __int128 (unavailable on Win32).
constexpr TwoWordProduct ExactMantissaProduct(std::uint32_t mantissa) noexcept {
    TwoWordProduct product{0u,0u};
    for(unsigned bit=0u;bit<24u;++bit) {
        if((mantissa&(std::uint32_t{1}<<bit))==0u)continue;
        const auto addLow=kNativeScaleIntegerMantissa<<bit;
        const auto addHigh=bit==0u?0u:
            (kNativeScaleIntegerMantissa>>(64u-bit));
        const auto before=product.low;
        product.low+=addLow;
        product.high+=addHigh+static_cast<std::uint64_t>(product.low<before);
    }
    return product;
}
constexpr unsigned BitLength64(std::uint64_t word) noexcept {
    unsigned n=0u;
    while(word!=0u){++n;word>>=1u;}
    return n;
}
// All shifts are explicitly checked: no 64-bit shift by 64 and no
// signed shift or signed overflow on either Windows x86 or ARM64.
constexpr std::uint64_t RightShiftRoundNearestEven(
    std::uint64_t word,unsigned shift) noexcept {
    if(shift==0u)return word;
    if(shift>64u)return 0u;
    if(shift==64u)return word>0x8000000000000000ull?1u:0u;
    const auto kept=word>>shift;
    const auto mask=(std::uint64_t{1}<<shift)-1u;
    const auto discarded=word&mask;
    const auto half=std::uint64_t{1}<<(shift-1u);
    return kept+static_cast<std::uint64_t>(
        discarded>half || (discarded==half && (kept&1u)!=0u));
}

struct ScaledBinary32 final {
    bool supported;
    std::uint32_t bits;
};
// Original x87 conversion with 64-bit intermediate significand. The
// operation is defined for all finite incoming binary32 values and
// +/-inf; unverified NaN payload/invalid exceptions are NOT fabricated.
constexpr ScaledBinary32 SourceScaleByOriginalBinary64Tenth(
    std::uint32_t sourceBits) noexcept {
    const auto sign=sourceBits&0x80000000u;
    const auto exponent=(sourceBits>>23u)&0xffu;
    const auto fraction=sourceBits&0x007fffffu;
    if(exponent==255u) {
        if(fraction!=0u)return {false,0u};
        return {true,sourceBits};  // +/-Infinity times positive 0.1
    }
    if((sourceBits&0x7fffffffu)==0u)return {true,sourceBits};

    const auto mantissa=exponent==0u?fraction:(0x800000u|fraction);
    const int sourcePower=exponent==0u?-149:static_cast<int>(exponent)-150;
    const auto p=ExactMantissaProduct(mantissa);
    const unsigned n=p.high!=0u?64u+BitLength64(p.high):
        BitLength64(p.low);
    // Normalized 64-bit x87 extended significand and its signed power.
    std::uint64_t x87Mantissa=0u;
    int x87Power=sourcePower+kNativeScaleIntegerExponent+
        static_cast<int>(n)-64;
    if(n<=64u) {
        x87Mantissa=p.low<<(64u-n);
    }else {
        const unsigned removed=n-64u; // at most 13, guaranteed <64
        const auto upper=(p.high<<(64u-removed))|(p.low>>removed);
        const auto discarded=p.low&((std::uint64_t{1}<<removed)-1u);
        const auto half=std::uint64_t{1}<<(removed-1u);
        const bool increment=discarded>half ||
            (discarded==half && (upper&1u)!=0u);
        if(increment && upper==0xffffffffffffffffull) {
            x87Mantissa=0x8000000000000000ull;
            ++x87Power;
        }else {
            x87Mantissa=upper+static_cast<std::uint64_t>(increment);
        }
    }
    int topExponent=x87Power+63;
    if(topExponent>=-126) {
        auto binary32Mantissa=RightShiftRoundNearestEven(x87Mantissa,40u);
        if(binary32Mantissa==0x1000000u) {
            binary32Mantissa>>=1u;
            ++topExponent;
        }
        if(topExponent>127)return {true,sign|0x7f800000u};
        return {true,sign|(
            (static_cast<std::uint32_t>(topExponent+127)<<23u))|
            (static_cast<std::uint32_t>(binary32Mantissa)&0x007fffffu)};
    }
    // Subnormal float32 rounds to integral multiples of 2^-149.
    // At a normal/subnormal crossover rounding may yield 0x00800000.
    const unsigned removed=static_cast<unsigned>(-x87Power-149);
    const auto subnormal=RightShiftRoundNearestEven(x87Mantissa,removed);
    if(subnormal>=0x800000u)return {true,sign|0x00800000u};
    return {true,sign|static_cast<std::uint32_t>(subnormal)};
}

constexpr bool IsOriginalSourceAssignment(std::size_t table,std::size_t slot,
                                           std::uint32_t callbackVA) noexcept {
    return table==kSourceTableOrdinal && slot==kSourcePropertySlot &&
           callbackVA==kNativeSourceVA;
}
enum class Status : std::uint8_t {
    Applied, UnrecognizedSource, RefusedBadOwnedMemory, RefusedUnverifiedNaN
};
inline std::uint32_t ReadDword(const std::uint8_t* src) noexcept {
    return std::uint32_t(src[0])|
           (std::uint32_t(src[1])<<8u)|
           (std::uint32_t(src[2])<<16u)|
           (std::uint32_t(src[3])<<24u);
}
inline void WriteDword(std::vector<std::uint8_t>& obj,std::size_t off,
                       std::uint32_t value) noexcept {
    for(std::size_t i=0u;i<4u;++i)
        obj[off+i]=static_cast<std::uint8_t>(value>>(i*8u));
}
inline Status ApplySourceEffect(
    std::size_t table,std::size_t slot,std::uint32_t callbackVA,
    const std::array<std::uint8_t,16>& decodedProxyData,
    std::vector<std::uint8_t>& originalObjectBytes) noexcept {
    if(!IsOriginalSourceAssignment(table,slot,callbackVA))
        return Status::UnrecognizedSource;
    if(originalObjectBytes.size()<kNativeOwnerSpeedOffset+4u)
        return Status::RefusedBadOwnedMemory;
    const auto sourceBits=ReadDword(
        decodedProxyData.data()+kSourceDecodedValueOffset);
    const auto result=SourceScaleByOriginalBinary64Tenth(sourceBits);
    if(!result.supported)return Status::RefusedUnverifiedNaN;
    const auto prior=ReadDword(originalObjectBytes.data()+
                               kNativeOwnerSpeedOffset);
    // FUCOMPP sees equal +0 and -0: original callback skips FSTP and
    // preserves the PRIOR sign bit. A rounded-to-zero nonzero source,
    // in contrast, is nonzero in the 80-bit x87 comparison and writes.
    if((sourceBits&0x7fffffffu)==0u &&
       (prior&0x7fffffffu)==0u)
        return Status::Applied;
    WriteDword(originalObjectBytes,kNativeOwnerSpeedOffset,result.bits);
    return Status::Applied;
}
inline constexpr bool CanPublishRetailProxyPointers() noexcept {return false;}
static_assert(IsOriginalSourceAssignment(128u,14u,kNativeSourceVA));
static_assert(!IsOriginalSourceAssignment(128u,13u,kNativeSourceVA));
static_assert(kNativeScaleConstantBits==0x3fb999999999999aull);
static_assert(SourceScaleByOriginalBinary64Tenth(0x3f800000u).bits==0x3dcccccdu);
static_assert(SourceScaleByOriginalBinary64Tenth(0xbf800000u).bits==0xbdcccccdu);
static_assert(SourceScaleByOriginalBinary64Tenth(0x00800000u).bits==0x000ccccdu);
static_assert(SourceScaleByOriginalBinary64Tenth(0x7f7fffffu).bits==0x7dccccccu);
static_assert(SourceScaleByOriginalBinary64Tenth(0x00000001u).bits==0u);
static_assert(SourceScaleByOriginalBinary64Tenth(0x80000000u).bits==0x80000000u);
static_assert(!SourceScaleByOriginalBinary64Tenth(0x7fc00001u).supported);
static_assert(!CanPublishRetailProxyPointers());
} // namespace sin_compat::original_client_custom_beam_speed_proxy
