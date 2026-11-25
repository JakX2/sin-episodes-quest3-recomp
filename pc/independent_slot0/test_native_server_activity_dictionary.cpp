#include "native_server_activity_dictionary.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
using namespace sin_port::native_weapon_tables;
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto get=reinterpret_cast<GetServerActivityDictionaryApi>(GetProcAddress(server,server_activity_dictionary_export));
    assert(get&&!GetProcAddress(client,server_activity_dictionary_export)&&!get(0,sizeof(ServerActivityDictionaryApi))&&!get(1,0));
    auto* api=get(1,sizeof(ServerActivityDictionaryApi));assert(api);
    std::vector<ServerEventDictionaryEntry> rows(923);ServerActivityDictionary d{{rows.data(),923,0,4,0},-1};
    int id=-1;assert(api->shared(&d,"ACT_JETPACK_IDLE",208)&&api->lookup(&d,"act_jetpack_idle",&id)&&id==208);
    assert(!api->shared(&d,"DUPLICATE_ID",208)&&d.last_shared_request==208);
    assert(!api->shared(&d,"ACT_JETPACK_IDLE",900)&&d.last_shared_request==900&&d.names.highest==208);
    assert(!api->private_id(&d,"act_jetpack_idle",&id)&&id==-1);
    assert(api->private_id(&d,"CUSTOM_ACTIVITY",&id)&&id==209&&api->private_id(&d,"custom_activity",&id)&&id==209);
    assert(api->clear(&d)&&d.names.generation==5&&d.names.highest==209&&api->reset_highest(&d)&&d.names.highest==0);
    // Synthetic complete-size seed tests contract only; original 921
    // names verified separately by owner audit tool.
    char names[921][32]{};ServerActivitySeed seed[921]{};
    for(int i=0;i<921;++i){std::snprintf(names[i],32,"ACT_TEST_%d",i);seed[i]={i,names[i]};}
    seed[208].name="ACT_JETPACK_IDLE";seed[229].name="ACT_JETPACK_RELOAD_SMG";
    seed[920].id=921;assert(!api->seed(&d,seed,921)&&d.names.count==0);
    seed[920].id=920;seed[920].name=seed[208].name;assert(!api->seed(&d,seed,921)&&d.names.count==0);
    seed[920].name="ACT_HL2MP_JUMP_SLAM";
    assert(api->seed(&d,seed,921)&&d.names.count==921&&d.names.highest==920&&d.last_shared_request==920);
    assert(api->lookup(&d,"act_jetpack_reload_smg",&id)&&id==229);
    assert(api->private_id(&d,"CUSTOM_ACTIVITY",&id)&&id==921);
    assert(api->private_id(&d,"SECOND",&id)&&id==922&&!api->private_id(&d,"THIRD",&id));
    assert(api->clear(&d)&&api->seed(&d,seed,921)&&d.names.highest==922);
    assert(api->private_id(&d,"NEXT",&id)&&id==923&&d.names.generation==6);
    FreeLibrary(server);FreeLibrary(client);std::puts("PASS native activity dictionary, collision rules and complete-size seed contract");
}
