#include "native_server_npc_schedule_maintenance.h"
#include "native_server_grunt_helper_task.h"
#include "native_server_npc_shared_attack.h"
#include <cmath>
#include <cstring>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleStateApi* __cdecl
SiN_GetServerNpcScheduleStateApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleValidityApi* __cdecl
SiN_GetServerNpcScheduleValidityApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcStateApi* __cdecl
SiN_GetServerNpcStateReconsiderApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleResolutionApi* __cdecl
SiN_GetServerNpcScheduleResolutionApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcConditionApi* __cdecl
SiN_GetServerNpcConditionApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcWeaponRuntimeApi* __cdecl
SiN_GetServerNpcWeaponRuntimeApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerGruntTaskDispatchApi* __cdecl
SiN_GetServerGruntTaskDispatchApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNativeActivityServicesApi* __cdecl
SiN_GetServerNativeActivityServicesApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerEliteTaskApi* __cdecl SiN_GetServerEliteTaskApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerJetpackTaskApi* __cdecl SiN_GetServerJetpackTaskApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerJetpackScheduleResolutionApi* __cdecl SiN_GetServerJetpackScheduleResolutionApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerGruntCombatTaskApi* __cdecl SiN_GetServerGruntCombatTaskApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerGruntHelperTaskApi* __cdecl SiN_GetServerGruntHelperTaskApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerGruntInjuryTaskApi* __cdecl SiN_GetServerGruntInjuryTaskApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerEliteNativeTaskApi* __cdecl SiN_GetServerEliteNativeTaskApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntGatherPointApi* __cdecl SiN_GetServerNpcGruntGatherPointApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntGatherNativeApi* __cdecl SiN_GetServerNpcGruntGatherNativeApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntPerceptionSpeechApi* __cdecl SiN_GetServerNpcGruntPerceptionSpeechApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntPerceptionSlotsApi* __cdecl SiN_GetServerNpcGruntPerceptionSlotsApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcNativeSlotResolutionApi* __cdecl SiN_GetServerNpcNativeSlotResolutionApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntSquadReportApi* __cdecl SiN_GetServerNpcGruntSquadReportApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntNativeStateApi* __cdecl SiN_GetServerNpcGruntNativeStateApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntScheduleLineApi* __cdecl SiN_GetServerNpcGruntScheduleLineApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
struct NativeRequestRoute {
    const ServerNpcGruntSpecialCombatBindings* selector;
    const ServerNpcGruntScheduleLineServices* line;
    ServerNpcNativeRequestSchedulerResult* result;
    const ServerNpcGruntJetSelectorBindings* jet=nullptr;
    ServerNpcNativeJetRequestSchedulerResult* jet_result=nullptr;
    const ServerNpcGruntScheduleConeServices* cone=nullptr;
};
struct GatherRoute {
    const ServerNpcGruntGatherConditionsBindings* bindings;
    const ServerNpcGruntPointPolicyBindings* point;
    ServerNpcGatherSchedulerResult* result;
};
template<class T>T read(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T>void write(void* p,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
bool runnable(void* actor) noexcept {const int s=read<int>(actor,0x7a4);return s!=4&&s!=2;}
struct Call {
    void* a;unsigned bytes;float now;
    const ServerNpcScheduleMaintenanceBindings& b;
    ServerNpcScheduleMaintenanceResult& out;
    const ServerNpcScheduleStateApi* schedule;
    const ServerNpcConditionApi* cond;
    const ServerNpcStateApi* state;
    const ServerNpcScheduleResolutionApi* resolution;
    const ServerNpcScheduleValidityApi* validity;
    const ServerNpcScheduleBindings& sb;
    const ServerGruntTaskBindings* tasks;
    ServerGruntSchedulerResult* grunt;
    const ServerNpcSubclassSchedulerBindings* subclass;
    ServerNpcSubclassSchedulerResult* sub;
    const ServerGruntCombatTaskBindings* combat;
    ServerGruntCombatSchedulerStats* combat_stats;
    const ServerGruntHelperTaskBindings* full;
    ServerGruntFullTaskStats* full_stats;
    const ServerNpcNativeGatherRoute* native_gather;
    const NativeRequestRoute* request;
    void publish_extended(const ServerGruntTaskResult& result) noexcept{
        sub->task_failures+=result.lifecycle.failed;
        if(sub->subclass==NpcSubclassElite){
            sub->elite={};sub->elite.delegated=1;sub->elite.grunt=result;++sub->delegated;
        }else if(sub->subclass==NpcSubclassJetpack){
            sub->jetpack={};sub->jetpack.delegated=1;sub->jetpack.grunt=result;++sub->delegated;
        }else{
            sub->grunt=result;sub->delegated+=result.parent_called;
        }
    }
    bool task(const ServerNpcScheduleTask& t,bool running) noexcept {
        if(subclass&&sub){
            ++sub->task_calls;
            if(combat&&combat_stats){
                const auto* api=SiN_GetServerGruntCombatTaskApi(1,sizeof(ServerGruntCombatTaskApi));
                if(!api)return false;
                // Override-owned routes must remain ahead of inherited routes.
                const bool override_owned=(sub->subclass==NpcSubclassElite&&
                    ((!running&&t.id==3)||t.id==67||t.id==71))||
                    (sub->subclass==NpcSubclassJetpack&&running&&t.id==67);
                if(full&&full_stats&&!override_owned){
                    const auto* helper=SiN_GetServerGruntHelperTaskApi(1,sizeof(ServerGruntHelperTaskApi));
                    const auto* injury=SiN_GetServerGruntInjuryTaskApi(1,sizeof(ServerGruntInjuryTaskApi));
                    if(!helper||!injury)return false;
                    const bool helper_route=helper->supports(t.id,running)!=0,injury_route=injury->supports(t.id,running)!=0;
                    if(helper_route||injury_route){
                        ++full_stats->calls;full_stats->helper_calls+=helper_route;full_stats->injury_calls+=injury_route;
                        if(!helper->execute(a,bytes,&t,running,now,full,&full_stats->last))return false;
                        publish_extended(full_stats->last.injury.combat.base);return true;
                    }
                }
                if(!override_owned&&api->supports(t.id,running)){
                    ++combat_stats->calls;
                    if(!api->execute(a,bytes,&t,running,now,combat,&combat_stats->last))return false;
                    publish_extended(combat_stats->last.base);
                    return true;
                }
            }
            if(sub->subclass==NpcSubclassElite){
                const auto* api=SiN_GetServerEliteTaskApi(1,sizeof(ServerEliteTaskApi));
                if(combat&&combat_stats){
                    const auto* native=SiN_GetServerEliteNativeTaskApi(1,sizeof(ServerEliteNativeTaskApi));
                    if(!native||!native->execute(a,bytes,&t,running,now,&subclass->elite,combat,&sub->elite,combat_stats))return false;
                }else if(!api||!api->execute(a,bytes,&t,running,now,&subclass->elite,&sub->elite))return false;
                sub->delegated+=sub->elite.delegated;
                sub->task_failures+=sub->elite.delegated?sub->elite.grunt.lifecycle.failed:sub->elite.lifecycle.failed;
            }else if(sub->subclass==NpcSubclassJetpack){
                const auto* api=SiN_GetServerJetpackTaskApi(1,sizeof(ServerJetpackTaskApi));
                if(!api||!api->execute(a,bytes,&t,running,now,&subclass->jetpack,&sub->jetpack))return false;
                sub->delegated+=sub->jetpack.delegated;
                sub->task_failures+=sub->jetpack.delegated?sub->jetpack.grunt.lifecycle.failed:sub->jetpack.lifecycle.failed;
            }else{
                const auto* api=SiN_GetServerGruntTaskDispatchApi(1,sizeof(ServerGruntTaskDispatchApi));
                if(!api||!api->execute(a,bytes,&t,running,now,&subclass->grunt,&sub->grunt))return false;
                sub->delegated+=sub->grunt.parent_called;
                sub->task_failures+=sub->grunt.lifecycle.failed;
            }
            return true;
        }
        if(tasks){
            const auto* api=SiN_GetServerGruntTaskDispatchApi(1,sizeof(ServerGruntTaskDispatchApi));
            if(!api||!grunt||!api->execute(a,bytes,&t,running,now,tasks,&grunt->last))return false;
            switch(grunt->last.route){
            case GruntTaskNative:++grunt->native_tasks;break;
            case GruntTaskParent:++grunt->parent_tasks;break;
            case GruntTaskHostedSpecial:++grunt->special_tasks;break;
            default:return false;
            }
            grunt->task_failures+=grunt->last.lifecycle.failed;
            return true;
        }
        auto cb=running?b.run_task:b.start_task;
        return cb&&cb(b.context,a,&t);
    }
    bool test(int id,bool& value) noexcept {
        ServerNpcConditionResult cr{};if(!cond->test(a,bytes,id,NpcConditionCurrent,&sb.conditions,&cr))return false;
        value=cr.value!=0;return true;
    }
    bool transition(bool idle=false) noexcept {
        if(!idle)++out.transitions;
        if(b.animation){
            const auto* api=SiN_GetServerNpcWeaponRuntimeApi(1,sizeof(ServerNpcWeaponRuntimeApi));
            return api&&(idle?api->request(a,bytes,1,b.animation,&out.animation):api->update(a,bytes,b.animation,&out.animation));
        }
        return idle?(b.idle&&b.idle(b.context,a)):(b.transition&&b.transition(b.context,a));
    }
    bool budget(bool& stop) noexcept {
        if(!b.budget||!b.budget(b.context,a,&stop))return false;
        if(stop)out.budget_yield=1;
        return true;
    }
    bool choose_state() noexcept{
        if(native_gather&&native_gather->native_state){
            if(!subclass||!native_gather->choose_result||!native_gather->choose_calls)return false;
            const auto* api=SiN_GetServerNpcGruntNativeStateApi(1,sizeof(ServerNpcGruntNativeStateApi));
            auto bound=*native_gather->native_state;bound.original_vtable=subclass->original_vtable;
            ++*native_gather->choose_calls;
            const int ok=api&&api->choose(a,bytes,now,&b.selection.state,&bound,native_gather->choose_result);
            out.ideal=native_gather->choose_result->state;return ok!=0;
        }
        return state->choose_ideal(a,bytes,&b.selection.state,&out.ideal)!=0;
    }
    bool apply_state(int desired) noexcept{
        if(native_gather&&native_gather->native_state){
            if(!subclass||!native_gather->apply_result||!native_gather->apply_calls)return false;
            const auto* api=SiN_GetServerNpcGruntNativeStateApi(1,sizeof(ServerNpcGruntNativeStateApi));
            auto bound=*native_gather->native_state;bound.original_vtable=subclass->original_vtable;
            ++*native_gather->apply_calls;
            const int ok=api&&api->apply(a,bytes,desired,now,&b.selection.state,&bound,native_gather->apply_result);
            out.applied=native_gather->apply_result->state;return ok!=0;
        }
        return state->apply(a,bytes,desired,now,&b.selection.state,&out.applied)!=0;
    }
    bool pick(bool failed) noexcept {
        if(request){
            auto& r=*request->result;const auto* api=SiN_GetServerNpcGruntScheduleLineApi(1,sizeof(ServerNpcGruntScheduleLineApi));
            const auto* cb=request->jet?request->jet->combat:request->selector?request->selector->combat:nullptr;
            if(!api||!cb||!cb->tail)return false;
            ++r.requests;if(failed)++r.failure_requests;
            if(request->jet_result){request->jet_result->tail={};request->jet_result->special={};}
            const int ok=failed?grunt_schedule_line_failure_internal(a,bytes,now,cb->tail->request,request->line,&r.request,request->cone):
                request->jet?grunt_schedule_line_jet_internal(a,bytes,now,request->jet,request->line,&r.request,&r.tail,&r.combat,&request->jet_result->tail,&request->jet_result->special,request->cone):
                request->cone?grunt_schedule_cone_normal_internal(a,bytes,now,request->selector,request->line,&r.request,&r.tail,&r.combat,&r.special,request->cone):
                api->choose_normal(a,bytes,now,request->selector,request->line,&r.request,&r.tail,&r.combat,&r.special);
            if(!ok)return false;
            // Report the same installed descriptor in the legacy maintenance result; do not retranslate.
            out.selection={};
            out.selection.requested=r.request.requested;out.selection.translated=r.request.acquisition.translated;
            out.selection.global=r.request.acquisition.lookup_id;out.selection.ideal=r.request.owner_global;
            out.selection.token=r.request.acquisition.schedule_token;out.selection.visited=r.request.acquisition.records;
            out.selection.fallback=r.request.acquisition.fallback;out.selection.forced=r.request.forced_schedule;
            out.selection.failed=failed;out.selection.component_deactivated=r.request.deactivated;
            out.selection.component_hit=r.request.acquisition.behavior_lookup&&!r.request.acquisition.owner_lookup&&out.selection.token;
        }else if(subclass&&sub&&sub->subclass==NpcSubclassJetpack){
            const auto* api=SiN_GetServerJetpackScheduleResolutionApi(1,sizeof(ServerJetpackScheduleResolutionApi));
            if(!api||!api->select(a,bytes,now,failed,&b.selection,&subclass->jet_selector,&out.selection,&sub->jet_selector))return false;
        }else if(native_gather&&native_gather->slots){
            const auto* api=SiN_GetServerNpcNativeSlotResolutionApi(1,sizeof(ServerNpcNativeSlotResolutionApi));
            ++*native_gather->slot_selections;
            if(!api||!api->select(a,bytes,now,failed,&b.selection,&native_gather->slots->slots,&out.selection,native_gather->slot_selection))return false;
        }else if(!resolution->select(a,bytes,now,failed,&b.selection,&out.selection))return false;
        ++out.selected;if(failed)++out.failed_selected;
        if(!out.selection.token)return !failed;
        if(failed){
            const void* row=nullptr;unsigned n=0;
            if(!sb.storage||!sb.storage(sb.context,out.selection.token,&row,&n)||!row||n<4)return false;
            write(a,0x79c,read<int>(row,0));
        }
        return schedule->install(a,bytes,out.selection.token,now,&sb,&out.state)!=0;
    }
    bool inspect() noexcept {return schedule->inspect(a,bytes,&sb,&out.state)!=0;}
};
int run_impl(void* actor,unsigned bytes,float now,const ServerNpcScheduleMaintenanceBindings* b,ServerNpcScheduleMaintenanceResult* out,const ServerGruntTaskBindings* tasks=nullptr,ServerGruntSchedulerResult* grunt=nullptr,const ServerNpcSubclassSchedulerBindings* subclass=nullptr,ServerNpcSubclassSchedulerResult* sub=nullptr,const ServerGruntCombatTaskBindings* combat=nullptr,ServerGruntCombatSchedulerStats* combat_stats=nullptr,const ServerGruntHelperTaskBindings* full=nullptr,ServerGruntFullTaskStats* full_stats=nullptr,const GatherRoute* gather=nullptr,const ServerNpcNativeGatherRoute* native_gather=nullptr,const NativeRequestRoute* request=nullptr) noexcept {
    if(!out)return 0;*out={};
    if(!actor||bytes<0xdd8||!b||!std::isfinite(now))return 0;
    const auto* sa=SiN_GetServerNpcScheduleStateApi(1,sizeof(ServerNpcScheduleStateApi));
    const auto* ca=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));
    const auto* st=SiN_GetServerNpcStateReconsiderApi(1,sizeof(ServerNpcStateApi));
    const auto* re=SiN_GetServerNpcScheduleResolutionApi(1,sizeof(ServerNpcScheduleResolutionApi));
    const auto* va=SiN_GetServerNpcScheduleValidityApi(1,sizeof(ServerNpcScheduleValidityApi));
    if(!sa||!ca||!st||!re||!va)return 0;
    Call q{actor,bytes,now,*b,*out,sa,ca,st,re,va,b->selection.grunt.schedule,tasks,grunt,subclass,sub,combat,combat_stats,full,full_stats,native_gather,request};
    bool stop=false;
    for(unsigned iteration=0;iteration<10&&!stop;++iteration){
        ++out->iterations;
        if(read<std::uint32_t>(actor,0x798)&&read<int>(actor,0x7a4)==4){
            if(!sa->advance(actor,bytes,&q.sb,&out->state))return 0;++out->advanced;
        }
        if(!va->validate(actor,bytes,now,&b->validity,&out->validity))return 0;
        if(!out->validity.valid||read<int>(actor,0x850)!=read<int>(actor,0x858)){
            if(!b->invalidated||!b->invalidated(b->context,actor))return 0;++out->invalidated;
            bool forced=false;
            if(!q.test(0x3f,forced))return 0;
            if(!forced&&(!read<std::uint8_t>(actor,0x84d)||!read<std::uint8_t>(actor,0x84e))){
                if(native_gather){
                    const auto* api=SiN_GetServerNpcGruntGatherNativeApi(1,sizeof(ServerNpcGruntGatherNativeApi));
                    auto bound=*native_gather->bindings;
                    const auto& selection=b->selection;
                    bound.gather.base.gate.original_vtable=subclass->original_vtable;
                    bound.gather.base.post.schedule=selection.grunt.schedule;
                    bound.gather.base.perception.enemy_switch.best.memory.entries=selection.state.entities.entries;
                    bound.gather.base.perception.enemy_switch.best.memory.count=selection.state.entities.count;
                    ++*native_gather->calls;
                    if(native_gather->combat_line){
                        if(!native_gather->reports||!native_line_squad_report_gather(actor,bytes,now,&bound,native_gather->reports,native_gather->result,native_gather->report_result,native_gather->combat_line,native_gather->combat_line_result,native_gather->weapon_policy,native_gather->weapon_policy_result))return 0;
                    }else if(native_gather->reports){
                        const auto* reports=SiN_GetServerNpcGruntSquadReportApi(1,sizeof(ServerNpcGruntSquadReportApi));
                        if(!reports||!reports->gather(actor,bytes,now,&bound,native_gather->reports,native_gather->result,native_gather->report_result))return 0;
                    }else if(native_gather->slots){
                        const auto* slots=SiN_GetServerNpcGruntPerceptionSlotsApi(1,sizeof(ServerNpcGruntPerceptionSlotsApi));
                        if(!slots||!slots->gather(actor,bytes,now,&bound,native_gather->slots,native_gather->result,native_gather->slots_result))return 0;
                    }else if(native_gather->dialogue){
                        const auto* dialogue=SiN_GetServerNpcGruntPerceptionSpeechApi(1,sizeof(ServerNpcGruntPerceptionSpeechApi));
                        if(!dialogue||!dialogue->gather(actor,bytes,now,&bound,native_gather->dialogue,native_gather->result,native_gather->dialogue_result))return 0;
                    }else if(!api||!api->gather(actor,bytes,now,&bound,native_gather->result))return 0;
                }else if(gather){
                    const auto* api=SiN_GetServerNpcGruntGatherPointApi(1,sizeof(ServerNpcGruntGatherPointApi));
                    ++gather->result->gather_calls;
                    if(!api||!api->gather(actor,bytes,now,gather->bindings,gather->point,&gather->result->gather,&gather->result->point))return 0;
                }else if(!b->before_selection||!b->before_selection(b->context,actor))return 0;
            }
            if(!st->should_reconsider(actor,bytes,&b->selection.state,&out->ideal))return 0;
            if(out->ideal.reconsider){
                if(!q.choose_state())return 0;
                write(actor,0x858,out->ideal.ideal);
            }
            bool failed=false;if(!q.test(0x21,failed))return 0;
            if(failed&&read<int>(actor,0x850)==read<int>(actor,0x858)){
                if(!q.pick(true))return 0;
            }else{
                if(!q.apply_state(read<int>(actor,0x858))||!q.pick(false))return 0;
            }
        }
        if(!read<std::uint32_t>(actor,0x798)&&!q.pick(false))return 0;
        if(!q.inspect())return 0;
        if(!out->state.schedule||!out->state.count){
            out->missing_schedule=1;return q.transition(true);
        }
        if(out->state.status==0){
            if(!out->state.has_task)return 0;
            // All eight verified +430 targets are the same no-op.
            const auto task=out->state.task;
            write(actor,0x7a4,1);write(actor,0x7b0,0);write(actor,0x7ac,now);
            ++out->started;if(!q.task(task,false))return 0;
            if(runnable(actor)){
                bool failed=false;if(!q.test(0x21,failed))return 0;
                if(!failed&&(!b->after_start||!b->after_start(b->context,actor)))return 0;
            }
        }
        if(!q.transition())return 0;
        const int status=read<int>(actor,0x7a4);
        if(status==4||status==0){
            if(!q.budget(stop))return 0;
            continue;
        }
        bool failed=false;if(!q.test(0x21,failed))return 0;
        if(!runnable(actor)||failed){stop=true;continue;}
        if(!q.inspect()||!out->state.has_task)return 0;
        const auto task=out->state.task;
        for(unsigned inner=0;inner<8;++inner){
            ++out->runs;if(!q.task(task,true))return 0;
            if(!read<unsigned>(actor,0x7b4)||read<int>(actor,0x7a4)==4)break;
            if(!q.test(0x21,failed))return 0;if(failed)break;
            if(b->check_inner_budget){
                if(!q.budget(stop))return 0;if(stop)break;
            }
            if(inner==7)out->inner_limit=1;
        }
        if(runnable(actor)){
            if(!q.test(0x21,failed))return 0;
            if(!failed){
                bool motion=false;if(!b->motion_gate||!b->motion_gate(b->context,actor,&motion))return 0;
                if(motion)write<std::uint8_t>(actor,0x98a,read<std::uint8_t>(actor,0x98a)|4u);
                if(!b->after_run||!b->after_run(b->context,actor))return 0;
            }
        }
        if(read<int>(actor,0x7a4)==4&&!stop){if(!q.budget(stop))return 0;}else stop=true;
    }
    if(!stop&&out->iterations==10)out->iteration_limit=1;
    return q.transition()&&q.inspect();
}
int __cdecl run(void* a,unsigned bytes,float now,const ServerNpcScheduleMaintenanceBindings* b,ServerNpcScheduleMaintenanceResult* out) noexcept {
    return run_impl(a,bytes,now,b,out);
}
int __cdecl run_grunt(void* a,unsigned bytes,float now,const ServerGruntSchedulerBindings* b,ServerGruntSchedulerResult* out) noexcept {
    if(!out)return 0;*out={};
    if(!a||bytes<0xfcc||!b||b->maintenance.selection.jetpack||b->maintenance.validity.jetpack)return 0;
    auto tasks=b->tasks;
    const auto& m=b->maintenance;
    // Use the same mapped current conditions and entity table as selection.
    tasks.lifecycle.conditions=m.selection.grunt.schedule.conditions;
    tasks.entities=m.selection.state.entities;
    ServerNativeActivitySession session{};
    ServerNpcTransitionBindings pending{};
    if(m.animation){
        const auto* services=SiN_GetServerNativeActivityServicesApi(1,sizeof(ServerNativeActivityServicesApi));
        const auto& runtime=*m.animation;
        const auto& assignment=runtime.npc.transition.assignment;
        if(assignment.translation.actor_class!=NpcActivityGrunt||
           runtime.inherited.weapons.entries!=tasks.entities.entries||runtime.inherited.weapons.count!=tasks.entities.count||
           !services||!services->prepare(a,bytes,&assignment,&runtime.inherited,&session))return 0;
        pending=runtime.npc.transition;pending.assignment=session.bound;
        tasks.transition=&pending;
    }
    return run_impl(a,bytes,now,&m,&out->maintenance,&tasks,out);
}
int __cdecl classify(std::uint32_t vtable,unsigned* out) noexcept {
    if(!out)return 0;*out=NpcSubclassGround;
    switch(vtable){
    case 0x2248df04:*out=NpcSubclassElite;return 1;
    case 0x224954a4:case 0x2249aaa4:case 0x224a416c:return 1;
    case 0x2248c514:case 0x2248e83c:case 0x2249964c:case 0x224a4a4c:*out=NpcSubclassJetpack;return 1;
    default:return 0;
    }
}
// Own the copied pointer chain for one synchronous maintenance call.
struct NativeRequestSession {
    ServerNpcGruntAssaultScheduleBindings schedule{};
    ServerNpcGruntAssaultActivationBindings activation{};
    ServerNpcGruntAssaultArbitrationBindings arbitration{};
    ServerNpcGruntScheduleAcquireBindings acquire{};
    ServerNpcGruntScheduleRequestBindings request{};
    ServerNpcGruntSelectorTailBindings tail{};
    ServerNpcGruntCombatSelectorBindings combat{};
    ServerNpcGruntSpecialCombatBindings selector{};
    ServerNpcGruntJetSelectorBindings jet{};
    NativeRequestRoute route{};
    bool prepare(const NativeRequestRoute& input,const ServerNpcSubclassSchedulerBindings& b) noexcept {
        if(!input.result||!input.line||bool(input.selector)==bool(input.jet)||bool(input.jet)!=bool(input.jet_result))return false;
        if(input.jet_result&&input.result!=&input.jet_result->base)return false;
        if(input.cone&&input.cone->line!=input.line)return false;
        const auto* cb=input.jet?input.jet->combat:input.selector->combat;
        if(!cb||!cb->tail)return false;
        const auto* req=cb->tail->request;
        if(!req||!req->acquisition||!req->acquisition->arbitration)return false;
        const auto& ac=*req->acquisition;const auto& arb=*ac.arbitration;
        if(!arb.activation||!arb.activation->schedule||ac.special_translate)return false;
        const auto& sel=b.maintenance.selection;
        schedule=*arb.activation->schedule;
        schedule.admission.actor_vtable=b.original_vtable;
        schedule.admission.entries=sel.state.entities.entries;schedule.admission.count=sel.state.entities.count;
        schedule.admission.conditions=sel.grunt.schedule.conditions;
        activation=*arb.activation;activation.schedule=&schedule;
        arbitration=arb;arbitration.activation=&activation;
        acquire=ac;acquire.arbitration=&arbitration;acquire.schedule=sel.grunt.schedule;
        acquire.owner_schedules=sel.grunt.schedule.conditions;acquire.registry_head=sel.head;
        request=*req;request.acquisition=&acquire;
        tail=*cb->tail;tail.request=&request;
        combat=*cb;combat.tail=&tail;
        if(input.jet){
            jet=*input.jet;jet.combat=&combat;
            route={nullptr,input.line,input.result,&jet,input.jet_result,input.cone};
        }else{
            selector=*input.selector;selector.combat=&combat;
            route={&selector,input.line,input.result,nullptr,nullptr,input.cone};
        }
        return true;
    }
};
int run_subclass_impl(void* a,unsigned bytes,float now,const ServerNpcSubclassSchedulerBindings* b,ServerNpcSubclassSchedulerResult* out,const ServerGruntCombatTaskBindings* combat=nullptr,ServerGruntCombatSchedulerStats* stats=nullptr,const ServerGruntHelperTaskBindings* full=nullptr,ServerGruntFullTaskStats* full_stats=nullptr,const ServerNpcSharedTaskBindings* shared=nullptr,ServerNpcSharedTaskStats* shared_stats=nullptr,const ServerNpcSharedAttackBindings* attack=nullptr,ServerNpcSharedAttackStats* attack_stats=nullptr,const ServerNpcSharedFacingBindings* facing=nullptr,ServerNpcSharedFacingStats* facing_stats=nullptr,const ServerNpcSharedTargetAnimationBindings* target_animation=nullptr,ServerNpcSharedTargetAnimationStats* target_animation_stats=nullptr,const ServerNpcSharedMovementBindings* movement=nullptr,ServerNpcSharedMovementStats* movement_stats=nullptr,const ServerNpcSharedCueBindings* cue=nullptr,ServerNpcSharedCueStats* cue_stats=nullptr,const ServerNpcSharedObservationBindings* observe=nullptr,ServerNpcSharedObservationStats* observe_stats=nullptr,const ServerNpcSharedSoundCoverBindings* sound_cover=nullptr,ServerNpcSharedSoundCoverStats* sound_cover_stats=nullptr,const ServerNpcSharedTacticalCoverBindings* tactical=nullptr,ServerNpcSharedTacticalCoverStats* tactical_stats=nullptr,const GatherRoute* gather=nullptr,const ServerNpcNativeGatherRoute* native_gather=nullptr,const NativeRequestRoute* request=nullptr,const ServerNpcScriptedSchedulerBindings* script=nullptr,ServerNpcScriptedSchedulerStats* script_stats=nullptr,const native_map::ScriptedNpcStartMotionBindings* motion=nullptr,ServerNpcScriptedMotionSchedulerStats* motion_stats=nullptr,const native_map::ScriptedNpcCleanupBindings* cleanup=nullptr,
    native_map::ScriptedNpcCleanupResult* cleaned=nullptr) noexcept {
    if(cleaned)*cleaned={};
    if(cleanup&&(!script||!cleaned))return 0;
    if(!out)return 0;*out={};
    if(script_stats)*script_stats={};
    if(motion_stats)*motion_stats={};
    if(motion&&(!script||!motion_stats||!motion->base||motion->base!=script->start))return 0;
    if(script&&(!shared||!script_stats||!script->run||!script->start||script->start->task!=script->run))return 0;
    if(!a||!b||!classify(b->original_vtable,&out->subclass))return 0;
    const bool jet=out->subclass==NpcSubclassJetpack;
    if(bytes<(jet?0x1028u:out->subclass==NpcSubclassElite?0xfe0u:0xfccu))return 0;
    auto binding=*b;auto& m=binding.maintenance;
    m.selection.jetpack=jet;m.validity.jetpack=jet;
    // One condition namespace and serial table across all stages.
    const auto& reg=m.selection.state.entities;
    const auto& sb=m.selection.grunt.schedule;
    m.selection.grunt.entities=reg;m.selection.state.schedule=sb;m.validity.schedule=sb;
    binding.grunt.lifecycle.conditions=sb.conditions;binding.grunt.entities=reg;
    NativeRequestSession request_session{};
    NativeRequestRoute from_gather{};
    if(native_gather&&native_gather->request_result){
        if(request)return 0;
        from_gather={native_gather->request_selector,native_gather->request_line,native_gather->request_result,native_gather->jet_request_selector,native_gather->jet_request_result,native_gather->request_cone};
        request=&from_gather;
    }
    if(request){
        if(jet!=bool(request->jet)||bytes<(jet?0x1085u:out->subclass==NpcSubclassElite?0xfe0u:0xfd8u)||!request_session.prepare(*request,binding))return 0;
        request=&request_session.route;
    }
    binding.elite.grunt=binding.grunt;binding.jetpack.grunt=binding.grunt;
    for(unsigned i=0;i<3;++i)binding.jet_selector.default_inspect[i]=binding.jetpack.default_inspect[i];
    ServerNativeActivitySession session{};ServerNpcTransitionBindings pending{};
    if(m.animation){
        const auto* api=SiN_GetServerNativeActivityServicesApi(1,sizeof(ServerNativeActivityServicesApi));
        const auto& runtime=*m.animation;const auto& assignment=runtime.npc.transition.assignment;
        if(assignment.translation.actor_class!=(jet?NpcActivityJetpack:NpcActivityGrunt)||
           runtime.inherited.weapons.entries!=reg.entries||runtime.inherited.weapons.count!=reg.count||
           !api||!api->prepare(a,bytes,&assignment,&runtime.inherited,&session))return 0;
        pending=runtime.npc.transition;pending.assignment=session.bound;
        binding.grunt.transition=&pending;binding.elite.grunt.transition=&pending;binding.jetpack.grunt.transition=&pending;
    }
    ServerNpcSharedCueBindings cue_bound{};
    if(cue){cue_bound=*cue;cue_bound.original_vtable=b->original_vtable;cue=&cue_bound;}
    ServerNpcSharedObservationBindings observation_bound{};
    if(observe){observation_bound=*observe;observation_bound.original_vtable=b->original_vtable;observe=&observation_bound;}
    ServerNpcSharedSoundCoverBindings sound_cover_bound{};
    if(sound_cover){sound_cover_bound=*sound_cover;sound_cover_bound.original_vtable=b->original_vtable;sound_cover=&sound_cover_bound;}
    ServerNpcSharedTacticalCoverBindings tactical_bound{};
    if(tactical){tactical_bound=*tactical;tactical_bound.original_vtable=b->original_vtable;tactical=&tactical_bound;}
    ServerNpcGruntEventSpeechBindings speech_bound{};
    ServerNpcSharedParentSession shared_session{};
    // Scoped coherent registry and clock graph for the script routes.
    native_map::ScriptedSequenceControlBindings script_control{};
    native_map::ScriptedSequenceAnimationBindings script_animation{};
    native_map::ScriptedSequencePossessionBindings script_possession{};
    native_map::ScriptedSequenceFinishBindings script_finish{};
    native_map::ScriptedNpcTaskBindings script_run{};
    native_map::ScriptedNpcStartTaskBindings script_start{};
    native_map::ScriptedNpcStartMotionBindings script_motion{};
    native_map::ScriptedNpcCleanupBindings script_cleanup{};
    if(script){
        if(!script->run->finish||!script->run->finish->animation||!script->run->finish->animation->control||
           !script->run->finish->possession)return 0;
        script_finish=*script->run->finish;script_animation=*script_finish.animation;
        script_control=*script_animation.control;
        script_control.entries=reg.entries;script_control.entry_count=reg.count;script_control.now=&now;
        script_possession=*script_finish.possession;script_possession.entries=reg.entries;script_possession.entry_count=reg.count;
        script_possession.now=&now;
        script_animation.control=&script_control;script_finish.animation=&script_animation;script_finish.possession=&script_possession;
        if(m.animation)script_animation.assignment=&session.bound;
        script_run=*script->run;script_run.finish=&script_finish;script_run.lifecycle=&binding.grunt.lifecycle;
        script_start=*script->start;script_start.task=&script_run;
        if(motion){script_motion=*motion;script_motion.base=&script_start;}
        if(cleanup){
            if(cleanup->control!=script->run->finish->animation->control)return 0;
            script_cleanup=*cleanup;script_cleanup.control=&script_control;script_cleanup.conditions=&sb.conditions;
        }
    }
    if(shared){
        ServerGruntTaskBindings forwarded{};
        if(!shared_stats||!bind_shared_parent_internal(a,bytes,now,&binding.grunt,&m.selection,shared,shared_stats,&shared_session,&forwarded,attack,attack_stats,facing,facing_stats,target_animation,target_animation_stats,movement,movement_stats,cue,cue_stats,observe,observe_stats,sound_cover,sound_cover_stats,tactical,tactical_stats))return 0;
        if(script){
            shared_session.script_run=&script_run;shared_session.script_start=&script_start;shared_session.script_stats=script_stats;
            if(motion){shared_session.script_motion=&script_motion;shared_session.script_motion_stats=motion_stats;}
            if(cleanup){shared_session.script_cleanup=&script_cleanup;shared_session.script_cleaned=cleaned;}
        }
        if(request){
            shared_session.native_acquire=&request_session.acquire;
            shared_session.native_line=request->line;
            shared_session.native_cone=request->cone;
        }
        if(native_gather&&native_gather->speech_tasks){
            if(!native_gather->speech_stats)return 0;
            speech_bound=*native_gather->speech_tasks;
            speech_bound.speech.sentence.speech.original_vtable=b->original_vtable;
            shared_session.native_speech=&speech_bound;shared_session.native_speech_stats=native_gather->speech_stats;
            *native_gather->speech_stats={};
        }
        binding.grunt=forwarded;binding.elite.grunt=forwarded;binding.jetpack.grunt=forwarded;
    }
    ServerGruntCombatTaskBindings combat_bound{};
    if(combat){
        if(!stats)return 0;*stats={};
        combat_bound=*combat;combat_bound.grunt=binding.grunt;
        combat_bound.weapon.entities=reg;
    }
    ServerGruntHelperTaskBindings full_bound{};
    if(full){
        if(!combat||!full_stats)return 0;*full_stats={};
        full_bound=*full;full_bound.injury.combat=combat_bound;
    }
    const int status=run_impl(a,bytes,now,&m,&out->maintenance,nullptr,nullptr,&binding,out,combat?&combat_bound:nullptr,stats,full?&full_bound:nullptr,full_stats,gather,native_gather,request);
    if(request && &request->result->scheduler!=out)request->result->scheduler=*out;
    return status;
}
int __cdecl run_native_request(void* a,unsigned bytes,float now,const ServerNpcNativeRequestSchedulerBindings* b,ServerNpcNativeRequestSchedulerResult* out,ServerGruntCombatSchedulerStats* combat_stats,ServerGruntFullTaskStats* full_stats) noexcept{
    if(!out)return 0;*out={};
    if(!a||!b||!b->selector||!b->selector->combat||!b->selector->combat->tail||!b->line)return 0;
    unsigned kind=0;if(!classify(b->scheduler.original_vtable,&kind)||kind==NpcSubclassJetpack||bytes<(kind==NpcSubclassElite?0xfe0u:0xfd8u))return 0;
    if(b->full_tasks&&(!combat_stats||!full_stats))return 0;
    const NativeRequestRoute route{b->selector,b->line,out};
    return run_subclass_impl(a,bytes,now,&b->scheduler,&out->scheduler,b->full_tasks?&b->full_tasks->injury.combat:nullptr,combat_stats,b->full_tasks,full_stats,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,&route);
}
int run_scripted_scheduler(void* a,unsigned bytes,float now,const ServerNpcScriptedSchedulerBindings* b,
    ServerNpcSubclassSchedulerResult* out,ServerNpcScriptedSchedulerStats* script_stats,
    const native_map::ScriptedNpcStartMotionBindings* motion=nullptr,ServerNpcScriptedMotionSchedulerStats* motion_stats=nullptr,
    const native_map::ScriptedNpcCleanupBindings* cleanup=nullptr,native_map::ScriptedNpcCleanupResult* cleaned=nullptr) noexcept {
    if(cleaned)*cleaned={};
    if(motion_stats)*motion_stats={};
    if(out)*out={};if(script_stats)*script_stats={};
    if(!out||!b||!b->scheduler||!b->shared||!script_stats)return 0;
    ServerNpcSharedTaskStats shared_stats{};
    return run_subclass_impl(a,bytes,now,b->scheduler,out,nullptr,nullptr,nullptr,nullptr,b->shared,&shared_stats,
        nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,
        nullptr,nullptr,nullptr,nullptr,nullptr,b,script_stats,motion,motion_stats,cleanup,cleaned);
}
const ServerNpcNativeRequestSchedulerApi native_request_api{1,sizeof(ServerNpcNativeRequestSchedulerApi),&run_native_request};
int __cdecl run_native_jet_request(void* a,unsigned bytes,float now,const ServerNpcNativeJetRequestSchedulerBindings* b,ServerNpcNativeJetRequestSchedulerResult* out,ServerGruntCombatSchedulerStats* combat_stats,ServerGruntFullTaskStats* full_stats) noexcept{
    if(!out)return 0;*out={};
    if(!a||!b||!b->selector||!b->selector->combat||!b->selector->combat->tail||!b->line)return 0;
    unsigned kind=0;if(!classify(b->scheduler.original_vtable,&kind)||kind!=NpcSubclassJetpack||bytes<0x1085)return 0;
    if(b->full_tasks&&(!combat_stats||!full_stats))return 0;
    const NativeRequestRoute route{nullptr,b->line,&out->base,b->selector,out};
    return run_subclass_impl(a,bytes,now,&b->scheduler,&out->base.scheduler,b->full_tasks?&b->full_tasks->injury.combat:nullptr,combat_stats,b->full_tasks,full_stats,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,&route);
}
const ServerNpcNativeJetRequestSchedulerApi native_jet_request_api{1,sizeof(ServerNpcNativeJetRequestSchedulerApi),&run_native_jet_request};
int __cdecl run_subclass(void* a,unsigned bytes,float now,const ServerNpcSubclassSchedulerBindings* b,ServerNpcSubclassSchedulerResult* out) noexcept{
    return run_subclass_impl(a,bytes,now,b,out);
}
int __cdecl run_gather_scheduler(void* a,unsigned bytes,float now,const ServerNpcGatherSchedulerBindings* b,ServerNpcGatherSchedulerResult* out) noexcept{
    if(!out)return 0;out->scheduler={};out->gather={};out->point={};out->gather_calls=0;
    if(!a||!b)return 0;
    auto gather=b->gather;
    const auto& selection=b->scheduler.maintenance.selection;
    gather.base.gate.original_vtable=b->scheduler.original_vtable;
    gather.base.post.schedule=selection.grunt.schedule;
    gather.base.perception.enemy_switch.best.memory.entries=selection.state.entities.entries;
    gather.base.perception.enemy_switch.best.memory.count=selection.state.entities.count;
    const GatherRoute route{&gather,&b->point,out};
    return run_subclass_impl(a,bytes,now,&b->scheduler,&out->scheduler,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,&route);
}
const ServerNpcGatherSchedulerApi gather_scheduler_api{1,sizeof(ServerNpcGatherSchedulerApi),&run_gather_scheduler};
const ServerNpcSubclassSchedulerApi subclass_api{1,sizeof(ServerNpcSubclassSchedulerApi),&classify,&run_subclass};
const ServerGruntSchedulerApi grunt_api{1,sizeof(ServerGruntSchedulerApi),&run_grunt};
const ServerNpcScheduleMaintenanceApi api{1,sizeof(ServerNpcScheduleMaintenanceApi),&run};
}
// Internal linkage boundary for native combat-aware catalog service.
int run_subclass_combat_internal(void* a,unsigned bytes,float now,const ServerNpcSubclassSchedulerBindings* b,ServerNpcSubclassSchedulerResult* out,const ServerGruntCombatTaskBindings* combat,ServerGruntCombatSchedulerStats* stats,const ServerGruntHelperTaskBindings* full,ServerGruntFullTaskStats* full_stats,const ServerNpcSharedTaskBindings* shared,ServerNpcSharedTaskStats* shared_stats,const ServerNpcSharedAttackBindings* attack,ServerNpcSharedAttackStats* attack_stats,const ServerNpcSharedFacingBindings* facing,ServerNpcSharedFacingStats* facing_stats,const ServerNpcSharedTargetAnimationBindings* target_animation,ServerNpcSharedTargetAnimationStats* target_animation_stats,const ServerNpcSharedMovementBindings* movement,ServerNpcSharedMovementStats* movement_stats,const ServerNpcSharedCueBindings* cue,ServerNpcSharedCueStats* cue_stats,const ServerNpcSharedObservationBindings* observe,ServerNpcSharedObservationStats* observe_stats,const ServerNpcSharedSoundCoverBindings* sound_cover,ServerNpcSharedSoundCoverStats* sound_cover_stats,const ServerNpcSharedTacticalCoverBindings* tactical,ServerNpcSharedTacticalCoverStats* tactical_stats,const ServerNpcNativeGatherRoute* native_gather) noexcept{
    if(!combat||!stats)return 0;
    return run_subclass_impl(a,bytes,now,b,out,combat,stats,full,full_stats,shared,shared_stats,attack,attack_stats,facing,facing_stats,target_animation,target_animation_stats,movement,movement_stats,cue,cue_stats,observe,observe_stats,sound_cover,sound_cover_stats,tactical,tactical_stats,nullptr,native_gather);
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerNpcScheduleMaintenanceApi=_SiN_GetServerNpcScheduleMaintenanceApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleMaintenanceApi* __cdecl
SiN_GetServerNpcScheduleMaintenanceApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerNpcScheduleMaintenanceApi)?&api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerGruntSchedulerApi=_SiN_GetServerGruntSchedulerApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerGruntSchedulerApi* __cdecl
SiN_GetServerGruntSchedulerApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerGruntSchedulerApi)?&grunt_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcSubclassSchedulerApi=_SiN_GetServerNpcSubclassSchedulerApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcSubclassSchedulerApi* __cdecl
SiN_GetServerNpcSubclassSchedulerApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcSubclassSchedulerApi)?&subclass_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGatherSchedulerApi=_SiN_GetServerNpcGatherSchedulerApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGatherSchedulerApi* __cdecl
SiN_GetServerNpcGatherSchedulerApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGatherSchedulerApi)?&gather_scheduler_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcNativeRequestSchedulerApi=_SiN_GetServerNpcNativeRequestSchedulerApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcNativeRequestSchedulerApi* __cdecl SiN_GetServerNpcNativeRequestSchedulerApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcNativeRequestSchedulerApi)?&native_request_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcNativeJetRequestSchedulerApi=_SiN_GetServerNpcNativeJetRequestSchedulerApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcNativeJetRequestSchedulerApi* __cdecl SiN_GetServerNpcNativeJetRequestSchedulerApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcNativeJetRequestSchedulerApi)?&native_jet_request_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_RunServerNpcScriptedScheduler=_SiN_RunServerNpcScriptedScheduler")
extern "C" int __cdecl SiN_RunServerNpcScriptedScheduler(void* a,unsigned bytes,float now,
    const sin_port::native_weapon_tables::ServerNpcScriptedSchedulerBindings* b,sin_port::native_weapon_tables::ServerNpcSubclassSchedulerResult* out,
    sin_port::native_weapon_tables::ServerNpcScriptedSchedulerStats* stats) noexcept {
    return sin_port::native_weapon_tables::run_scripted_scheduler(a,bytes,now,b,out,stats);
}

