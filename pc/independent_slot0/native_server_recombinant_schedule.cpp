#include "native_server_recombinant_schedule.h"
#include <cstring>
extern "C" const sin_port::native_weapon_tables::ServerNpcScheduleNamespaceApi* __cdecl SiN_GetServerNpcScheduleNamespaceApi(unsigned,unsigned) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerNpcConditionApi* __cdecl SiN_GetServerNpcConditionApi(unsigned,unsigned) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerActivityDictionaryApi* __cdecl SiN_GetServerActivityDictionaryApi(unsigned,unsigned) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
constexpr ServerRecombinantScheduleTask tasks[]{
    {88,3.0f,nullptr,true},
    {89,24.0f,nullptr,false},
    {19,600.0f,nullptr,false},
    {43,0.0f,nullptr,false},
    {121,0.0f,nullptr,false},
    {60,0.0f,nullptr,false},
    {2,1.0f,nullptr,false},
    {98,0.0f,nullptr,false},
    {44,0.0f,nullptr,false},
    {121,0.0f,nullptr,false},
    {116,0.0f,nullptr,false},
    {117,180.0f,nullptr,false},
    {87,82.0f,nullptr,true},
    {4,99999.0f,nullptr,false},
    {116,0.0f,nullptr,false},
    {60,0.0f,nullptr,false},
    {95,0.0f,"ACT_FRENZY",false},
    {116,0.0f,nullptr,false},
    {92,1.0f,nullptr,false},
    {6,0.0f,nullptr,false},
    {116,0.0f,nullptr,false},
    {57,0.0f,nullptr,false},
    {92,1.0f,nullptr,false},
    {7,1.0f,nullptr,false},
    {116,0.0f,nullptr,false},
    {139,0.0f,nullptr,false},
    {43,0.0f,nullptr,false},
    {121,0.0f,nullptr,false},
    {116,0.0f,nullptr,false},
    {60,0.0f,nullptr,false},
    {3,1.0f,nullptr,false},
    {95,0.0f,"ACT_BIG_SLASH",false},
    {60,0.0f,nullptr,false},
    {95,0.0f,"ACT_LEAP_ATTACK",false},
    {86,0.0f,"ACT_FLY_ATTACK",false},
    {67,0.0f,nullptr,false},
    {92,0.0f,"ACT_FLY_LAND",false},
    {2,0.5f,nullptr,false},
    {60,0.0f,nullptr,false},
    {95,0.0f,"ACT_SIDESTEP",false},
    {142,0.0f,nullptr,false},
    {86,27.0f,nullptr,false},
    {131,0.0f,nullptr,false},
    {92,32.0f,nullptr,false},
    {60,0.0f,nullptr,false},
    {92,0.0f,"ACT_FLY_ATTACK_POUNCE_OFF",false},
    {86,0.0f,"ACT_FLY_AWAY",false},
    {131,0.0f,nullptr,false},
    {92,0.0f,"ACT_FLY_LAND_AWAY",false},
    {60,0.0f,nullptr,false},
    {116,0.0f,nullptr,false},
    {86,1.0f,nullptr,false},
    {56,0.0f,nullptr,false},
    {81,0.0f,nullptr,false},
    {60,0.0f,nullptr,false},
    {116,0.0f,nullptr,false},
    {60,0.0f,nullptr,false},
    {68,0.0f,nullptr,false},
    {114,0.5f,nullptr,false},
    {116,0.0f,nullptr,false},
    {60,0.0f,nullptr,false},
    {68,0.0f,nullptr,false},
    {114,0.5f,nullptr,false},
    {60,0.0f,nullptr,false},
    {13,100.0f,nullptr,false},
    {143,0.0f,nullptr,false},
    {121,0.0f,nullptr,false},
    {116,0.0f,nullptr,false},
    {116,0.0f,nullptr,false},
    {140,0.0f,nullptr,false},
    {60,0.0f,nullptr,false},
    {4,1.0f,nullptr,false},
    {90,2.0f,nullptr,false},
    {40,300.0f,nullptr,false},
    {44,0.0f,nullptr,false},
    {121,0.0f,nullptr,false},
    {86,1.0f,nullptr,false},
    {131,0.0f,nullptr,false},
    {141,0.0f,nullptr,false},
    {43,0.0f,nullptr,false},
    {121,0.0f,nullptr,false},
};
constexpr int conditions[]{25,29,30,20,22,21,23,36,33,17,16,67,33,25,17,25,10,17,25,8,16,17,35,24,46,53,48,51,54,2,25,10,8,16,17,24,35,51,52,53,48,54,2,25,29,17,68,67,68,67,11,12,20,11,12,20,25};
constexpr ServerRecombinantScheduleDefinition defs[]{
    {1,80,"SCHED_MAJOR_CHASE_ENEMY",0x224a0980u,0x2237a759u,tasks+0,6,conditions+0,12},
    {1,81,"SCHED_MAJOR_MOVE_TO_AMBUSH",0x224a0828u,0x2237a797u,tasks+6,7,conditions+12,3},
    {1,82,"SCHED_MAJOR_WAIT_AMBUSH",0x224a0788u,0x2237a7d5u,tasks+13,1,conditions+15,3},
    {1,83,"SCHED_MAJOR_FRENZY",0x224a06e0u,0x2237a813u,tasks+14,3,nullptr,0},
    {1,84,"SCHED_MAJOR_IDLE_STAND",0x224a0570u,0x2237a851u,tasks+17,3,conditions+18,12},
    {1,85,"SCHED_MAJOR_ALERT_STAND",0x224a03c0u,0x2237a88fu,tasks+20,4,conditions+30,13},
    {1,86,"SCHED_MAJOR_PHYSICS_TOSS",0x224a0208u,0x2237a8cdu,tasks+24,8,conditions+43,4},
    {2,80,"SCHED_MINOR_LEAP_ATTACK_ENEMY",0x224a2620u,0x2237e206u,tasks+32,7,conditions+47,2},
    {2,91,"SCHED_MINOR_SIDESTEP",0x224a2598u,0x2237e244u,tasks+39,1,nullptr,0},
    {2,81,"SCHED_MINOR_LAND_RECOVER",0x224a24a0u,0x2237e282u,tasks+40,5,nullptr,0},
    {2,82,"SCHED_MINOR_POUNCE_OFF",0x224a2368u,0x2237e2c0u,tasks+45,5,conditions+49,1},
    {2,83,"SCHED_MINOR_WAKE_ANGRY_NO_DISPLAY",0x224a2278u,0x2237e2feu,tasks+50,5,nullptr,0},
    {2,84,"SCHED_MINOR_STAND_AND_SPIT",0x224a2188u,0x2237e33cu,tasks+55,4,conditions+50,3},
    {2,90,"SCHED_MINOR_TRYING_TO_MORPH_UP",0x224a20c8u,0x2237e37au,tasks+59,4,nullptr,0},
    {2,85,"SCHED_MINOR_BACK_UP_AND_SPIT",0x224a1fa0u,0x2237e3b8u,tasks+63,5,conditions+53,3},
    {2,86,"SCHED_MINOR_HOP_RANDOMLY",0x224a1f18u,0x2237e3f6u,tasks+68,2,nullptr,0},
    {2,87,"SCHED_MINOR_HARASS_ENEMY",0x224a1e10u,0x2237e434u,tasks+70,6,conditions+56,1},
    {2,88,"SCHED_MINOR_FALL_TO_GROUND",0x224a1d78u,0x2237e472u,tasks+76,2,nullptr,0},
    {2,89,"SCHED_MINOR_ATTACK_BARREL",0x224a1cd0u,0x2237e4b0u,tasks+78,3,nullptr,0},
};
int __cdecl catalog(const ServerRecombinantScheduleDefinition** out,unsigned* n) noexcept{
    if(!out||!n)return 0;*out=defs;*n=sizeof(defs)/sizeof(defs[0]);return 1;
}
bool valid(const ServerRecombinantScheduleImage* im) noexcept{
    return im&&im->base&&im->base<=0xffffffffu-35u&&
        (im->kind==RecombinantMajor?im->rows==7:im->kind==RecombinantMinor&&im->rows==12)&&
        im->task_count<=64&&im->activities&&im->generation==im->activities->names.generation;
}
int __cdecl build(void* a,unsigned n,unsigned base,unsigned fallback,const ServerRecombinantScheduleBindings* b,ServerRecombinantScheduleImage* out,ServerRecombinantScheduleResult* o) noexcept {
    if(!o)return 0;*o={};o->failed_schedule=o->failed_task=o->failed_condition=-1;
    if(!a||n<4||!base||base>0xffffffffu-35u||(fallback>=base&&fallback-base<36u)||!b||!b->activities||!out)return 0;
    unsigned vt=0;std::memcpy(&vt,a,4);const unsigned kind=vt==recombinant_major_vtable?RecombinantMajor:vt==recombinant_minor_vtable?RecombinantMinor:0;
    if(!kind)return 0;
    const auto* ns=SiN_GetServerNpcScheduleNamespaceApi(1,sizeof(ServerNpcScheduleNamespaceApi));
    const auto* ca=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));
    const auto* da=SiN_GetServerActivityDictionaryApi(1,sizeof(ServerActivityDictionaryApi));
    if(!ns||!ca||!da)return 0;
    ServerRecombinantScheduleImage next{};next.kind=kind;next.base=base;next.head=next.fallback=fallback;
    next.activities=b->activities;next.generation=b->activities->names.generation;
    auto mapped=[&](int local,int& global){
        int reverse=-1;return ns->to_global(a,local,&b->namespaces,&global)&&global>=npc_global_condition_base&&ns->to_local(a,global,&b->namespaces,&reverse)&&reverse==local;
    };
    for(const auto& def:defs){
        if(def.kind!=kind)continue;o->failed_schedule=def.local;
        o->failed_task=o->failed_condition=-1;
        if(next.rows>=12||def.task_count>64-next.task_count)return 0;
        int global=-1;if(!mapped(def.local,global))return 0;
        for(unsigned j=0;j<next.rows;++j)if(next.entries[j].descriptor[0]==static_cast<unsigned>(global))return 0;
        const unsigned token=base+3*next.rows;
        auto& row=next.entries[next.rows];row.source=&def;row.offset=next.task_count;
        row.descriptor[0]=global;row.descriptor[1]=token+1;row.descriptor[2]=def.task_count;
        row.descriptor[11]=token+2;row.descriptor[12]=next.head;
        for(unsigned j=0;j<def.task_count;++j){
            o->failed_task=j;const auto& t=def.tasks[j];float arg=t.parameter;
            if(t.activity){
                int activity=-1;if(!da->lookup(b->activities,t.activity,&activity)||activity<0||activity>16777216)return 0;
                arg=static_cast<float>(activity);++o->private_activities;
            }else if(t.schedule_reference){int other=-1;if(!mapped(static_cast<int>(arg),other))return 0;}
            next.tasks[next.task_count++]={t.id,arg};++o->tasks;
        }
        o->failed_task=-1;
        for(unsigned j=0;j<def.condition_count;++j){
            o->failed_condition=j;ServerNpcConditionResult cr{};
            if(!ca->map(a,def.conditions[j],&b->namespaces,&cr)||!cr.mapped||cr.bit<0||cr.bit>=256)return 0;
            row.descriptor[3+cr.bit/32]|=1u<<(cr.bit%32);++o->interrupts;
        }
        next.head=token;++next.rows;++o->rows;
    }
    if(!valid(&next))return 0;*out=next;o->failed_schedule=o->failed_task=o->failed_condition=-1;return 1;
}
int __cdecl storage(const ServerRecombinantScheduleImage* im,unsigned token,const void** out,unsigned* n) noexcept{
    if(!out||!n)return 0;*out=nullptr;*n=0;
    if(!valid(im)||token<im->base)return 0;
    const unsigned i=(token-im->base)/3,k=(token-im->base)%3;
    if(i>=im->rows)return 0;const auto& row=im->entries[i];
    if(!row.source||row.offset>im->task_count||row.descriptor[2]>im->task_count-row.offset)return 0;
    if(k==0){*out=row.descriptor;*n=sizeof(row.descriptor);}
    else if(k==1){*out=im->tasks+row.offset;*n=row.descriptor[2]*sizeof(ServerNpcScheduleTask);}
    else{*out=row.source->name;*n=static_cast<unsigned>(std::strlen(row.source->name)+1);}
    return 1;
}
int __cdecl find(const ServerRecombinantScheduleImage* im,int local,unsigned* out) noexcept{
    if(!out)return 0;*out=0;if(!valid(im))return 0;
    for(unsigned i=0;i<im->rows;++i)if(im->entries[i].source&&im->entries[i].source->local==local){*out=im->base+3*i;break;}
    return 1;
}
const ServerRecombinantScheduleApi api{1,sizeof(ServerRecombinantScheduleApi),&catalog,&build,&storage,&find};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantScheduleApi=_SiN_GetServerRecombinantScheduleApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantScheduleApi* __cdecl SiN_GetServerRecombinantScheduleApi(unsigned v,unsigned n) noexcept{
    using namespace sin_port::native_weapon_tables;return v==1&&n==sizeof(ServerRecombinantScheduleApi)?&api:nullptr;
}
