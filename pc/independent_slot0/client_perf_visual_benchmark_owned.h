#pragma once
// Phase 485: authentic original 2006 SiN CPerfVisualBenchmark::Init
// descriptor metadata, rebuilt as an INDEPENDENT x86-safe owner.
// Original full 0x44d-byte method: 0x24182800..0x24182c4c, SHA256
// b0d4cc7e256fd830307d51db70757bb857c0951ec220976a4e281c796ae89b51.
// Original 0x24182500 inserts and copies a six-DWORD (24-byte) item.
// These are declarative convar/label strings and raw numeric values,
// NOT original native strings/vtables/pointers/commands being executed.
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace sin_independent::perf_visual_benchmark {
inline constexpr std::uint32_t kOriginalInitVA=0x24182800u;
inline constexpr std::uint32_t kOriginalInsertVA=0x24182500u;
inline constexpr std::uint32_t kOriginalFlagOffset=0x18u;
inline constexpr std::uint32_t kWeightOnePointFive=0x3fc00000u;
inline constexpr std::uint32_t kWeightTen=0x41200000u;
inline constexpr std::size_t kMaxIndependentRecords=64u;

struct Descriptor final {
    std::uint32_t callsite_va;
    const char* command;
    const char* setting_low;
    const char* setting_high;
    const char* label;
    std::uint32_t weight_bits;
};
inline constexpr std::array<Descriptor,22> kOriginalDescriptors{{
    {0x24182838u,"","","","Default",kWeightOnePointFive},
    {0x24182872u,"r_drawdetailprops","0","1","detail props",kWeightOnePointFive},
    {0x241828a2u,"r_drawworld","0","1","world geometry",kWeightOnePointFive},
    {0x241828d2u,"r_drawentities","0","1","entities",kWeightOnePointFive},
    {0x24182902u,"r_3dsky","0","1","3D skybox",kWeightOnePointFive},
    {0x24182932u,"r_drawdecals","0","1","decals",kWeightOnePointFive},
    {0x24182962u,"mat_stub","1","0","material system and below",kWeightOnePointFive},
    {0x2418299au,"mat_viewportscale",".2","1.0","fillrate",kWeightOnePointFive},
    // The original reuses the preceding "fillrate" string for this row.
    // Do NOT invent a corrected name during parity reconstruction.
    {0x241829cau,"r_drawstaticprops","0","1","fillrate",kWeightOnePointFive},
    {0x241829fau,"r_drawbrushmodels","0","1","brush models",kWeightOnePointFive},
    {0x24182a2au,"r_renderoverlayfragment","0","1","overlays",kWeightOnePointFive},
    {0x24182a5au,"r_drawdisp","0","1","displacements",kWeightOnePointFive},
    {0x24182a8au,"r_drawviewmodel","0","1","viewmodel",kWeightOnePointFive},
    {0x24182abau,"cl_drawhud","0","1","hud",kWeightOnePointFive},
    {0x24182aeau,"r_drawparticles","0","1","particles",kWeightOnePointFive},
    {0x24182b1au,"r_drawsprites","0","1","sprites",kWeightOnePointFive},
    {0x24182b4au,"mat_bumpmap","0","1","bump mapping",kWeightTen},
    {0x24182b7au,"mat_specular","0","1","specularity",kWeightTen},
    {0x24182baau,"mat_drawwater","0","1","water",kWeightOnePointFive},
    {0x24182bdau,"r_dynamic","0","1","dynamic lighting",kWeightOnePointFive},
    {0x24182c0au,"r_shadows","0","1","shadows",kWeightOnePointFive},
    {0x24182c3au,"r_drawropes","0","1","ropes",kWeightOnePointFive}
}};
constexpr bool ValidateMetadata() noexcept {
    if(kOriginalDescriptors.size()!=22)return false;
    unsigned ten=0;
    for(std::size_t i=0;i<kOriginalDescriptors.size();++i){
        const auto& row=kOriginalDescriptors[i];
        if(!row.command || !row.setting_low || !row.setting_high ||
           !row.label || row.callsite_va<kOriginalInitVA ||
           row.callsite_va>0x24182c3au ||
           (i && row.callsite_va<=kOriginalDescriptors[i-1].callsite_va) ||
           (row.weight_bits!=kWeightTen &&
            row.weight_bits!=kWeightOnePointFive))return false;
        if(row.weight_bits==kWeightTen)++ten;
    }
    return ten==2;
}
static_assert(ValidateMetadata(),"Original 22 benchmark descriptors");
struct NativeCandidate24 final {
    // Candidate logical six DWORDs: the original helper copies all
    // six, but CPerfVisualBenchmark::Init explicitly initializes only
    // five known slots in the temporary. The sixth is NOT authenticated
    // as a meaningful original constant. Independent policy sets it 0.
    const char* command=nullptr;
    const char* low=nullptr;
    const char* high=nullptr;
    const char* label=nullptr;
    std::uint32_t weight_bits=0;
    std::uint32_t independent_tail_zero=0;
};
#if defined(_WIN32) && defined(_M_IX86)
static_assert(sizeof(NativeCandidate24)==24,"Original 24-byte x86 record");
static_assert(offsetof(NativeCandidate24,weight_bits)==16,
              "Original fifth known field at +0x10");
static_assert(offsetof(NativeCandidate24,independent_tail_zero)==20,
              "Copied but not explicitly source-assigned sixth DWORD");
#endif
enum class Status {
    Ready, CapacityRefused, AllocationRefused, MissingSourceMetadata
};
struct Report final {
    Status result=Status::MissingSourceMetadata;
    std::size_t appended=0;
    std::size_t total=0;
};
class OwnedRegistry final {
public:
    const std::vector<NativeCandidate24>& records() const noexcept {
        return records_;
    }
    bool benchmarkRunning() const noexcept{return benchmark_running_;}
    void Clear() noexcept {
        records_.clear();
        benchmark_running_=false;
    }
    // An independent atomic emulation of the original append sequence,
    // not a call to actual CPerfVisualBenchmark or engine ConVars.
    Report InitializeKnownDescriptors() noexcept {
        if(records_.size()>kMaxIndependentRecords-kOriginalDescriptors.size())
            return {Status::CapacityRefused,0,records_.size()};
        try {
            auto staged=records_;
            staged.reserve(staged.size()+kOriginalDescriptors.size());
            for(const auto& row:kOriginalDescriptors){
                if(!row.command||!row.setting_low||!row.setting_high||
                   !row.label)
                    return {Status::MissingSourceMetadata,0,records_.size()};
                staged.push_back({
                    row.command,row.setting_low,row.setting_high,
                    row.label,row.weight_bits,0u});
            }
            records_.swap(staged);
            // Original 0x24182c41 writes this+0x18 = 0 after all 22
            // insertions and then returns true at 0x24182c46.
            benchmark_running_=false;
            return {Status::Ready,kOriginalDescriptors.size(),records_.size()};
        }catch(...){return {Status::AllocationRefused,0,records_.size()};}
    }
private:
    std::vector<NativeCandidate24> records_{};
    bool benchmark_running_=false;
};
} // namespace sin_independent::perf_visual_benchmark
