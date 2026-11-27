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
    // Ordered +infinity is greater than 10; unordered NaN is false.
    o->accepted=o->damage>o->threshold;
    o->completed=1;return 1;
}
// Both public version-1 ABIs use this one original-method implementation.
// Preserve each ABI's parameter and result types; do not reinterpret-cast
// unrelated structs even though their current layouts are similar.
int __cdecl evaluate(void* actor,unsigned bytes,const void* info,unsigned info_bytes,
    const ServerRecombinantMinorDamageGateBindings* b,ServerRecombinantMinorDamageGateResult* o) noexcept {
    return evaluate_core(actor,bytes,info,info_bytes,b,o);
}
int __cdecl evaluate_metric(const void* actor,unsigned bytes,const void* info,unsigned info_bytes,
    const ServerRecombinantMinorMetricGateBindings* b,ServerRecombinantMinorMetricGateResult* o) noexcept {
    if(!o)return 0;
    *o={};
    ServerRecombinantMinorDamageGateBindings bindings{};
    if(b){bindings.context=b->context;bindings.mode_blocks=b->manager_suppresses;}
    ServerRecombinantMinorDamageGateResult result{};
    const int ok=evaluate_core(actor,bytes,info,info_bytes,b?&bindings:nullptr,&result);
    o->completed=result.completed;
    o->manager_calls=result.mode_queries;
    o->suppressed=result.suppressed;
    o->compared=result.damage_read;
    o->allowed=result.accepted;
    o->observed=result.damage;
    o->threshold=result.threshold;
    return ok;
}
const ServerRecombinantMinorDamageGateApi api{1,sizeof(ServerRecombinantMinorDamageGateApi),&evaluate};
const ServerRecombinantMinorMetricGateApi metric_api{1,sizeof(ServerRecombinantMinorMetricGateApi),&evaluate_metric};
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
