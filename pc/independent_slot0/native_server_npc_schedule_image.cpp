#include "native_server_npc_schedule_image.h"
#include <cmath>
#include <cstring>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleCatalogApi* __cdecl SiN_GetServerNpcScheduleCatalogApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleNamespaceApi* __cdecl SiN_GetServerNpcScheduleNamespaceApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcConditionApi* __cdecl SiN_GetServerNpcConditionApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerActivityDictionaryApi* __cdecl SiN_GetServerActivityDictionaryApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
bool private_activity(const ServerGruntRegistration* r,int index,float& value) noexcept{
    const auto* api=SiN_GetServerActivityDictionaryApi(1,sizeof(ServerActivityDictionaryApi));
    constexpr const char* names[]{"ACT_COMBINE_THROW_GRENADE","ACT_COMBINE_LAUNCH_GRENADE","ACT_COMBINE_BUGBAIT","ACT_COMBINE_AR2_ALTFIRE","ACT_MELEE_KICK","ACT_GRUNT_SHOT_IN_HELMET"};
    if(!api||!r||!r->activities_ready||!r->activity_owner||r->activity_generation!=r->activity_owner->names.generation||index<0||index>=6)return false;
    int id=-1;if(!api->lookup(r->activity_owner,names[index],&id)||id<0||id!=r->activities[index]||id>16777216)return false;
    value=static_cast<float>(id);return true;
}
int __cdecl build(void* actor,unsigned family,std::uint32_t base,std::uint32_t fallback,const ServerScheduleImageBindings* b,ServerScheduleImage* out,ServerScheduleImageResult* result) noexcept{
    if(!result)return 0;*result={};result->failed_schedule=result->failed_task=result->failed_condition=-1;
    if(!actor||family>NpcSubclassJetpack||!base||base>0xffffffffu-137u||(fallback>=base&&fallback-base<138u)||!b||!out)return 0;
    const auto* cat=SiN_GetServerNpcScheduleCatalogApi(1,sizeof(ServerNpcScheduleCatalogApi));
    const auto* ns=SiN_GetServerNpcScheduleNamespaceApi(1,sizeof(ServerNpcScheduleNamespaceApi));
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));
    const ServerScheduleCatalogEntry* entries=nullptr;unsigned count=0;
    if(!cat||!ns||!cond||!cat->all(&entries,&count)||count!=46)return 0;
    ServerScheduleImage staged{};staged.family=family;staged.token_base=base;staged.head=staged.fallback_head=fallback;
    // Base initializes first, then subclass. Parser prepends new descriptors.
    for(unsigned pass=0;pass<(family==NpcSubclassGround?1u:2u);++pass){
        const unsigned wanted=pass?family:static_cast<unsigned>(NpcSubclassGround);
        for(unsigned i=0;i<count;++i){
            const auto& e=entries[i];if(e.family!=wanted)continue;
            result->failed_schedule=e.local_schedule;result->failed_task=result->failed_condition=-1;
            if(staged.row_count>=46||e.task_count>270-staged.task_count)return 0;
            int global=-1;if(!ns->to_global(actor,e.local_schedule,&b->namespace_map,&global)||global<npc_global_condition_base)return 0;
            for(unsigned j=0;j<staged.row_count;++j)if(staged.rows[j].descriptor[0]==static_cast<std::uint32_t>(global))return 0;
            auto& row=staged.rows[staged.row_count];row.source=&e;row.task_offset=staged.task_count;
            const auto token=base+3u*staged.row_count;
            row.descriptor[0]=static_cast<std::uint32_t>(global);row.descriptor[1]=token+1;row.descriptor[2]=e.task_count;
            row.descriptor[11]=token+2;row.descriptor[12]=staged.head;
            for(unsigned j=0;j<e.task_count;++j){
                result->failed_task=static_cast<int>(j);const auto& task=e.tasks[j];
                float parameter=task.argument;
                if(task.argument_kind==ScheduleArgumentPrivateActivity){
                    if(!private_activity(b->registration,static_cast<int>(parameter),parameter))return 0;
                    staged.activity_owner=b->registration->activity_owner;staged.activity_generation=b->registration->activity_generation;++result->private_activities;
                }else if(task.argument_kind==ScheduleArgumentSchedule){
                    int mapped=-1,local=-1;
                    if(!ns->to_global(actor,static_cast<int>(parameter),&b->namespace_map,&mapped)||mapped<npc_global_condition_base||
                       !ns->to_local(actor,mapped,&b->namespace_map,&local)||local!=static_cast<int>(parameter))return 0;
                }else if(task.argument_kind==ScheduleArgumentDamageToken){
                    if(!b->damage_token||!b->damage_token(b->context,"DMG_BURN",&parameter))return 0;++result->damage_tokens;
                }else if(task.argument_kind>ScheduleArgumentDamageToken)return 0;
                if(!std::isfinite(parameter))return 0;
                staged.tasks[staged.task_count++]={task.local_task,parameter};++result->tasks;
            }
            result->failed_task=-1;
            for(unsigned j=0;j<e.condition_count;++j){
                result->failed_condition=static_cast<int>(j);ServerNpcConditionResult cr{};
                if(!cond->map(actor,e.conditions[j],&b->namespace_map,&cr)||!cr.mapped||cr.bit<0||cr.bit>=256)return 0;
                row.descriptor[3+cr.bit/32]|=1u<<(cr.bit%32);++result->interrupts;
            }
            staged.head=token;++staged.row_count;++result->rows;
        }
    }
    if(staged.activity_owner&&staged.activity_owner->names.generation!=staged.activity_generation)return 0;
    result->failed_schedule=result->failed_task=result->failed_condition=-1;
    *out=staged;return 1;
}
int __cdecl storage(const ServerScheduleImage* image,std::uint32_t token,const void** out,unsigned* bytes) noexcept{
    if(!out||!bytes)return 0;*out=nullptr;*bytes=0;
    if(!image||!image->token_base||image->row_count>46||image->task_count>270||token<image->token_base||
       (image->activity_owner&&image->activity_owner->names.generation!=image->activity_generation))return 0;
    const auto offset=token-image->token_base,index=offset/3,kind=offset%3;
    if(index>=image->row_count)return 0;const auto& row=image->rows[index];
    if(!row.source||row.task_offset>image->task_count||row.descriptor[2]>image->task_count-row.task_offset)return 0;
    if(kind==0){*out=row.descriptor;*bytes=sizeof(row.descriptor);}
    else if(kind==1){*out=image->tasks+row.task_offset;*bytes=row.descriptor[2]*sizeof(ServerNpcScheduleTask);}
    else{*out=row.source->name;*bytes=static_cast<unsigned>(std::strlen(row.source->name)+1);}
    return 1;
}
const ServerNpcScheduleImageApi api{1,sizeof(ServerNpcScheduleImageApi),&build,&storage};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerNpcScheduleImageApi=_SiN_GetServerNpcScheduleImageApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleImageApi* __cdecl SiN_GetServerNpcScheduleImageApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcScheduleImageApi)?&api:nullptr;
}
