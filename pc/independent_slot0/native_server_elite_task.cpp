#include "native_server_elite_task.h"
#include "native_server_grunt_combat_task.h"
#include <cmath>
#include <cstring>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerGruntTaskDispatchApi* __cdecl SiN_GetServerGruntTaskDispatchApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcTaskLifecycleApi* __cdecl SiN_GetServerNpcTaskLifecycleApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerWeaponEventSoundApi* __cdecl SiN_GetServerWeaponEventSoundApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcReloadServicesApi* __cdecl SiN_GetServerNpcReloadServicesApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcParentWeaponEventApi* __cdecl SiN_GetServerNpcParentWeaponEventApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerGruntCombatTaskApi* __cdecl SiN_GetServerGruntCombatTaskApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* p,unsigned off) noexcept{T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T>void wr(void* p,unsigned off,T v) noexcept{std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
bool play(void* w,unsigned n,int id,const ServerEliteTaskBindings& b,ServerEliteTaskResult& out) noexcept{
    const auto* api=SiN_GetServerWeaponEventSoundApi(1,sizeof(ServerWeaponEventSoundApi));
    return api&&api->play(w,n,WeaponActivityChaingun,id,0,&b.reload.original.weapon.sound,&out.sound);
}
int __cdecl spin_update(void* w,unsigned n,float now,const ServerEliteTaskBindings* b,ServerEliteTaskResult* out) noexcept{
    if(!out)return 0;*out={};if(!w||n<0x598||!b||!std::isfinite(now))return 0;
    const int state=rd<int>(w,0x594);
    if(state==3){
        if(!b->stop_sound||!b->stop_sound(b->context,w,13)||!play(w,n,14,*b,*out))return 0;
        wr(w,0x594,0);out->spin_changed=1;
    }else if(state==1||state==2){
        const float stamp=rd<float>(w,state==1?0x590:0x58c);
        if(!std::isfinite(stamp))return 0;
        if(static_cast<double>(now)-stamp>=1.0){
            wr(w,0x594,state+1);out->spin_changed=1;
            if(state==1){wr(w,0x590,now);wr(w,0x58c,now);}
        }
    }
    return 1;
}
int execute_impl(void* a,unsigned n,const ServerNpcScheduleTask* t,bool running,float now,const ServerEliteTaskBindings* b,ServerEliteTaskResult* out,const ServerGruntCombatTaskBindings* combat=nullptr,ServerGruntCombatSchedulerStats* stats=nullptr) noexcept{
    if(!out)return 0;*out={};if(!a||n<0xfe0||!t||!b||!std::isfinite(now))return 0;
    const auto& reg=b->grunt.entities;
    if(reg.count>sin_independent::kEntityHandleCapacity||(reg.count&&!reg.entries))return 0;
    const auto registry=sin_independent::EntityHandleView(reg.entries,reg.count);
    const auto* life=SiN_GetServerNpcTaskLifecycleApi(1,sizeof(ServerNpcTaskLifecycleApi));
    const auto* grunt=SiN_GetServerGruntTaskDispatchApi(1,sizeof(ServerGruntTaskDispatchApi));
    if(!life||!grunt)return 0;
    auto complete=[&](){return life->complete(a,n,now,false,&b->grunt.lifecycle,&out->lifecycle)!=0;};
    auto delegate=[&](){
        out->delegated=1;
        if(combat&&stats){
            const auto* api=SiN_GetServerGruntCombatTaskApi(1,sizeof(ServerGruntCombatTaskApi));
            if(!api)return false;
            if(api->supports(t->id,running)){
                auto bound=*combat;bound.grunt=b->grunt;bound.weapon.entities=reg;
                ++stats->calls;
                const int status=api->execute(a,n,t,running,now,&bound,&stats->last);
                out->grunt=stats->last.base;return status!=0;
            }
        }
        return grunt->execute(a,n,t,running,now,&b->grunt,&out->grunt)!=0;
    };
    auto timer=[&](double delta){const float deadline=static_cast<float>(now+delta);if(!std::isfinite(deadline))return false;wr(a,0x9dc,deadline);out->timer_written=1;return true;};
    auto spin=[&](bool& ready){
        void* w=registry.Resolve(rd<std::uint32_t>(a,0xfdc));out->weapon_live=w!=nullptr;
        unsigned size=0;const auto& wb=b->reload.original.weapon;
        if(!w||!wb.extent||!wb.extent(wb.context,w,&size)||size<0x598)return false;
        const int state=rd<int>(w,0x594);ready=state==2;out->spin_ready=ready;
        if(state==0){if(!play(w,size,13,*b,*out))return false;wr(w,0x594,1);wr(w,0x590,now);wr(w,0x58c,now);out->spin_changed=1;}
        return true;
    };
    if(running){
        if(t->id==0x43){
            if(rd<std::uint8_t>(a,0xfd9))return delegate();
            bool ready=false;if(!spin(ready))return 0;return ready?complete():1;
        }
        if(t->id==0x47){
            const float deadline=rd<float>(a,0x9dc);if(!std::isfinite(deadline))return 0;
            if(now<deadline)return 1;
            if(registry.Resolve(rd<std::uint32_t>(a,0x78c))){
                const auto* reload=SiN_GetServerNpcReloadServicesApi(1,sizeof(ServerNpcReloadServicesApi));
                const auto* parent=SiN_GetServerNpcParentWeaponEventApi(1,sizeof(ServerNpcParentWeaponEventApi));
                auto binding=b->reload;binding.original.weapon.weapons=reg;binding.conditions=b->grunt.lifecycle.conditions;
                ServerNpcReloadSession session{};ServerAnimationEvent event{3018,nullptr,0,now,0,a};
                if(!reload||!parent||!reload->prepare(a,n,&binding,&session)||parent->handle(a,n,&event,now,&session.bound,&out->reload)!=1)return 0;
            }
            return complete();
        }
        return delegate();
    }
    if(t->id==0x43){
        bool ready=false;if(!spin(ready))return 0;wr<std::uint8_t>(a,0xfd9,ready);return ready?delegate():1;
    }
    if(t->id==0x47){
        void* w=registry.Resolve(rd<std::uint32_t>(a,0xfdc));out->weapon_live=w!=nullptr;
        if(w){const auto& wb=b->reload.original.weapon;unsigned size=0;if(!wb.extent||!wb.extent(wb.context,w,&size)||size<0x598)return 0;wr(w,0x594,3);out->spin_changed=1;}
        return timer(1);
    }
    if(t->id==3){
        if(!std::isfinite(t->parameter)||std::trunc(t->parameter)!=1.f)return 1;
        void* e=registry.Resolve(rd<std::uint32_t>(a,0x8a0));void* target=nullptr;
        if(e&&(!b->grunt.combat_target||!b->grunt.combat_target(b->grunt.context,e,&target)))return 0;
        if(!target)return complete();
        bool player=false,gate=false;if(!b->is_player||!b->is_player(b->context,target,&player))return 0;
        if(!player)return complete();
        if(!b->grunt.target_gate||!b->grunt.target_gate(b->grunt.context,target,a,&gate))return 0;
        const float prior=rd<float>(a,0x99c);if(!std::isfinite(prior))return 0;
        if(gate||static_cast<double>(now)-prior<=3.0)return complete();
        wr(a,0x99c,now);
        if(!b->grunt.cue||!b->grunt.cue(b->grunt.context,a,0x7cc,2,2))return 0;out->cue_called=1;
        if(!timer(2)||!b->idle||!b->idle(b->context,a))return 0;out->idle_called=1;return 1;
    }
    return delegate();
}
int __cdecl execute(void* a,unsigned n,const ServerNpcScheduleTask* t,bool running,float now,const ServerEliteTaskBindings* b,ServerEliteTaskResult* out) noexcept{
    return execute_impl(a,n,t,running,now,b,out);
}
int __cdecl execute_native(void* a,unsigned n,const ServerNpcScheduleTask* t,bool running,float now,const ServerEliteTaskBindings* b,const ServerGruntCombatTaskBindings* combat,ServerEliteTaskResult* out,ServerGruntCombatSchedulerStats* stats) noexcept{
    if(!combat||!stats)return 0;
    return execute_impl(a,n,t,running,now,b,out,combat,stats);
}
const ServerEliteNativeTaskApi native_api{1,sizeof(ServerEliteNativeTaskApi),&execute_native};
const ServerEliteTaskApi api{1,sizeof(ServerEliteTaskApi),&execute,&spin_update};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerEliteTaskApi=_SiN_GetServerEliteTaskApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerEliteTaskApi* __cdecl SiN_GetServerEliteTaskApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerEliteTaskApi)?&api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerEliteNativeTaskApi=_SiN_GetServerEliteNativeTaskApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerEliteNativeTaskApi* __cdecl SiN_GetServerEliteNativeTaskApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerEliteNativeTaskApi)?&native_api:nullptr;
}
