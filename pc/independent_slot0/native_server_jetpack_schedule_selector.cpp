#include "native_server_jetpack_schedule_selector.h"
#include <cstring>
#include <cmath>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcConditionApi* __cdecl SiN_GetServerNpcConditionApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcBehaviorSelectionApi* __cdecl SiN_GetServerNpcBehaviorSelectionApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerGruntScheduleSelectorApi* __cdecl SiN_GetServerGruntScheduleSelectorApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* p,unsigned off) noexcept{T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
int __cdecl select(void* a,unsigned n,float now,const ServerJetpackSelectorBindings* b,ServerJetpackSelectorResult* out) noexcept{
    if(!out)return 0;*out={};out->schedule=-1;
    if(!a||n<0x1028||!b||!std::isfinite(now))return 0;
    const auto& g=b->grunt;const auto& reg=g.entities;
    if(reg.count>sin_independent::kEntityHandleCapacity||(reg.count&&!reg.entries))return 0;
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));
    const auto* behavior=SiN_GetServerNpcBehaviorSelectionApi(1,sizeof(ServerNpcBehaviorSelectionApi));
    if(!cond||!behavior)return 0;
    bool ok=true;
    auto has=[&](int id){ServerNpcConditionResult cr{};++out->condition_tests;if(!ok||!cond->test(a,n,id,NpcConditionCurrent,&g.schedule.conditions,&cr)){ok=false;return false;}return cr.value!=0;};
    auto choose=[&](int id){if(!ok)return 0;out->schedule=id;return 1;};
    auto combat=[&](int& id){++out->combat;return g.combat_schedule&&g.combat_schedule(g.context,a,&id);};
    if(has(0x46))return choose(0x69);
    if(!ok||!behavior->select(a,n,&g.behavior,&out->behavior))return 0;
    if(out->behavior.selected){out->parent=1;return g.parent&&g.parent(g.context,a,&out->schedule);}
    const int state=rd<int>(a,0x850);
    if(state==4){
        const auto* api=SiN_GetServerGruntScheduleSelectorApi(1,sizeof(ServerGruntScheduleSelectorApi));
        out->ground=1;if(!api||!api->select(a,n,&g,&out->grunt))return 0;return choose(out->grunt.schedule);
    }
    auto registry=sin_independent::EntityHandleView(reg.entries,reg.count);
    void* enemy=registry.Resolve(rd<std::uint32_t>(a,0x8a0));
    out->enemy_live=enemy!=nullptr;
    bool live=false;
    if(enemy){unsigned extent=0;if(!b->extent||!b->extent(b->context,enemy,&extent)||extent<0x90)return 0;live=rd<int>(enemy,0x8c)==0;}
    if(live){
        void* attachment=registry.Resolve(rd<std::uint32_t>(a,0x1024));out->attachment_live=attachment!=nullptr;
        if(!attachment||!b->attachment_enemy||!b->attachment_enemy(b->context,attachment,enemy,true))return 0;
        out->enemy_sent=1;
        if(has(0xb))return choose(0x76);
        if(has(0x4f))return choose(0x77);
        const float deadline=rd<float>(a,0x60c);if(!std::isfinite(deadline))return 0;
        if(now<deadline||has(0x16))return choose(0x29);
        int id=0;if(!ok||!combat(id))return 0;return choose(id);
    }
    void* target=registry.Resolve(rd<std::uint32_t>(a,0x8a8));
    bool inspection=target!=nullptr;
    for(unsigned i=0;i<3&&!inspection;++i)inspection=rd<float>(a,0x100c+4*i)!=b->default_inspect[i];
    out->inspection=inspection;
    if(inspection){
        if(has(0x4f))return choose(0x78);
        if(target)return choose(0x29);
        const int mode=rd<int>(a,0xfec);if(mode==0||mode==1)return choose(0x76);
    }
    if(state==3){int id=0;if(!ok||!combat(id))return 0;if(id)return choose(id);}
    if(state==2){
        if(has(0x10)||has(0x11)){bool yes=false;if(!g.investigate||!g.investigate(g.context,a,&yes))return 0;if(yes)return choose(0x1c);}
        if(has(0x33))return choose(0xa);
    }
    return choose(0x76);
}
const ServerJetpackSelectorApi api{1,sizeof(ServerJetpackSelectorApi),&select};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerJetpackScheduleSelectorApi=_SiN_GetServerJetpackScheduleSelectorApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerJetpackSelectorApi* __cdecl SiN_GetServerJetpackScheduleSelectorApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerJetpackSelectorApi)?&api:nullptr;
}