#pragma comment(linker,"/EXPORT:SiN_RunServerNpcScriptedSchedulerWithMotion=_SiN_RunServerNpcScriptedSchedulerWithMotion")
extern "C" int __cdecl SiN_RunServerNpcScriptedSchedulerWithMotion(void* a,unsigned bytes,float now,
    const sin_port::native_weapon_tables::ServerNpcScriptedSchedulerBindings* b,sin_port::native_weapon_tables::ServerNpcSubclassSchedulerResult* out,
    sin_port::native_weapon_tables::ServerNpcScriptedSchedulerStats* stats,const sin_port::native_map::ScriptedNpcStartMotionBindings* motion,
    sin_port::native_weapon_tables::ServerNpcScriptedMotionSchedulerStats* motion_stats) noexcept {
    if(!motion||!motion_stats){if(out)*out={};if(stats)*stats={};if(motion_stats)*motion_stats={};return 0;}
    return sin_port::native_weapon_tables::run_scripted_scheduler(a,bytes,now,b,out,stats,motion,motion_stats);
}

#pragma comment(linker,"/EXPORT:SiN_RunServerNpcScriptedSchedulerWithCleanup=_SiN_RunServerNpcScriptedSchedulerWithCleanup")
extern "C" int __cdecl SiN_RunServerNpcScriptedSchedulerWithCleanup(void* a,unsigned n,float now,
    const sin_port::native_weapon_tables::ServerNpcScriptedSchedulerBindings* b,sin_port::native_weapon_tables::ServerNpcSubclassSchedulerResult* out,
    sin_port::native_weapon_tables::ServerNpcScriptedSchedulerStats* stats,const sin_port::native_map::ScriptedNpcStartMotionBindings* motion,
    sin_port::native_weapon_tables::ServerNpcScriptedMotionSchedulerStats* motion_stats,const sin_port::native_map::ScriptedNpcCleanupBindings* cleanup,
    sin_port::native_map::ScriptedNpcCleanupResult* cleaned) noexcept {
    if(!cleanup||!cleaned||bool(motion)!=bool(motion_stats)){if(out)*out={};if(stats)*stats={};if(motion_stats)*motion_stats={};if(cleaned)*cleaned={};return 0;}
    return sin_port::native_weapon_tables::run_scripted_scheduler(a,n,now,b,out,stats,motion,motion_stats,cleanup,cleaned);
}
