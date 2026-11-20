// Public source-free MSVC native regression for original SiN Stats
// value-type factory and its +0x4c reader dispatch indirections.
// This proves exact reconstructed metadata, NOT message value decoding.
#include "client_stats_value_factory_contract.h"
#include <array>
#include <cstdint>
#include <cstdio>

namespace types=sin_independent::stats_value_types;
static int Fail(int code) {
    std::fprintf(stderr,"Stats value factory contract failure: %d\n",code);
    return code;
}
int main() {
    if(types::kTypeCount!=16 || !types::CheckTable() ||
       types::kOriginalFactoryVA!=0x24205bc0u ||
       types::kOriginalJumpTableVA!=0x24205df0u ||
       types::kValueVirtualOffset!=0x4cu)
        return Fail(1);

    constexpr std::array<std::uint8_t,16> readerOffsets{{
        0x24,0x20,0x14,0x10,0x0c,0x0c,0x08,0x1c,
        0x18,0x18,0x18,0x04,0x20,0x20,0x20,0x20
    }};
    constexpr std::array<unsigned,16> allocation{{
        16,16,20,20,20,20,20,20,
        28,28,28,32,16,16,20,20
    }};
    constexpr std::array<std::uint32_t,16> valueUpdates{{
        0x24205420u,0x242054c0u,0x24205580u,0x24205600u,
        0x24205670u,0x242056b0u,0x24205710u,0x24205780u,
        0x24205820u,0x24205820u,0x24205820u,0x242058c0u,
        0x24205960u,0x24205a10u,0x24205960u,0x24205a10u
    }};
    for(unsigned index=0;index<types::kTypeCount;++index){
        const auto* descriptor=types::TryGet(index);
        std::uint8_t slot=0xff;
        if(!descriptor || !types::IsKnown(index) ||
           descriptor->type!=index ||
           descriptor->allocation_bytes!=allocation[index] ||
           descriptor->reader_slot_offset!=readerOffsets[index] ||
           descriptor->value_update_va!=valueUpdates[index] ||
           !types::ResolveReaderSlot(index,slot) ||
           slot!=readerOffsets[index])
            return Fail(2);
    }
    for(unsigned invalid:{16u,17u,255u,65535u}){
        std::uint8_t slot=0;
        if(types::IsKnown(invalid) || types::TryGet(invalid) ||
           types::ResolveReaderSlot(invalid,slot) || slot!=0xff ||
           types::SameReaderOperation(0,invalid))
            return Fail(3);
    }
    // Cases 8..10 share the same virtual method but each pushes a
    // DIFFERENT literal parameter at factory construction.
    if(types::kCases[8].value_vtable_va!=types::kCases[9].value_vtable_va ||
       types::kCases[9].value_vtable_va!=types::kCases[10].value_vtable_va ||
       types::kCases[8].constructor_va!=types::kCases[10].constructor_va ||
       types::kCases[8].config_bits!=0x3f800000u ||
       types::kCases[9].config_bits!=0x3c888889u ||
       types::kCases[10].config_bits!=0x3991a2b4u)
        return Fail(4);
    // Same underlying reader virtual slot alone is NOT a value-format
    // proof. The original class type and config remain independent.
    if(!types::SameReaderOperation(0,0) ||
       !types::SameReaderOperation(4,5) ||
       !types::SameReaderOperation(8,9) ||
       !types::SameReaderOperation(12,15) ||
       types::SameReaderOperation(7,8) ||
       types::SameReaderOperation(8,11))
        return Fail(5);
    // The original code returned a null for unsupported factory
    // discriminants. NEVER forward them to an arbitrary virtual call.
    std::uint8_t slot=0;
    if(types::ResolveReaderSlot(0xFFFFFFFFu,slot) ||
       slot!=0xff)return Fail(6);
    std::puts("PASS 16 SHA-grounded original SiN stat-value factory cases, +0x4c thunk targets and reader-slot offsets");
    return 0;
}
