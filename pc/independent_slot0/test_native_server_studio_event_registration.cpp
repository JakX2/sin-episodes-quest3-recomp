#include "native_server_studio_event_registration.h"
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
#include <vector>
using namespace sin_port::native_weapon_tables;
struct F {
    alignas(4) unsigned char data[0x200]{};
    std::vector<int> trace;
    template<class T> void put(unsigned off,T v){std::memcpy(data+off,&v,sizeof(v));}
    template<class T> T get(unsigned off){T v{};std::memcpy(&v,data+off,sizeof(v));return v;}
    void seed(){
        std::memset(data,0,sizeof(data));
        put(0x0c,0x1000u);put(0x18,3);put(0x1c,0x40);
        ServerStudioEvent records[3]{{.1f,42,0,"legacy",0},{.2f,-1,0x400,"known",0xc0},{.3f,-1,0x400,"new",0x80}};
        std::memcpy(data+0x40,records,sizeof(records));
        std::strcpy(reinterpret_cast<char*>(data+0x150),"CLIENT_TEST");
        std::strcpy(reinterpret_cast<char*>(data+0x160),"SERVER_TEST");
        trace.clear();
    }
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto get=reinterpret_cast<GetServerStudioEventRegistrationApi>(GetProcAddress(server,server_studio_event_registration_export));
    auto ge=reinterpret_cast<GetServerAnimationEventsApi>(GetProcAddress(server,server_animation_events_export));
    assert(get&&ge&&!GetProcAddress(client,server_studio_event_registration_export)&&!get(0,sizeof(ServerStudioEventRegistrationApi))&&!get(1,0));
    auto* api=get(1,sizeof(ServerStudioEventRegistrationApi));auto* events=ge(1,sizeof(ServerAnimationEventsApi));assert(api&&events);
    F f;f.seed();
    ServerStudioEventRegistrationBindings b{&f,
        [](void* p,const char* n,int* id) noexcept -> int{
            auto& f=*static_cast<F*>(p);
            assert(f.get<unsigned>(0x0c)==0x3000u);
            if(!std::strcmp(n,"CLIENT_TEST")){f.trace.push_back(1);*id=1101;}
            else{assert(!std::strcmp(n,"SERVER_TEST"));f.trace.push_back(3);*id=-1;}
            return 1;},
        [](void* p,const char* n,int* id) noexcept -> int{
            auto& f=*static_cast<F*>(p);assert(!std::strcmp(n,"SERVER_TEST"));f.trace.push_back(4);*id=1102;return 1;},
        [](void* p,int id,unsigned* flags) noexcept -> int{
            auto& f=*static_cast<F*>(p);assert(id==1101&&f.get<int>(0x94)==1101);f.trace.push_back(2);*flags=0x10;return 1;}
    };
    ServerStudioEventRegistrationResult out{};
    assert(api->apply(f.data,sizeof(f.data),0,&b,&out)&&out.visited==3&&out.named==2&&out.existing==1&&out.created==1&&out.failed_index==-1);
    assert((f.trace==std::vector<int>{1,2,3,4})&&f.get<int>(0x44)==42&&f.get<int>(0x94)==1101&&f.get<unsigned>(0x98)==0x410&&f.get<int>(0xe4)==1102&&f.get<unsigned>(0xe8)==0x400);
    // Copy to typed records for actual server iterator (no raw alias).
    ServerStudioEvent registered[3]{};std::memcpy(registered,f.data+0x40,sizeof(registered));
    ServerAnimationEventSequence v{registered,3,0,0};ServerAnimationEvent event{};
    assert(events->next(&v,0,.5f,0,1,&event)==1&&event.event==42);
    assert(events->next(&v,0,.5f,1,1,&event)==3&&event.event==1102);
    assert(events->next(&v,0,.5f,3,1,&event)==0);
    f.seed();f.put(0x90+0x4c,0x7fffffffu);
    assert(!api->apply(f.data,sizeof(f.data),0,&b,&out)&&out.failed_index==1&&f.trace.empty()&&f.get<unsigned>(0xc)==0x1000u);
    f.seed();f.put(0x90+0x4c,0u);
    assert(!api->apply(f.data,sizeof(f.data),0,&b,&out)&&f.trace.empty());
    f.seed();f.put(0x18,0);
    assert(api->apply(f.data,sizeof(f.data),0,nullptr,&out)&&!out.visited&&f.get<unsigned>(0xc)==0x3000u);
    assert(!api->apply(f.data,0x1f,0,&b,&out));
    FreeLibrary(server);FreeLibrary(client);
    std::puts("PASS native studio named event registration and server event filter handoff");
}
