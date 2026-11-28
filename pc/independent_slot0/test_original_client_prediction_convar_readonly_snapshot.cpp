// Phase1658: read-only cl_predict ConVar singleton/parent/numeric startup
// evidence, original-free MSVC Win32 x86 fake memory ONLY.
#include "original_client_prediction_convar_readonly_snapshot.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1658 ConVar owner readback requires actual MSVC PE32 x86
#endif
namespace snap=sin_independent::original_client_prediction_convar_snapshot;
namespace {
constexpr std::uintptr_t kFakeImageBase=0x51000000u;
struct Cell {
    std::uintptr_t address=0u;
    std::uint32_t value=0u;
    bool readable=true;
};
struct FakeMemory {
    std::array<Cell,2> cells{};
    unsigned read_count=0u;
};
FakeMemory make() noexcept {
    FakeMemory f{};
    f.cells[0]={kFakeImageBase+snap::kParentSlotRva,
                static_cast<std::uint32_t>(kFakeImageBase+snap::kClPredictRva),
                true};
    f.cells[1]={kFakeImageBase+snap::kValueSlotRva,1u,true};
    return f;
}
bool readFake(void* context,std::uintptr_t address,
              std::uint32_t* result) noexcept {
    auto* memory=static_cast<FakeMemory*>(context);
    if(!memory || !result)return false;
    ++memory->read_count;
    for(const auto& record:memory->cells)
        if(record.address==address) {
            if(!record.readable)return false;
            *result=record.value;
            return true;
        }
    return false;
}
snap::Result probe(FakeMemory& memory) noexcept {
    return snap::Observe(kFakeImageBase,{&memory,&readFake});
}
int fail(unsigned code,const char* why) {
    std::fprintf(stderr,
        "FAIL Phase1658 original-free cl_predict ConVar observation %u: %s\n",
        code,why);
    return static_cast<int>(code);
}
} // namespace
int main() {
    auto f=make();
    const auto result=probe(f);
    if(result.status!=snap::Status::VerifiedSelfParent ||
       !result.parent_is_expected_self ||
       result.numeric_value!=1 ||
       result.successful_reads!=2u || f.read_count!=2u ||
       result.retail_virtual_invocations!=0u)
        return fail(1,"original default cl_predict=1 self-parent fake state");
    f=make();f.cells[1].value=0u;
    if(probe(f).status!=snap::Status::VerifiedSelfParent ||
       probe(f).numeric_value!=0)
        return fail(2,"cl_predict=0 must be a permitted numeric value");
    f=make();f.cells[1].value=0xFFFFFFFFu;
    if(probe(f).numeric_value!=-1)
        return fail(3,"ConVar negative integer must retain signed bits");
    f=make();f.cells[0].value=0u;
    if(probe(f).status!=snap::Status::ParentNull || f.read_count!=1u)
        return fail(4,"null original ConVar parent dereference refused");
    f=make();f.cells[0].value=0x43000000u;
    if(probe(f).status!=snap::Status::ParentAliasesExternal ||
       f.read_count!=1u)
        return fail(5,"external ConVar alias must never be dereferenced");
    for(unsigned i=0u;i<2u;++i) {
        f=make();
        f.cells[i].readable=false;
        const auto missing=probe(f);
        const auto expected=(i==0)?
            snap::Status::ParentUnreadable:
            snap::Status::NumericValueUnreadable;
        if(missing.status!=expected ||
           missing.successful_reads!=i)
            return fail(6,"unreadable original ConVar field not refused");
    }
    f=make();f.read_count=0;
    if(snap::Observe(0u,{&f,&readFake}).status!=
           snap::Status::InvalidBase ||
       snap::Observe(0xFFFFFF00u,{&f,&readFake}).status!=
           snap::Status::InvalidBase ||
       snap::Observe(kFakeImageBase,{nullptr,nullptr}).status!=
           snap::Status::MissingReader ||
       f.read_count!=0)
        return fail(7,"invalid PE32 base or absent reader not refused");
    f=make();f.cells[0].value^=1u;
    if(probe(f).status!=snap::Status::ParentAliasesExternal)
        return fail(8,"corrupt original m_pParent value accepted");
    std::puts("PASS Phase1658 cl_predict original-free owner ConVar "
              "observation: exact two DWORDs, self parent, 0/1/negative "
              "numeric value, null/external alias refusing dereference, "
              "unreadable data and 32-bit module base overflow guard");
    return 0;
}
