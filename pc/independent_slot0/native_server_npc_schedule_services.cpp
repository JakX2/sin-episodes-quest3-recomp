#include "native_server_npc_schedule_services.h"
#include "native_server_grunt_helper_task.h"
#include "native_server_npc_shared_task.h"
#include <cmath>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcSubclassSchedulerApi* __cdecl SiN_GetServerNpcSubclassSchedulerApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcClassScheduleServicesApi* __cdecl SiN_GetServerNpcClassScheduleServicesApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcSpecialScheduleApi* __cdecl SiN_GetServerNpcSpecialScheduleApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
struct Call{
    void* actor;unsigned bytes;float now;
    const ServerNpcScheduleServicesBindings& source;
    ServerNpcScheduleServicesResult& out;
    ServerNpcClassScheduleTranslationBindings translation;
    ServerNpcInheritedScheduleBindings inherited;
    ServerNpcSpecialScheduleBindings special;
};
int run_impl(void* a,unsigned bytes,float now,const ServerNpcScheduleServicesBindings* b,ServerNpcScheduleServicesResult* out,const ServerGruntCombatTaskBindings* combat=nullptr,ServerGruntCombatSchedulerStats* stats=nullptr,const ServerGruntHelperTaskBindings* full=nullptr,ServerGruntFullTaskStats* full_stats=nullptr,const ServerNpcSharedTaskBindings* shared=nullptr,ServerNpcSharedTaskStats* shared_stats=nullptr) noexcept{
    if(!out)return 0;*out={};
    if(!a||!b||!std::isfinite(now))return 0;
    const auto* api=SiN_GetServerNpcSubclassSchedulerApi(1,sizeof(ServerNpcSubclassSchedulerApi));
    if(!api)return 0;
    unsigned kind=0;if(!api->classify(b->scheduler.original_vtable,&kind)||bytes<(kind==NpcSubclassJetpack?0x1085u:kind==NpcSubclassElite?0xfe0u:0xfccu))return 0;
    Call q{a,bytes,now,*b,*out,b->translation,b->inherited,b->special};
    auto bound=b->scheduler;
    auto& selection=bound.maintenance.selection;
    q.translation.conditions=selection.grunt.schedule.conditions;
    q.translation.entities=selection.state.entities;
    q.inherited.entities=selection.state.entities;
    q.special.state=selection.state;q.special.state.schedule=selection.grunt.schedule;
    selection.context=&q;
    selection.translate=[](void* p,void* actor,int id,int* result) noexcept -> int{
        auto& q=*static_cast<Call*>(p);if(actor!=q.actor)return 0;
        const auto* svc=SiN_GetServerNpcClassScheduleServicesApi(1,sizeof(ServerNpcClassScheduleServicesApi));
        ++q.out.translations;
        if(!svc)return 0;
        const int status=svc->translate(actor,q.bytes,q.source.scheduler.original_vtable,id,q.now,&q.translation,&q.inherited,&q.out.translation);
        q.out.inherited_calls+=q.out.translation.inherited_calls;q.out.burst_calls+=q.out.translation.burst_calls;q.out.interval_calls+=q.out.translation.interval_calls;
        if(!status)return 0;*result=q.out.translation.translation.output;return 1;
    };
    selection.special_select=[](void* p,void* actor,int state,int* result) noexcept -> int{
        auto& q=*static_cast<Call*>(p);if(actor!=q.actor)return 0;
        const auto* svc=SiN_GetServerNpcSpecialScheduleApi(1,sizeof(ServerNpcSpecialScheduleApi));
        ++q.out.special_selections;
        if(!svc||!svc->select(actor,q.bytes,&q.special,&q.out.special)||q.out.special.state!=state)return 0;
        *result=q.out.special.schedule;return 1;
    };
    selection.component_schedule=[](void* p,std::uint32_t component,int id,std::uint32_t* result) noexcept -> int{
        const auto& s=static_cast<Call*>(p)->source.scheduler.maintenance.selection;
        return s.component_schedule&&s.component_schedule(s.context,component,id,result);
    };
    selection.fail_select=[](void* p,void* actor,int schedule,int task,int failure,int* result) noexcept -> int{
        const auto& s=static_cast<Call*>(p)->source.scheduler.maintenance.selection;
        return s.fail_select&&s.fail_select(s.context,actor,schedule,task,failure,result);
    };
    selection.deactivate=[](void* p,void* actor,std::uint32_t component) noexcept -> int{
        const auto& s=static_cast<Call*>(p)->source.scheduler.maintenance.selection;
        return s.deactivate&&s.deactivate(s.context,actor,component);
    };
    if(combat)return run_subclass_combat_internal(a,bytes,now,&bound,&out->scheduler,combat,stats,full,full_stats,shared,shared_stats);
    return api->run(a,bytes,now,&bound,&out->scheduler);
}
int __cdecl run(void* a,unsigned bytes,float now,const ServerNpcScheduleServicesBindings* b,ServerNpcScheduleServicesResult* out) noexcept{
    return run_impl(a,bytes,now,b,out);
}
const ServerNpcScheduleServicesApi api{1,sizeof(ServerNpcScheduleServicesApi),&run};
 }
int run_schedule_services_combat_internal(void* a,unsigned bytes,float now,const ServerNpcScheduleServicesBindings* b,ServerNpcScheduleServicesResult* out,const ServerGruntCombatTaskBindings* combat,ServerGruntCombatSchedulerStats* stats,const ServerGruntHelperTaskBindings* full,ServerGruntFullTaskStats* full_stats,const ServerNpcSharedTaskBindings* shared,ServerNpcSharedTaskStats* shared_stats) noexcept{
    if(!combat||!stats)return 0;return run_impl(a,bytes,now,b,out,combat,stats,full,full_stats,shared,shared_stats);
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerNpcScheduleServicesApi=_SiN_GetServerNpcScheduleServicesApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleServicesApi* __cdecl SiN_GetServerNpcScheduleServicesApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcScheduleServicesApi)?&api:nullptr;
}
