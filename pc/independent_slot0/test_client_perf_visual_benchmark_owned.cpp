// Phase 485 Win32 native original CPerfVisualBenchmark descriptor fixture.
// Synthetic only: no engine cvars, no original DLL loading, no rendering.
#include "client_perf_visual_benchmark_owned.h"
#include <cstdio>
#include <cstring>

namespace p=sin_independent::perf_visual_benchmark;
static int Fail(int id){
    std::fprintf(stderr,"original benchmark owned descriptor fixture failed %d\n",id);
    return id;
}
static bool Is(const p::NativeCandidate24& item,const char* command,
               const char* low,const char* high,const char* label,
               std::uint32_t bits){
    return std::strcmp(item.command,command)==0 &&
           std::strcmp(item.low,low)==0 &&
           std::strcmp(item.high,high)==0 &&
           std::strcmp(item.label,label)==0 &&
           item.weight_bits==bits &&
           item.independent_tail_zero==0;
}
int main(){
    static_assert(p::kOriginalDescriptors.size()==22,"original item count");
    static_assert(p::ValidateMetadata(),"source-derived registry");
    static_assert(sizeof(p::NativeCandidate24)==24,"Win32 six DWORDs");
    p::OwnedRegistry registry;
    if(!registry.records().empty()||registry.benchmarkRunning())return Fail(1);
    auto result=registry.InitializeKnownDescriptors();
    if(result.result!=p::Status::Ready ||
       result.appended!=22 || result.total!=22 ||
       registry.records().size()!=22||registry.benchmarkRunning())
        return Fail(2);
    const auto& rows=registry.records();
    if(!Is(rows[0],"","","","Default",p::kWeightOnePointFive) ||
       !Is(rows[1],"r_drawdetailprops","0","1","detail props",
           p::kWeightOnePointFive) ||
       !Is(rows[6],"mat_stub","1","0","material system and below",
           p::kWeightOnePointFive) ||
       !Is(rows[7],"mat_viewportscale",".2","1.0","fillrate",
           p::kWeightOnePointFive) ||
       !Is(rows[8],"r_drawstaticprops","0","1","fillrate",
           p::kWeightOnePointFive) ||
       !Is(rows[13],"cl_drawhud","0","1","hud",
           p::kWeightOnePointFive) ||
       !Is(rows[16],"mat_bumpmap","0","1","bump mapping",p::kWeightTen) ||
       !Is(rows[17],"mat_specular","0","1","specularity",p::kWeightTen) ||
       !Is(rows[21],"r_drawropes","0","1","ropes",
           p::kWeightOnePointFive))return Fail(3);

    // Every row in the original 22-call listing has an exact ordered
    // descriptor; no metadata loss or invented nonzero sixth field.
    unsigned highWeightCount=0;
    for(std::size_t i=0;i<p::kOriginalDescriptors.size();++i){
        const auto& expected=p::kOriginalDescriptors[i];
        const auto& actual=rows[i];
        if(!Is(actual,expected.command,expected.setting_low,
               expected.setting_high,expected.label,expected.weight_bits))
            return Fail(4);
        if(actual.weight_bits==p::kWeightTen)++highWeightCount;
        if(i>0 && expected.callsite_va<=
                   p::kOriginalDescriptors[i-1].callsite_va)
            return Fail(5);
    }
    if(highWeightCount!=2)return Fail(6);

    // Original method performs 22 appends each time it is invoked.
    // Owned cap is independent safety policy; second attempt reaches
    // 44 entries, third would exceed 64 and must publish NOTHING.
    result=registry.InitializeKnownDescriptors();
    if(result.result!=p::Status::Ready ||
       result.total!=44 || result.appended!=22 ||
       registry.records().size()!=44)return Fail(7);
    for(std::size_t i=0;i<22;++i)
        if(!Is(registry.records()[22+i],
               p::kOriginalDescriptors[i].command,
               p::kOriginalDescriptors[i].setting_low,
               p::kOriginalDescriptors[i].setting_high,
               p::kOriginalDescriptors[i].label,
               p::kOriginalDescriptors[i].weight_bits))
            return Fail(8);
    result=registry.InitializeKnownDescriptors();
    if(result.result!=p::Status::CapacityRefused ||
       result.total!=44 || result.appended!=0 ||
       registry.records().size()!=44)return Fail(9);

    // Explicit independent teardown. No engine ConVar writes occur.
    registry.Clear();
    if(!registry.records().empty()||registry.benchmarkRunning())
        return Fail(10);
    result=registry.InitializeKnownDescriptors();
    if(result.result!=p::Status::Ready ||
       registry.records().size()!=22)return Fail(11);

    std::puts("PASS exact 22 original 2006 benchmark names/settings/labels/weights, six-DWORD x86 owned layout, ordered appends, 64-cap rollback and reset");
    return 0;
}
