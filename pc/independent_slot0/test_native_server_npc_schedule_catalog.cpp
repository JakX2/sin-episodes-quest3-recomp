#include "native_server_npc_schedule_catalog.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <cstdio>
using namespace sin_port::native_weapon_tables;
int main(int argc,char** argv){
    assert(argc==3);auto c=LoadLibraryA(argv[1]),s=LoadLibraryA(argv[2]);assert(c&&s);
    auto g=reinterpret_cast<GetServerNpcScheduleCatalogApi>(GetProcAddress(s,server_npc_schedule_catalog_export));
    assert(g&&!GetProcAddress(c,server_npc_schedule_catalog_export)&&!g(0,sizeof(ServerNpcScheduleCatalogApi))&&!g(1,0));
    const auto* api=g(1,sizeof(ServerNpcScheduleCatalogApi));assert(api);
    const ServerScheduleCatalogEntry* rows=nullptr;unsigned count=0;assert(api->all(&rows,&count)&&count==46);
    unsigned family[3]{},tasks=0,conds=0;
    for(unsigned i=0;i<count;++i){
        const auto& e=rows[i];assert(e.family<3&&e.name&&e.task_count);++family[e.family];tasks+=e.task_count;conds+=e.condition_count;
        const ServerScheduleCatalogEntry* found=nullptr;assert(api->find(e.family,e.local_schedule,&found)&&found==&e);
        for(unsigned j=0;j<e.task_count;++j)assert(e.tasks[j].local_task>=0&&e.tasks[j].argument_kind<=ScheduleArgumentDamageToken);
        for(unsigned j=0;j<e.condition_count;++j)assert(e.conditions[j]>=0&&e.conditions[j]<256);
    }
    assert(family[NpcSubclassGround]==37&&family[NpcSubclassElite]==2&&family[NpcSubclassJetpack]==7&&tasks==270&&conds==200);
    const ServerScheduleCatalogEntry* e=nullptr;
    assert(api->find(NpcSubclassElite,118,&e)&&!std::strcmp(e->name,"SCHED_ELITE_RANGE_ATTACK1")&&e->task_count==8);
    assert(api->find(NpcSubclassJetpack,118,&e)&&!std::strcmp(e->name,"SCHED_JETPACK_PATROL")&&e->task_count==8&&e->tasks[0].local_task==165&&e->tasks[7].argument_kind==ScheduleArgumentSchedule&&e->tasks[7].argument==118);
    assert(api->find(NpcSubclassGround,107,&e)&&e->task_count==20&&e->tasks[18].local_task==147&&e->tasks[19].argument==113);
    assert(api->find(NpcSubclassElite,107,&e)&&e->family==NpcSubclassGround);
    assert(api->find(NpcSubclassGround,106,&e)&&e->tasks[2].argument_kind==ScheduleArgumentPrivateActivity&&e->tasks[2].argument==3);
    assert(api->find(NpcSubclassJetpack,105,&e)&&e->tasks[3].argument_kind==ScheduleArgumentDamageToken);
    assert(api->find(NpcSubclassGround,105,&e)&&!e);
    assert(!api->find(99,118,&e));
    FreeLibrary(s);FreeLibrary(c);std::puts("PASS original 46 Grunt-family schedule definitions, local IDs and typed task arguments");
}
