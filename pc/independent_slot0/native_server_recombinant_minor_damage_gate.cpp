#include "native_server_recombinant_minor_damage_gate.h"
#include <cmath>
#include <cstring>
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* p,unsigned at) noexcept {
    T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+at,sizeof(v));return v;
}
int __cdecl evaluate(void* actor,unsigned bytes,const void* info,unsigned info_bytes,
    const ServerRecombinantMinorDamageGateBindings* b,ServerRecombinantMinorDamageGateResult* o) noexcept {
    if(!o)return 0;*o={};o->threshold=10.f;
    if(!actor||bytes<4||rd<unsigned>(actor,0)!=recombinant_minor_vtable||
       !b||!b->mode_blocks)return 0;
    bool blocked=false;++o->mode_queries;
    if(!b->mode_blocks(b->context,&blocked))return 0;
    if(blocked){o->suppressed=o->completed=1;return 1;}
    if(!info||info_bytes<0x30u)return 0;
    o->damage=rd<float>(info,0x2c);o->damage_read=1;
    // The FCOMP + (AH & 0x41) branch accepts strictly greater-than;
    // equal, below and unordered (NaN) all return false.
    // The original x87 strict-greater compare accepts +infinity, rejects
    // NaN and -infinity. Do not impose an extra finite-only filter.
    o->accepted=o->damage>o->threshold;
    o->completed=1;return 1;
}
const ServerRecombinantMinorDamageGateApi api{1,sizeof(ServerRecombinantMinorDamageGateApi),&evaluate};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantMinorDamageGateApi=_SiN_GetServerRecombinantMinorDamageGateApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantMinorDamageGateApi* __cdecl SiN_GetServerRecombinantMinorDamageGateApi(unsigned v,unsigned n) noexcept{
    using namespace sin_port::native_weapon_tables;
    return v==1&&n==sizeof(ServerRecombinantMinorDamageGateApi)?&api:nullptr;
}
