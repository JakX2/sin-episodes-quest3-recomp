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
    FreeLibrary(server);FreeLibrary(client);return 0;
}
