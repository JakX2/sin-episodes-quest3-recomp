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

    // Both published names represent the very same original 0x2237e8c0.
    // Compare every result member and callback count, including failures.
    const auto get_metric=reinterpret_cast<GetServerRecombinantMinorMetricGateApi>(
        GetProcAddress(server,server_recombinant_minor_metric_gate_export));
    assert(get_metric&&!GetProcAddress(client,server_recombinant_minor_metric_gate_export));
    assert(!get_metric(0,sizeof(ServerRecombinantMinorMetricGateApi)));
    assert(!get_metric(1,sizeof(ServerRecombinantMinorMetricGateApi)-1));
    const auto* metric=get_metric(1,sizeof(ServerRecombinantMinorMetricGateApi));
    assert(metric);
    ServerRecombinantMinorMetricGateBindings mb{b.context,b.mode_blocks};
    auto compare=[&](const void* input,unsigned extent,
                     bool blocked,bool fail,unsigned actor_bytes,bool valid_actor) {
        f.put(f.actor,0,valid_actor?recombinant_minor_vtable:recombinant_major_vtable);
        f.blocked=blocked;f.fail=fail;
        ServerRecombinantMinorDamageGateResult d{};
        ServerRecombinantMinorMetricGateResult m{};
        const unsigned before=f.calls;
        const int dr=api->evaluate(f.actor,actor_bytes,input,extent,&b,&d);
        const unsigned called=f.calls-before;
        const unsigned before_metric=f.calls;
        const int mr=metric->evaluate(f.actor,actor_bytes,input,extent,&mb,&m);
        assert(dr==mr&&called==f.calls-before_metric);
        assert(d.completed==m.completed&&d.mode_queries==m.manager_calls&&
               d.suppressed==m.suppressed&&d.damage_read==m.compared&&
               d.accepted==m.allowed&&d.threshold==m.threshold);
        // Compare float representations, including unordered NaN payloads.
        assert(std::memcmp(&d.damage,&m.observed,sizeof(float))==0);
        return d;
    };
    for(float sample:{-100.f,9.999f,10.f,10.001f,100.f,
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity()}) {
        f.put(f.info,0x2c,sample);
        const auto d=compare(f.info,sizeof(f.info),false,false,sizeof(f.actor),true);
        assert(d.completed&&d.damage_read&&d.accepted==(sample>10.f));
    }
    assert(compare(nullptr,0,true,false,sizeof(f.actor),true).suppressed);
    assert(!compare(f.info,0x2f,false,false,sizeof(f.actor),true).completed);
    assert(!compare(nullptr,0,false,false,sizeof(f.actor),true).completed);
    assert(!compare(f.info,sizeof(f.info),false,true,sizeof(f.actor),true).completed);
    assert(!compare(f.info,sizeof(f.info),false,false,3,true).mode_queries);
    assert(!compare(f.info,sizeof(f.info),false,false,sizeof(f.actor),false).mode_queries);
    FreeLibrary(server);FreeLibrary(client);return 0;
}
