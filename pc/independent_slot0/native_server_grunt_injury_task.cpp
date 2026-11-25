#include "native_server_grunt_injury_task.h"
#include <cmath>
#include <cstring>
#include <limits>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerGruntCombatTaskApi* __cdecl SiN_GetServerGruntCombatTaskApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcTaskLifecycleApi* __cdecl SiN_GetServerNpcTaskLifecycleApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcTransitionApi* __cdecl SiN_GetServerNpcTransitionApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* p,unsigned off) noexcept{T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T>void wr(void* p,unsigned off,T v) noexcept{std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
int __cdecl supports(int id,bool running) noexcept{
    return running?id==139:(id==98||id==141||id==144||id==149||id==150||id==152);
}
int __cdecl execute(void* a,unsigned n,const ServerNpcScheduleTask* t,bool running,float now,const ServerGruntInjuryTaskBindings* b,ServerGruntInjuryTaskResult* out) noexcept{
    if(!out)return 0;*out={};out->activity=-1;
    if(!a||n<0xfcc||!t||!b||!std::isfinite(now))return 0;
    const auto& g=b->combat.grunt;const auto& reg=g.entities;
    if(reg.count>sin_independent::kEntityHandleCapacity||(reg.count&&!reg.entries))return 0;
    const auto* combat=SiN_GetServerGruntCombatTaskApi(1,sizeof(ServerGruntCombatTaskApi));
    if(!supports(t->id,running))return combat&&combat->execute(a,n,t,running,now,&b->combat,&out->combat);
    const auto* life=SiN_GetServerNpcTaskLifecycleApi(1,sizeof(ServerNpcTaskLifecycleApi));if(!life)return 0;
    auto& o=out->combat.base;o.route=GruntTaskNative;o.task=t->id;o.activity=-1;out->recovered=1;
    const auto registry=sin_independent::EntityHandleView(reg.entries,reg.count);
    auto entity=[&](unsigned offset){return registry.Resolve(rd<std::uint32_t>(a,offset));};
    auto complete=[&](bool force=false){return life->complete(a,n,now,force,&g.lifecycle,&o.lifecycle)!=0;};
    auto fail=[&](int code){out->failure=code;return life->fail(a,n,now,code,&g.lifecycle,&o.lifecycle)!=0;};
    auto parent=[&](){o.parent_called=1;return g.parent&&g.parent(g.context,a,t,running);};
    auto pending=[&](int id){
        out->activity=o.activity=id;o.activity_requested=1;
        if(g.transition){
            const auto* api=SiN_GetServerNpcTransitionApi(1,sizeof(ServerNpcTransitionApi));
            return api&&api->set_pending(a,n,id,g.transition,&o.transition);
        }
        return b->combat.activity&&b->combat.activity(b->combat.context,a,id,false);
    };
    if(running){
        double error=0;
        if(!b->face_grenade||!b->face_grenade(b->context,a)||!b->yaw_error||!b->yaw_error(b->context,a,&error)||!std::isfinite(error))return 0;
        if(std::fabs(error)<.006){out->forced_completion=1;return complete(true);}return 1;
    }
    wr(a,0x9f0,.75f);
    switch(t->id){
    case 98:{
        if(rd<std::uint32_t>(a,0x9d0))return parent();
        void* e=entity(0x8a0);out->enemy_live=e!=nullptr;if(!e)return parent();
        if(!(rd<std::uint8_t>(a,0x98b)&0x20)){
            bool player=false;if(!b->is_player||!b->is_player(b->context,e,&player))return 0;
            if(player){
                unsigned size=0;if(!b->extent||!b->extent(b->context,e,&size)||size<0xa0)return 0;
                if(rd<int>(e,0x9c)<=20){
                    const auto squad=rd<std::uint32_t>(a,0x9c4);
                    if(squad&&(!b->squad_flag||!b->squad_flag(b->context,squad,0x20000000)))return 0;
                    if(!g.cue||!g.cue(g.context,a,0x7ec,-1,2)||!b->cue_finish||!b->cue_finish(b->context,a,2,0))return 0;
                    out->cue_called=1;
                }
            }
        }
        bool npc=false;unsigned caps=0;
        if(!b->npc_caps||!b->npc_caps(b->context,e,&npc,&caps))return 0;
        return !npc||(caps&0x2000)?parent():complete();
    }
    case 141:{
        const auto squad=rd<std::uint32_t>(a,0x9c4);if(!squad)return complete();
        int count=0;if(!b->combat.squad_count||!b->combat.squad_count(b->combat.context,squad,&count))return 0;
        if(count<=1)return complete();
        void* e=entity(0x8a0);void* target=nullptr;
        if(e&&(!g.combat_target||!g.combat_target(g.context,e,&target)))return 0;
        if(!target)return complete();
        bool gate=false;
        if(!b->cover_gate||!b->cover_gate(b->context,a,&gate))return 0;if(!gate)return complete();
        if(!g.target_gate||!g.target_gate(g.context,target,a,&gate))return 0;if(!gate)return complete();
        bool found=false;double dot=0;
        if(!b->cover_candidate||!b->cover_candidate(b->context,a,&found,&dot)||!std::isfinite(dot))return 0;
        if(!found)return 1;
        return pending(dot>0?57:56);
    }
    case 144:{
        const double type=std::trunc(static_cast<double>(t->parameter));
        if(!std::isfinite(type)||type<std::numeric_limits<int>::min()||type>std::numeric_limits<int>::max())return 0;
        wr<std::uint8_t>(a,0xcb1,0);
        void* attacker=entity(0xc3c);out->attacker_live=attacker!=nullptr;if(!attacker)attacker=a;
        bool player=false;if(!b->is_player||!b->is_player(b->context,attacker,&player))return 0;
        if(player){
            unsigned size=0;if(!b->extent||!b->extent(b->context,attacker,&size)||size<0x984)return 0;
            wr(attacker,0x980,rd<std::uint32_t>(a,0xc40));
        }
        if(!b->damage||!b->damage(b->context,a,attacker,static_cast<float>(rd<int>(a,0x9c)),static_cast<int>(type),-.1f,.1f))return 0;
        out->damage_called=1;return complete();
    }
    case 149:{
        void* target=entity(0x8a8);if(!target)return fail(11);
        bool gated=false;if(!b->path_gate||!b->path_gate(b->context,a,target,&gated))return 0;
        if(gated)return fail(11);
        bool success=false;++out->path_called;
        if(!b->target_path||!b->target_path(b->context,a,target,&success))return 0;
        return success?complete():fail(11);
    }
    case 150:{
        const auto token=rd<std::uint32_t>(a,0xca4);
        if(!token){
            int code=0;if(!b->failure_token||!b->failure_token(b->context,"Squadmate got up",&code)||!code)return 0;
            return fail(code);
        }
        void* peer=nullptr;unsigned size=0;
        if(!g.storage||!g.storage(g.context,token,&peer,&size)||!peer)return 0;
        return b->face_helper&&b->face_helper(b->context,a,peer)&&complete();
    }
    case 152:{
        void* target=entity(0x8a0);if(!target)target=a;
        bool yes=false;if(!b->dodge_probe||!b->dodge_probe(b->context,a,target,&yes))return 0;
        if(!yes&&!fail(8))return 0;
        ++out->path_called;if(!b->dodge_path||!b->dodge_path(b->context,a,&yes))return 0;
        if(!yes&&!fail(8))return 0;
        return complete(); // original still calls completion, failure suppresses it
    }
    }
    return 0;
}
const ServerGruntInjuryTaskApi api{1,sizeof(ServerGruntInjuryTaskApi),&supports,&execute};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerGruntInjuryTaskApi=_SiN_GetServerGruntInjuryTaskApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerGruntInjuryTaskApi* __cdecl SiN_GetServerGruntInjuryTaskApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerGruntInjuryTaskApi)?&api:nullptr;
}
