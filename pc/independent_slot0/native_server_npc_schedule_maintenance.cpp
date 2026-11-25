#include "native_server_npc_schedule_maintenance.h"
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
namespace sin_port::native_weapon_tables {
namespace {
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
    bool pick(bool failed) noexcept {
        if(!resolution->select(a,bytes,now,failed,&b.selection,&out.selection))return false;
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
int __cdecl run(void* actor,unsigned bytes,float now,const ServerNpcScheduleMaintenanceBindings* b,ServerNpcScheduleMaintenanceResult* out) noexcept {
    if(!out)return 0;*out={};
    if(!actor||bytes<0xdd8||!b||!std::isfinite(now))return 0;
    const auto* sa=SiN_GetServerNpcScheduleStateApi(1,sizeof(ServerNpcScheduleStateApi));
    const auto* ca=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));
    const auto* st=SiN_GetServerNpcStateReconsiderApi(1,sizeof(ServerNpcStateApi));
    const auto* re=SiN_GetServerNpcScheduleResolutionApi(1,sizeof(ServerNpcScheduleResolutionApi));
    const auto* va=SiN_GetServerNpcScheduleValidityApi(1,sizeof(ServerNpcScheduleValidityApi));
    if(!sa||!ca||!st||!re||!va)return 0;
    Call q{actor,bytes,now,*b,*out,sa,ca,st,re,va,b->selection.grunt.schedule};
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
                if(!b->before_selection||!b->before_selection(b->context,actor))return 0;
            }
            if(!st->should_reconsider(actor,bytes,&b->selection.state,&out->ideal))return 0;
            if(out->ideal.reconsider){
                if(!st->choose_ideal(actor,bytes,&b->selection.state,&out->ideal))return 0;
                write(actor,0x858,out->ideal.ideal);
            }
            bool failed=false;if(!q.test(0x21,failed))return 0;
            if(failed&&read<int>(actor,0x850)==read<int>(actor,0x858)){
                if(!q.pick(true))return 0;
            }else{
                if(!st->apply(actor,bytes,read<int>(actor,0x858),now,&b->selection.state,&out->applied)||!q.pick(false))return 0;
            }
        }
        if(!read<std::uint32_t>(actor,0x798)&&!q.pick(false))return 0;
        if(!q.inspect())return 0;
        if(!out->state.schedule||!out->state.count){
            out->missing_schedule=1;return q.transition(true);
        }
        if(out->state.status==0){
            if(!out->state.has_task||!b->start_task)return 0;
            // All eight verified +430 targets are the same no-op.
            const auto task=out->state.task;
            write(actor,0x7a4,1);write(actor,0x7b0,0);write(actor,0x7ac,now);
            ++out->started;if(!b->start_task(b->context,actor,&task))return 0;
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
        if(!q.inspect()||!out->state.has_task||!b->run_task)return 0;
        const auto task=out->state.task;
        for(unsigned inner=0;inner<8;++inner){
            ++out->runs;if(!b->run_task(b->context,actor,&task))return 0;
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
const ServerNpcScheduleMaintenanceApi api{1,sizeof(ServerNpcScheduleMaintenanceApi),&run};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerNpcScheduleMaintenanceApi=_SiN_GetServerNpcScheduleMaintenanceApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleMaintenanceApi* __cdecl
SiN_GetServerNpcScheduleMaintenanceApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerNpcScheduleMaintenanceApi)?&api:nullptr;
}
