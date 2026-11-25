#include "native_server_event_dictionary.h"
#include "native_server_animation_events.h"
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
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto get=reinterpret_cast<GetServerEventDictionaryApi>(GetProcAddress(server,server_event_dictionary_export));
    auto ge=reinterpret_cast<GetServerAnimationEventsApi>(GetProcAddress(server,server_animation_events_export));
    assert(get&&ge&&!GetProcAddress(client,server_event_dictionary_export)&&!get(0,sizeof(ServerEventDictionaryApi))&&!get(1,0));
    auto* api=get(1,sizeof(ServerEventDictionaryApi));auto* events=ge(1,sizeof(ServerAnimationEventsApi));assert(api&&events);
    ServerEventDictionaryEntry rows[28]{};ServerEventDictionary d{rows,28,0,7,0};
    int id=-1;unsigned flags=0;
    assert(api->lookup(&d,"missing",&id)&&id==-1&&api->flags(&d,1,&flags)&&flags==0xffffffffu);
    assert(api->seed(&d)&&d.count==24&&d.highest==23&&!api->seed(&d));
    for(int i=0;i<24;++i){assert(rows[i].id==i&&!rows[i].private_event&&api->lookup(&d,rows[i].name,&id)&&id==i);}
    assert(api->lookup(&d,"ae_cl_playsound",&id)&&id==14&&api->flags(&d,id,&flags)&&flags==16);
    assert(!api->shared(&d,"AE_CL_PLAYSOUND",99,1)&&!api->shared(&d,"new",14,1)&&d.count==24);
    assert(!api->private_id(&d,"ae_empty",&id)&&id==-1);
    assert(api->shared(&d,"CUSTOM_SHARED",90,4)&&d.highest==90);
    assert(api->private_id(&d,"COMBINE_AE_BEGIN_ALTFIRE",&id)&&id==91);
    assert(api->private_id(&d,"combine_ae_begin_altfire",&id)&&id==91&&d.count==26);
    assert(api->private_id(&d,"COMBINE_AE_ALTFIRE",&id)&&id==92&&d.highest==92);
    // Two named studio records: known client flag, new private event.
    alignas(4) unsigned char data[0x180]{};
    int count=2,offset=0x40;std::memcpy(data+0x18,&count,4);std::memcpy(data+0x1c,&offset,4);
    ServerStudioEvent records[2]{{.1f,-1,0x400,"",0xc0},{.2f,-1,0x400,"",0x80}};
    std::memcpy(data+0x40,records,sizeof(records));std::strcpy(reinterpret_cast<char*>(data+0x100),"AE_CL_PLAYSOUND");
    records[1].unknown4c=0x90;std::memcpy(data+0x90,records+1,sizeof(records[1]));std::strcpy(reinterpret_cast<char*>(data+0x120),"NEW_EVENT");
    ServerStudioEventRegistrationResult rr{};
    assert(api->apply(&d,data,sizeof(data),0,&rr)&&rr.existing==1&&rr.created==1&&d.count==28&&d.highest==93);
    std::memcpy(records,data+0x40,sizeof(records));
    assert(records[0].event==14&&records[0].type==0x410&&records[1].event==93&&records[1].type==0x400);
    ServerAnimationEventSequence sequence{records,2,0,0};ServerAnimationEvent e{};
    assert(events->next(&sequence,0,.5f,0,1,&e)==2&&e.event==93);
    assert(!api->private_id(&d,"OVERFLOW",&id)&&id==-1&&d.count==28);
    assert(!api->reset_highest(&d)&&api->clear(&d)&&d.count==0&&d.generation==8&&d.highest==93);
    assert(api->seed(&d)&&d.highest==93&&api->private_id(&d,"AFTER_CLEAR",&id)&&id==94);
    assert(api->clear(&d)&&api->reset_highest(&d)&&api->seed(&d)&&d.highest==23);
    FreeLibrary(server);FreeLibrary(client);std::puts("PASS original event dictionary, shared seed, private collision, generation and studio registration");
}
