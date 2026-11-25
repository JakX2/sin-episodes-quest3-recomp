#include "native_server_npc_shared_attack.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <cstdio>
#include <vector>
using namespace sin_port::native_weapon_tables;
struct F{
    unsigned char actor[0xc1c]{},desc[0x48]{},component[9]{};
    bool handled=false,aligned=true;float x=1;std::vector<int> trace;
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T>T get(unsigned off){T v{};std::memcpy(&v,actor+off,sizeof(v));return v;}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto c=LoadLibraryA(argv[1]),s=LoadLibraryA(argv[2]);assert(c&&s);
    auto g=reinterpret_cast<GetServerNpcSharedAttackApi>(GetProcAddress(s,server_npc_shared_attack_export));
    assert(g&&!GetProcAddress(c,server_npc_shared_attack_export)&&!g(0,sizeof(ServerNpcSharedAttackApi)));
    const auto* api=g(1,sizeof(ServerNpcSharedAttackApi));assert(api);
    F f;f.put(f.desc,0x34,npc_global_condition_base);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    f.put(f.actor,0x9b0,std::uint16_t(2));f.put(f.actor,0x9b2,std::uint16_t(4));
    f.put(f.actor,0x9b4,1.f);f.put(f.actor,0x9b8,2.f);f.put(f.actor,0x9bc,.1f);f.put(f.actor,0x9c0,.2f);
    ServerNpcSharedAttackBindings b{};b.context=&f;b.shared.context=&f;b.shared.lifecycle.context=&f;
    b.shared.lifecycle.conditions.context=&f;b.shared.lifecycle.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    b.shared.lifecycle.random_int=[](void* p,unsigned lo,unsigned hi,unsigned* out) noexcept -> int{assert(lo==2&&hi==4);*out=3;F::self(p).trace.push_back(100);return 1;};
    b.shared.lifecycle.random_float=[](void* p,float lo,float hi,double* out) noexcept -> int{assert((lo==1&&hi==2)||(lo==.1f&&hi==.2f));*out=lo;F::self(p).trace.push_back(101);return 1;};
    b.shared.pending=[](void* p,void*,int id) noexcept -> int{auto& f=F::self(p);assert(f.get<int>(0x878)!=id);f.trace.push_back(id);return 1;};
    b.shared.turn=[](void* p,void*) noexcept -> int{F::self(p).trace.push_back(102);return 1;};
    b.aim_vector=[](void* p,void*,float* v) noexcept -> int{auto& f=F::self(p);v[0]=f.x;v[1]=v[2]=0;f.trace.push_back(103);return 1;};
    b.aim_aligned=[](void* p,void*,const float*,bool* out) noexcept -> int{auto& f=F::self(p);*out=f.aligned;f.trace.push_back(104);return 1;};
    b.aim_apply=[](void* p,void*,const float*,bool aligned) noexcept -> int{F::self(p).trace.push_back(aligned?105:106);return 1;};
    b.refresh_parameters=[](void* p,void*) noexcept -> int{F::self(p).trace.push_back(107);return 1;};
    b.shared.component_storage=[](void* p,std::uint32_t tok,void** out,unsigned* n) noexcept -> int{assert(tok==77);auto& f=F::self(p);*out=f.component;*n=9;return 1;};
    b.shared.component_task=[](void* p,std::uint32_t,const ServerNpcScheduleTask*,bool) noexcept -> int{auto& f=F::self(p);assert(f.component[8]);f.component[8]=f.handled;f.trace.push_back(108);return 1;};
    unsigned reset=0;
    f.put(f.actor,0x9ae,std::uint16_t(2));f.put(f.actor,0x9a8,1.f);
    assert(api->advance_timer(f.actor,sizeof(f.actor),10,&b.shared.lifecycle,&reset)&&!reset&&f.get<float>(0x9a8)==10&&f.get<std::uint16_t>(0x9ae)==1&&!f.actor[0x9ac]);
    assert(api->advance_timer(f.actor,sizeof(f.actor),10,&b.shared.lifecycle,&reset)&&reset&&f.get<float>(0x9a8)==11&&f.get<std::uint16_t>(0x9ae)==3&&f.actor[0x9ac]);
    ServerNpcSharedAttackResult o{};
    auto run=[&](int id,bool running,float now){f.trace.clear();f.put(f.actor,0x7a4,1);f.put(f.actor,0x7d0,0u);ServerNpcScheduleTask t{id,0};const int ok=api->execute(f.actor,sizeof(f.actor),&t,running,now,&b,&o);if(!ok)std::fprintf(stderr,"attack task %d (%d) failed\n",id,running);assert(ok);};
    const int activity[]{16,17,64,65,66,93,94};
    for(int i=67;i<=73;++i){f.put(f.actor,0x878,activity[i-67]);run(i,false,20);assert(o.activity==activity[i-67]&&f.trace==std::vector<int>{activity[i-67]});}
    f.put(f.actor,0x8ec,0x20000000u);
    f.actor[0x3e0]=1;f.put(f.actor,0x3ec,7);f.put(f.actor,0x884,7);
    run(69,true,21);assert(o.lifecycle.completed&&(f.trace==std::vector<int>{102,103,106}));
    run(71,true,21);assert(o.lifecycle.completed&&o.timer_reset&&f.get<float>(0x9a8)==21&&(f.trace==std::vector<int>{102,103,104,105,100}));
    f.put(f.actor,0x8ec,4u);run(69,true,22);assert(o.lifecycle.completed&&(f.trace==std::vector<int>{102,103}));
    f.put(f.actor,0xc18,77u);f.handled=true;run(67,false,22);assert(o.component_handled&&!o.special&&f.trace==std::vector<int>{108});
    f.handled=false;run(67,false,22);assert(o.component_called&&!o.component_handled&&(f.trace==std::vector<int>{108,16}));
    f.put(f.actor,0x8ec,0x1000000u);f.actor[0x9ac]=1;f.put(f.actor,0x9a8,30.f);
    run(67,false,25);assert(o.special&&o.failed_code==10&&o.lifecycle.failed&&!o.component_called);
    run(67,true,25);assert(o.lifecycle.completed&&o.special);
    f.actor[0x9ac]=0;run(67,false,25);assert(o.activity==72);
    f.put(f.actor,0x9ae,std::uint16_t(1));f.actor[0x1f0]=1;run(67,false,30);
    assert(o.timer_advanced&&o.timer_reset&&o.parameters_refreshed&&o.deadline_changed&&f.actor[0x200]&&f.get<float>(0x60c)==31.f&&o.activity==16&&(f.trace==std::vector<int>{100,101,107,16}));

