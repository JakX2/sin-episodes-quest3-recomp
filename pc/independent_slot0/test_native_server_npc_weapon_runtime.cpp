#include "native_server_npc_weapon_runtime.h"
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
#include <initializer_list>
using namespace sin_port::native_weapon_tables;
struct F {
    alignas(4) unsigned char actor[0xfcc]{},weapon[0x4c0]{},component[0x28]{},path[0x48]{};
    alignas(4) unsigned char npc_desc[2][0x40]{},weapon_desc[2][0x40]{};
    ServerActivityDescriptor nr[2]{},wr[2]{};
    std::uint32_t ng=3,wg=3;
    ServerActivityGenerationNode nroot{&ng,-1},wroot{&wg,-1};
    ServerActivityCompositeView nv{nr,2,&nroot,1},wv{wr,2,&wroot,1};
    ServerStudioEvent ne{.04f,20,0,"npc",0},we{.04f,21,0,"weapon",0};
    sin_independent::EntityHandleEntry slots[3]{};
    std::vector<int> trace;
    template<class T> void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T> T get(void* p,unsigned off){T v{};std::memcpy(&v,static_cast<unsigned char*>(p)+off,sizeof(v));return v;}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto get=reinterpret_cast<GetServerNpcWeaponRuntimeApi>(GetProcAddress(server,server_npc_weapon_runtime_export));
    assert(get&&!GetProcAddress(client,server_npc_weapon_runtime_export)&&!get(0,sizeof(ServerNpcWeaponRuntimeApi))&&!get(1,0));
    auto* api=get(1,sizeof(ServerNpcWeaponRuntimeApi));assert(api);
    F f;f.slots[1].serial=7;f.slots[1].entity=reinterpret_cast<CBaseEntity*>(f.weapon);
    f.slots[2].serial=8;f.slots[2].entity=reinterpret_cast<CBaseEntity*>(f.actor);
    for(unsigned i=0;i<2;++i){
        f.nr[i]={f.npc_desc[i],0x40,0};f.wr[i]={f.weapon_desc[i],0x40,0};
        f.npc_desc[i][0xd]=f.weapon_desc[i][0xd]=0x10;
        f.put(f.npc_desc[i],0x10,i?309:1);f.put(f.weapon_desc[i],0x10,i?309:157);
        f.put(f.npc_desc[i],0x14,1);f.put(f.weapon_desc[i],0x14,1);
    }
    f.put(f.actor,0x78c,0x7001u);f.put(f.actor,0x878,1);f.put(f.actor,0x880,1);
    f.put(f.actor,0x908,0x5678u);f.put(f.actor,0x8f8,0x1000u);f.put(f.component,0x24,0x2000u);
    f.put(f.path,0x44,1u);f.put(f.weapon,0x494,0x8002u);f.put(f.weapon,0x4a0,0xffffffffu);
    f.actor[0xc64]=1;
    for(void* p:{static_cast<void*>(f.actor),static_cast<void*>(f.weapon)}){
        f.put(p,0x1f0,std::uint8_t{1});f.put(p,0x28,9.875f);f.put(p,0x2c,10.f);
        f.put(p,0x3ec,0);f.put(p,0x3e1,std::uint8_t{1});f.put(p,0x3e8,.5f);f.put(p,0x3c0,1.f);
    }
    ServerEventDictionaryEntry ar[16]{},er[32]{};
    ServerActivityDictionary activities{{ar,16,0,3,0},-1};ServerEventDictionary events{er,32,0,0,0};
    ServerNpcWeaponRuntimeBindings b{};b.npc.context=&f;
    b.inherited.weapons={f.slots,3,&f,[](void* p,void* w,unsigned* out) noexcept -> int {assert(w==F::self(p).weapon);*out=WeaponActivityAssault;return 1;},nullptr};
    auto& as=b.npc.transition.assignment;as.context=&f;
    auto& tr=as.translation;tr.context=&f;tr.actor_class=NpcActivityGrunt;tr.activities=&activities;tr.events=&events;
    tr.metadata=[](void* p,void* actor,const ServerActivityCompositeView** out) noexcept -> int {auto& f=F::self(p);*out=actor==f.actor?&f.nv:&f.wv;return 1;};
    tr.grunt.component_identity=[](void*,void*,std::uint32_t,bool* out) noexcept -> int {*out=false;return 1;};
    tr.random.random=[](void*,int lo,int hi,int,int* out) noexcept -> int {assert(lo==0&&hi==0);*out=0;return 1;};
    as.reset.context=&f;
    as.reset.duration=[](void* p,void* actor,int,double* out) noexcept -> int {auto& f=F::self(p);*out=actor==f.actor?2.:4.;return 1;};
    as.reset.distance=[](void*,void*,int,double* out) noexcept -> int {*out=0;return 1;};
    as.reset.flags=[](void*,void*,int,unsigned* out) noexcept -> int {*out=1;return 1;};
    as.dictionary.dictionary=&events;
    as.dictionary.descriptor=[](void*,void*,int,void**,unsigned*,unsigned*) noexcept -> int {return 0;};
    as.component_notify=[](void* p,void* a,std::uint32_t c) noexcept -> int {auto& f=F::self(p);assert(a==f.actor&&c==0x5678);f.trace.push_back(60);return 1;};
    b.npc.frame.activities=&activities;b.npc.frame.frame.dictionary=as.dictionary;
    auto& nf=b.npc.frame.frame.sequence.frame;nf.context=&f;
    nf.model_available=[](void* p,void*) noexcept -> int {F::self(p).trace.push_back(10);return 1;};
    auto& adv=nf.advance;adv.base.context=&f;
    adv.base.duration=as.reset.duration;adv.base.movement_distance=as.reset.distance;
    adv.base.finish_threshold=[](void*,void*,int,double* out) noexcept -> int {*out=.9;return 1;};
    adv.base.invalidate=[](void* p,void* actor,unsigned flags) noexcept -> int {auto& f=F::self(p);assert(flags==8);f.trace.push_back(actor==f.actor?11:12);return 1;};
    adv.base.cache=[](void*,void*,std::uint32_t,float) noexcept -> int {return 1;};
    adv.layers.vectors.storage=[](void*,std::uint32_t,void**,unsigned*) noexcept -> int {return 0;};
    adv.layers.vectors.resize=[](void*,std::uint32_t,unsigned,void**,unsigned*) noexcept -> int {return 0;};
    adv.base.vectors=adv.layers.vectors;
    adv.layers.cycle_rate=[](void*,void*,int,double* out) noexcept -> int {*out=1.;return 1;};
    nf.events.context=&f;nf.events.storage=adv.layers.vectors.storage;
    nf.events.sequence=[](void* p,void* source,int seq,ServerAnimationEventSequence* out) noexcept -> int {
        auto& f=F::self(p);assert(seq==1);*out={source==f.actor?&f.ne:&f.we,1,1,0};return 1;};
    nf.events.cycle_rate=[](void* p,void* source,int,double* out) noexcept -> int {*out=source==F::self(p).actor?.5:.25;return 1;};
    nf.events.emit=[](void* p,void* receiver,int layer,const ServerAnimationEvent* e) noexcept {
        auto& f=F::self(p);assert(receiver==f.actor&&layer==-1&&((e->event==20&&e->source==f.actor)||(e->event==21&&e->source==f.weapon)));f.trace.push_back(e->event);};
    b.npc.storage=[](void* p,std::uint32_t word,const void** out,unsigned* extent) noexcept -> int {
        auto& f=F::self(p);if(word==0x1000){*out=f.component;*extent=sizeof(f.component);}
        else if(word==0x2000){*out=f.path;*extent=sizeof(f.path);}else return 0;return 1;};
    auto& w=b.weapon;w.context=&f;w.animation.context=&f;w.animation.activities=&activities;w.animation.events=&events;
    w.animation.metadata=tr.metadata;w.animation.random=tr.random;w.animation.reset=as.reset;w.animation.dictionary=as.dictionary;
    w.extent=[](void* p,void* weapon,unsigned* out) noexcept -> int {assert(weapon==F::self(p).weapon);*out=sizeof(F::self(p).weapon);return 1;};
    w.combat_owner=[](void* p,void* owner,void** out) noexcept -> int {assert(owner==F::self(p).actor);*out=owner;return 1;};
    w.is_player=[](void*,void*,bool* out) noexcept -> int {*out=false;return 1;};
    w.frame.cycle=adv.base;w.events=nf.events;
    // All old translation, parent, capabilities, assignment notification,
    // tick weapon resolver and post-animation callbacks remain null.
    assert(!tr.weapon&&!tr.grunt.parent_translate&&!as.capabilities&&!as.weapon_notify&&!b.npc.weapon&&!b.npc.weapon_post);
    ServerNpcWeaponRuntimeResult out{};
    assert(api->request(f.actor,sizeof(f.actor),16,&b,&out)&&out.request.assigned&&out.request.selected.activity==309&&out.weapon_notifications==1&&out.weapon_assignment.live&&out.weapon_assignment.animation.assigned);
    assert(f.get<int>(f.actor,0x3ec)==1&&f.get<int>(f.weapon,0x3ec)==1&&f.get<int>(f.weapon,0x4bc)==309);
    assert(f.get<float>(f.actor,0x3e8)==0.f&&f.get<float>(f.weapon,0x3e8)==0.f&&f.get<float>(f.weapon,0x3c0)==2.f);
    assert((f.trace==std::vector<int>{60}));
    f.trace.clear();ServerNpcRunAnimationInput in{10.125f,false,false};
    assert(api->run(f.actor,sizeof(f.actor),&in,&b,&out)&&out.tick.animation.frame.frame.advanced&&out.tick.weapon_posted&&out.weapon_frames==1&&out.weapon_frame.advanced);
    assert(f.get<float>(f.actor,0x3e8)==.0625f&&f.get<float>(f.weapon,0x3e8)==.0625f&&out.weapon_frame.events.emitted==1);
    assert((f.trace==std::vector<int>{10,11,20,12,21}));
    assert(f.get<unsigned>(f.weapon,0x494)==0x8002u&&f.get<unsigned>(f.weapon,0x4a0)==0xffffffffu);
    // Stale active handle does not advance or deliver from reused slot.
    f.trace.clear();f.put(f.actor,0x78c,0x9001u);in.now=10.25f;
    assert(api->run(f.actor,sizeof(f.actor),&in,&b,&out)&&!out.tick.weapon_posted&&!out.weapon_frames);
    assert(f.get<float>(f.weapon,0x3e8)==.0625f&&(f.trace==std::vector<int>{10,11}));

