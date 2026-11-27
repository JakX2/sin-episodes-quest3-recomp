#include "native_server_recombinant_schedule.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
using namespace sin_port::native_weapon_tables;
struct F{
    alignas(4) unsigned char actor[0xc70]{},desc[0x48]{};
    const ServerRecombinantScheduleApi* api=nullptr;
    ServerRecombinantScheduleImage im{};
    unsigned resets=0;
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    const auto get=reinterpret_cast<GetServerRecombinantScheduleApi>(GetProcAddress(server,server_recombinant_schedule_export));
    const auto gd=reinterpret_cast<GetServerActivityDictionaryApi>(GetProcAddress(server,server_activity_dictionary_export));
    const auto gs=reinterpret_cast<GetServerNpcScheduleStateApi>(GetProcAddress(server,server_npc_schedule_state_export));
    assert(get&&gd&&gs&&!GetProcAddress(client,server_recombinant_schedule_export)&&!get(2,sizeof(ServerRecombinantScheduleApi)));
    F f;f.api=get(1,sizeof(ServerRecombinantScheduleApi));const auto* da=gd(1,sizeof(ServerActivityDictionaryApi));const auto* sa=gs(1,sizeof(ServerNpcScheduleStateApi));assert(f.api&&da&&sa);
    f.put(f.desc,4,npc_global_condition_base+100);f.put(f.desc,8,0);f.put(f.desc,12,127);f.put(f.desc,16,npc_global_condition_base+227);
    f.put(f.desc,0x34,npc_global_condition_base);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    ServerRecombinantScheduleBindings b{};b.namespaces.context=&f;
    b.namespaces.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept ->int{auto& f=*static_cast<F*>(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    ServerEventDictionaryEntry names[16]{};ServerActivityDictionary dictionary{{names,16,0,1,920},0};b.activities=&dictionary;
    const char* private_names[]{"ACT_FRENZY","ACT_BIG_SLASH","ACT_LEAP_ATTACK","ACT_FLY_ATTACK","ACT_FLY_LAND","ACT_SIDESTEP","ACT_FLY_ATTACK_POUNCE_OFF","ACT_FLY_AWAY","ACT_FLY_LAND_AWAY"};
    int ids[9]{};for(unsigned i=0;i<9;++i)assert(da->private_id(&dictionary,private_names[i],&ids[i]));
    const ServerRecombinantScheduleDefinition* defs=nullptr;unsigned count=0;assert(f.api->catalog(&defs,&count)&&count==19);
    ServerRecombinantScheduleResult r{};
    const unsigned wanted_rows[]{7,12},wanted_tasks[]{32,49},wanted_interrupts[]{47,10},wanted_private[]{2,7};
    ServerNpcScheduleBindings sb{};sb.conditions=b.namespaces;sb.context=&f;
    sb.storage=[](void* p,unsigned token,const void** out,unsigned* n) noexcept ->int{auto& f=*static_cast<F*>(p);return f.api->storage(&f.im,token,out,n);};
    sb.reset_movement=[](void* p,void*,unsigned) noexcept ->int{++static_cast<F*>(p)->resets;return 1;};
    for(unsigned kind=1;kind<=2;++kind){
        f.put(f.actor,0,kind==1?recombinant_major_vtable:recombinant_minor_vtable);
        assert(f.api->build(f.actor,sizeof(f.actor),1000,800,&b,&f.im,&r)&&r.rows==wanted_rows[kind-1]&&r.tasks==wanted_tasks[kind-1]&&r.interrupts==wanted_interrupts[kind-1]&&r.private_activities==wanted_private[kind-1]);
        assert(f.im.head==1000+3*(r.rows-1)&&f.im.fallback==800);
        unsigned previous=800;
        for(unsigned i=0;i<f.im.rows;++i){
            const auto& row=f.im.entries[i];const auto* def=row.source;assert(def->kind==kind&&def->source_va&&def->parser_call);
            unsigned token=0;assert(f.api->find(&f.im,def->local,&token)&&token==1000+3*i);
            const void* p=nullptr;unsigned n=0;assert(f.api->storage(&f.im,token,&p,&n)&&n==52&&row.descriptor[12]==previous&&row.descriptor[0]==unsigned(npc_global_condition_base+100+def->local));previous=token;
            assert(f.api->storage(&f.im,token+2,&p,&n)&&!std::strcmp(static_cast<const char*>(p),def->name));
            ServerNpcScheduleStateResult s{};assert(sa->install(f.actor,sizeof(f.actor),token,10.f,&sb,&s)&&s.count==int(def->task_count));
            for(unsigned j=0;j<def->task_count;++j){
                assert(sa->inspect(f.actor,sizeof(f.actor),&sb,&s)&&s.has_task&&s.index==int(j)&&s.task.id==def->tasks[j].id);
                const auto& t=def->tasks[j];float expected=t.parameter;
                if(t.activity){int id=-1;assert(da->lookup(&dictionary,t.activity,&id));expected=float(id);}
                assert(s.task.parameter==expected&&sa->advance(f.actor,sizeof(f.actor),&sb,&s));
            }
            assert(s.complete);
            for(unsigned bit=0;bit<128;++bit){
                bool expected=false;for(unsigned j=0;j<def->condition_count;++j)expected|=def->conditions[j]==int(bit);
                unsigned actual=0;assert(sa->interrupt(f.actor,sizeof(f.actor),bit,false,&sb,&actual)&&actual==unsigned(expected));
            }
        }
        auto prior=f.im;
        auto bad=b;bad.activities=nullptr;
        assert(!f.api->build(f.actor,sizeof(f.actor),1000,800,&bad,&f.im,&r)&&!std::memcmp(&prior,&f.im,sizeof(prior)));
        assert(!f.api->build(f.actor,sizeof(f.actor),1000,1010,&b,&f.im,&r));
    }
    unsigned token=0;assert(f.api->find(&f.im,91,&token)&&token==1003);
    assert(f.api->find(&f.im,79,&token)&&!token);
    assert(f.resets==19);
    auto prior=f.im;ServerActivityDictionary empty{{names,16,0,1,920},0};b.activities=&empty;
    assert(!f.api->build(f.actor,sizeof(f.actor),1000,800,&b,&f.im,&r)&&r.failed_schedule==80&&r.failed_task==1&&!std::memcmp(&prior,&f.im,sizeof(prior)));
    ++dictionary.names.generation;const void* p=nullptr;unsigned n=0;assert(!f.api->storage(&f.im,1000,&p,&n)&&!f.api->find(&f.im,80,&token));
    FreeLibrary(server);FreeLibrary(client);return 0;
}
