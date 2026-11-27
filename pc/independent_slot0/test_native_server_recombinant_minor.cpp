#include "native_server_recombinant_minor.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <string>
#include <limits>
#include <limits>
using namespace sin_port::native_weapon_tables;
struct F{
    alignas(4) unsigned char actor[0xc50]{},entity[0x220]{},info[0x50]{},desc[0x48]{};
    sin_independent::EntityHandleEntry entries[2]{};
    std::string calls;int index=3,cls=1;unsigned object_flags=0;
    bool special=false,player=false,same=false;
    const ServerNpcConditionApi* ca=nullptr;ServerNpcConditionMapBindings conditions{};
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T>T get(void* p,unsigned off){T v{};std::memcpy(&v,static_cast<unsigned char*>(p)+off,sizeof(v));return v;}
    bool has(int id){ServerNpcConditionResult r{};assert(ca->test(actor,sizeof(actor),id,NpcConditionCurrent,&conditions,&r)&&r.mapped);return r.value!=0;}
    void cond(int id,bool value){ServerNpcConditionResult r{};assert(ca->store(actor,sizeof(actor),id,NpcConditionCurrent,value,&conditions,&r)&&r.mapped);}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    const auto get=reinterpret_cast<GetServerRecombinantMinorApi>(GetProcAddress(server,server_recombinant_minor_export));
    const auto gc=reinterpret_cast<GetServerNpcConditionApi>(GetProcAddress(server,server_npc_condition_export));
    assert(get&&gc&&!GetProcAddress(client,server_recombinant_minor_export)&&!get(2,sizeof(ServerRecombinantMinorApi)));
    const auto* api=get(1,sizeof(ServerRecombinantMinorApi));F f;f.ca=gc(1,sizeof(ServerNpcConditionApi));assert(api&&f.ca);
    f.put(f.actor,0,recombinant_minor_vtable);f.put(f.actor,0x238,0x7001u);
    f.put(f.actor,0x16c,77u);f.put(f.entity,0x16c,88u);
    f.put(f.desc,0x34,npc_global_condition_base);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    f.conditions.context=&f;f.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept ->int{auto& f=*static_cast<F*>(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    f.entries[1].entity=reinterpret_cast<CBaseEntity*>(f.entity);f.entries[1].serial=7;
    ServerRecombinantPolicyBindings policy{};policy.context=&f;policy.conditions=f.conditions;policy.entries=f.entries;policy.entry_count=2;
    policy.schedule_matches=[](void* p,void*,int id,bool ideal,bool* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(id==80&&ideal);f.calls+='S';*out=f.same;return 1;};
    ServerRecombinantDamageBindings d{&policy,&f};
    d.extent=[](void* p,void* a,unsigned* n) noexcept ->int{auto& f=*static_cast<F*>(p);*n=a==f.entity?sizeof(f.entity):0;return *n!=0;};
    d.classification=[](void* p,void* a,int* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.entity);f.calls+='C';*out=f.cls;return 1;};
    d.color_fade=[](void* p,void*,int x,int y) noexcept ->int{auto& f=*static_cast<F*>(p);assert(x==5&&y==30);f.calls+='F';return 1;};
    d.parent_ignite=[](void* p,void*,const ServerRecombinantIgniteRequest* in) noexcept ->int{auto& f=*static_cast<F*>(p);assert(in->lifetime==30.f&&in->mode==1&&!in->arg2&&!in->arg3&&!in->arg4&&!in->arg5);f.calls+='I';return 1;};
    d.parent_damage=[](void* p,void* a,const void* info,unsigned n,int* out) noexcept ->int{
        auto& f=*static_cast<F*>(p);assert(a==f.actor&&info!=f.info&&n==0x48&&!std::memcmp(info,f.info,n));f.calls+='D';*out=7;return 1;
    };
    ServerRecombinantMinorBindings b{&d,&f,1};
    b.local_player=[](void* p,void** out) noexcept ->int{auto& f=*static_cast<F*>(p);f.calls+='P';*out=f.entity;return 1;};
    b.refresh_velocity=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor);f.calls+='R';f.put(a,0x250,1.f);return 1;};
    b.set_velocity=[](void* p,void* a,const float v[3]) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor);f.calls+='V';std::memcpy(f.actor+0x250,v,12);return 1;};
    b.schedule_touch_think=[](void* p,void* a,float now,int context) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor&&now==10.f&&!context&&f.get<unsigned>(a,0x7c)==0x2237ca30u&&!f.actor[0xc41]);f.calls+='T';return 1;};
    b.entity_index=[](void* p,unsigned token,int* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(token==77||token==88);f.calls+='N';*out=f.index;return 1;};
    b.set_schedule=[](void* p,void*,int id) noexcept ->int{auto& f=*static_cast<F*>(p);assert(id==80);f.calls+='A';return 1;};
    b.parent_gather=[](void* p,void*) noexcept ->int{auto& f=*static_cast<F*>(p);f.calls+='G';f.cond(66,true);return 1;};
    b.npc_or_special=[](void* p,void* a,bool* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.entity&&!f.has(66));f.calls+='Q';*out=f.special;return 1;};
    b.is_player=[](void* p,void* a,bool* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.entity);f.calls+='Y';*out=f.player;return 1;};
    b.object_flags=[](void* p,unsigned token,unsigned* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(token==91);f.calls+='O';*out=f.object_flags;return 1;};
    ServerRecombinantMinorResult o{};
    f.put(f.info,0x24,0x7001u);f.put(f.info,0x28,0x7001u);
    for(unsigned vt:{0x2249f65cu,recombinant_major_vtable,recombinant_minor_vtable}){
        f.put(f.entity,0,vt);assert(api->damage(f.actor,sizeof(f.actor),f.info,sizeof(f.info),10.f,&b,&o)&&o.immune&&o.source_class==10&&f.calls.empty());
    }
    f.put(f.entity,0,0x2249aaa4u);f.cls=1;f.actor[0xc41]=1;f.put(f.info,0,4000.f);f.put(f.actor,0xb8,0x1000u);
    assert(api->damage(f.actor,sizeof(f.actor),f.info,sizeof(f.info),10.f,&b,&o)&&o.local_matches&&o.velocity_refreshed&&!o.detached&&o.impulse[0]==200.f&&o.velocity[0]==201.f&&o.parent_called&&f.calls=="CPRVD");
    f.calls.clear();f.put(f.info,0,5000.f);f.put(f.actor,0xb8,0u);
    assert(api->damage(f.actor,sizeof(f.actor),f.info,sizeof(f.info),10.f,&b,&o)&&o.squared==62500.&&!o.detached&&o.velocity[0]==451.f&&f.calls=="CPVD");
    f.calls.clear();f.put(f.info,0,5001.f);
    assert(api->damage(f.actor,sizeof(f.actor),f.info,sizeof(f.info),10.f,&b,&o)&&o.squared>62500.&&o.detached&&o.think_scheduled&&f.has(67)&&o.velocity[2]==-.01f&&f.calls=="CPVTD");
    f.calls.clear();f.actor[0xc41]=1;f.put(f.info,0x28,0x8001u);
    assert(api->damage(f.actor,sizeof(f.actor),f.info,sizeof(f.info),10.f,&b,&o)&&!o.attacker_live&&!o.local_matches&&!o.velocity_set&&f.calls=="CPD");
    f.calls.clear();b.max_clients=2;assert(api->damage(f.actor,sizeof(f.actor),f.info,sizeof(f.info),10.f,&b,&o)&&o.local_matches&&o.detached&&f.calls=="CVTD");
    f.calls.clear();f.actor[0xc41]=0;f.put(f.info,0x38,8u);f.put(f.info,0x24,0x8001u);
    assert(api->damage(f.actor,sizeof(f.actor),f.info,sizeof(f.info),10.f,&b,&o)&&o.ignited&&!o.color_called&&f.calls=="ID");
    f.actor[0xbf]=8;f.put(f.actor,0x9c,1);f.calls.clear();
    assert(api->damage(f.actor,sizeof(f.actor),f.info,sizeof(f.info),10.f,&b,&o)&&o.color_called&&o.schedule_set&&o.ignited&&f.calls=="FNSAID");
    f.same=true;f.calls.clear();assert(api->damage(f.actor,sizeof(f.actor),f.info,sizeof(f.info),10.f,&b,&o)&&!o.schedule_set&&f.calls=="FNSID");
    f.index=2;f.calls.clear();assert(api->damage(f.actor,sizeof(f.actor),f.info,sizeof(f.info),10.f,&b,&o)&&!o.schedule_tests&&f.calls=="FNID");
    f.index=3;f.put(f.actor,0x9c,2);f.calls.clear();assert(api->damage(f.actor,sizeof(f.actor),f.info,sizeof(f.info),10.f,&b,&o)&&!o.index_called&&f.calls=="FID");
    f.calls.clear();assert(api->gather(f.actor,sizeof(f.actor),&b,&o)&&o.ground_live&&!o.index_called&&!f.has(66)&&f.calls=="G");
    f.actor[0xbc]=1;f.index=0;f.calls.clear();assert(api->gather(f.actor,sizeof(f.actor),&b,&o)&&!o.ground_accepted&&!f.has(66)&&f.calls=="GN");
    f.index=3;f.special=true;f.calls.clear();assert(api->gather(f.actor,sizeof(f.actor),&b,&o)&&o.ground_accepted&&f.has(66)&&f.calls=="GNQ");
    f.special=false;f.player=true;f.calls.clear();assert(api->gather(f.actor,sizeof(f.actor),&b,&o)&&f.has(66)&&f.calls=="GNQY");
    f.player=false;f.put(f.entity,0x218,91u);f.object_flags=4;f.calls.clear();assert(api->gather(f.actor,sizeof(f.actor),&b,&o)&&f.has(66)&&f.calls=="GNQYO");
    f.object_flags=0;f.calls.clear();assert(api->gather(f.actor,sizeof(f.actor),&b,&o)&&!f.has(66)&&f.calls=="GNQYO");
    f.put(f.actor,0x238,0x8001u);f.calls.clear();assert(api->gather(f.actor,sizeof(f.actor),&b,&o)&&!o.ground_live&&!f.has(66)&&f.calls=="G");
    assert(!api->damage(f.actor,sizeof(f.actor),f.info,0x47,10.f,&b,&o)&&!o.completed);

    // Original Minor virtual +0x1d4: return actor origin with a fixed
    // +6 Z, refreshing the dirty origin before reading any coordinate.
    const auto get_point=reinterpret_cast<GetServerRecombinantMinorOriginPointApi>(
        GetProcAddress(server,server_recombinant_minor_origin_point_export));
    assert(get_point&&!GetProcAddress(client,server_recombinant_minor_origin_point_export)&&
        !get_point(0,sizeof(ServerRecombinantMinorOriginPointApi))&&
        !get_point(1,sizeof(ServerRecombinantMinorOriginPointApi)-1));
    const auto* point_api=get_point(1,sizeof(ServerRecombinantMinorOriginPointApi));
    assert(point_api);
    ServerRecombinantMinorOriginPointBindings pb{&f,
        [](void* p,void* actor) noexcept ->int{
            auto& fixture=*static_cast<F*>(p);
            assert(actor==fixture.actor);
            fixture.calls+='O';
            fixture.put(actor,0x2bc,1.5f);
            fixture.put(actor,0x2c0,2.5f);
            fixture.put(actor,0x2c4,-8.f);
            fixture.put(actor,0xb8,0u);
            return 1;
        }};
    ServerRecombinantMinorOriginPointResult po{};
    float point[3]{77.f,88.f,99.f};
    f.put(f.actor,0,recombinant_minor_vtable);
    f.put(f.actor,0xb8,0u);
    f.put(f.actor,0x2bc,10.f);f.put(f.actor,0x2c0,-20.f);
    f.put(f.actor,0x2c4,30.5f);f.calls.clear();
    assert(point_api->point(f.actor,sizeof(f.actor),point,&pb,&po)&&
        po.completed&&!po.refresh_calls&&f.calls.empty()&&
        po.origin[0]==10.f&&po.origin[1]==-20.f&&po.origin[2]==30.5f&&
        point[0]==10.f&&point[1]==-20.f&&point[2]==36.5f&&
        po.point[0]==point[0]&&po.point[1]==point[1]&&po.point[2]==point[2]);
    // No Source service needed on clean origin, even without bindings.
    assert(point_api->point(f.actor,0x2c8,point,nullptr,&po)&&po.completed&&!po.refresh_calls);
    f.put(f.actor,0xb8,0x800u);f.calls.clear();
    assert(point_api->point(f.actor,sizeof(f.actor),point,&pb,&po)&&
        po.completed&&po.refresh_calls==1&&f.calls=="O"&&
        po.origin[0]==1.5f&&po.origin[1]==2.5f&&po.origin[2]==-8.f&&
        point[0]==1.5f&&point[1]==2.5f&&point[2]==-2.f);
    // A failed refresh never publishes partial output; exact extent.
    f.put(f.actor,0xb8,0x800u);f.calls.clear();
    ServerRecombinantMinorOriginPointBindings bad_pb{&f,
        [](void* p,void* actor) noexcept ->int{
            auto& fixture=*static_cast<F*>(p);
            assert(actor==fixture.actor);fixture.calls+='F';return 0;
        }};
    point[0]=77.f;point[1]=88.f;point[2]=99.f;
    assert(!point_api->point(f.actor,sizeof(f.actor),point,&bad_pb,&po)&&
        !po.completed&&po.refresh_calls==1&&f.calls=="F"&&
        point[0]==77.f&&point[1]==88.f&&point[2]==99.f);
    f.calls.clear();
    assert(!point_api->point(f.actor,sizeof(f.actor),point,nullptr,&po)&&
        !po.completed&&po.refresh_calls==1&&f.calls.empty());
    assert(!point_api->point(f.actor,0x2c7,point,&pb,&po)&&
        !po.completed&&!po.refresh_calls&&f.calls.empty());
    f.put(f.actor,0xb8,0u);
    const auto qnan=std::numeric_limits<float>::quiet_NaN();
    f.put(f.actor,0x2c4,qnan);
    assert(!point_api->point(f.actor,sizeof(f.actor),point,&pb,&po)&&
        !po.completed&&point[0]==77.f&&point[1]==88.f&&point[2]==99.f);
    f.put(f.actor,0x2c4,3.f);f.put(f.actor,0,recombinant_major_vtable);
    assert(!point_api->point(f.actor,sizeof(f.actor),point,&pb,&po)&&
        !po.completed&&point[0]==77.f);
    f.put(f.actor,0,recombinant_minor_vtable);
    assert(!point_api->point(f.actor,sizeof(f.actor),nullptr,&pb,&po)&&!po.completed);
    assert(!point_api->point(f.actor,sizeof(f.actor),point,&pb,nullptr));

    // Original Minor virtual +0x398 (2237e8c0): query manager +170,
    // short-circuit on true, otherwise accept candidate +2c > 10.f.
    const auto get_metric=reinterpret_cast<GetServerRecombinantMinorMetricGateApi>(
        GetProcAddress(server,server_recombinant_minor_metric_gate_export));
    assert(get_metric&&!GetProcAddress(client,server_recombinant_minor_metric_gate_export)&&
        !get_metric(0,sizeof(ServerRecombinantMinorMetricGateApi))&&
        !get_metric(1,sizeof(ServerRecombinantMinorMetricGateApi)-1));
    const auto* metric=get_metric(1,sizeof(ServerRecombinantMinorMetricGateApi));
    assert(metric);
    struct MetricHost {bool suppress=false,fail=false;unsigned calls=0;} mh;
    ServerRecombinantMinorMetricGateBindings mb{&mh,
        [](void* p,bool* value) noexcept ->int{
            auto& host=*static_cast<MetricHost*>(p);
            assert(value);
            ++host.calls;
            *value=host.suppress;
            return !host.fail;
        }};
    ServerRecombinantMinorMetricGateResult mo{};
    f.put(f.actor,0,recombinant_minor_vtable);
    auto evaluate=[&](float value) {
        f.put(f.entity,0x2c,value);
        return metric->evaluate(f.actor,sizeof(f.actor),f.entity,sizeof(f.entity),&mb,&mo);
    };
    assert(evaluate(11.f)&&mo.completed&&mo.manager_calls==1&&
        !mo.suppressed&&mo.compared==1&&mo.allowed==1&&
        mo.observed==11.f&&mo.threshold==10.f&&mh.calls==1);
    assert(evaluate(10.f)&&mo.completed&&!mo.allowed&&mo.compared==1&&mh.calls==2);
    assert(evaluate(9.999f)&&mo.completed&&!mo.allowed&&mh.calls==3);
    assert(evaluate(10.001f)&&mo.completed&&mo.allowed&&mh.calls==4);
    // FCOMP unordered and infinite inputs retain original result semantics.
    assert(evaluate(std::numeric_limits<float>::quiet_NaN())&&
        mo.completed&&mo.compared&&!mo.allowed);
    assert(evaluate(std::numeric_limits<float>::infinity())&&
        mo.completed&&mo.allowed);
    assert(evaluate(-std::numeric_limits<float>::infinity())&&
        mo.completed&&!mo.allowed);
    mh.suppress=true;const auto before=mh.calls;
    // The global manager short-circuits even a null/invalid candidate.
    assert(metric->evaluate(f.actor,sizeof(f.actor),nullptr,0,&mb,&mo)&&
        mo.completed&&mo.suppressed&&!mo.compared&&!mo.allowed&&
        mo.manager_calls==1&&mh.calls==before+1);
    mh.suppress=false;
    assert(!metric->evaluate(f.actor,sizeof(f.actor),f.entity,0x2f,&mb,&mo)&&
        !mo.completed&&mo.manager_calls==1&&!mo.compared);
    assert(!metric->evaluate(f.actor,sizeof(f.actor),nullptr,0,&mb,&mo)&&
        !mo.completed&&mo.manager_calls==1&&!mo.compared);
    mh.fail=true;
    assert(!evaluate(20.f)&&!mo.completed&&mo.manager_calls==1&&!mo.compared);
    mh.fail=false;
    const auto untouched=mh.calls;
    f.put(f.actor,0,recombinant_major_vtable);
    assert(!evaluate(20.f)&&!mo.completed&&mh.calls==untouched);
    f.put(f.actor,0,recombinant_minor_vtable);
    assert(!metric->evaluate(f.actor,3,f.entity,sizeof(f.entity),&mb,&mo)&&
        !mo.completed&&mh.calls==untouched);
    auto missing=mb;missing.manager_suppresses=nullptr;
    assert(!metric->evaluate(f.actor,sizeof(f.actor),f.entity,sizeof(f.entity),&missing,&mo)&&
        !mo.completed&&mh.calls==untouched);
    assert(!metric->evaluate(f.actor,sizeof(f.actor),f.entity,sizeof(f.entity),&mb,nullptr));
    FreeLibrary(server);FreeLibrary(client);return 0;
}