    auto gp=reinterpret_cast<GetServerNpcAttackParameterApi>(GetProcAddress(s,server_npc_attack_parameter_export));
    assert(gp&&!GetProcAddress(c,server_npc_attack_parameter_export)&&!gp(0,sizeof(ServerNpcAttackParameterApi))&&!gp(1,0));
    const auto* params=gp(1,sizeof(ServerNpcAttackParameterApi));assert(params);
    unsigned char weapon[4]{};
    sin_independent::EntityHandleEntry slots[2]{};slots[1].serial=7;slots[1].entity=reinterpret_cast<CBaseEntity*>(weapon);
    unsigned kind=0;
    b.weapons={slots,2,&kind,[](void* p,void*,unsigned* out) noexcept -> int{*out=*static_cast<unsigned*>(p);return 1;},nullptr};
    f.put(f.actor,0x78c,0x7001u);f.put(f.actor,0xc18,0u);
    const int lows[]{2,1,1,1,1,1,1,4},highs[]{5,3,1,1,1,1,3,7};
    const float delays[]{.075f,.075f,0.f,.4f,.6f,.4f,.7f,.1f};
    ServerNpcAttackParameterResult pr{};
    f.put(f.actor,0x9ae,std::uint16_t(55));f.put(f.actor,0x9a8,99.f);
    for(kind=WeaponActivityAssault;kind<=WeaponActivitySmg;++kind){
        assert(params->refresh(f.actor,sizeof(f.actor),&b.weapons,nullptr,nullptr,&pr));
        const unsigned i=kind-1;
        assert(pr.weapon_class==kind&&pr.lower==static_cast<unsigned>(lows[i])&&pr.upper==static_cast<unsigned>(highs[i])&&pr.short_delay==delays[i]);
        assert(f.get<std::uint16_t>(0x9b0)==lows[i]&&f.get<std::uint16_t>(0x9b2)==highs[i]&&f.get<float>(0x9bc)==delays[i]&&f.get<float>(0x9c0)==delays[i]);
        assert(f.get<float>(0x9b4)==.3f&&f.get<float>(0x9b8)==.6f&&f.get<std::uint16_t>(0x9ae)==55&&f.get<float>(0x9a8)==99.f);
    }
    kind=WeaponActivityAssault;f.put(f.actor,0xc18,77u);
    assert(!params->refresh(f.actor,sizeof(f.actor),&b.weapons,nullptr,nullptr,&pr)&&f.get<float>(0x9bc)==.1f);
    assert(params->refresh(f.actor,sizeof(f.actor),&b.weapons,&f,[](void* p,std::uint32_t token) noexcept -> int{auto& f=F::self(p);assert(token==77&&f.get<float>(0x9bc)==.075f&&f.get<std::uint16_t>(0x9b0)==2);return 1;},&pr)&&pr.component_notified);
    f.put(f.actor,0xc18,0u);f.put(f.actor,0x78c,0x8001u);
    assert(!params->refresh(f.actor,sizeof(f.actor),&b.weapons,nullptr,nullptr,&pr));
    f.put(f.actor,0x78c,0x7001u);
    // Force a new long-delay cycle with the legacy full refresh disabled.
    b.refresh_parameters=nullptr;f.put(f.actor,0x9ae,std::uint16_t(1));f.put(f.actor,0x9a8,40.f);f.actor[0x9ac]=0;
    f.put(f.actor,0x9b0,std::uint16_t(2));f.put(f.actor,0x9b2,std::uint16_t(4));f.put(f.actor,0x9b4,1.f);f.put(f.actor,0x9b8,2.f);
    run(67,false,40);
    assert(o.special&&o.timer_reset&&o.parameters_refreshed&&o.activity==16&&f.get<float>(0x9a8)==41.f&&f.get<float>(0x60c)==41.f&&f.get<float>(0x9bc)==.075f&&f.get<float>(0x9b4)==.3f);
    FreeLibrary(s);FreeLibrary(c);std::puts("PASS native shared attack animation and capability timer routes");
}
