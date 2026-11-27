#include "native_server_recombinant_activity_transition.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <string>
using namespace sin_port::native_weapon_tables;
struct F {
    alignas(4) unsigned char actor[0xc30]{};
    std::string calls;
    float draw=1.f,scalar=0;
    template<class T>void put(unsigned off,T v){std::memcpy(actor+off,&v,sizeof(v));}
    template<class T>T get(unsigned off){T v{};std::memcpy(&v,actor+off,sizeof(v));return v;}
};
int main(int argc,char** argv){
    assert(argc==3);
    auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    const auto get=reinterpret_cast<GetServerRecombinantActivityTransitionApi>(GetProcAddress(server,server_recombinant_activity_transition_export));
    const auto rg=reinterpret_cast<GetServerRecombinantRegistrationApi>(GetProcAddress(server,server_recombinant_registration_export));
    assert(get&&rg&&!GetProcAddress(client,server_recombinant_activity_transition_export)&&!get(2,sizeof(ServerRecombinantActivityTransitionApi)));
    const auto* api=get(1,sizeof(ServerRecombinantActivityTransitionApi));const auto* reg=rg(1,sizeof(ServerRecombinantRegistrationApi));assert(api&&reg);
    ServerEventDictionaryEntry names[16]{};
    ServerActivityDictionary dictionary{{names,16,0,1,920},0};
    ServerRecombinantActivityRegistration major_ids{};
    assert(reg->register_activities(RecombinantMajor,&dictionary,nullptr,&major_ids)&&reg->valid(&major_ids));
    F f;ServerRecombinantPolicyBindings policy{};policy.context=&f;policy.activities=&dictionary;
    policy.sound=[](void* ctx,void* a,const char* s,int x,int y) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);assert(a==f.actor&&!x&&!y&&!std::strcmp(s,"NPC_Leperkin.Frenzy"));f.calls+='S';return 1;
    };
    ServerRecombinantActivityTransitionBindings b{};b.policy=&policy;b.major_activities=&major_ids;b.context=&f;
    b.random_float=[](void* ctx,float lo,float hi,float* out) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);assert((lo==.9f&&hi==1.1f)||(lo==0.f&&(hi==.75f||hi==.25f)));
        f.calls+='R';*out=f.draw;return 1;
    };
    b.set_frenzy_scalar=[](void* ctx,void* a,float v) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);assert(a==f.actor);f.calls+='F';f.scalar=v;return 1;
    };
    b.set_minor_scalar=[](void* ctx,void* a,float v) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);assert(a==f.actor);f.calls+='M';f.scalar=v;return 1;
    };
    ServerRecombinantActivityTransitionResult o{};
    f.put(0,recombinant_major_vtable);f.draw=1.025f;f.put(0x878,11);
    assert(api->major(f.actor,sizeof(f.actor),major_ids.ids[1],&b,&o)&&o.frenzy&&o.sound_calls==1&&o.scalar_calls==1&&o.draw==1.025f&&f.scalar==1.025f&&f.calls=="SRF");
    f.calls.clear();f.put(0x878,11);f.actor[0xc14]=0;f.actor[0xc15]=1;
    assert(api->major(f.actor,sizeof(f.actor),29,&b,&o)&&o.latch_written&&!o.latch_cleared&&o.latch_a==1&&o.latch_b==0&&f.calls.empty());
    f.put(0x878,27);f.actor[0xc14]=1;f.actor[0xc15]=1;
    assert(api->major(f.actor,sizeof(f.actor),29,&b,&o)&&o.latch_written&&o.latch_cleared&&!o.latch_a&&!o.latch_b);
    f.put(0x878,29);f.actor[0xc14]=1;f.actor[0xc15]=1;
    assert(api->major(f.actor,sizeof(f.actor),27,&b,&o)&&o.latch_written&&o.latch_cleared&&o.latch_a&&!o.latch_b);
    f.put(0x878,29);f.actor[0xc14]=1;f.actor[0xc15]=1;
    assert(api->major(f.actor,sizeof(f.actor),3,&b,&o)&&!o.latch_written&&o.latch_cleared&&!o.latch_a&&!o.latch_b);
    f.put(0x878,3);f.actor[0xc14]=1;f.actor[0xc15]=1;
    assert(api->major(f.actor,sizeof(f.actor),3,&b,&o)&&!o.latch_written&&!o.latch_cleared&&o.latch_a&&o.latch_b);
    f.put(0,recombinant_minor_vtable);f.calls.clear();f.draw=.5f;
    assert(api->minor(f.actor,sizeof(f.actor),1,&b,&o)&&o.scalar_calls==1&&o.low==0&&o.high==.75f&&f.scalar==.5f&&f.calls=="RM");
    f.calls.clear();f.draw=.125f;
    assert(api->minor(f.actor,sizeof(f.actor),10,&b,&o)&&o.scalar_calls==1&&o.high==.25f&&f.scalar==.125f&&f.calls=="RM");
    f.calls.clear();assert(api->minor(f.actor,sizeof(f.actor),100,&b,&o)&&!o.scalar_calls&&f.calls.empty());
    assert(!api->major(f.actor,sizeof(f.actor),major_ids.ids[1],&b,&o));
    f.put(0,recombinant_major_vtable);auto bad=b;bad.major_activities=nullptr;
    assert(!api->major(f.actor,sizeof(f.actor),29,&bad,&o));
    ++dictionary.names.generation;
    assert(!api->major(f.actor,sizeof(f.actor),major_ids.ids[1],&b,&o));
    FreeLibrary(server);FreeLibrary(client);return 0;
}
