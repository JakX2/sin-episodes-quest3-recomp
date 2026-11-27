#include "native_server_recombinant_minor_damage_gate.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <limits>
#include <cmath>
#include <initializer_list>
using namespace sin_port::native_weapon_tables;
struct F {
    alignas(4) unsigned char actor[8]{},info[0x48]{};
    unsigned calls=0;bool blocked=false,fail=false;
    template<class T>void put(void* p,unsigned offset,T v){std::memcpy(static_cast<unsigned char*>(p)+offset,&v,sizeof(v));}
};
int main(int argc,char** argv){
    assert(argc==3);
    auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    const auto get=reinterpret_cast<GetServerRecombinantMinorDamageGateApi>(GetProcAddress(server,server_recombinant_minor_damage_gate_export));
    assert(get&&!GetProcAddress(client,server_recombinant_minor_damage_gate_export)&&!get(2,sizeof(ServerRecombinantMinorDamageGateApi)));
    const auto* api=get(1,sizeof(ServerRecombinantMinorDamageGateApi));assert(api);
    F f;f.put(f.actor,0,recombinant_minor_vtable);
    ServerRecombinantMinorDamageGateBindings b{&f,[](void* context,bool* blocked) noexcept ->int{
        auto& f=*static_cast<F*>(context);++f.calls;*blocked=f.blocked;return !f.fail;
    }};
    ServerRecombinantMinorDamageGateResult o{};
    auto call=[&](float damage){
        f.put(f.info,0x2c,damage);return api->evaluate(f.actor,sizeof(f.actor),f.info,sizeof(f.info),&b,&o);
    };
    assert(call(11.f)&&o.completed&&o.mode_queries==1&&o.damage_read==1&&o.accepted==1&&o.damage==11.f&&o.threshold==10.f&&f.calls==1);
    assert(call(10.f)&&!o.accepted&&o.damage_read&&f.calls==2);
    assert(call(9.999f)&&!o.accepted&&f.calls==3);
    assert(call(1000.f)&&o.accepted&&f.calls==4);
    assert(call(-50.f)&&!o.accepted&&f.calls==5);
    assert(call(std::numeric_limits<float>::quiet_NaN())&&!o.accepted&&o.damage_read);
    f.blocked=true;
    assert(api->evaluate(f.actor,sizeof(f.actor),nullptr,0,&b,&o)&&o.suppressed&&!o.damage_read&&!o.accepted);
    f.blocked=false;assert(!api->evaluate(f.actor,sizeof(f.actor),f.info,0x2f,&b,&o)&&!o.completed);
    f.fail=true;assert(!call(11.f)&&!o.completed);
    f.fail=false;f.put(f.actor,0,recombinant_major_vtable);
    const unsigned prior=f.calls;
    assert(!call(11.f)&&f.calls==prior&&!o.completed);
    assert(!api->evaluate(nullptr,0,nullptr,0,&b,&o));

    // The later Phase 1625 metric export is an ABI-compatible alias of
    // the already reconstructed Phase 1613 Minor +0x398 method.
    // These are not two independent retail methods.
    const auto get_metric=reinterpret_cast<GetServerRecombinantMinorMetricGateApi>(
        GetProcAddress(server,server_recombinant_minor_metric_gate_export));
    assert(get_metric&&!GetProcAddress(client,server_recombinant_minor_metric_gate_export));
    const auto* metric=get_metric(1,sizeof(ServerRecombinantMinorMetricGateApi));
    assert(metric);
    ServerRecombinantMinorMetricGateBindings mb{b.context,b.mode_blocks};
    ServerRecombinantMinorMetricGateResult mo{};
    f.put(f.actor,0,recombinant_minor_vtable);
    f.blocked=f.fail=false;
    for(const float value : {0.f,9.999f,10.f,10.001f,11.f,
                            std::numeric_limits<float>::infinity(),
                            -std::numeric_limits<float>::infinity(),
                            std::numeric_limits<float>::quiet_NaN()}) {
        f.put(f.info,0x2c,value);
        const int original=api->evaluate(f.actor,sizeof(f.actor),f.info,sizeof(f.info),&b,&o);
        const int alias=metric->evaluate(f.actor,sizeof(f.actor),f.info,sizeof(f.info),&mb,&mo);
        assert(original==alias&&o.completed==mo.completed&&
            o.mode_queries==mo.manager_calls&&o.suppressed==mo.suppressed&&
            o.damage_read==mo.compared&&o.accepted==mo.allowed&&
            o.threshold==mo.threshold);
        assert((std::isnan(value)&&std::isnan(o.damage)&&std::isnan(mo.observed))||
               (!std::isnan(value)&&o.damage==mo.observed));
        if(std::isinf(value)&&value>0)assert(o.accepted&&mo.allowed);
    }
    f.blocked=true;
    const int old_short=api->evaluate(f.actor,sizeof(f.actor),nullptr,0,&b,&o);
    const int new_short=metric->evaluate(f.actor,sizeof(f.actor),nullptr,0,&mb,&mo);
    assert(old_short&&new_short&&o.completed&&mo.completed&&
        o.suppressed&&mo.suppressed&&!o.damage_read&&!mo.compared);
    f.blocked=false;f.fail=true;
    assert(!api->evaluate(f.actor,sizeof(f.actor),f.info,sizeof(f.info),&b,&o));
    assert(!metric->evaluate(f.actor,sizeof(f.actor),f.info,sizeof(f.info),&mb,&mo));
    assert(!o.completed&&!mo.completed&&o.mode_queries==mo.manager_calls);
    f.fail=false;f.put(f.actor,0,recombinant_major_vtable);
    const unsigned before=f.calls;
    assert(!api->evaluate(f.actor,sizeof(f.actor),f.info,sizeof(f.info),&b,&o));
    assert(!metric->evaluate(f.actor,sizeof(f.actor),f.info,sizeof(f.info),&mb,&mo));
    assert(f.calls==before&&!o.completed&&!mo.completed);
    FreeLibrary(server);FreeLibrary(client);return 0;
}
