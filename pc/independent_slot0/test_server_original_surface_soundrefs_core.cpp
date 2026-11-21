// Fully fabricated, independent and deterministic C++ fixture for the
// original 2006 server's ten-field surface sound-reference loop. This
// deliberately does NOT call the original sound cache or real GetString.
#include "server_original_surface_soundrefs_core.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <tuple>
#include <vector>
namespace s=sin_independent::server_original_surface_soundrefs;
static int Fail(int n){std::fprintf(stderr,"phase523 surface sound index test=%d\n",n);return n;}
struct Entry {unsigned surface,field;std::uint16_t value;};
int main(){
    constexpr unsigned kCount=94; // owner engine table count, not fake asset text.
    std::array<s::SoundIndices,kCount> fake{};
    fake[0].stepleft=11;
    fake[0].strainSound=21;
    fake[1].impactSoft=12;
    fake[1].impactHard=13;
    fake[85].rolling=88;
    fake[93].bulletImpact=91;
    std::vector<Entry> order;
    auto get=[&](unsigned id)->const s::SoundIndices* {
        return id<fake.size()?&fake[id]:nullptr;
    };
    auto forward=[&](unsigned surface,unsigned field,std::uint16_t token)->bool {
        order.push_back({surface,field,token});
        return true;
    };
    const auto r=s::Scan(kCount,get,forward);
    if(!r.complete()||r.surfaces!=94||r.slots_examined!=940||
       r.nonzero_ids!=6||r.zero_ids!=934||r.forwarded!=6||order.size()!=6)
        return Fail(1);
    const std::array<Entry,6> expected{{
       {0,0,11},{0,9,21},{1,2,12},{1,3,13},{85,7,88},{93,6,91}
    }};
    for(unsigned i=0;i<expected.size();++i){
        if(order[i].surface!=expected[i].surface||
           order[i].field!=expected[i].field||
           order[i].value!=expected[i].value)return Fail(2);
    }
    if(s::kSlotsPerSurface!=10 || !s::kFieldNames[0] ||
       std::strcmp(s::kFieldNames[0],"stepleft")!=0)
       return Fail(3);
    order.clear();
    const auto shortStop=s::Scan(2u,get,[&](unsigned,unsigned,std::uint16_t){
        return false;});
    if(shortStop.outcome!=s::Outcome::CallbackRefused ||
       shortStop.forwarded!=0||shortStop.nonzero_ids!=1||
       shortStop.slots_examined!=1)return Fail(4);
    const auto noSource=s::Scan(2u,[](unsigned)->const s::SoundIndices*{
        return nullptr;
    },forward);
    if(noSource.outcome!=s::Outcome::MissingData||
       noSource.surfaces!=0||noSource.forwarded!=0)return Fail(5);
    const auto refuse=s::Scan(16385u,get,forward);
    if(refuse.outcome!=s::Outcome::InvalidCount||
       refuse.slots_examined!=0||refuse.forwarded!=0)return Fail(6);
    const auto empty=s::Scan(0u,get,forward);
    if(!empty.complete()||empty.surfaces!=0)return Fail(7);
    if(offsetof(s::SurfaceView,sounds)!=0x2c||
       sizeof(s::SoundIndices)!=20) return Fail(8);
    std::puts("PASS source-exact ten Source 2004 surface sound indices: 94 fabricated surfaces, ordered 10-slot scan, zero skip, bounded-refused inputs, no real audio mutation");
    return 0;
}
