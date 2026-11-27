#include "native_server_recombinant_minor_damage_gate.h"
#include <cstring>
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* p,unsigned at) noexcept {
    T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+at,sizeof(v));return v;
}
int evaluate_core(const void* actor,unsigned bytes,const void* info,unsigned info_bytes,
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
    // The retail unordered comparison rejects NaN, but +infinity is greater.
    o->accepted=o->damage>o->threshold;
    o->completed=1;return 1;
}
// Phase 1613 is the canonical implementation of original 2237e8c0.
int __cdecl evaluate(void* actor,unsigned bytes,const void* info,unsigned info_bytes,
    const ServerRecombinantMinorDamageGateBindings* b,ServerRecombinantMinorDamageGateResult* o) noexcept {
    return evaluate_core(actor,bytes,info,info_bytes,b,o);
}
// Phase 1625 recovered the same original method. Preserve its published ABI
// as a compatibility adapter, but never implement the original branch twice.
int __cdecl metric_adapter(const void* actor,unsigned bytes,const void* info,unsigned info_bytes,
    const ServerRecombinantMinorMetricGateBindings* b,ServerRecombinantMinorMetricGateResult* o) noexcept {
    if(!o)return 0;
    const ServerRecombinantMinorDamageGateBindings canonical{
        b?b->context:nullptr,b?b->manager_suppresses:nullptr
    };
    ServerRecombinantMinorDamageGateResult r{};
    const int ok=evaluate_core(actor,bytes,info,info_bytes,b?&canonical:nullptr,&r);
    *o={r.completed,r.mode_queries,r.suppressed,r.damage_read,r.accepted,r.damage,r.threshold};
    return ok;
}
const ServerRecombinantMinorDamageGateApi api{1,sizeof(ServerRecombinantMinorDamageGateApi),&evaluate};
const ServerRecombinantMinorMetricGateApi metric_api{1,sizeof(ServerRecombinantMinorMetricGateApi),&metric_adapter};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantMinorDamageGateApi=_SiN_GetServerRecombinantMinorDamageGateApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantMinorDamageGateApi* __cdecl SiN_GetServerRecombinantMinorDamageGateApi(unsigned v,unsigned n) noexcept{
    using namespace sin_port::native_weapon_tables;
    return v==1&&n==sizeof(ServerRecombinantMinorDamageGateApi)?&api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantMinorMetricGateApi=_SiN_GetServerRecombinantMinorMetricGateApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantMinorMetricGateApi* __cdecl
SiN_GetServerRecombinantMinorMetricGateApi(unsigned version,unsigned size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerRecombinantMinorMetricGateApi)?&metric_api:nullptr;
}
