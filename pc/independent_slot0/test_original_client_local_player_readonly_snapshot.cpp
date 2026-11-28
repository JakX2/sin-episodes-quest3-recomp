// Phase1660: original-free native Win32 x86 fake-memory C_BasePlayer
// snapshot tests. Never call original C_BasePlayer, original prediction,
// original CreateInterface, original DllMain, or original engine code.
#include "original_client_local_player_readonly_snapshot.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1660 local-player snapshot test requires native MSVC x86
#endif
namespace snap=sin_independent::original_client_local_player_readonly_snapshot;
namespace {
constexpr std::uintptr_t kInventedClientBase=0x51000000u;
constexpr std::uintptr_t kInventedPlayer=0x56000000u;
constexpr std::uint32_t kInventedVptr=static_cast<std::uint32_t>(
    kInventedClientBase+snap::kOriginalCBasePlayerPrimaryVtableRva);
struct Word {
    std::uintptr_t address=0u;
    std::uint32_t value=0u;
};
struct Fake {
    std::array<Word,8> sequence{};
    std::size_t next_read=0u;
    std::size_t fail_read=0u;
    bool address_mismatch=false;
};
Fake invented() noexcept {
    Fake f{};
    f.sequence={{
        {kInventedClientBase+snap::kPlayerGlobalRva,
         static_cast<std::uint32_t>(kInventedPlayer)},
        {kInventedPlayer,kInventedVptr},
        {kInventedPlayer+snap::kOriginalScalarOffset,0xF5A0C3D2u},
        {kInventedPlayer+snap::kOriginalViewOffset,0x3F800000u},
        {kInventedPlayer+snap::kOriginalViewOffset+4u,0xFF800000u},
        {kInventedPlayer+snap::kOriginalViewOffset+8u,0x7FC01234u},
        {kInventedPlayer,kInventedVptr},
        {kInventedClientBase+snap::kPlayerGlobalRva,
         static_cast<std::uint32_t>(kInventedPlayer)}
    }};
    return f;
}
bool reader(void* opaque,std::uintptr_t address,
            std::uint32_t* result) noexcept {
    auto* f=static_cast<Fake*>(opaque);
    if(!f || !result || f->next_read>=f->sequence.size())
        return false;
    const auto index=f->next_read++;
    if(f->fail_read==index+1u)return false;
    if(f->sequence[index].address!=address) {
        f->address_mismatch=true;
        return false;
    }
    *result=f->sequence[index].value;
    return true;
}
snap::Result probe(Fake& f) noexcept {
    return snap::Observe(kInventedClientBase,{&f,&reader});
}
int fail(unsigned code,const char* why) {
    std::fprintf(stderr,
        "FAIL Phase1660 original-free C_BasePlayer readonly %u: %s\n",
        code,why);
    return static_cast<int>(code);
}
} // namespace
int main() {
    auto f=invented();
    auto got=probe(f);
    if(got.status!=snap::Status::VerifiedExactBasePlayer ||
       got.successful_reads!=8u || f.next_read!=8u ||
       f.address_mismatch || !got.exact_primary_vptr_match ||
       got.scalar!=0xF5A0C3D2u ||
       got.vector_bits[0]!=0x3F800000u ||
       got.vector_bits[1]!=0xFF800000u ||
       got.vector_bits[2]!=0x7FC01234u ||
       got.retail_method_invocations!=0u ||
       got.retail_virtual_invocations!=0u)
        return fail(1,"eight bounded DWORDs with bit-exact view expected");
    if(snap::Observe(kInventedClientBase,{nullptr,nullptr}).status!=
       snap::Status::MissingReader)
        return fail(2,"missing reader was not rejected");
    f=invented();
    if(snap::Observe(0u,{&f,&reader}).status!=
           snap::Status::InvalidModuleBase ||
       snap::Observe(0xFFFFFF00u,{&f,&reader}).status!=
           snap::Status::InvalidModuleBase ||
       f.next_read!=0u)
        return fail(3,"invalid module base unexpectedly accessed memory");
    f=invented();
    f.sequence[0].value=0u;
    got=probe(f);
    if(got.status!=snap::Status::NoLocalPlayer ||
       got.successful_reads!=1u || f.next_read!=1u ||
       got.exact_primary_vptr_match)
        return fail(4,"normal pre-engine no-player state was not recognized");
    f=invented();
    f.sequence[0].value=0x56000001u;
    got=probe(f);
    if(got.status!=snap::Status::InvalidPlayerAddress ||
       got.successful_reads!=1u || f.next_read!=1u)
        return fail(5,"unaligned player pointer not refused");
    f=invented();
    f.sequence[0].value=0xFFFFFFF0u;
    got=probe(f);
    if(got.status!=snap::Status::InvalidPlayerAddress ||
       f.next_read!=1u)
        return fail(6,"32-bit player view span overflow not refused");
    f=invented();
    f.sequence[1].value=0x5F000010u;  // simulated derived/foreign class
    got=probe(f);
    if(got.status!=snap::Status::UnrecognizedDynamicVptr ||
       got.successful_reads!=2u || f.next_read!=2u)
        return fail(7,"unknown/derived primary vptr not read-avoided");
    f=invented();
    f.sequence[6].value=kInventedVptr+4u;
    got=probe(f);
    if(got.status!=snap::Status::VptrChanged ||
       got.successful_reads!=7u || f.next_read!=7u)
        return fail(8,"changed primary vptr accepted as stable");
    f=invented();
    f.sequence[7].value=kInventedPlayer+16u;
    got=probe(f);
    if(got.status!=snap::Status::GlobalChanged ||
       got.successful_reads!=8u || f.next_read!=8u)
        return fail(9,"changed local player pointer accepted as stable");
    for(std::size_t i=1u;i<=8u;++i) {
        f=invented();
        f.fail_read=i;
        got=probe(f);
        const auto expected=
            (i==1u || i==8u)?snap::Status::GlobalUnreadable:
            (i==2u || i==7u)?snap::Status::VptrUnreadable:
            i==3u?snap::Status::ScalarUnreadable:
            snap::Status::VectorUnreadable;
        if(got.status!=expected ||
           got.successful_reads!=i-1u || f.next_read!=i ||
           f.address_mismatch)
            return fail(10,"unreadable original host field not safely refused");
    }
    f=invented();
    f.sequence[2].value=0u;
    f.sequence[3].value=0xFFFFFFFFu;
    f.sequence[4].value=0u;
    f.sequence[5].value=0x80000000u;
    got=probe(f);
    if(got.status!=snap::Status::VerifiedExactBasePlayer ||
       got.scalar!=0u || got.vector_bits[0]!=0xFFFFFFFFu ||
       got.vector_bits[1]!=0u || got.vector_bits[2]!=0x80000000u)
        return fail(11,"zero/negative/float-bitfield payload changed");
    f=invented();
    f.sequence[3].address+=4u;
    got=probe(f);
    if(got.status!=snap::Status::VectorUnreadable ||
       !f.address_mismatch)
        return fail(12,"out-of-order/offset player field not rejected");
    std::puts("PASS Phase1660 MSVC x86 original-free: no-player pre-DllMain "
              "state; exact C_BasePlayer RTTI primary vptr gating; 8 "
              "bounded DWORD reads; 3 raw-bit prediction view words; "
              "runtime pointer/vptr turnover; null, alignment, overflow, "
              "read-failure and foreign-derived object refusals; "
              "zero original game method calls");
    return 0;
}
