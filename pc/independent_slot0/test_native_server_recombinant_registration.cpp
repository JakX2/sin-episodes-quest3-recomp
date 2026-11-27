#include "native_server_recombinant_registration.h"
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
    alignas(4) unsigned char actor[8]{};
    const char* sound_names[8]{};
    unsigned sound_calls=0,fail_sound=~0u;
    const ServerRecombinantActivityDefinition* defs=nullptr;
    unsigned count=0,seen=0;
    int ids[10]{};
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    const auto get=reinterpret_cast<GetServerRecombinantRegistrationApi>(GetProcAddress(server,server_recombinant_registration_export));
    const auto gd=reinterpret_cast<GetServerActivityDictionaryApi>(GetProcAddress(server,server_activity_dictionary_export));
    assert(get&&gd&&!GetProcAddress(client,server_recombinant_registration_export)&&!get(2,sizeof(ServerRecombinantRegistrationApi)));
    const auto* api=get(1,sizeof(ServerRecombinantRegistrationApi));const auto* da=gd(1,sizeof(ServerActivityDictionaryApi));assert(api&&da);
    ServerEventDictionaryEntry names[32]{};ServerActivityDictionary d{{names,32,0,1,920},0};F f;
    ServerRecombinantRegistrationBindings b{&f,[](void* p,const ServerRecombinantActivityDefinition* def,int id) noexcept {
        auto& f=*static_cast<F*>(p);assert(f.seen<f.count&&def==f.defs+f.seen&&def->original_slot&&def->original_store);f.ids[f.seen++]=id;
    }};
    ServerRecombinantActivityRegistration major{},minor{};
    assert(api->definitions(RecombinantMajor,&f.defs,&f.count)&&f.count==7);
    assert(api->register_activities(RecombinantMajor,&d,&b,&major)&&major.ready&&major.count==7&&f.seen==7&&api->valid(&major));
    for(unsigned i=0;i<7;++i)assert(major.ids[i]==921+int(i)&&major.ids[i]==f.ids[i]);
    assert(api->definitions(RecombinantMinor,&f.defs,&f.count)&&f.count==10);f.seen=0;
    assert(api->register_activities(RecombinantMinor,&d,&b,&minor)&&minor.ready&&minor.count==10&&f.seen==10&&api->valid(&minor));
    assert(d.names.count==16&&minor.ids[0]==928&&minor.ids[1]==major.ids[0]&&minor.ids[2]==929&&minor.ids[3]==930&&minor.ids[4]==931&&minor.ids[9]==936);
    f.seen=0;ServerRecombinantActivityRegistration again{};
    assert(api->register_activities(RecombinantMinor,&d,&b,&again)&&d.names.count==16&&f.seen==10&&api->valid(&again));
    for(unsigned i=0;i<10;++i)assert(again.ids[i]==minor.ids[i]);
    ServerRecombinantPolicyBindings policy{};policy.activities=&d;ServerRecombinantDecisionBindings decision{};decision.policy=&policy;
    ServerRecombinantTaskBindings t{};t.decision=&decision;ServerRecombinantMinorTaskBindings source{};source.task=&t;source.arrival_activity=-1;source.custom_activity=-1;
    ServerRecombinantMinorTaskBindings bound{};
    assert(api->bind_minor(&minor,&source,&bound)&&bound.task==&t&&bound.custom_activity==930&&bound.arrival_activity==931&&source.custom_activity==-1);
    assert(!api->bind_minor(&major,&source,&bound));
    ++d.names.generation;assert(!api->valid(&major)&&!api->valid(&minor)&&!api->bind_minor(&minor,&source,&bound));
    ServerEventDictionaryEntry tiny[1]{};ServerActivityDictionary limited{{tiny,1,0,1,920},0};ServerRecombinantActivityRegistration partial{};
    assert(!api->register_activities(RecombinantMajor,&limited,nullptr,&partial)&&partial.count==1&&!partial.ready&&limited.names.count==1&&!api->valid(&partial));
    ServerEventDictionaryEntry collision[4]{};ServerActivityDictionary shared{{collision,4,0,1,920},0};
    assert(da->shared(&shared,"ACT_FLY_ATTACK",100));
    assert(!api->register_activities(RecombinantMajor,&shared,nullptr,&partial)&&!partial.count&&!partial.ready);

    // Original Major +0x180 22377070 and Minor +0x180 2237b3e0
    // are short, ordered calls to the inherited sound precache service.
    const auto get_sounds=reinterpret_cast<GetServerRecombinantSoundPrecacheApi>(
        GetProcAddress(server,server_recombinant_sound_precache_export));
    assert(get_sounds&&!GetProcAddress(client,server_recombinant_sound_precache_export)&&
        !get_sounds(2,sizeof(ServerRecombinantSoundPrecacheApi))&&
        !get_sounds(1,0));
    const auto* sounds=get_sounds(1,sizeof(ServerRecombinantSoundPrecacheApi));
    assert(sounds);
    ServerRecombinantSoundPrecacheBindings sb{&f,
        [](void* context,void* actor,const char* name) noexcept ->int{
            auto& fixture=*static_cast<F*>(context);
            assert(actor==fixture.actor&&name&&fixture.sound_calls<8);
            const unsigned i=fixture.sound_calls++;
            fixture.sound_names[i]=name;
            return i!=fixture.fail_sound;
        }};
    ServerRecombinantSoundPrecacheResult sr{};
    const auto put_vtable=[&](unsigned vt){std::memcpy(f.actor,&vt,4);};
    put_vtable(recombinant_major_vtable);
    assert(sounds->precache(f.actor,sizeof(f.actor),&sb,&sr)&&
        sr.completed&&sr.kind==RecombinantMajor&&sr.attempted==3&&
        sr.registered==3&&sr.failed_index==-1&&f.sound_calls==3);
    const char* major_sounds[]{"NPC_Leperkin.Idle","NPC_Leperkin.Run","NPC_Leperkin.WalkFast"};
    for(unsigned i=0;i<3;++i)assert(!std::strcmp(f.sound_names[i],major_sounds[i]));
    f.sound_calls=0;put_vtable(recombinant_minor_vtable);
    assert(sounds->precache(f.actor,sizeof(f.actor),&sb,&sr)&&
        sr.completed&&sr.kind==RecombinantMinor&&sr.attempted==4&&
        sr.registered==4&&sr.failed_index==-1&&f.sound_calls==4);
    const char* minor_sounds[]{"NPC_gib_hopper.Idle","NPC_gib_hopper.Walk",
        "NPC_gib_hopper.WalkBackwards","NPC_gib_hopper.Run"};
    for(unsigned i=0;i<4;++i)assert(!std::strcmp(f.sound_names[i],minor_sounds[i]));
    // First failed service halts the sequence without forgetting
    // the already accepted precache requests.
    f.sound_calls=0;f.fail_sound=1;
    assert(!sounds->precache(f.actor,sizeof(f.actor),&sb,&sr)&&
        !sr.completed&&sr.kind==RecombinantMinor&&sr.attempted==2&&
        sr.registered==1&&sr.failed_index==1&&f.sound_calls==2);
    f.sound_calls=0;f.fail_sound=~0u;
    assert(!sounds->precache(f.actor,3,&sb,&sr)&&!sr.completed&&!f.sound_calls);
    assert(!sounds->precache(f.actor,sizeof(f.actor),nullptr,&sr)&&!f.sound_calls);
    put_vtable(0x2249aaa4u);
    assert(!sounds->precache(f.actor,sizeof(f.actor),&sb,&sr)&&
        !sr.completed&&!sr.kind&&!f.sound_calls);
    assert(!sounds->precache(f.actor,sizeof(f.actor),&sb,nullptr)&&!f.sound_calls);
    FreeLibrary(server);FreeLibrary(client);return 0;
}
