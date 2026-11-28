#include "sin_compat/original_client_custom_handle_proxy.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace original=sin_compat::original_client_custom_handle_proxy;

// Independently reconstruct the actual original x86 SAR ECX,0x0B,
// SHL ECX,0x0C, AND EAX,0x7FF, OR, with sign-extension explicitly
// encoded as unsigned arithmetic (no implementation-defined C++ shifts).
static std::uint32_t OriginalX86InstructionResult(std::uint32_t raw) {
    if(raw==0x001fffffu)return 0xffffffffu;
    std::uint32_t arithmetic=raw>>11u;
    if((raw&0x80000000u)!=0u)
        arithmetic|=0xffe00000u;
    arithmetic<<=12u;
    return arithmetic|(raw&0x7ffu);
}
static void CheckExact(std::uint32_t raw) {
    assert(original::DecodeSourceHandle(raw)==OriginalX86InstructionResult(raw));
}
int main() {
    static_assert(original::kNativeOriginalMethodBytes==49u);
    static_assert(original::kNativeHandleProxyVA==0x241A8DB0u);
    static_assert(original::kNativeMoveParentProxyVA==0x241A8DF0u);
    static_assert(original::kSourceHandleAssignments==30u);
    static_assert(original::kSourceMoveParentAssignments==5u);
    static_assert(!original::CanPublishRetailProxyPointers());
    assert(original::IsOriginalHandleProxy(0x241A8DB0u));
    assert(original::IsOriginalHandleProxy(0x241A8DF0u));
    assert(!original::IsOriginalHandleProxy(0x24126C90u));
    assert(!original::IsOriginalHandleProxy(0u));

    constexpr std::array<std::uint32_t,22> boundaries{{
        0u,1u,2u,0x7feu,0x7ffu,0x800u,0x801u,
        0xfffu,0x1000u,0x1001u,0x1ffffdu,0x1ffffeu,
        0x1fffffu,0x200000u,0x200001u,0x3fffffu,
        0x7fffffffu,0x80000000u,0x800007ffu,
        0x80000800u,0xfffffffeu,0xffffffffu
    }};
    for(const auto raw:boundaries)CheckExact(raw);
    assert(original::DecodeSourceHandle(0x1fffffu)==0xffffffffu);
    assert(original::DecodeSourceHandle(0x7ffu)==0x7ffu);
    assert(original::DecodeSourceHandle(0x800u)==0x1000u);
    assert(original::DecodeSourceHandle(0x801u)==0x1001u);
    // Source is 11-bit entity index plus 21-bit packed serial; test
    // rollover and invalid-handle exceptional branch exhaustively
    // over 16-bit lower packed values.
    for(std::uint32_t raw=0;raw<65536u;++raw)CheckExact(raw);
    // Deterministic broad high-bit fuzz to include signed-SAR behavior.
    std::uint32_t prng=0x9E3779B9u;
    for(std::uint32_t i=0u;i<100000u;++i) {
        prng^=prng<<13u;
        prng^=prng>>17u;
        prng^=prng<<5u;
        CheckExact(prng);
    }
    assert(!original::CanPublishRetailProxyPointers());
    std::puts("PASS Phase1709 exact 49-byte original handle/moveparent x86 "
              "SAR/SHL/index sentinel semantics, bounded host DWORD only");
    return 0;
}
