#include "native_server_recombinant_damage.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <string>
#include <initializer_list>
using namespace sin_port::native_weapon_tables;
struct F {
    alignas(4) unsigned char actor[0xc50]{},source[4]{},info[0x44]{};
    sin_independent::EntityHandleEntry entries[2]{};
    float draws[2]{};
    unsigned used=0;int source_class=1,expected_health=100;
    std::string calls;
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T>T get(void* p,unsigned off){T v{};std::memcpy(&v,static_cast<unsigned char*>(p)+off,sizeof(v));return v;}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    const auto get=reinterpret_cast<GetServerRecombinantDamageApi>(GetProcAddress(server,server_recombinant_damage_export));
    assert(get&&!GetProcAddress(client,server_recombinant_damage_export)&&!get(2,sizeof(ServerRecombinantDamageApi)));
    const auto* api=get(1,sizeof(ServerRecombinantDamageApi));assert(api);
    F f;f.put(f.actor,0,recombinant_major_vtable);f.put(f.actor,0x98,100);f.put(f.actor,0x9c,100);f.put(f.actor,0x878,10);
    f.entries[1].entity=reinterpret_cast<CBaseEntity*>(f.source);f.entries[1].serial=7;
    ServerRecombinantPolicyBindings policy{};policy.entries=f.entries;policy.entry_count=2;
    ServerRecombinantDamageBindings b{&policy,&f};
    b.extent=[](void* p,void* a,unsigned* n) noexcept ->int{auto& f=*static_cast<F*>(p);*n=a==f.source?sizeof(f.source):0;return *n!=0;};
    b.classification=[](void* p,void* a,int* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.source);f.calls+='C';*out=f.source_class;return 1;};
    b.color_fade=[](void* p,void* a,int x,int y) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor&&x==5&&y==50);f.calls+='F';return 1;};
    b.parent_damage=[](void* p,void* a,const void* info,unsigned n,int* out) noexcept ->int{
        auto& f=*static_cast<F*>(p);assert(a==f.actor&&info==f.info&&n==sizeof(f.info));f.calls+='D';*out=7;return 1;
    };
    b.parent_ignite=[](void* p,void* a,const ServerRecombinantIgniteRequest* in) noexcept ->int{
        auto& f=*static_cast<F*>(p);assert(a==f.actor&&in->lifetime==100.f&&in->mode==1&&!in->arg2&&!in->arg3&&!in->arg4&&!in->arg5);f.calls+='I';
        f.actor[0xbf]|=8;return 1;
    };
    b.random_float=[](void* p,float lo,float hi,float* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(lo==-2&&hi==2&&f.used<2);f.calls+='R';*out=f.draws[f.used++];return 1;};
    b.health_changed=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor&&f.get<int>(f.actor,0x9c)==f.expected_health);f.calls+='H';return 1;};
    b.pending_activity=[](void* p,void* a,int v) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor&&v==(f.get<int>(a,0x878)==10?113:f.get<int>(a,0x878)==6?112:111));f.calls+='A';return 1;};
    ServerRecombinantDamageResult o{};ServerRecombinantDamageExpiryResult ex{};
    f.put(f.info,0x24,0x7001u);f.put(f.info,0x38,8u);f.put(f.info,0x2c,4.f);
    f.put(f.source,0,recombinant_major_vtable);
    assert(api->damage_major(f.actor,sizeof(f.actor),f.info,sizeof(f.info),10.f,&b,&o)&&o.immune&&o.source_class==10&&!o.parent_called&&f.calls.empty());
    f.put(f.source,0,0x2249aaa4u);f.source_class=10;
    assert(api->damage_major(f.actor,sizeof(f.actor),f.info,sizeof(f.info),10.f,&b,&o)&&o.immune&&f.calls=="C");
    f.source_class=1;f.calls.clear();
    assert(api->damage_major(f.actor,sizeof(f.actor),f.info,sizeof(f.info),10.f,&b,&o)&&o.accumulated&&o.accumulated_damage==4.f&&o.deadline==15.f&&!o.threshold_reached&&o.parent_called&&o.damage_result==7&&f.calls=="CFD");
    assert(api->expire_major(f.actor,sizeof(f.actor),14.99f,&ex)&&!ex.cleared&&ex.prior==4.f);
    assert(api->expire_major(f.actor,sizeof(f.actor),15.f,&ex)&&ex.cleared&&f.get<float>(f.actor,0xc3c)==0.f&&f.get<float>(f.actor,0xc40)==15.f);
    f.calls.clear();f.put(f.info,0x24,0x8001u);f.put(f.info,0x2c,5.f);
    assert(api->damage_major(f.actor,sizeof(f.actor),f.info,sizeof(f.info),16.f,&b,&o)&&!o.source_live&&o.accumulated_damage==5.f&&!o.threshold_reached&&f.calls=="FD");
    f.calls.clear();f.draws[0]=-2.f;f.draws[1]=2.f;
    assert(api->damage_major(f.actor,sizeof(f.actor),f.info,sizeof(f.info),17.f,&b,&o)&&o.accumulated_damage==10.f&&o.deadline==22.f&&o.threshold_reached&&o.ignited&&o.ignition.random_calls==2&&o.ignition.new_health==60&&o.ignition.health_written&&o.ignition.activity==113&&f.calls=="FIRRHAD");
    f.calls.clear();assert(api->damage_major(f.actor,sizeof(f.actor),f.info,sizeof(f.info),18.f,&b,&o)&&o.immune&&!o.color_called&&!o.accumulated&&f.calls.empty());
    f.put(f.info,0x38,0x20000008u);
    assert(api->damage_major(f.actor,sizeof(f.actor),f.info,sizeof(f.info),18.f,&b,&o)&&!o.immune&&o.color_called&&!o.accumulated&&f.calls=="FD");
    f.calls.clear();f.put(f.info,0x38,0u);
    assert(api->damage_major(f.actor,sizeof(f.actor),f.info,sizeof(f.info),19.f,&b,&o)&&!o.color_called&&!o.accumulated&&f.calls=="D");
    ServerRecombinantIgniteRequest req{100.f,1,0,0,0,0};ServerRecombinantIgniteResult ir{};
    f.put(f.actor,0x9c,39);f.put(f.actor,0x878,6);f.calls.clear();f.used=0;f.draws[0]=-2;
    assert(api->ignite_major(f.actor,sizeof(f.actor),&req,&b,&ir)&&ir.random_calls==1&&!ir.health_written&&ir.new_health==39&&ir.activity==112&&f.calls=="IRA");
    // First threshold equality takes a fresh draw, which may even raise health.
    f.put(f.actor,0x9c,40);f.expected_health=40;f.put(f.actor,0x878,1);f.calls.clear();f.used=0;f.draws[1]=1;
    assert(api->ignite_major(f.actor,sizeof(f.actor),&req,&b,&ir)&&ir.random_calls==2&&ir.new_health==55&&ir.health_written&&ir.activity==111&&f.calls=="IRRHA");
    f.put(f.actor,0x9c,30);f.put(f.actor,0x878,7);f.calls.clear();f.used=0;
    assert(api->ignite_major(f.actor,sizeof(f.actor),&req,&b,&ir)&&!ir.pending_called&&ir.activity==-1&&f.calls=="IR");
    f.put(f.actor,0xc40,0.f);f.put(f.actor,0xc3c,12.f);
    assert(api->expire_major(f.actor,sizeof(f.actor),99.f,&ex)&&!ex.cleared&&ex.prior==12.f);
    // Phase 1618: the native Major damage/ignition path must request a
    // pending activity through the existing Phase 1617 lifecycle, not
    // through the former stand-alone host +4dc callback.
    const auto get_composed=reinterpret_cast<GetServerRecombinantDamagePendingApi>(
        GetProcAddress(server,server_recombinant_damage_pending_export));
    assert(get_composed&&!GetProcAddress(client,server_recombinant_damage_pending_export)&&
        !get_composed(0,sizeof(ServerRecombinantDamagePendingApi))&&
        !get_composed(1,0));
    const auto* composed=get_composed(1,sizeof(ServerRecombinantDamagePendingApi));
    assert(composed);
    const auto get_registration=reinterpret_cast<GetServerRecombinantRegistrationApi>(
        GetProcAddress(server,server_recombinant_registration_export));
    assert(get_registration);
    const auto* registration=get_registration(1,sizeof(ServerRecombinantRegistrationApi));
    assert(registration);
    ServerEventDictionaryEntry major_names[16]{};
    ServerActivityDictionary major_dictionary{{major_names,16,0,1,920},0};
    ServerRecombinantActivityRegistration major_ids{};
    assert(registration->register_activities(RecombinantMajor,&major_dictionary,nullptr,&major_ids));
    assert(registration->valid(&major_ids));
    policy.activities=&major_dictionary;
    ServerRecombinantActivityTransitionBindings activity{};
    activity.policy=&policy;activity.major_activities=&major_ids;
    ServerNpcTransitionBindings pending{};
    pending.assignment.translation.actor_class=NpcActivityHosted;
    pending.assignment.translation.metadata=[](void*,void*,const ServerActivityCompositeView** view) noexcept ->int{
        *view=nullptr;return 0;
    };
    ServerRecombinantPendingActivityBindings pending_bindings{&pending,&activity};
    ServerRecombinantDamagePendingBindings combined{&b,&pending_bindings};
    ServerRecombinantDamagePendingResult session{};
    f.put(f.actor,0,recombinant_major_vtable);
    f.put(f.actor,0x878,10);f.put(f.actor,0x880,10);f.put(f.actor,0x9c,35);
    f.put(f.actor,0x98,100);f.expected_health=35;
    f.used=0;f.draws[0]=-2.f;f.calls.clear();
    assert(composed->ignite_major(f.actor,sizeof(f.actor),&req,&combined,&session));
    assert(session.ignition.completed&&session.ignition.pending_called&&session.ignition.activity==113&&
        session.ignition.random_calls==1&&!session.ignition.health_written&&
        session.pending_calls==1&&session.requested_activity==113&&
        session.pending.pending.skipped&&!session.pending.transition_calls&&
        f.get<int>(f.actor,0x880)==113&&f.get<int>(f.actor,0x878)==10&&f.calls=="IR");

    // A real threshold-crossing Major damage call must propagate the
    // private Ignite request, then return to the inherited damage owner.
    f.actor[0xbf]=0;f.put(f.actor,0xc3c,9.f);
    f.put(f.actor,0x880,10);f.put(f.info,0x24,0x8001u);
    f.put(f.info,0x38,8u);f.put(f.info,0x2c,1.f);
    f.used=0;f.calls.clear();
    assert(composed->damage_major(f.actor,sizeof(f.actor),f.info,sizeof(f.info),20.f,&combined,&session));
    assert(session.damage.completed&&session.damage.ignited&&session.damage.threshold_reached&&
        session.damage.parent_called&&session.damage.damage_result==7&&
        session.damage.ignition.activity==113&&session.pending_calls==1&&
        session.requested_activity==113&&session.pending.pending.skipped&&
        f.get<int>(f.actor,0x880)==113&&f.get<int>(f.actor,0x878)==10&&f.calls=="FIRD");
    f.calls.clear();
    assert(composed->damage_major(f.actor,sizeof(f.actor),f.info,sizeof(f.info),21.f,&combined,&session));
    assert(session.damage.immune&&!session.damage.ignited&&!session.pending_calls&&f.calls.empty());

    // Complete the other half of the session: native pending selection
    // followed by the Phase 1617 update and Major class override.
    const auto get_pending=reinterpret_cast<GetServerRecombinantPendingActivityApi>(
        GetProcAddress(server,server_recombinant_pending_activity_export));
    assert(get_pending);
    const auto* pending_api=get_pending(1,sizeof(ServerRecombinantPendingActivityApi));
    assert(pending_api);
    unsigned char sequence_bytes[0x40]{};
    sequence_bytes[0x0d]=0x10;
    f.put(sequence_bytes,0x10,113);
    f.put(sequence_bytes,0x14,1);
    ServerActivityDescriptor descriptor{sequence_bytes,sizeof(sequence_bytes),0};
    ServerActivityGenerationNode gen_root{&major_dictionary.names.generation,-1};
    ServerActivityCompositeView sequence_view{&descriptor,1,&gen_root,1};
    ServerEventDictionaryEntry event_names[4]{};
    ServerEventDictionary event_dictionary{event_names,4,0,1,0};
    auto& assignment=pending.assignment;
    assignment.translation.context=&sequence_view;
    assignment.translation.metadata=[](void* context,void* actor,const ServerActivityCompositeView** out) noexcept ->int{
        assert(actor);*out=static_cast<const ServerActivityCompositeView*>(context);return 1;
    };
    assignment.translation.activities=&major_dictionary;
    assignment.translation.events=&event_dictionary;
    assignment.translation.weapon=[](void*,void*,int requested,bool* required,int* out) noexcept ->int{
        *required=false;*out=requested;return 1;
    };
    assignment.translation.other=[](void*,void*,int requested,int* out) noexcept ->int{
        *out=requested;return 1;
    };
    assignment.translation.random.random=[](void*,int lo,int hi,int,int* out) noexcept ->int{
        assert(lo==0&&hi==0);*out=0;return 1;
    };
    assignment.reset.duration=[](void*,void*,int seq,double* out) noexcept ->int{
        assert(seq==0);*out=1.;return 1;
    };
    assignment.reset.distance=[](void*,void*,int,double* out) noexcept ->int{
        *out=0.;return 1;
    };
    assignment.reset.flags=[](void*,void*,int,unsigned* out) noexcept ->int{
        *out=1;return 1;
    };
    assignment.dictionary.dictionary=&event_dictionary;
    assignment.dictionary.descriptor=[](void*,void*,int,void**,unsigned*,unsigned*) noexcept ->int{return 0;};
    assignment.locomotion=[](void*,void*,int,bool* out) noexcept ->int{*out=false;return 1;};
    assignment.view_vector=[](void*,void*,int,float* out) noexcept ->int{
        out[0]=out[1]=out[2]=0.f;return 1;
    };
    assignment.view_dirty=[](void*,void*) noexcept ->int{return 1;};
    assignment.component_notify=[](void*,void* actor,std::uint32_t c) noexcept ->int{
        assert(actor&&c==0x5678u);return 1;
    };
    assignment.activity_changed=[](void*,void*,int) noexcept ->int{
        assert(false);return 0; // the native Major override must replace it
    };
    pending.nodes=[](void*,void*,const ServerTransitionView** out) noexcept ->int{
        *out=nullptr;return 0;
    };
    f.put(f.actor,0x9c,35);f.put(f.actor,0x878,10);f.put(f.actor,0x880,10);
    f.put(f.actor,0x3ec,0);f.put(f.actor,0x78c,0xffffffffu);
    f.put(f.actor,0x908,0x5678u);f.actor[0x3e1]=1;
    f.used=0;f.calls.clear();
    assert(composed->ignite_major(f.actor,sizeof(f.actor),&req,&combined,&session));
    assert(session.pending_calls==1&&session.pending.pending.reselected&&
        !session.pending.pending.skipped&&!session.pending.transition_calls&&
        f.get<int>(f.actor,0x878)==10&&f.get<int>(f.actor,0x880)==113&&
        f.get<int>(f.actor,0x884)==0&&f.get<int>(f.actor,0x888)==113);
    ServerRecombinantPendingActivityResult step{};
    assert(pending_api->update(f.actor,sizeof(f.actor),&pending_bindings,&step));
    assert(step.pending.assignment.assigned&&step.transition_calls==1&&
        step.activity.completed&&step.activity.kind==RecombinantMajor&&
        step.activity.previous==10&&step.activity.activity==113&&
        f.get<int>(f.actor,0x878)==113&&f.get<int>(f.actor,0x880)==113);
    assert(pending_api->update(f.actor,sizeof(f.actor),&pending_bindings,&step)&&
        step.pending.skipped&&!step.transition_calls);

    // The remaining original ignite choices also reach the pending owner.
    for(const int previous : {6,1,7}){
        f.put(f.actor,0x878,previous);f.put(f.actor,0x880,previous);
        f.put(f.actor,0x9c,35);f.used=0;f.calls.clear();
        assert(composed->ignite_major(f.actor,sizeof(f.actor),&req,&combined,&session));
        const int expected=previous==6?112:previous==1?111:-1;
        assert(session.ignition.activity==expected&&session.requested_activity==expected&&
            session.pending_calls==static_cast<unsigned>(expected!=-1)&&f.calls=="IR");
        assert(f.get<int>(f.actor,0x880)==(expected==-1?previous:expected)&&
            f.get<int>(f.actor,0x878)==previous);
    }
    // A provider failure after the request is written cannot retroactively
    // undo the inherited Ignite or erase the attempted pending activity.
    auto metadata_ok=assignment.translation.metadata;
    assignment.translation.metadata=[](void*,void*,const ServerActivityCompositeView**) noexcept ->int{
        return -1;
    };
    f.put(f.actor,0x878,10);f.put(f.actor,0x880,10);
    f.put(f.actor,0x9c,35);f.used=0;f.calls.clear();
    assert(!composed->ignite_major(f.actor,sizeof(f.actor),&req,&combined,&session));
    assert(!session.ignition.completed&&session.pending_calls==1&&
        session.requested_activity==113&&f.get<int>(f.actor,0x880)==113&&
        f.get<int>(f.actor,0x878)==10&&f.calls=="IR");
    assignment.translation.metadata=metadata_ok;

    // Invalid registration and actor identity must fail before Ignite
    // mutates health, fire state, animation or calls any host service.
    f.calls.clear();f.actor[0xbf]=0;f.put(f.actor,0x880,10);
    ++major_dictionary.names.generation;
    assert(!composed->ignite_major(f.actor,sizeof(f.actor),&req,&combined,&session)&&
        !session.pending_calls&&f.get<int>(f.actor,0x880)==10&&f.calls.empty());
    --major_dictionary.names.generation;
    f.put(f.actor,0,recombinant_minor_vtable);
    assert(!composed->ignite_major(f.actor,sizeof(f.actor),&req,&combined,&session)&&
        !session.pending_calls&&f.calls.empty());
    f.put(f.actor,0,recombinant_major_vtable);
    pending.assignment.translation.actor_class=NpcActivityGrunt;
    assert(!composed->ignite_major(f.actor,sizeof(f.actor),&req,&combined,&session)&&
        !session.pending_calls&&f.calls.empty());
    pending.assignment.translation.actor_class=NpcActivityHosted;
    assert(!composed->ignite_major(f.actor,0xc15,&req,&combined,&session)&&
        !session.pending_calls&&f.calls.empty());
    assert(!composed->ignite_major(f.actor,sizeof(f.actor),&req,&combined,nullptr)&&f.calls.empty());
    f.put(f.actor,0,recombinant_minor_vtable);
    assert(!api->damage_major(f.actor,sizeof(f.actor),f.info,sizeof(f.info),20.f,&b,&o)&&!o.completed);
    FreeLibrary(server);FreeLibrary(client);return 0;
}