    // Both source event paths now use original Grunt +2c0 rather than
    // the test's former hosted receiver.
    auto gg=reinterpret_cast<GetServerNpcWeaponGruntFrameApi>(GetProcAddress(server,server_npc_weapon_grunt_frame_export));
    auto gr=reinterpret_cast<GetServerGruntRegistrationApi>(GetProcAddress(server,server_grunt_registration_export));
    auto gd=reinterpret_cast<GetServerEventDictionaryApi>(GetProcAddress(server,server_event_dictionary_export));
    assert(gg&&gr&&gd&&!GetProcAddress(client,server_npc_weapon_grunt_frame_export)&&!gg(0,sizeof(ServerNpcWeaponGruntFrameApi))&&!gg(1,0));
    auto* grunt=gg(1,sizeof(ServerNpcWeaponGruntFrameApi));auto* reg=gr(1,sizeof(ServerGruntRegistrationApi));auto* dict=gd(1,sizeof(ServerEventDictionaryApi));assert(grunt&&reg&&dict);
    assert(dict->seed(&events));
    ServerGruntRegistration ids{};assert(reg->grunt(&activities,&events,nullptr,&ids));
    ServerNpcWeaponGruntFrameBindings gb{};gb.runtime=b;gb.ids=&ids;gb.handler.context=&f;
    gb.runtime.npc.frame.frame.sequence.frame.events.emit=nullptr;gb.runtime.weapon.events.emit=nullptr;
    gb.handler.debug=[](void* p,const char* message) noexcept {assert(!std::strcmp(message,"BLOCKING!\n"));F::self(p).trace.push_back(20);};
    gb.handler.weapon=[](void* p,void* actor,std::uint32_t handle,void** weapon) noexcept -> int {
        auto& f=F::self(p);assert(actor==f.actor&&handle==0x7001u);*weapon=sin_independent::EntityHandleView(f.slots,3).Resolve(handle);return 1;};
    gb.handler.weapon_event=[](void* p,void* weapon,int event,void* actor) noexcept {
        auto& f=F::self(p);assert(actor==f.actor&&weapon==f.weapon&&event==0xc1f);f.trace.push_back(21);};
    f.put(f.actor,0x78c,0x7001u);f.put(f.actor,0xc38,3u);f.put(f.actor,0xc90,0.f);
    f.ne.event=20;f.we.event=ids.events[1];f.we.type=0x401;
    auto seed_frame=[&](){
        f.trace.clear();
        for(void* p:{static_cast<void*>(f.actor),static_cast<void*>(f.weapon)}){
            f.put(p,0x28,9.875f);f.put(p,0x2c,10.f);f.put(p,0x3e8,0.f);f.put(p,0x398,0.f);f.put(p,0x3e0,std::uint8_t{0});
        }
        in.now=10.125f;
    };
    seed_frame();ServerNpcWeaponGruntFrameResult gout{};
    assert(grunt->run(f.actor,sizeof(f.actor),&in,&gb,&gout)&&gout.handled==2&&gout.npc_events==1&&gout.weapon_events==1&&!gout.failed_status);
    assert(gout.last.route==GruntAnimAlt&&f.actor[0xc35]&&f.get<unsigned>(f.actor,0xc38)==2&&f.get<float>(f.actor,0xc90)==20.125f);
    assert((f.trace==std::vector<int>{10,11,20,12,21}));
    // Unrecovered event3 is not falsely reported as a parent success.
    seed_frame();f.ne.event=3;
    assert(!grunt->run(f.actor,sizeof(f.actor),&in,&gb,&gout)&&gout.failed_status==1&&gout.failed_event==3&&gout.failed_source==f.actor&&!gout.handled&&!gout.weapon_events);
    assert((f.trace==std::vector<int>{10,11}));
    // Stale generation fails before advancement.
    seed_frame();assert(dict->clear(&events));
    assert(!grunt->run(f.actor,sizeof(f.actor),&in,&gb,&gout)&&f.trace.empty());
    FreeLibrary(server);FreeLibrary(client);std::puts("PASS native NPC request through weapon reset and active synchronized animation tick");
}
