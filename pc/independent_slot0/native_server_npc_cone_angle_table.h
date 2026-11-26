#pragma once
#include <cstdint>
#include <cstring>
#include <limits>
namespace sin_port::cone_distance_math {
// Original 22268910: fild i; fmul qword 22454578 (2*pi/256);
// fsin; fstp float into 225818a8[i]. Quarter-wave data reconstructed
// from that instruction sequence, not a table copied from the original DLL.
// Symmetry was checked against all 256 outputs at PC=53 and PC=64.
inline constexpr std::uint32_t sine_quarter[65]{
    0x00000000u,0x3cc90ab0u,0x3d48fb30u,0x3d96a905u,0x3dc8bd36u,0x3dfab273u,0x3e164083u,0x3e2f10a2u,
    0x3e47c5c2u,0x3e605c13u,0x3e78cfccu,0x3e888e93u,0x3e94a031u,0x3ea09ae5u,0x3eac7cd4u,0x3eb8442au,
    0x3ec3ef15u,0x3ecf7bcau,0x3edae880u,0x3ee63375u,0x3ef15aeau,0x3efc5d27u,0x3f039c3du,0x3f08f59bu,
    0x3f0e39dau,0x3f13682au,0x3f187fc0u,0x3f1d7fd1u,0x3f226799u,0x3f273656u,0x3f2beb4au,0x3f3085bbu,
    0x3f3504f3u,0x3f396842u,0x3f3daef9u,0x3f41d870u,0x3f45e403u,0x3f49d112u,0x3f4d9f02u,0x3f514d3du,
    0x3f54db31u,0x3f584853u,0x3f5b941au,0x3f5ebe05u,0x3f61c598u,0x3f64aa59u,0x3f676bd8u,0x3f6a09a7u,
    0x3f6c835eu,0x3f6ed89eu,0x3f710908u,0x3f731447u,0x3f74fa0bu,0x3f76ba07u,0x3f7853f8u,0x3f79c79du,
    0x3f7b14beu,0x3f7c3b28u,0x3f7d3aacu,0x3f7e1324u,0x3f7ec46du,0x3f7f4e6du,0x3f7fb10fu,0x3f7fec43u,
    0x3f800000u
};
inline constexpr std::uint32_t sine_bits(unsigned index) noexcept {
    index&=255u;
    // The original finite double pi approximation and FSIN produce a
    // positive residual, not mathematical zero, at index 128.
    if(index==128)return 0x250d3000u;
    const unsigned j=index&127u;
    return sine_quarter[j<=64?j:128-j]^(index>=128?0x80000000u:0u);
}
inline constexpr std::uint32_t sine_digest() noexcept {
    std::uint32_t hash=2166136261u;
    for(unsigned i=0;i<256;++i)hash=(hash^sine_bits(i))*16777619u;
    return hash;
}
static_assert(sine_digest()==0xccd98925u,"original sine table witness");
inline float sine_lookup(unsigned index) noexcept {
    static_assert(sizeof(float)==4&&std::numeric_limits<float>::is_iec559,"IEEE-754 binary32 required");
    const auto bits=sine_bits(index);float out;std::memcpy(&out,&bits,4);return out;
}
}
