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

    // Exercise the real assignment -> +4e4 -> publication path through
    // both native APIs, with a different context for assignment callbacks.
    const auto gc=reinterpret_cast<GetServerRecombinantActivityAssignmentApi>(GetProcAddress(server,server_recombinant_activity_assignment_export));
    assert(gc&&!GetProcAddress(client,server_recombinant_activity_assignment_export)&&
        !gc(0,sizeof(ServerRecombinantActivityAssignmentApi))&&!gc(1,0));
    const auto* composed=gc(1,sizeof(ServerRecombinantActivityAssignmentApi));assert(composed);
    struct Host {F* fixture;int next;bool fail_component;};
    Host host{&f,29,false};
    ServerNpcActivityAssignmentBindings ab{};
    ab.context=&host;ab.translation.actor_class=NpcActivityHosted;
    ServerEventDictionaryEntry er[4]{};ServerEventDictionary ed{er,4,0,1,0};
    ab.translation.events=&ed;ab.dictionary.dictionary=&ed;
    ab.reset.context=&f;
    ab.reset.duration=[](void* p,void* a,int seq,double* out) noexcept ->int{
        auto& f=*static_cast<F*>(p);assert(a==f.actor&&seq==0);*out=2.;return 1;
    };
    ab.reset.distance=[](void*,void*,int,double* out) noexcept ->int{*out=0.;return 1;};
    ab.reset.flags=[](void*,void*,int,unsigned* out) noexcept ->int{*out=1;return 1;};
    ab.weapon_notify=[](void* p,void* a,std::uint32_t h,int act,float duration) noexcept ->int{
        auto& x=*static_cast<Host*>(p);assert(a==x.fixture->actor&&h==0x1234u&&act==x.next&&duration==2.f);
        x.fixture->calls+='W';return 1;
    };
    ab.view_vector=[](void* p,void* a,int act,float* out) noexcept ->int{
        auto& x=*static_cast<Host*>(p);assert(a==x.fixture->actor&&act==x.next);
        out[0]=out[1]=0.f;out[2]=42.f;x.fixture->calls+='V';return 1;
    };
    ab.view_dirty=[](void* p,void* a) noexcept ->int{
        auto& x=*static_cast<Host*>(p);assert(a==x.fixture->actor);
        x.fixture->calls+='D';return 1;
    };
    ab.component_notify=[](void* p,void* a,std::uint32_t c) noexcept ->int{
        auto& x=*static_cast<Host*>(p);assert(a==x.fixture->actor&&c==0x5678u&&x.fixture->get<int>(0x878)==x.next);
        x.fixture->calls+='C';return !x.fail_component;
    };
    // This must not be used by the composed route.
    ab.activity_changed=[](void*,void*,int) noexcept ->int{assert(false);return 0;};
    ab.translation.context=&f;
    ab.translation.metadata=[](void* p,void* a,const ServerActivityCompositeView** out) noexcept ->int{
        auto& f=*static_cast<F*>(p);assert(a==f.actor);f.calls+='Q';*out=nullptr;return 0;
    };
    ServerRecombinantActivityAssignmentBindings cb{&ab,&b};
    ServerRecombinantActivityAssignmentResult co{};
    f.put(0,recombinant_major_vtable);f.put(0x878,27);f.put(0x3ec,0);
    f.put(0x3e1,static_cast<unsigned char>(1));f.put(0x78c,0x1234u);f.put(0x908,0x5678u);
    f.calls.clear();f.actor[0xc14]=1;f.actor[0xc15]=1;
    auto direct=[&](int act){
        host.next=act;const ServerNpcActivityAssignmentInput in{act,0,act,act};
        return composed->assign(f.actor,sizeof(f.actor),&in,&cb,&co);
    };
    assert(direct(29)&&co.assignment.assigned&&co.transition_calls==1&&co.transition.previous==27&&
        co.transition.latch_cleared&&!f.actor[0xc14]&&!f.actor[0xc15]&&f.get<int>(0x878)==29&&f.calls=="WVDC");
    f.calls.clear();
    assert(direct(27)&&co.transition.previous==29&&co.transition.latch_written&&f.actor[0xc14]&&
        !f.actor[0xc15]&&f.calls=="WVC");
    f.calls.clear();
    assert(direct(27)&&!co.transition_calls&&!co.assignment.activity_changed&&f.calls=="WVC");

    f.calls.clear();f.draw=1.025f;
    assert(direct(major_ids.ids[1])&&co.transition.frenzy&&co.transition.previous==27&&f.calls=="WVSRFC");
    // Hosted Minor uses the same assignment but its own native override.
    f.put(0,recombinant_minor_vtable);f.put(0x878,6);f.calls.clear();f.draw=.5f;
    auto mb=b;
    mb.set_minor_scalar=[](void* p,void* a,float v) noexcept ->int{
        auto& f=*static_cast<F*>(p);assert(a==f.actor&&f.get<int>(0x878)==6);
        f.calls+='M';f.scalar=v;return 1;
    };
    cb.transition=&mb;
    assert(direct(1)&&co.transition.kind==RecombinantMinor&&co.transition_calls==1&&co.assignment.assigned&&
        f.get<int>(0x878)==1&&f.scalar==.5f&&f.calls=="WVRMC");
    f.put(0x878,6);f.calls.clear();f.draw=2.f;
    assert(!direct(1)&&co.transition_calls==1&&!co.transition.completed&&!co.assignment.assigned&&
        f.get<int>(0x878)==6&&f.calls=="WVR");
    f.calls.clear();
    assert(composed->request(f.actor,sizeof(f.actor),10,&cb,&co)&&co.assignment.skipped&&!co.transition_calls&&f.calls=="Q");

    // Exercise a non-skipped request through the already recovered scripted
    // activity selection route; the class override is a no-op for activity15.
    ServerActivityCompositeView empty_view{};
    struct Selection {F* fixture;const ServerActivityCompositeView* view;} selection{&f,&empty_view};
    ab.translation.context=&selection;ab.translation.activities=&dictionary;
    ab.translation.metadata=[](void* p,void* a,const ServerActivityCompositeView** out) noexcept ->int{
        auto& s=*static_cast<Selection*>(p);assert(a==s.fixture->actor);s.fixture->calls+='Q';*out=s.view;return 1;
    };
    ab.translation.weapon=[](void*,void*,int act,bool* required,int* out) noexcept ->int{
        *required=false;*out=act;return 1;
    };
    ab.translation.other=[](void*,void*,int act,int* out) noexcept ->int{*out=act;return 1;};
    ab.translation.scripted_sequence=[](void*,void*,int* out) noexcept ->int{*out=0;return 1;};
    f.calls.clear();host.next=15;
    assert(composed->request(f.actor,sizeof(f.actor),15,&cb,&co)&&co.assignment.assigned&&
        co.transition_calls==1&&co.transition.completed&&!co.transition.scalar_calls&&
        f.get<int>(0x878)==15&&f.get<int>(0x880)==15&&f.get<int>(0x884)==0&&f.calls=="QWVC");
    // A later failure is not a transaction rollback: publication occurs
    // before notifying the component, exactly as the underlying assignment.
    f.put(0x878,6);f.calls.clear();f.draw=.125f;host.fail_component=true;
    assert(!direct(10)&&co.transition.completed&&co.assignment.activity_changed&&
        !co.assignment.assigned&&f.get<int>(0x878)==10&&f.calls=="WVRMC");
    host.fail_component=false;
    // Wrong actor or assignment class is rejected before any callbacks.
    f.calls.clear();f.put(0,0u);
    assert(!direct(1)&&!co.transition_calls&&f.calls.empty());
    f.put(0,recombinant_minor_vtable);ab.translation.actor_class=NpcActivityGrunt;
    assert(!direct(1)&&!co.transition_calls&&f.calls.empty());
    ab.translation.actor_class=NpcActivityHosted;f.put(0,recombinant_major_vtable);
    assert(!composed->request(f.actor,0xc15,10,&cb,&co)&&f.calls.empty());
    cb.transition=&b;
    ++dictionary.names.generation;
    assert(!direct(29)&&!co.transition_calls&&f.calls.empty());
    --dictionary.names.generation;
    ++dictionary.names.generation;
    assert(!api->major(f.actor,sizeof(f.actor),major_ids.ids[1],&b,&o));
    FreeLibrary(server);FreeLibrary(client);return 0;
}
