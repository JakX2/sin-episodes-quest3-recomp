#include "native_server_activity_metadata.h"
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
struct Fixture {
    alignas(4) unsigned char studio[2][0x180]{};
    ServerActivityDescriptor views[2]{{studio[0],sizeof(studio[0]),0},{studio[1],sizeof(studio[1]),0}};
    template<class T> void put(unsigned i,unsigned off,T v){std::memcpy(studio[i]+off,&v,sizeof(v));}
    template<class T> T get(unsigned i,unsigned off){T v{};std::memcpy(&v,studio[i]+off,sizeof(v));return v;}
    void seed(unsigned i,const char* activity,const char* event,int id,int weight){
        std::memset(studio[i],0,sizeof(studio[i]));
        put(i,8,0xc0);put(i,0x10,id);put(i,0x14,weight);put(i,0x18,1);put(i,0x1c,0x40);
        ServerStudioEvent e{.25f,-1,0x400,"",0xb0};std::memcpy(studio[i]+0x40,&e,sizeof(e));
        std::strcpy(reinterpret_cast<char*>(studio[i]+0xc0),activity);
        std::strcpy(reinterpret_cast<char*>(studio[i]+0xf0),event);
    }
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto gm=reinterpret_cast<GetServerActivityMetadataApi>(GetProcAddress(server,server_activity_metadata_export));
    auto ga=reinterpret_cast<GetServerActivityDictionaryApi>(GetProcAddress(server,server_activity_dictionary_export));
    auto ge=reinterpret_cast<GetServerEventDictionaryApi>(GetProcAddress(server,server_event_dictionary_export));
    assert(gm&&ga&&ge&&!GetProcAddress(client,server_activity_metadata_export)&&!gm(0,sizeof(ServerActivityMetadataApi))&&!gm(1,0));
    auto* api=gm(1,sizeof(ServerActivityMetadataApi));auto* activity=ga(1,sizeof(ServerActivityDictionaryApi));auto* event=ge(1,sizeof(ServerEventDictionaryApi));assert(api&&activity&&event);
    ServerEventDictionaryEntry ar[8]{},er[32]{};
    ServerActivityDictionary a{{ar,8,0,4,0},-1};ServerEventDictionary e{er,32,0,9,0};
    assert(activity->shared(&a,"ACT_JETPACK_IDLE",208)&&event->seed(&e));
    Fixture f;f.seed(0,"act_jetpack_idle","AE_CL_PLAYSOUND",-1,2);f.seed(1,"CUSTOM_ACTIVITY","CUSTOM_EVENT",-1,-3);
    std::uint32_t generation=3;ServerActivityMetadataView v{f.views,2,&generation};ServerActivityMetadataResult result{};
    assert(api->refresh(&a,&e,&v,&result)&&result.refreshed&&result.visited==2&&result.activity_writes==2&&result.events_existing==1&&result.events_created==1&&generation==4);
    assert(f.get<unsigned char>(0,0xd)==0x30&&f.get<int>(0,0x10)==208&&f.get<int>(0,0x44)==14&&f.get<unsigned>(0,0x48)==0x410);
    assert(f.get<int>(1,0x10)==209&&f.get<int>(1,0x44)==24&&a.names.highest==209&&e.highest==24);
    ServerSequenceChoiceEntry c{};
    assert(api->get(&a,&f.views[1],&c)&&c.activity==209&&c.weight==-3);
    f.put(1,0x10,111); // same generation: no automatic rewrite
    assert(api->refresh(&a,&e,&v,&result)&&!result.refreshed&&!result.visited);
    assert(api->get(&a,&f.views[1],&c)&&c.activity==111);
    assert(api->register_activity(&a,&f.views[1],&c)&&c.activity==209);
    // Generation changes force rewrite even when marked registered.
    assert(activity->clear(&a)&&activity->shared(&a,"ACT_JETPACK_IDLE",208));
    assert(api->refresh(&a,&e,&v,&result)&&generation==5&&f.get<int>(1,0x10)==210&&result.events_existing==2&&!result.events_created);
    assert(f.get<unsigned>(1,0x48)==0x401); // known private now contributes flags
    // Empty activity string leaves existing numeric activity untouched.
    f.seed(1,"","CUSTOM_EVENT",45,7);
    assert(api->get(&a,&f.views[1],&c)&&c.activity==45&&c.weight==7&&f.get<unsigned char>(1,0xd)==0x10);
    generation=0;f.put(1,8,0x7fffffffu);
    assert(!api->refresh(&a,&e,&v,&result)&&result.failed_index==1&&generation==0&&!result.visited);
    f.put(1,8,0xc0);f.put(1,0x1c,0x7fffffffu);
    assert(!api->refresh(&a,&e,&v,&result)&&result.failed_index==1&&generation==0&&result.visited==1);
    f.put(1,0x1c,0x40);
    assert(api->refresh(&a,&e,&v,&result)&&generation==5);

    auto gc=reinterpret_cast<GetServerActivityChoiceApi>(GetProcAddress(server,server_activity_choice_export));
    assert(gc&&!GetProcAddress(client,server_activity_choice_export)&&!gc(0,sizeof(ServerActivityChoiceApi))&&!gc(1,0));
    auto* choice=gc(1,sizeof(ServerActivityChoiceApi));assert(choice);
    // Read immediate children only; recursively publish on refresh.
    std::uint32_t words[]{5,5,0,5};
    ServerActivityGenerationNode groups[]{{words,-1},{words+1,0},{words+2,1},{words+3,0}};
    ServerActivityCompositeView composite{f.views,2,groups,4};
    std::uint32_t observed=0;assert(choice->generation(&composite,&observed)&&observed==5);
    ServerActivityChoiceResult cr{};
    f.seed(0,"ACT_JETPACK_IDLE","AE_CL_PLAYSOUND",-1,-2);
    f.seed(1,"CUSTOM_ACTIVITY","CUSTOM_EVENT",-1,9);
    // Negative current sequence returns before lazy registration of later rows.
    assert(choice->choose(&a,&e,&composite,208,0,true,nullptr,&cr)&&cr.selected==0&&cr.scanned==1&&cr.lazy_registrations==1&&!cr.refresh.refreshed);
    assert(f.get<unsigned char>(0,0xd)==0x10&&f.get<unsigned char>(1,0xd)==0&&words[2]==0);
    assert(choice->choose(&a,&e,&composite,210,-1,false,nullptr,&cr)&&cr.selected==1&&cr.scanned==2&&cr.lazy_registrations==1);
    // A stale immediate child forces complete registration before early exit.
    words[3]=4;
    assert(choice->choose(&a,&e,&composite,208,0,true,nullptr,&cr)&&cr.selected==0&&cr.refresh.refreshed&&cr.refresh.visited==2&&cr.scanned==1&&!cr.lazy_registrations);
    for(auto w:words)assert(w==5);
    assert(f.get<unsigned char>(1,0xd)==0x30);
    words[1]=0xffffffffu;assert(choice->generation(&composite,&observed)&&observed==0xffffffffu);
    f.put(1,0x1c,0x7fffffffu);
    assert(!choice->choose(&a,&e,&composite,208,0,true,nullptr,&cr)&&cr.failed_index==1&&words[1]==0xffffffffu);
    groups[2].parent=2;assert(!choice->generation(&composite,&observed));groups[2].parent=1;
    assert(choice->choose(&a,&e,nullptr,208,0,true,nullptr,&cr)&&cr.selected==0&&!cr.scanned);
    FreeLibrary(server);FreeLibrary(client);std::puts("PASS native activity generation refresh, lazy registration and event dictionary composition");
}
