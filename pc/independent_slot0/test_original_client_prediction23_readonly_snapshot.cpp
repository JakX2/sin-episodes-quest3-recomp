// Phase 1658: 24-read synthetic owner original prediction snapshot.
// Fake PE32 memory ONLY. No original DLL, interface, or vcall is executed.
#include "original_client_prediction23_readonly_snapshot.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1658 owner prediction fixture requires native MSVC Windows x86
#endif
namespace snap=sin_independent::original_client_prediction23_readonly_snapshot;
namespace {
constexpr std::uintptr_t kFakeModule=0x52000000u;
struct Word {std::uintptr_t address=0u;std::uint32_t value=0u;bool readable=true;};
struct Memory {
    std::array<Word,24> words{};
    unsigned read_count=0u;
};
Memory invented() noexcept {
    Memory state{};
    state.words[0]={kFakeModule+snap::kSingletonRva,
                    static_cast<std::uint32_t>(kFakeModule+snap::kVtableRva),
                    true};
    for(unsigned i=0u;i<snap::kVirtualCount;++i)
        state.words[i+1u]={
            kFakeModule+snap::kVtableRva+4u*i,
            static_cast<std::uint32_t>(kFakeModule+snap::kTargets[i]),true};
    return state;
}
bool readMemory(void* opaque,std::uintptr_t address,std::uint32_t* word) noexcept {
    auto* memory=static_cast<Memory*>(opaque);
    if(!memory || !word)return false;
    ++memory->read_count;
    for(const auto& record:memory->words) {
        if(record.address==address) {
            if(!record.readable)return false;
            *word=record.value;
            return true;
        }
    }
    return false;
}
snap::Result probe(Memory& memory) noexcept {
    return snap::Observe(kFakeModule,{&memory,&readMemory});
}
int fail(unsigned code,const char* why) {
    std::fprintf(stderr,
        "FAIL Phase1658 original-free 23-slot prediction owner readback "
        "%u: %s\n",code,why);
    return static_cast<int>(code);
}
} // namespace
int main() {
    auto memory=invented();
    const auto verified=probe(memory);
    if(verified.status!=snap::Status::Verified ||
       verified.matched_slots!=23u ||
       verified.expected_slots!=23u ||
       verified.actual_method_invocations!=0u ||
       memory.read_count!=24u)
        return fail(1,"correct 23 virtuals require exactly 24 DWORD reads");
    if(snap::Observe(kFakeModule,{nullptr,nullptr}).status!=
       snap::Status::MissingReader)
        return fail(2,"missing reader was not rejected");
    memory.read_count=0u;
    if(snap::Observe(0u,{&memory,&readMemory}).status!=
       snap::Status::InvalidBase ||
       snap::Observe(0xFFFFFF00u,{&memory,&readMemory}).status!=
       snap::Status::InvalidBase || memory.read_count!=0u)
        return fail(3,"invalid 32-bit module base was dereferenced");
    for(std::size_t i=0u;i<memory.words.size();++i) {
        auto wrong=invented();
        wrong.words[i].value^=0x00000001u;
        const auto got=probe(wrong);
        const auto expected=(i==0)?
            snap::Status::SingletonVptrMismatch:
            snap::Status::VirtualSlotMismatch;
        if(got.status!=expected || got.status==snap::Status::Verified)
            return fail(4,"forged vptr or virtual pointer accepted");
        wrong=invented();
        wrong.words[i].readable=false;
        const auto missing=probe(wrong);
        const auto expected_missing=(i==0)?
            snap::Status::SingletonVptrUnreadable:
            snap::Status::VirtualSlotUnreadable;
        if(missing.status!=expected_missing ||
           missing.matched_slots!=(i==0u?0u:static_cast<unsigned>(i-1u)))
            return fail(5,"unreadable memory accepted/incorrect stop count");
    }
    memory=invented();
    memory.words[0].value=static_cast<std::uint32_t>(
        kFakeModule+snap::kTargets[0]);
    if(probe(memory).status!=snap::Status::SingletonVptrMismatch)
        return fail(6,"retail singleton mistaken for first code pointer");
    // All byte offsets and original target RVAs are pinned constants,
    // never downloaded original code or used to make a virtual call.
    if(snap::kVtableRva!=0x2A363Cu ||
       snap::kSingletonRva!=0x37FA00u ||
       snap::kTargets[0]!=0x195A50u ||
       snap::kTargets[1]!=0x195F80u ||
       snap::kTargets[22]!=0x1977B0u)
        return fail(7,"original prediction identity constant drift");
    std::puts("PASS Phase1658 original-free Win32 x86 owner prediction "
              "observer: 24 exact DWORDs, 23 vtable targets, each-byte "
              "pointer mutation/read failure, singleton/vtable separation, "
              "32-bit overflow refusal and zero retail method invocations");
    return 0;
}
