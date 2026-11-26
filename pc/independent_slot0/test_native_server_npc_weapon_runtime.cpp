#include "native_server_npc_weapon_runtime.h"
#include "native_server_npc_active_weapon_step.h"
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
    alignas(4) unsigned char actor[0x1088]{},weapon[0x598]{},component[0x28]{},path[0x48]{};
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

    auto gnative=reinterpret_cast<GetServerNpcNativeWeaponEventFrameApi>(GetProcAddress(server,server_npc_native_weapon_event_frame_export));
    assert(gnative&&!GetProcAddress(client,server_npc_native_weapon_event_frame_export)&&!gnative(0,sizeof(ServerNpcNativeWeaponEventFrameApi))&&!gnative(1,0));
    auto* native=gnative(1,sizeof(ServerNpcNativeWeaponEventFrameApi));assert(native);
    ServerNpcNativeWeaponEventFrameBindings nb{};nb.frame=gb;
    nb.frame.handler.weapon=nullptr;nb.frame.handler.weapon_event=nullptr;nb.frame.handler.parent=nullptr;
    struct Info {F* f;alignas(4) unsigned char bytes[0x164]{};} info{&f};
    f.put(info.bytes,0x160,28);f.put(f.weapon,0x51c,std::uint16_t{5});
    auto& eb=nb.events.weapon;eb.context=&f;eb.extent=w.extent;eb.sound.context=&f;
    eb.sound.emit=[](void* p,void* weapon,int id,float) noexcept -> int {
        auto& f=F::self(p);assert(weapon==f.weapon);f.trace.push_back(100+id);return 1;};
    eb.sound.interval=[](void*,void*,double* out) noexcept -> int {*out=.0625;return 1;};
    eb.secondary=[](void* p,void* weapon,unsigned kind) noexcept -> int {
        auto& f=F::self(p);assert(weapon==f.weapon&&kind==WeaponActivityAssault);f.trace.push_back(21);return 1;};
    eb.prepare=[](void* p,void*,void*,unsigned,const ServerAnimationEvent*) noexcept -> int {F::self(p).trace.push_back(1);return 1;};
    eb.primary_effect=[](void* p,void*,void*,unsigned,const ServerAnimationEvent*) noexcept -> int {F::self(p).trace.push_back(2);return 1;};
    eb.flash=[](void* p,void*) noexcept -> int {F::self(p).trace.push_back(3);return 1;};
    nb.events.context=&info;
    nb.events.script_info=[](void* p,std::uint16_t id,const void** out,unsigned* n) noexcept -> int {
        auto& q=*static_cast<Info*>(p);assert(id==5);q.f->trace.push_back(80);*out=q.bytes;*n=sizeof(q.bytes);return 1;};
    nb.events.clear_condition=[](void* p,void*,int cond) noexcept -> int {
        static_cast<Info*>(p)->f->trace.push_back(200+cond);return 1;};
    f.ne.event=3015;f.we.event=ids.events[1];f.we.type=0x401;
    f.put(f.weapon,0x4ec,3);f.put(f.actor,0xc38,3u);
    seed_frame();ServerNpcNativeWeaponEventFrameResult nout{};
    assert(native->run(f.actor,sizeof(f.actor),&in,&nb,&nout)&&nout.frame.handled==2&&nout.native_parent==1&&nout.native_altfire==1);
    assert(nout.last.altfire.secondary_called&&f.get<int>(f.weapon,0x4ec)==28&&f.get<unsigned>(f.actor,0xc38)==2);
    assert((f.trace==std::vector<int>{10,11,109,80,203,204,205,12,21}));
    f.ne.event=20;f.we.event=3002;f.we.type=0;
    seed_frame();f.put(f.weapon,0x580,0.f);
    assert(native->run(f.actor,sizeof(f.actor),&in,&nb,&nout)&&nout.native_parent==1&&nout.last.parent.weapon.primary_called&&nout.last.parent.weapon.sound.emitted==2);
    assert(f.get<int>(f.weapon,0x4ec)==27&&(f.trace==std::vector<int>{10,11,20,12,1,102,102,2,3}));
    f.ne.event=ids.events[1];f.ne.type=0x401;eb.secondary=nullptr;
    seed_frame();const auto count_before=f.get<unsigned>(f.actor,0xc38);const auto deadline_before=f.get<float>(f.actor,0xc90);
    assert(!native->run(f.actor,sizeof(f.actor),&in,&nb,&nout)&&nout.frame.failed_status==-1&&nout.native_altfire==1&&!nout.frame.weapon_events);
    assert(f.get<unsigned>(f.actor,0xc38)==count_before&&f.get<float>(f.actor,0xc90)==deadline_before);

    auto gscript=reinterpret_cast<GetServerNpcScriptWeaponEventFrameApi>(GetProcAddress(server,server_npc_script_weapon_event_frame_export));
    assert(gscript&&!GetProcAddress(client,server_npc_script_weapon_event_frame_export)&&!gscript(0,sizeof(ServerNpcScriptWeaponEventFrameApi))&&!gscript(1,0));
    auto* script=gscript(1,sizeof(ServerNpcScriptWeaponEventFrameApi));assert(script);
    struct ScriptFixture {
        F* f;
        alignas(4) unsigned char rows[16]{},record[0x168]{},descriptor[0x48]{};
    } sf{&f};
    f.put(sf.rows,12,0x1234u);f.put(sf.record,0x160,32);f.put(sf.record,0x164,7);
    f.put(sf.descriptor,0x34,npc_global_condition_base+50);f.put(sf.descriptor,0x38,3);f.put(sf.descriptor,0x3c,5);
    ServerNpcScriptWeaponEventFrameBindings sb{};sb.frame=nb;
    sb.frame.events.script_info=nullptr;sb.frame.events.clear_condition=nullptr;
    sb.frame.events.weapon.sound.interval=nullptr;sb.frame.events.weapon.sound.max_clip=nullptr;
    sb.scripts={sf.rows,sizeof(sf.rows),1,sf.record,sizeof(sf.record),&sf,
        [](void* p,std::uint32_t token,const void** data,unsigned* n) noexcept -> int {
            auto& s=*static_cast<ScriptFixture*>(p);assert(token==0x1234u);s.f->trace.push_back(80);*data=s.record;*n=sizeof(s.record);return 1;}};
    sb.conditions.context=&sf;sb.conditions.descriptor=[](void* p,void* actor,const void** data,unsigned* n) noexcept -> int {
        auto& s=*static_cast<ScriptFixture*>(p);assert(actor==s.f->actor);s.f->trace.push_back(70);*data=s.descriptor;*n=sizeof(s.descriptor);return 1;};
    f.ne.event=2;f.ne.type=0;f.we.event=3002;f.we.type=0;
    f.put(f.weapon,0x51c,std::uint16_t{0});f.put(f.weapon,0x4ec,0);f.put(f.weapon,0x4f0,-1);
    f.put(f.actor,0x7d0,0xffffffffu);f.put(f.actor,0xfc8,-1);f.put(f.weapon,0x580,0.f);
    seed_frame();ServerNpcScriptWeaponEventFrameResult sout{};
    assert(script->run(f.actor,sizeof(f.actor),&in,&sb,&sout)&&sout.frame.frame.handled==2&&sout.fixed_resets==1&&sout.frame.native_parent==1);
    assert(sout.script_lookups==2&&sout.condition_calls==3&&sout.sound_intervals==2&&f.get<int>(f.actor,0xfc8)==2);
    assert(f.get<int>(f.weapon,0x4ec)==31&&f.get<int>(f.weapon,0x4f0)==7&&f.get<unsigned>(f.actor,0x7d0)==(0xffffffffu&~(7u<<18)));
    assert((f.trace==std::vector<int>{10,11,109,80,80,70,70,70,12,1,102,102,2,3}));
    // Event2 clears conditions and publishes marker even with stale weapon.
    f.put(f.actor,0x78c,0x9001u);f.put(f.actor,0x7d0,0xffffffffu);f.put(f.actor,0xfc8,-1);
    seed_frame();assert(script->run(f.actor,sizeof(f.actor),&in,&sb,&sout)&&sout.fixed_resets==1&&sout.frame.frame.handled==1&&!sout.last.reload.live&&!sout.script_lookups);
    assert(f.get<int>(f.actor,0xfc8)==2&&f.get<unsigned>(f.actor,0x7d0)==(0xffffffffu&~(7u<<18)));
    assert((f.trace==std::vector<int>{10,11,70,70,70}));
    f.put(f.actor,0x78c,0x7001u);
    // Original 220433fb future-deadline path still advances both animations.
    auto get_step=reinterpret_cast<GetServerNpcActiveWeaponStepApi>(GetProcAddress(server,server_npc_active_weapon_step_export));
    assert(get_step&&!GetProcAddress(client,server_npc_active_weapon_step_export)&&!get_step(0,sizeof(ServerNpcActiveWeaponStepApi))&&!get_step(1,0));
    const auto* step=get_step(1,sizeof(ServerNpcActiveWeaponStepApi));assert(step);
    ServerNpcScheduleServicesBindings schedule{};
    schedule.scheduler.original_vtable=0x2249aaa4u;schedule.scheduler.grunt.registration=&ids;
    schedule.scheduler.maintenance.selection.state.entities=b.inherited.weapons;
    schedule.scheduler.maintenance.selection.grunt.schedule.conditions=sb.conditions;
    ServerNpcWeaponAmmoBindings ammo{};ammo.scripts=&sb.scripts;
    ServerNpcWeaponMeleeBindings melee{};ServerNpcWeaponPolicyBindings policy{&ammo,&melee};
    ServerNpcActiveWeaponStepBindings step_bind{};step_bind.schedule=&schedule;step_bind.policy=&policy;step_bind.frame=&sb;step_bind.context=&f;
    step_bind.reset_movement=[](void* p,void* a) noexcept -> int{auto& f=F::self(p);assert(a==f.actor);f.trace.push_back(90);return 1;};
    step_bind.motor_activity=[](void* p,std::uint32_t token,int id) noexcept -> int{assert(token==0x5678u&&id==-1);F::self(p).trace.push_back(91);return 1;};
    auto authoritative=sb;
    // Deliberately omit duplicate animation-side registry/scripts/map.
    authoritative.frame.frame.runtime.inherited.weapons={};authoritative.scripts={};authoritative.conditions={};
    step_bind.frame=&authoritative;
    static ServerNpcActiveWeaponStepResult step_out;
    f.put(f.actor,0x864,20.f);f.actor[0x7b8]=f.actor[0x7b9]=1;
    f.put(f.weapon,0x4ec,0);f.put(f.weapon,0x4f0,-1);f.put(f.actor,0x7d0,0xffffffffu);f.put(f.weapon,0x580,0.f);
    seed_frame();
    assert(step->run(f.actor,sizeof(f.actor),&in,&step_bind,&step_out)&&step_out.completed&&!step_out.think_called&&step_out.reset_called&&step_out.motor_called&&step_out.animation_called);
    assert(step_out.animation.fixed_resets==1&&step_out.animation.frame.frame.handled==2&&f.get<int>(f.weapon,0x4ec)==31);
    assert((f.trace==std::vector<int>{90,91,10,11,109,80,80,70,70,70,12,1,102,102,2,3}));
    seed_frame();f.put(f.actor,0x864,in.now); // equality requires scheduler; missing chain rejects.
    assert(!step->run(f.actor,sizeof(f.actor),&in,&step_bind,&step_out)&&!step_out.animation_called&&f.trace.empty());
    f.put(f.actor,0x864,20.f);step_bind.motor_activity=nullptr;
    assert(!step->run(f.actor,sizeof(f.actor),&in,&step_bind,&step_out)&&step_out.reset_called&&step_out.motor_called&&!step_out.animation_called&&(f.trace==std::vector<int>{90}));
    f.actor[0x7b8]=f.actor[0x7b9]=0;
    // Jetpack uses the same active-update deadline branch and frame handler.
    auto get_jet_step=reinterpret_cast<GetServerNpcActiveJetWeaponStepApi>(GetProcAddress(server,server_npc_active_jet_weapon_step_export));
    assert(get_jet_step&&!GetProcAddress(client,server_npc_active_jet_weapon_step_export)&&!get_jet_step(0,sizeof(ServerNpcActiveJetWeaponStepApi))&&!get_jet_step(1,0));
    const auto* jet_step=get_jet_step(1,sizeof(ServerNpcActiveJetWeaponStepApi));assert(jet_step);
    ServerNpcActiveJetWeaponStepBindings jet_bind{};jet_bind.base=step_bind;
    static ServerNpcActiveJetWeaponStepResult jet_out;
    schedule.scheduler.original_vtable=0x2249964cu;
    authoritative.frame.frame.runtime.npc.transition.assignment.translation.actor_class=NpcActivityJetpack;
    f.put(f.actor,0x864,20.f);f.put(f.weapon,0x4ec,0);f.put(f.weapon,0x4f0,-1);f.put(f.weapon,0x580,0.f);
    seed_frame();
    assert(!jet_step->run(f.actor,0x1084,&in,&jet_bind,&jet_out)&&!jet_out.animation_called&&f.trace.empty());
    assert(!step->run(f.actor,sizeof(f.actor),&in,&step_bind,&step_out)&&!step_out.animation_called&&f.trace.empty());
    assert(jet_step->run(f.actor,sizeof(f.actor),&in,&jet_bind,&jet_out)&&jet_out.completed&&!jet_out.think_called&&jet_out.animation.fixed_resets==1&&f.get<int>(f.weapon,0x4ec)==31);
    assert((f.trace==std::vector<int>{10,11,109,80,80,70,70,70,12,1,102,102,2,3}));
    // Original post-animation orchestration: use independent bounded nav/path tokens.
    auto gp=reinterpret_cast<GetServerNpcActivePostAnimationApi>(GetProcAddress(server,server_npc_active_post_animation_export));
    assert(gp&&!GetProcAddress(client,server_npc_active_post_animation_export)&&!gp(0,sizeof(ServerNpcActivePostAnimationApi)));
    const auto* post=gp(1,sizeof(ServerNpcActivePostAnimationApi));assert(post);
    struct PostFixture {F* f;alignas(4) unsigned char nav[0x28]{},path[0x48]{},group[0x4c]{};unsigned dirty{};} pf{&f};
    f.put(pf.nav,0x24,0x2222u);f.put(pf.path,0x44,2u);
    ServerNpcActivePostAnimationBindings pb{};pb.context=&pf;
    pb.resolve=[](void* p,std::uint32_t token,void** data,unsigned* n) noexcept -> int {
        auto& q=*static_cast<PostFixture*>(p);
        if(token==0x1111u){*data=q.nav;*n=sizeof(q.nav);return 1;}
        if(token==0x2222u){*data=q.path;*n=sizeof(q.path);return 1;}
        if(token==0x3333u){*data=&q.dirty;*n=4;return 1;}
        if(token==444u){*data=q.group;*n=sizeof(q.group);return 1;}
        if(token==0x1000u){*data=q.f->component;*n=sizeof(q.f->component);return 1;}
        if(token==0x2000u){*data=q.f->path;*n=sizeof(q.f->path);return 1;}return 0;
    };
    pb.navigation_update=[](void* p,void* a,std::uint32_t token,float dt) noexcept -> int {
        auto& q=*static_cast<PostFixture*>(p);assert(a==q.f->actor&&token==0x1111u&&q.f->actor[0x794]==1&&dt>0.09f&&dt<0.11f);
        q.f->trace.push_back(201);return 1;
    };
    pb.prepare_pose=[](void* p,void*) noexcept -> int {static_cast<PostFixture*>(p)->f->trace.push_back(202);return 1;};
    pb.capability_pose=[](void* p,void* a) noexcept -> int {auto& q=*static_cast<PostFixture*>(p);q.f->trace.push_back(203);q.f->put(a,0x8ec,0x800000u);return 1;};
    pb.timed_pose=[](void* p,void*,float dt) noexcept -> int {assert(dt==0.2f);static_cast<PostFixture*>(p)->f->trace.push_back(204);return 1;};
    pb.finish_pose=[](void* p,void*) noexcept -> int {static_cast<PostFixture*>(p)->f->trace.push_back(205);return 1;};
    pb.motor_finish=[](void* p,void*,std::uint32_t token) noexcept -> int {assert(token==0x5678u);static_cast<PostFixture*>(p)->f->trace.push_back(206);return 1;};
    f.trace.clear();f.put(f.actor,0x8f8,0x1111u);f.put(f.actor,0x908,0x5678u);f.put(f.actor,0x78c,0xffffffffu);
    f.put(f.actor,0x8c,0);f.put(f.actor,0x910,0x7f7fffffu);f.put(f.actor,0x8ec,0x20000000u);
    f.put(f.actor,0x2c,1.f);f.put(f.actor,0x30,1.f);f.put(f.actor,0x16c,0x3333u);f.actor[0x1f0]=0;f.actor[0xc11]=0;
    ServerNpcActivePostAnimationResult po{};
    assert(post->run(f.actor,sizeof(f.actor),2.f,0.05f,&schedule,&pb,&po)&&po.completed&&po.path_changed&&po.timestamp_changed&&po.capability_called&&po.timed_called);
    assert(po.capabilities_first==0x20000000u&&po.capabilities_second==0x800000u&&pf.dirty==1&&f.actor[0xc11]==1&&!f.actor[0x794]&&f.get<float>(f.actor,0x910)==2.f&&f.get<float>(f.actor,0x30)==2.f);
    assert((f.trace==std::vector<int>{201,202,203,204,205,206}));
    f.trace.clear();pf.dirty=0;f.put(f.actor,0x8c,1);f.put(f.actor,0x8ec,0u);
    assert(post->run(f.actor,sizeof(f.actor),2.f,0.05f,&schedule,&pb,&po)&&po.completed&&!po.navigation_called&&!po.path_changed&&!po.timestamp_changed&&pf.dirty==0);
    assert((f.trace==std::vector<int>{202,205,206}));
    f.trace.clear();f.put(pf.path,0x44,0u);f.actor[0x1f0]=1;f.actor[0x200]=0;
    assert(post->run(f.actor,sizeof(f.actor),3.f,0.05f,&schedule,&pb,&po)&&po.path_changed&&po.timestamp_changed&&!f.actor[0xc11]&&f.actor[0x200]==1);
    auto ga=reinterpret_cast<GetServerNpcOuterAdmissionApi>(GetProcAddress(server,server_npc_outer_admission_export));
    assert(ga&&!GetProcAddress(client,server_npc_outer_admission_export)&&!ga(1,0));
    const auto* admission=ga(1,sizeof(ServerNpcOuterAdmissionApi));assert(admission);
    float warning_deadline=10.f;
    ServerNpcOuterAdmissionBindings admission_b{};admission_b.storage=&pb;admission_b.context=&f;admission_b.enabled=true;admission_b.step_threshold=7;admission_b.warning_deadline=&warning_deadline;
    admission_b.warning=[](void* p,void*) noexcept -> int{F::self(p).trace.push_back(210);return 1;};
    admission_b.pause_activity=[](void* p,void*,int id) noexcept -> int{assert(id==1);F::self(p).trace.push_back(211);return 1;};
    admission_b.component_gate=[](void* p,std::uint32_t token,bool* value) noexcept -> int{assert(token==123u);F::self(p).trace.push_back(212);*value=true;return 1;};
    admission_b.movement_probe=[](void* p,void*,int id,bool* hit) noexcept -> int{assert(id==5);F::self(p).trace.push_back(213);*hit=true;return 1;};
    ServerNpcOuterAdmissionResult admission_result{};ServerNpcOuterScriptGateResult sg{};
    constexpr unsigned vt=0x2249964cu;
    f.trace.clear();f.put(f.actor,0x3c0,0.f);f.actor[0x200]=0;
    assert(admission->admit(f.actor,sizeof(f.actor),10.f,vt,&admission_b,&admission_result)&&admission_result.admitted&&!admission_result.step_checked&&!admission_result.playback_changed);
    admission_b.global_flags=2;f.put(f.actor,0xbf4,6);
    assert(admission->admit(f.actor,sizeof(f.actor),10.f,vt,&admission_b,&admission_result)&&admission_result.admitted&&admission_result.playback_changed&&f.get<float>(f.actor,0x3c0)==1.f&&f.actor[0x200]);
    f.put(f.actor,0xbf4,7);f.put(pf.path,0,1u);
    assert(admission->admit(f.actor,sizeof(f.actor),10.f,vt,&admission_b,&admission_result)&&!admission_result.admitted&&admission_result.path_active&&!admission_result.playback_changed);
    f.put(pf.path,0,0u);f.actor[0x1f0]=0;pf.dirty=0;
    assert(admission->admit(f.actor,sizeof(f.actor),10.f,vt,&admission_b,&admission_result)&&!admission_result.admitted&&!admission_result.path_active&&admission_result.playback_changed&&pf.dirty==1&&f.get<float>(f.actor,0x3c0)==0.f);
    admission_b.global_flags=1;f.put(f.actor,0xfc8,2);
    assert(admission->admit(f.actor,sizeof(f.actor),10.f,vt,&admission_b,&admission_result)&&!admission_result.admitted&&admission_result.warning_called&&admission_result.marker_reset&&warning_deadline==15.f&&f.get<int>(f.actor,0xfc8)==-1);
    assert(admission->admit(f.actor,sizeof(f.actor),14.99f,vt,&admission_b,&admission_result)&&!admission_result.warning_called&&admission_result.pause_called);
    assert((f.trace==std::vector<int>{210,211,211}));
    f.trace.clear();f.put(f.actor,0xc18,0u);f.put(f.actor,0x850,4);f.actor[0xc10]=0;
    assert(admission->script_gate(f.actor,sizeof(f.actor),10.f,vt,&admission_b,&sg)&&sg.value&&sg.changed&&!sg.probe_called);
    f.put(f.actor,0x850,3);f.actor[0x968]=0;f.put(f.actor,0x97c,10.f);f.put(f.actor,0xb4,0u);
    assert(admission->script_gate(f.actor,sizeof(f.actor),10.f,vt,&admission_b,&sg)&&!sg.value&&sg.changed&&sg.probe_called);
    f.put(f.actor,0xc18,123u);
    assert(admission->script_gate(f.actor,sizeof(f.actor),10.f,vt,&admission_b,&sg)&&sg.component_called&&sg.value&&sg.changed);
    assert((f.trace==std::vector<int>{213,212}));
    f.put(f.actor,0xc18,0u);
    auto gcad=reinterpret_cast<GetServerNpcOuterCadenceApi>(GetProcAddress(server,server_npc_outer_cadence_export));
    assert(gcad&&!GetProcAddress(client,server_npc_outer_cadence_export)&&!gcad(1,0));
    const auto* cadence=gcad(1,sizeof(ServerNpcOuterCadenceApi));assert(cadence);
    ServerNpcOuterCadenceBindings cad{};cad.storage=&pb;cad.context=&f;
    cad.notify_think_flags=[](void* p,void*) noexcept -> int{F::self(p).trace.push_back(214);return 1;};
    cad.dynamic_mode=[](void*,void*,bool pvs) noexcept -> int{assert(pvs);return 1;};
    ServerNpcOuterCadenceResult co{};
    f.trace.clear();f.put(f.actor,0x74,100);f.put(f.actor,0xb8,0u);f.put(f.actor,0x50,0);
    assert(cadence->set_default(f.actor,sizeof(f.actor),-1.f,.05f,vt,&cad,&co)&&co.tick==-1&&co.flags_changed&&co.tick_written&&(f.get<unsigned>(f.actor,0xb8)&0x400000u));
    f.put(f.actor,0x868,0);f.put(f.actor,0x85c,3);f.put(f.actor,0x860,1);
    assert(cadence->finish(f.actor,sizeof(f.actor),10.f,.05f,vt,true,true,&cad,&co)&&co.completed&&co.mode_direct&&co.mode==0&&co.deadline==10.1f&&co.tick==202&&co.flags_changed);
    assert((f.trace==std::vector<int>{214,214}));
    cad.optimize_a=cad.optimize_b=true;
    for(int mode=0;mode<4;++mode)for(int slow=0;slow<2;++slow){
        f.put(f.actor,0x74,-1);f.put(f.actor,0x85c,mode);f.put(f.actor,0x860,slow);
        const float expected[]{10.1f,10.2f,10.4f,10.6f};
        assert(cadence->finish(f.actor,sizeof(f.actor),10.f,.05f,vt,true,true,&cad,&co)&&co.deadline==expected[mode]&&co.tick==(mode&&slow?204:202)&&!co.mode_direct);
    }
    f.put(f.actor,0x74,123);f.put(f.actor,0x864,99.f);
    assert(cadence->finish(f.actor,sizeof(f.actor),10.f,.05f,vt,true,true,&cad,&co)&&!co.unscheduled&&co.deadline_written&&co.deadline==0.f&&co.tick==123);
    f.put(f.actor,0x74,-1);f.put(f.actor,0x868,2);f.put(f.actor,0x864,99.f);
    assert(cadence->finish(f.actor,sizeof(f.actor),10.f,.05f,vt,true,false,&cad,&co)&&co.mode_direct&&co.mode==4&&!co.deadline_written&&co.deadline==99.f);
    f.put(f.actor,0x868,0);
    // A positive named think tick prevents setting the no-think flag.
    f.put(f.actor,0x44,0x2222u);f.put(f.actor,0x50,2);f.put(pf.path,8,1);f.put(f.actor,0xb8,0u);
    assert(cadence->set_default(f.actor,sizeof(f.actor),-1.f,.05f,vt,&cad,&co)&&!co.flags_changed);
    f.put(pf.path,8,0);f.put(pf.path,24,0);
    assert(cadence->set_default(f.actor,sizeof(f.actor),-1.f,.05f,vt,&cad,&co)&&co.flags_changed);
    f.put(f.actor,0x50,0);
    // Actual composed outer call. Future AI deadline still dispatches reload
    // and fire events, then post-animation and engine tick rescheduling.
    auto go=reinterpret_cast<GetServerNpcOuterUpdateApi>(GetProcAddress(server,server_npc_outer_update_export));
    assert(go&&!GetProcAddress(client,server_npc_outer_update_export)&&!go(0,sizeof(ServerNpcOuterUpdateApi)));
    const auto* outer_api=go(1,sizeof(ServerNpcOuterUpdateApi));assert(outer_api);
    ServerNpcOuterUpdateBindings outer{};outer.post=&pb;outer.admission=admission_b;outer.cadence=cad;outer.context=&f;outer.active_scene=true;
    outer.admission.global_flags=0;outer.admission.enabled=true;outer.cadence.optimize_a=outer.cadence.optimize_b=false;
    outer.pvs=[](void* p,std::uint32_t token,bool* visible) noexcept -> int{assert(token==0x3333u);F::self(p).trace.push_back(215);*visible=true;return 1;};
    outer.wake=[](void* p,void*,bool visible) noexcept -> int{assert(visible);F::self(p).trace.push_back(216);return 1;};
    outer.pending_impact=[](void* p,void* a) noexcept -> int{F::self(p).trace.push_back(217);static_cast<unsigned char*>(a)[0xbe1]=0;return 1;};
    ServerNpcOuterUpdateResult oo{};
    f.put(f.actor,0x74,123);f.put(f.actor,0xb8,0u);f.put(f.actor,0x85c,0);f.put(f.actor,0x860,0);
    f.put(f.actor,0x850,3);f.put(f.actor,0x868,0);f.put(f.actor,0x8c,1);f.put(f.actor,0x864,20.f);
    f.put(f.actor,0x8ec,0u);f.put(f.actor,0x78c,0x7001u);f.actor[0xc10]=0;f.actor[0xbe1]=1;
    // Both animation and post-animation must resolve the same nav/path.
    f.put(f.actor,0x8f8,0x1000u);f.put(f.component,0x24,0x2000u);
    f.put(f.weapon,0x4ec,0);f.put(f.weapon,0x4f0,-1);f.put(f.weapon,0x580,0.f);
    // The preceding single-step test intentionally froze playback and used
    // external dirty storage. Reset the animation fixture to its live state.
    f.put(f.actor,0x3c0,1.f);f.actor[0x1f0]=1;
    seed_frame();
    const int outer_status=outer_api->jet(f.actor,sizeof(f.actor),&in,.05f,&jet_bind,&outer,&jet_out,&po,&oo);
    if(!outer_status||!jet_out.animation.fixed_resets){
        std::fprintf(stderr,"outer status=%d impact=%u pvs=%u script=%u admitted=%u active=%u step=%u anim=%u events=%u reload=%u tail=%u finish=%u failed_event=%d trace:",outer_status,oo.impact_called,oo.pvs_called,oo.script.completed,oo.admission.admitted,oo.active_called,jet_out.completed,jet_out.animation_called,jet_out.animation.frame.frame.handled,jet_out.animation.fixed_resets,po.completed,oo.finish.completed,jet_out.animation.frame.frame.failed_event);
        for(int v:f.trace)std::fprintf(stderr," %d",v);std::fputc('\n',stderr);
    }
    assert(outer_status&&oo.completed&&oo.impact_called&&oo.active_called&&!oo.think_called&&po.completed&&jet_out.animation.fixed_resets==1);
    assert(f.get<int>(f.weapon,0x4ec)==31&&oo.finish.next_time==static_cast<float>(double(in.now)+.1)&&oo.finish.deadline==20.f);
    assert(f.trace.front()==217&&f.trace[1]==214&&f.trace[2]==215&&f.trace.back()==214);
    // Disabled branch resets navigation timestamp; no animation or pose work.
    f.trace.clear();outer.admission.global_flags=1;warning_deadline=100.f;
    assert(outer_api->jet(f.actor,sizeof(f.actor),&in,.05f,&jet_bind,&outer,&jet_out,&po,&oo)&&oo.completed&&!oo.active_called&&oo.nav_sentinel_written&&oo.admission.pause_called);
    assert(f.get<unsigned>(f.actor,0x910)==0x7f7fffffu&&!jet_out.animation_called&&!po.completed);
    assert((f.trace==std::vector<int>{214,215,70,211,214}));
    // Sleeping actor performs wake service and skips active scene.
    f.trace.clear();f.put(f.actor,0x868,2);
    assert(outer_api->jet(f.actor,sizeof(f.actor),&in,.05f,&jet_bind,&outer,&jet_out,&po,&oo)&&oo.completed&&oo.wake_called&&oo.sleep_bypassed&&!oo.active_called&&oo.finish.mode==4);
    assert((f.trace==std::vector<int>{214,215,70,216,214}));
    f.trace.clear();f.put(f.actor,0x868,0);f.put(f.actor,0x85c,0);outer.active_scene=false;
    assert(outer_api->jet(f.actor,sizeof(f.actor),&in,.05f,&jet_bind,&outer,&jet_out,&po,&oo)&&oo.completed&&oo.scene_bypassed&&!oo.active_called);
    assert((f.trace==std::vector<int>{214,215,70,214}));
    auto gwake=reinterpret_cast<GetServerNpcWakePolicyApi>(GetProcAddress(server,server_npc_wake_policy_export));
    assert(gwake&&!GetProcAddress(client,server_npc_wake_policy_export)&&!gwake(1,0));
    const auto* wake=gwake(1,sizeof(ServerNpcWakePolicyApi));assert(wake);
    struct WakeFixture {F* f;unsigned char player[0xc0]{},sounds[3][0x2c]{};bool live=true,visible=false,audible=false,sound_visible=false;int head=-1;unsigned calls=0;} wake_f{&f};
    ServerNpcWakePolicyBindings wb{};wb.context=&wake_f;wb.conditions=sb.conditions;wb.max_clients=1;
    wb.local_player=[](void* p,void** e) noexcept -> int{auto& w=*static_cast<WakeFixture*>(p);*e=w.live?w.player:nullptr;return 1;};
    wb.client=[](void* p,int index,void** e) noexcept -> int{assert(index==1);*e=static_cast<WakeFixture*>(p)->player;return 1;};
    wb.extent=[](void* p,void* e,unsigned* bytes) noexcept -> int{assert(e==static_cast<WakeFixture*>(p)->player);*bytes=0xc0;return 1;};
    wb.origin=[](void* p,void* e,float* v) noexcept -> int{auto& w=*static_cast<WakeFixture*>(p);assert(e==w.player||e==w.f->actor);v[0]=e==w.player?100.f:0.f;v[1]=v[2]=0;return 1;};
    wb.player_visible=[](void* p,void* player,void* actor,unsigned mask,bool* yes) noexcept -> int{auto& w=*static_cast<WakeFixture*>(p);assert(player==w.player&&actor==w.f->actor&&mask==0x4081);*yes=w.visible;return 1;};
    wb.missing_player=[](void* p) noexcept -> int{static_cast<WakeFixture*>(p)->f->trace.push_back(219);return 1;};
    wb.active_sound_head=[](void* p,int* index) noexcept -> int{*index=static_cast<WakeFixture*>(p)->head;return 1;};
    wb.sound=[](void* p,int index,const void** row,unsigned* bytes) noexcept -> int{if(index<0||index>=3)return 0;*row=static_cast<WakeFixture*>(p)->sounds[index];*bytes=0x2c;return 1;};
    wb.sound_audible=[](void* p,void*,std::uint32_t token,const void*,bool* yes) noexcept -> int{assert(token==55u);*yes=static_cast<WakeFixture*>(p)->audible;return 1;};
    wb.sound_visible=[](void* p,void*,const void*,bool* yes) noexcept -> int{*yes=static_cast<WakeFixture*>(p)->sound_visible;return 1;};
    wb.wake=[](void* p,void* actor,bool output) noexcept -> int{auto& w=*static_cast<WakeFixture*>(p);assert(actor==w.f->actor&&output);++w.calls;return 1;};
    ServerNpcWakePolicyResult wo{};
    f.put(f.actor,0x868,0);assert(wake->run(f.actor,sizeof(f.actor),vt,true,&wb,&wo)&&!wo.local_queried&&!wo.wake_calls);
    f.put(f.actor,0x868,2);wb.max_clients=2;
    assert(wake->run(f.actor,sizeof(f.actor),vt,false,&wb,&wo)&&wo.reason==NpcWakeMultiplayer&&!wo.local_queried);
    wb.max_clients=1;wake_f.live=false;f.trace.clear();
    assert(wake->run(f.actor,sizeof(f.actor),vt,true,&wb,&wo)&&wo.missing_player&&!wo.wake_calls&&(f.trace==std::vector<int>{219}));
    wake_f.live=true;f.put(f.actor,0x86c,100.f);
    assert(wake->run(f.actor,sizeof(f.actor),vt,false,&wb,&wo)&&wo.reason==NpcWakeProximity&&wo.distance_squared==10000.f);
    f.put(f.actor,0x86c,99.f);
    assert(wake->run(f.actor,sizeof(f.actor),vt,false,&wb,&wo)&&wo.proximity_checked&&!wo.wake_calls);
    f.put(wake_f.player,0xbc,0x8000u);f.put(f.actor,0x86c,100.f);
    assert(wake->run(f.actor,sizeof(f.actor),vt,true,&wb,&wo)&&wo.reason==NpcWakePvs&&!wo.proximity_checked);
    f.put(wake_f.player,0xbc,0u);f.put(f.actor,0x86c,0.f);f.put(f.actor,0x868,1);
    // Identity condition namespace for damage witnesses 16/17.
    alignas(4) unsigned char wake_desc[0x48]{};f.put(wake_desc,0x34,npc_global_condition_base);f.put(wake_desc,0x38,0);f.put(wake_desc,0x3c,127);
    wb.conditions.context=wake_desc;
    wb.conditions.descriptor=[](void* p,void*,const void** d,unsigned* bytes) noexcept -> int{*d=p;*bytes=0x48;return 1;};
    std::memset(f.actor+0x7cc,0,32);f.put(f.actor,0x7cc,1u<<17);
    assert(wake->run(f.actor,sizeof(f.actor),vt,false,&wb,&wo)&&wo.reason==NpcWakeDamage&&wo.damage_checked==2);
    f.put(f.actor,0x7cc,0u);wake_f.visible=true;
    assert(wake->run(f.actor,sizeof(f.actor),vt,true,&wb,&wo)&&wo.reason==NpcWakeVisiblePlayer&&wo.visibility_calls==1);
    wake_f.head=0;wake_f.audible=wake_f.sound_visible=true;
    f.put(f.actor,0x898,55u);f.put(f.actor,0xb4,0u);
    f.put(wake_f.sounds[0],0xc,4u);f.put(wake_f.sounds[0],0x18,std::int16_t{1});
    f.put(wake_f.sounds[1],0xc,8u);f.put(wake_f.sounds[1],0x18,std::int16_t{-1});
    assert(wake->run(f.actor,sizeof(f.actor),vt,true,&wb,&wo)&&wo.wake_calls==2&&wo.reason==NpcWakeSound&&wo.sounds_visited==2&&wo.audible_calls==1&&wo.sound_visibility_calls==1);
    wake_f.visible=false;wake_f.audible=false;
    assert(wake->run(f.actor,sizeof(f.actor),vt,false,&wb,&wo)&&!wo.wake_calls&&wo.audible_calls==1&&!wo.sound_visibility_calls);
    f.put(wake_f.sounds[1],0x18,std::int16_t{0});
    assert(!wake->run(f.actor,sizeof(f.actor),vt,false,&wb,&wo)&&!wo.completed);
    f.put(f.actor,0xb4,1u);assert(wake->run(f.actor,sizeof(f.actor),vt,false,&wb,&wo)&&!wo.sounds_visited);
    auto gtWake=reinterpret_cast<GetServerNpcWakeTransitionApi>(GetProcAddress(server,server_npc_wake_transition_export));
    assert(gtWake&&!GetProcAddress(client,server_npc_wake_transition_export)&&!gtWake(1,0));
    const auto* transition_wake=gtWake(1,sizeof(ServerNpcWakeTransitionApi));assert(transition_wake);
    struct WakeTransitionFixture {F* f;alignas(4) unsigned char member[0xfd8]{},dead[0xfd8]{},filtered[0xfd8]{};} wt{&f};
    sin_independent::EntityHandleEntry wake_slots[2]{};
    wake_slots[1].serial=7;wake_slots[1].entity=reinterpret_cast<CBaseEntity*>(wt.member);
    ServerNpcWakeTransitionBindings wtb{};wtb.storage=&pb;wtb.context=&wt;wtb.entities.entries=wake_slots;wtb.entities.count=2;
    wtb.extent=[](void* p,void* e,unsigned* n) noexcept -> int{
        auto& q=*static_cast<WakeTransitionFixture*>(p);if(e==q.f->actor)*n=sizeof(q.f->actor);else if(e==q.member||e==q.dead||e==q.filtered)*n=sizeof(q.member);else return 0;return 1;
    };
    wtb.refresh_entity=[](void* p,void* e) noexcept -> int{
        auto& q=*static_cast<WakeTransitionFixture*>(p);assert(!q.f->get<int>(e,0x868));q.f->trace.push_back(e==q.f->actor?220:221);return 1;
    };
    wtb.output=[](void* p,void* e,unsigned off) noexcept -> int{
        auto& q=*static_cast<WakeTransitionFixture*>(p);assert(off==0xbc8);q.f->trace.push_back(e==q.f->actor?222:223);return 1;
    };
    wtb.squad_next=[](void* p,std::uint32_t token,int* cursor,void** e) noexcept -> int{
        auto& q=*static_cast<WakeTransitionFixture*>(p);assert(token==444u);++*cursor;
        *e=*cursor==0?q.f->actor:*cursor==1?q.dead:*cursor==2?q.member:nullptr;return 1;
    };
    wtb.jet_prepare=[](void* p,void* e) noexcept -> int{auto& q=*static_cast<WakeTransitionFixture*>(p);assert(e==q.f->actor);q.f->trace.push_back(224);return 1;};
    wtb.jet_attachment=[](void* p,std::uint32_t token) noexcept -> int{assert(token==123u);static_cast<WakeTransitionFixture*>(p)->f->trace.push_back(225);return 1;};
    ServerNpcWakeTransitionResult wto{};
    constexpr unsigned ground_vt=0x2249aaa4u;
    f.put(f.actor,0x868,2);f.put(f.actor,0x78,0x24u);f.actor[0x870]=1;f.actor[0x1f0]=0;pf.dirty=0;
    f.put(f.actor,0x9c4,444u);
    f.put(wt.member,0,ground_vt);f.put(wt.member,0x868,1);wt.member[0x870]=1;f.put(wt.member,0x9c4,444u);
    f.put(wt.dead,0,ground_vt);f.put(wt.dead,0x8c,1);f.put(wt.dead,0x868,1);wt.dead[0x870]=1;
    f.trace.clear();
    assert(transition_wake->run(f.actor,sizeof(f.actor),ground_vt,false,&wtb,&wto)&&wto.completed&&wto.transitions==2&&wto.flags_changed==1&&wto.squad_visited==3&&wto.squad_woken==1&&wto.outputs==1);
    assert(f.get<unsigned>(f.actor,0x78)==4&&pf.dirty==1&&!wt.member[0x870]&&wt.dead[0x870]&&f.get<int>(wt.dead,0x868)==1);
    assert((f.trace==std::vector<int>{220,221,223}));
    f.trace.clear();assert(transition_wake->run(f.actor,sizeof(f.actor),ground_vt,true,&wtb,&wto)&&!wto.transitions&&f.trace.empty());
    f.put(f.actor,0x868,1);f.put(f.actor,0x1024,0x7001u);f.put(f.actor,0x218,123u);f.actor[0x870]=0;f.actor[0x1f0]=1;
    f.put(f.actor,0x78,0x20u);f.actor[0x200]=0;f.put(wt.member,0x868,1);
    assert(transition_wake->run(f.actor,sizeof(f.actor),vt,true,&wtb,&wto)&&wto.transitions==2&&wto.jet_prepared==1&&wto.jet_linked==1&&wto.jet_attachment==1&&wto.outputs==2&&f.actor[0x200]);
    assert((f.trace==std::vector<int>{224,221,223,225,220,222}));
    // Invalid linked handle fails after preparation but before base transition.
    f.trace.clear();f.put(f.actor,0x868,1);f.put(f.actor,0x1024,0x8001u);
    assert(!transition_wake->run(f.actor,sizeof(f.actor),vt,true,&wtb,&wto)&&wto.jet_prepared==1&&!wto.transitions&&f.get<int>(f.actor,0x868)==1);
    assert((f.trace==std::vector<int>{224}));
    auto gow=reinterpret_cast<GetServerNpcOuterWakeUpdateApi>(GetProcAddress(server,server_npc_outer_wake_update_export));
    assert(gow&&!GetProcAddress(client,server_npc_outer_wake_update_export)&&!gow(1,0));
    const auto* outer_wake=gow(1,sizeof(ServerNpcOuterWakeUpdateApi));assert(outer_wake);
    ServerNpcOuterWakeUpdateBindings owb{};owb.outer=outer;owb.policy=wb;owb.transition=wtb;
    owb.outer.wake=nullptr;owb.policy.wake=nullptr; // no legacy hosted wake decision or transition
    owb.transition.storage=nullptr;owb.transition.entities={};owb.policy.conditions={};
    owb.outer.admission.global_flags=0;owb.outer.active_scene=false;
    schedule.scheduler.original_vtable=ground_vt;
    f.put(f.actor,0x868,2);f.put(f.actor,0x85c,0);f.put(f.actor,0x86c,0.f);f.put(f.actor,0x78,0x20u);f.actor[0x870]=0;
    f.actor[0x1f0]=0;pf.dirty=0;f.put(f.actor,0x8c,0);
    ServerNpcOuterWakeUpdateResult owr{};
    f.trace.clear();
    assert(outer_wake->ground(f.actor,sizeof(f.actor),&in,.05f,&step_bind,&owb,&step_out,&po,&owr)&&owr.outer.completed&&owr.outer.wake_called&&!owr.outer.sleep_bypassed&&owr.outer.scene_bypassed);
    assert(owr.policy.reason==NpcWakePvs&&owr.transition.transitions==1&&owr.transition.outputs==1&&owr.transition.flags_changed==1&&pf.dirty==1);
    assert(f.get<int>(f.actor,0x868)==0&&f.get<unsigned>(f.actor,0x78)==0&&owr.outer.finish.mode==0);
    assert((f.trace==std::vector<int>{214,215,70,220,222,214}));
    // Next outer tick does not repeat the wake output.
    f.trace.clear();assert(outer_wake->ground(f.actor,sizeof(f.actor),&in,.05f,&step_bind,&owb,&step_out,&po,&owr)&&!owr.outer.wake_called&&!owr.transition.transitions&&(f.trace==std::vector<int>{214,215,70,214}));
    // Jetpack path resolves linked Grunt from authoritative schedule registry.
    schedule.scheduler.original_vtable=vt;
    const auto saved_registry=schedule.scheduler.maintenance.selection.state.entities;
    schedule.scheduler.maintenance.selection.state.entities=wtb.entities;
    f.put(f.actor,0x868,2);f.put(f.actor,0x1024,0x7001u);f.put(f.actor,0x218,123u);f.put(wt.member,0x868,2);
    f.trace.clear();
    assert(outer_wake->jet(f.actor,sizeof(f.actor),&in,.05f,&jet_bind,&owb,&jet_out,&po,&owr)&&owr.outer.completed&&owr.outer.scene_bypassed&&owr.transition.transitions==2&&owr.transition.jet_linked&&owr.transition.jet_attachment);
    assert((f.trace==std::vector<int>{214,215,70,224,221,223,225,220,222,214}));
    schedule.scheduler.maintenance.selection.state.entities=saved_registry;
    // Native +58 refresh without a hosted callback, including all field-read
    // branches and both direct network dirty paths. Preserve unrelated bits.
    wtb.refresh_entity=nullptr;f.actor[0x870]=0;
    f.put(f.actor,0x16c,0x3333u);f.actor[0x1f0]=0;
    constexpr unsigned prior_word=0x81234538u;
    unsigned refresh_cases=0;
    for(unsigned kind:{0u,1u,2u})for(unsigned link:{0u,1u})for(unsigned flags:{0u,0x80u,0x20000u,0x20080u}){
        f.put(f.actor,0x868,2);f.put(f.actor,0x78,0x20u);f.put(f.actor,0x38,kind);f.put(f.actor,0x240,link);f.put(f.actor,0xb8,flags);
        pf.dirty=prior_word;f.trace.clear();
        assert(transition_wake->run(f.actor,sizeof(f.actor),ground_vt,false,&wtb,&wto)&&wto.completed&&wto.transitions==1&&wto.refreshed==1&&wto.flags_changed==1&&!wto.outputs&&f.trace.empty());
        const unsigned expected=(!(flags&0x80u)&&(!kind||!link))?0x10u:(kind==1||(flags&0x20000u))?8u:0x20u;
        assert(pf.dirty==((prior_word&~0x38u)|expected|1u));
        ++refresh_cases;
    }
    assert(refresh_cases==24);
    f.put(f.actor,0x868,2);f.put(f.actor,0x78,0u);f.put(f.actor,0x16c,0u);
    assert(transition_wake->run(f.actor,sizeof(f.actor),ground_vt,false,&wtb,&wto)&&wto.refreshed&&!wto.flags_changed);
    f.put(f.actor,0x868,2);f.put(f.actor,0x16c,0x9999u);
    assert(!transition_wake->run(f.actor,sizeof(f.actor),ground_vt,false,&wtb,&wto)&&wto.transitions==1&&!wto.refreshed&&!wto.completed);
    f.put(f.actor,0x16c,0x3333u);
    // Same-frame wake and native weapon dispatch: unlike the earlier
    // scene-bypassed prefix, keep the active scene admitted. The authoritative
    // registry must contain both the equipped weapon and the linked Grunt.
    sin_independent::EntityHandleEntry live_slots[4]{};
    for(unsigned i=0;i<3;++i)live_slots[i]=f.slots[i];
    live_slots[3].serial=7;
    live_slots[3].entity=reinterpret_cast<CBaseEntity*>(wt.member);
    auto live_registry=saved_registry;live_registry.entries=live_slots;live_registry.count=4;
    schedule.scheduler.maintenance.selection.state.entities=live_registry;
    owb.outer.active_scene=true;owb.transition.refresh_entity=nullptr;
    f.put(f.actor,0x74,123);f.put(f.actor,0xb8,0u);f.put(f.actor,0x85c,0);f.put(f.actor,0x860,0);
    f.put(f.actor,0x850,3);f.put(f.actor,0x868,2);f.put(f.actor,0x8c,1);f.put(f.actor,0x864,20.f);
    f.put(f.actor,0x86c,0.f);f.put(f.actor,0x78,0x20u);f.actor[0x870]=0;
    f.put(f.actor,0x38,0u);f.put(f.actor,0x240,0u);f.put(f.actor,0x16c,0x3333u);
    f.put(f.actor,0x8ec,0u);f.put(f.actor,0x78c,0x7001u);f.actor[0xc10]=0;f.actor[0xbe1]=0;
    f.put(f.actor,0x1024,0x7003u);f.put(f.actor,0x218,123u);
    f.put(wt.member,0x868,2);f.put(wt.member,0x16c,0u);f.put(wt.member,0x78,0u);wt.member[0x870]=0;
    f.put(f.actor,0x8f8,0x1000u);f.put(f.component,0x24,0x2000u);
    f.put(f.actor,0x3c0,1.f);f.actor[0x1f0]=1;pf.dirty=0;
    f.put(f.weapon,0x4ec,0);f.put(f.weapon,0x4f0,-1);f.put(f.weapon,0x580,0.f);
    seed_frame();
    const int live_wake=outer_wake->jet(f.actor,sizeof(f.actor),&in,.05f,&jet_bind,&owb,&jet_out,&po,&owr);
    if(!live_wake){
        std::fprintf(stderr,"active wake: policy=%u transitions=%u admitted=%u active=%u animation=%u tail=%u finish=%u trace:",owr.policy.completed,owr.transition.transitions,owr.outer.admission.admitted,owr.outer.active_called,jet_out.animation_called,po.completed,owr.outer.finish.completed);
        for(int v:f.trace)std::fprintf(stderr," %d",v);std::fputc('\n',stderr);
    }
    assert(live_wake&&owr.outer.completed&&owr.policy.reason==NpcWakePvs&&owr.transition.completed&&owr.transition.transitions==2&&owr.transition.outputs==2&&owr.transition.refreshed==2);
    assert(owr.outer.wake_called&&!owr.outer.sleep_bypassed&&!owr.outer.scene_bypassed&&owr.outer.admission.admitted&&owr.outer.active_called&&!owr.outer.think_called);
    assert(jet_out.completed&&jet_out.animation.fixed_resets==1&&jet_out.animation.frame.frame.handled==2&&po.completed&&owr.outer.finish.completed);
    assert(f.get<int>(f.actor,0x868)==0&&f.get<int>(wt.member,0x868)==0&&f.get<int>(f.weapon,0x4ec)==31&&(pf.dirty&0x38u)==0x10u);
    const std::vector<int> live_prefix{214,215,70,224,223,225,222};
    assert(f.trace.size()>live_prefix.size()&&f.trace.back()==214);
    for(unsigned i=0;i<live_prefix.size();++i)assert(f.trace[i]==live_prefix[i]);
    schedule.scheduler.maintenance.selection.state.entities=saved_registry;
    // Original 22366a20 returns as soon as either thrust handle resolves.
    // Keep linked NPC and attachment activation separate from the guard.
    alignas(4) unsigned char thrust[4]{};
    sin_independent::EntityHandleEntry thrust_slots[3]{};
    thrust_slots[1]=wake_slots[1];thrust_slots[2].serial=9;
    thrust_slots[2].entity=reinterpret_cast<CBaseEntity*>(thrust);
    auto existing=wtb;existing.entities.entries=thrust_slots;existing.entities.count=3;
    existing.jet_prepare=nullptr;
    constexpr unsigned thrust_handle=0x9002u,stale_thrust=0x8002u,invalid_handle=0xffffffffu;
    for(unsigned left:{invalid_handle,stale_thrust,thrust_handle})for(unsigned right:{invalid_handle,stale_thrust,thrust_handle}){
        f.put(f.actor,0x868,2);f.put(f.actor,0x1024,0x7001u);f.put(f.actor,0x218,123u);
        f.put(f.actor,0xfe4,left);f.put(f.actor,0xfe8,right);
        f.put(f.actor,0x78,0u);f.actor[0x870]=0;f.put(f.actor,0x16c,0u);
        f.put(wt.member,0x868,2);f.put(wt.member,0x16c,0u);f.put(wt.member,0x78,0u);wt.member[0x870]=0;
        f.trace.clear();
        const bool reuse=left==thrust_handle||right==thrust_handle;
        const int status=transition_wake->run(f.actor,sizeof(f.actor),vt,true,&existing,&wto);
        if(reuse){
            assert(status&&wto.completed&&wto.jet_prepared==1&&wto.jet_linked==1&&wto.jet_attachment==1&&wto.transitions==2&&wto.outputs==2);
            assert((f.trace==std::vector<int>{223,225,222}));
        }else{
            assert(!status&&!wto.completed&&!wto.jet_prepared&&!wto.transitions&&f.trace.empty());
            assert(f.get<int>(f.actor,0x868)==2&&f.get<int>(wt.member,0x868)==2);
        }
        assert(f.get<unsigned>(f.actor,0xfe4)==left&&f.get<unsigned>(f.actor,0xfe8)==right);
    }
    // Native squad scan: stale and invalid handles, dead and self rows,
    // the embedded component fallback, and one live eligible recipient.
    sin_independent::EntityHandleEntry squad_slots[4]{};
    squad_slots[0].serial=1;squad_slots[0].entity=reinterpret_cast<CBaseEntity*>(f.actor);
    squad_slots[1].serial=2;squad_slots[1].entity=reinterpret_cast<CBaseEntity*>(wt.dead);
    squad_slots[2].serial=3;squad_slots[2].entity=reinterpret_cast<CBaseEntity*>(wt.filtered);
    squad_slots[3].serial=4;squad_slots[3].entity=reinterpret_cast<CBaseEntity*>(wt.member);
    auto native_squad=wtb;native_squad.entities.entries=squad_slots;native_squad.entities.count=4;native_squad.squad_next=nullptr;
    const unsigned squad_rows[]{0xffffffffu,0x9003u,0x2001u,0x1000u,0x3002u,0x4003u};
    for(unsigned i=0;i<6;++i)f.put(pf.group,8+4*i,squad_rows[i]);
    f.put(pf.group,0x48,6);
    f.put(f.actor,0,ground_vt);f.put(f.actor,0x204,1u);f.put(f.actor,0x8c,0);
    f.put(f.actor,0x868,2);f.put(f.actor,0x9c4,444u);f.actor[0x870]=1;
    f.put(wt.dead,0x204,1u);f.put(wt.dead,0x8c,1);f.put(wt.dead,0x868,2);
    f.put(wt.filtered,0,ground_vt);f.put(wt.filtered,0xfc,0x22446954u);f.put(wt.filtered,0x868,2);
    f.put(wt.member,0xfc,0x22446954u);f.put(wt.member,0x11c,9u);f.put(wt.member,0x204,0u);
    f.put(wt.member,0x868,2);f.put(wt.member,0x8c,0);wt.member[0x870]=1;
    f.trace.clear();
    assert(transition_wake->run(f.actor,sizeof(f.actor),ground_vt,false,&native_squad,&wto)&&wto.completed&&wto.squad_visited==3&&wto.squad_woken==1&&wto.transitions==2&&wto.outputs==1);
    assert(f.get<int>(wt.dead,0x868)==2&&f.get<int>(wt.filtered,0x868)==2&&f.get<int>(wt.member,0x868)==0&&!wt.member[0x870]&&(f.trace==std::vector<int>{223}));
    // A foreign eligible row must not be treated as a known rejection leaf.
    f.put(f.actor,0x868,2);f.put(wt.filtered,0,0x1234u);f.put(wt.filtered,0x11c,9u);
    f.trace.clear();assert(!transition_wake->run(f.actor,sizeof(f.actor),ground_vt,false,&native_squad,&wto)&&!wto.completed&&wto.transitions==1&&wto.squad_visited==2&&f.trace.empty());
    f.put(f.actor,0x868,2);f.put(pf.group,0x48,17);
    assert(!transition_wake->run(f.actor,sizeof(f.actor),ground_vt,false,&native_squad,&wto)&&!wto.completed&&!wto.squad_visited);
    f.actor[0x870]=0;
    // An unconnected OnWake output must not require an event queue host.
    // A connected output must still fail closed if its dispatch is missing.
    auto empty_output=wtb;empty_output.output=nullptr;
    for(unsigned action_head:{0u,0xdeadbeefu}){
        f.put(f.actor,0x868,2);f.put(f.actor,0x78,0x20u);f.put(f.actor,0x16c,0u);
        f.put(f.actor,0xbdc,action_head);f.actor[0x870]=0;
        f.trace.clear();
        const int ok=transition_wake->run(f.actor,sizeof(f.actor),ground_vt,true,&empty_output,&wto);
        assert(bool(ok)==!action_head&&bool(wto.completed)==!action_head&&wto.outputs==(action_head?0u:1u));
        assert(wto.transitions==1&&wto.refreshed==1&&wto.flags_changed==1&&f.get<int>(f.actor,0x868)==0&&f.trace.empty());
        assert(f.get<unsigned>(f.actor,0xbdc)==action_head);
    }
    f.put(f.actor,0xbdc,0u);
    // Exercise actual original-shaped output action rows through the new
    // independent DLL export and then through the wake output adapter.
    auto get_void_output=reinterpret_cast<GetServerEntityVoidOutputApi>(GetProcAddress(server,server_entity_void_output_export));
    assert(get_void_output&&!GetProcAddress(client,server_entity_void_output_export)&&!get_void_output(0,sizeof(ServerEntityVoidOutputApi))&&!get_void_output(1,0));
    const auto* void_output=get_void_output(1,sizeof(ServerEntityVoidOutputApi));assert(void_output);
    struct OutputFixture {
        F* f;const ServerEntityVoidOutputApi* api;
        alignas(4) unsigned char rows[4][0x1c]{};
        std::vector<ServerEntityVoidOutputAction> actions;
        std::vector<unsigned> order;
        ServerEntityVoidOutputBindings bindings{};
        ServerEntityVoidOutputResult result{};
        const ServerEntityNamedEventQueueApi* queue_api{};
        alignas(4) unsigned char queue_root[0x38]{},queue_rows[8][0x38]{};
        unsigned queue_allocated{};
        ServerEntityNamedEventQueueBindings queue_bindings{};
        ServerEntityNamedEventQueueResult queue_result{};
        bool service_locked{},nested{};
        float live_clock{};std::uint8_t live_step{};
        int* live_budget{};unsigned live_releases{},live_policy{};
    } outf{&f,void_output};
    outf.bindings.context=&outf;
    outf.bindings.resolve=[](void* p,unsigned token,void** out,unsigned* n) noexcept -> int{
        auto& q=*static_cast<OutputFixture*>(p);
        if(token<1||token>4)return 0;*out=q.rows[token-1];*n=sizeof(q.rows[0]);return 1;
    };
    outf.bindings.enqueue=[](void* p,void* activator,void* caller,const ServerEntityVoidOutputAction* a) noexcept -> int{
        auto& q=*static_cast<OutputFixture*>(p);assert(activator==q.f->actor&&caller==q.f->actor);
        q.actions.push_back(*a);q.order.push_back(a->action_id);return 1;
    };
    outf.bindings.debug_action=[](void* p,void* caller,const ServerEntityVoidOutputAction* a) noexcept -> int{
        auto& q=*static_cast<OutputFixture*>(p);assert(caller==q.f->actor);
        q.order.push_back(100+a->action_id);return 1;
    };
    outf.bindings.release=[](void* p,unsigned token,void* row) noexcept -> int{
        auto& q=*static_cast<OutputFixture*>(p);assert(token>=1&&token<=4&&row==q.rows[token-1]);
        q.order.push_back(200+token);std::memset(row,0xcc,0x1c);return 1;
    };
    auto seed_outputs=[&](){
        std::memset(outf.rows,0,sizeof(outf.rows));outf.actions.clear();outf.order.clear();
        const unsigned counts[]{1,2,1,0xffffffffu};
        for(unsigned i=0;i<4;++i){
            f.put(outf.rows[i],0,500u+i);f.put(outf.rows[i],4,600u+i);
            f.put(outf.rows[i],8,i==1?777u:0u);f.put(outf.rows[i],0xc,.25f*(i+1));
            f.put(outf.rows[i],0x10,counts[i]);f.put(outf.rows[i],0x14,10u+i);
            f.put(outf.rows[i],0x18,i==3?0u:i+2);
        }
        f.put(f.actor,0xbdc,1u);
    };
    seed_outputs();f.actor[0xa8]|=0x10;
    assert(void_output->fire(f.actor+0xbc8,0x18,f.actor,f.actor,sizeof(f.actor),.5f,&outf.bindings,&outf.result));
    assert(outf.result.completed&&outf.result.visited==4&&outf.result.queued==4&&outf.result.parameter_overrides==1&&outf.result.countdowns==3&&outf.result.removed==2&&outf.result.debugged==4);
    assert((outf.order==std::vector<unsigned>{10,110,201,11,111,12,112,203,13,113}));
    assert(f.get<unsigned>(f.actor,0xbdc)==2&&f.get<unsigned>(outf.rows[1],0x18)==4&&f.get<unsigned>(outf.rows[1],0x10)==1&&f.get<unsigned>(outf.rows[3],0x10)==0xffffffffu);
    assert(outf.actions[0].delay==.75f&&outf.actions[1].delay==.5f&&outf.actions[2].delay==1.25f&&outf.actions[3].delay==1.5f);
    for(unsigned i=0;i<4;++i){
        const auto& a=outf.actions[i];assert(a.target==500+i&&a.input==600+i&&a.action_id==10+i);
        assert(a.parameter==(i==1?777u:0u)&&a.variant_type==(i==1?2u:0u));
    }
    // A repeat firing consumes the remaining finite row; unlimited stays.
    outf.order.clear();outf.actions.clear();f.actor[0xa8]&=~0x10u;
    assert(void_output->fire(f.actor+0xbc8,0x18,f.actor,f.actor,sizeof(f.actor),0.f,&outf.bindings,&outf.result)&&outf.result.queued==2&&outf.result.removed==1&&!outf.result.debugged);
    assert(f.get<unsigned>(f.actor,0xbdc)==4&&(outf.order==std::vector<unsigned>{11,202,13}));
    // A malformed cycle is bounded and does not repeatedly enqueue.
    f.put(outf.rows[3],0x18,4u);outf.order.clear();
    assert(!void_output->fire(f.actor+0xbc8,0x18,f.actor,f.actor,sizeof(f.actor),0.f,&outf.bindings,&outf.result)&&outf.result.queued==1&&!outf.result.completed);
    assert((outf.order==std::vector<unsigned>{13}));
    // A connected NPC wake now uses native action traversal, not a mock
    // output marker. Only enqueue and allocator services are hosted.
    seed_outputs();auto connected=empty_output;connected.context=&outf;
    connected.output=[](void* p,void* actor,unsigned off) noexcept -> int{
        auto& q=*static_cast<OutputFixture*>(p);assert(off==0xbc8&&actor==q.f->actor&&q.f->get<int>(actor,0x868)==0);
        return q.api->fire(static_cast<unsigned char*>(actor)+off,0x18,actor,actor,sizeof(q.f->actor),0.f,&q.bindings,&q.result);
    };
    f.put(f.actor,0x868,2);f.put(f.actor,0x78,0x20u);
    assert(transition_wake->run(f.actor,sizeof(f.actor),ground_vt,true,&connected,&wto)&&wto.completed&&wto.outputs==1&&outf.result.queued==4&&outf.result.removed==2);
    assert((outf.order==std::vector<unsigned>{10,201,11,12,203,13}));
    assert(f.get<unsigned>(f.actor,0xbdc)==2&&outf.actions[0].delay==.25f&&outf.actions[1].delay==.5f);
    // Idempotent wake does not re-fire remaining actions.
    assert(transition_wake->run(f.actor,sizeof(f.actor),ground_vt,true,&connected,&wto)&&!wto.outputs&&outf.actions.size()==4);
    f.put(f.actor,0xbdc,0u);
    // Native PE32 named event queue: allocation, normalized target/input,
    // void/string variants, absolute deadlines and stable doubly linked order.
    auto get_named_queue=reinterpret_cast<GetServerEntityNamedEventQueueApi>(GetProcAddress(server,server_entity_named_event_queue_export));
    assert(get_named_queue&&!GetProcAddress(client,server_entity_named_event_queue_export)&&!get_named_queue(0,sizeof(ServerEntityNamedEventQueueApi))&&!get_named_queue(1,0));
    outf.queue_api=get_named_queue(1,sizeof(ServerEntityNamedEventQueueApi));assert(outf.queue_api);
    constexpr unsigned root_token=99u;
    outf.queue_bindings.context=&outf;
    outf.queue_bindings.resolve=[](void* p,unsigned token,void** out,unsigned* n) noexcept -> int{
        auto& q=*static_cast<OutputFixture*>(p);
        if(!token||token>q.queue_allocated)return 0;*out=q.queue_rows[token-1];*n=0x38;return 1;
    };
    outf.queue_bindings.allocate=[](void* p,unsigned* token,void** row,unsigned* n) noexcept -> int{
        auto& q=*static_cast<OutputFixture*>(p);
        if(q.queue_allocated==8)return 0;*row=q.queue_rows[q.queue_allocated];*token=++q.queue_allocated;*n=0x38;
        std::memset(*row,0xcc,*n);return 1;
    };
    outf.queue_bindings.string_nonempty=[](void*,unsigned token,bool* nonempty) noexcept -> int{
        if(token==999u)return 0;*nonempty=token!=666u;return 1;
    };
    auto reset_queue=[&](){std::memset(outf.queue_root,0,sizeof(outf.queue_root));std::memset(outf.queue_rows,0,sizeof(outf.queue_rows));outf.queue_allocated=0;};
    auto enqueue_named=[&](const ServerEntityVoidOutputAction& a,float now){
        return outf.queue_api->enqueue(outf.queue_root,sizeof(outf.queue_root),root_token,now,0x3000u,0x4001u,&a,&outf.queue_bindings,&outf.queue_result);
    };
    auto check_queue=[&](std::initializer_list<unsigned> expected){
        unsigned prev=root_token,next=f.get<unsigned>(outf.queue_root,0x30);
        for(unsigned token:expected){
            assert(next==token&&token&&token<=outf.queue_allocated);
            auto* row=outf.queue_rows[token-1];assert(f.get<unsigned>(row,0x34)==prev);
            prev=token;next=f.get<unsigned>(row,0x30);
        }
        assert(!next);
    };
    reset_queue();
    const ServerEntityVoidOutputAction events_to_queue[]{
        {500u,600u,0u,0u,10u,2.f},
        {666u,600u,777u,2u,11u,1.f},
        {500u,666u,0u,0u,12u,3.f},
        {0u,0u,0u,0u,13u,1.f}
    };
    for(const auto& a:events_to_queue)assert(enqueue_named(a,10.f)&&outf.queue_result.completed&&outf.queue_result.inserted);
    check_queue({2,4,1,3});
    for(unsigned i=0;i<4;++i){
        auto* row=outf.queue_rows[i];const auto& a=events_to_queue[i];
        assert(f.get<float>(row,0)==10.f+a.delay&&f.get<unsigned>(row,4)==(a.target==666u?0u:a.target)&&f.get<unsigned>(row,8)==(a.input==666u?0u:a.input));
        assert(f.get<unsigned>(row,0xc)==0x3000u&&f.get<unsigned>(row,0x10)==0x4001u&&f.get<unsigned>(row,0x14)==a.action_id);
        assert(f.get<unsigned>(row,0x18)==0xffffffffu&&f.get<unsigned>(row,0x1c)==a.parameter&&f.get<unsigned>(row,0x28)==0xffffffffu&&f.get<unsigned>(row,0x2c)==a.variant_type);
    }
    auto bad=events_to_queue[0];bad.target=999u;
    assert(!enqueue_named(bad,10.f)&&!outf.queue_result.allocated&&outf.queue_allocated==4);check_queue({2,4,1,3});
    // Corrupt a traversed predecessor link and reject before allocating.
    f.put(outf.queue_rows[1],0x34,1u);
    assert(!enqueue_named(events_to_queue[0],10.f)&&!outf.queue_result.allocated&&outf.queue_allocated==4);
    f.put(outf.queue_rows[1],0x34,root_token);check_queue({2,4,1,3});
    // Compose real OnWake traversal with native named queue insertion.
    // Known Grunt +8 virtual returns actor+378 (22001b00).
    reset_queue();seed_outputs();
    const auto saved_handle=f.get<unsigned>(f.actor,0x378);f.put(f.actor,0x378,0x3000u);
    outf.bindings.enqueue=[](void* p,void* activator,void* caller,const ServerEntityVoidOutputAction* a) noexcept -> int{
        auto& q=*static_cast<OutputFixture*>(p);assert(activator==q.f->actor&&caller==q.f->actor);
        return q.queue_api->enqueue(q.queue_root,sizeof(q.queue_root),99u,20.f,q.f->get<unsigned>(activator,0x378),q.f->get<unsigned>(caller,0x378),a,&q.queue_bindings,&q.queue_result);
    };
    f.put(f.actor,0x868,2);f.put(f.actor,0x78,0x20u);
    assert(transition_wake->run(f.actor,sizeof(f.actor),ground_vt,true,&connected,&wto)&&wto.completed&&wto.outputs==1&&outf.result.queued==4&&outf.result.removed==2&&outf.queue_allocated==4);
    check_queue({1,2,3,4});
    for(unsigned i=0;i<4;++i){
        assert(f.get<float>(outf.queue_rows[i],0)==20.f+.25f*(i+1)&&f.get<unsigned>(outf.queue_rows[i],0xc)==0x3000u&&f.get<unsigned>(outf.queue_rows[i],0x10)==0x3000u);
        assert(f.get<unsigned>(outf.queue_rows[i],0x14)==10u+i&&f.get<unsigned>(outf.queue_rows[i],0x1c)==(i==1?777u:0u));
    }
    assert((outf.order==std::vector<unsigned>{201,203})&&f.get<unsigned>(f.actor,0xbdc)==2);
    assert(transition_wake->run(f.actor,sizeof(f.actor),ground_vt,true,&connected,&wto)&&!wto.outputs&&outf.queue_allocated==4);
    // Service the actual four wake records, including a nested output that
    // inserts an earlier deadline while the current node is being delivered.
    auto get_service=reinterpret_cast<GetServerEntityEventServiceApi>(GetProcAddress(server,server_entity_event_service_export));
    assert(get_service&&!GetProcAddress(client,server_entity_event_service_export)&&!get_service(0,sizeof(ServerEntityEventServiceApi))&&!get_service(1,0));
    const auto* service=get_service(1,sizeof(ServerEntityEventServiceApi));assert(service);
    for(unsigned i=0;i<4;++i){
        f.put(outf.queue_rows[i],0xc,0x8002u);f.put(outf.queue_rows[i],0x10,0x8002u);
    }
    f.put(outf.queue_rows[0],0x18,0x7001u); // named and direct
    f.put(outf.queue_rows[1],0x18,0x7001u); // direct only
    f.put(outf.queue_rows[2],0x18,0x6001u); // stale direct: class fallback
    f.put(outf.queue_rows[2],0x10,0x6001u); // stale caller -> null
    ServerEntityEventServiceBindings service_bind{};service_bind.context=&outf;
    service_bind.entities.entries=f.slots;service_bind.entities.count=3;
    service_bind.resolve=outf.queue_bindings.resolve;
    service_bind.lock=[](void* p,bool enter) noexcept -> int{
        auto& q=*static_cast<OutputFixture*>(p);assert(q.service_locked!=enter);q.service_locked=enter;
        q.order.push_back(enter?900u:901u);return 1;
    };
    service_bind.find_named=[](void* p,void* prev,unsigned name,void* activator,void* caller,void** entity) noexcept -> int{
        auto& q=*static_cast<OutputFixture*>(p);assert(q.service_locked&&activator==q.f->actor);
        assert(caller==(name==502u?nullptr:q.f->actor));*entity=nullptr;
        if(name==500u)*entity=!prev?q.f->actor:prev==q.f->actor?q.f->weapon:nullptr;
        if(name==504u&&!prev)*entity=q.f->actor;
        return 1;
    };
    service_bind.find_class=[](void* p,void* prev,unsigned name,void** entity) noexcept -> int{
        auto& q=*static_cast<OutputFixture*>(p);assert(q.service_locked&&(name==502u||name==503u));
        *entity=name==502u&&!prev?q.f->weapon:nullptr;return 1;
    };
    service_bind.accept=[](void* p,void* target,unsigned input,void* activator,void* caller,const void* variant,unsigned id) noexcept -> int{
        auto& q=*static_cast<OutputFixture*>(p);assert(q.service_locked&&activator==q.f->actor&&caller==(id==12u?nullptr:q.f->actor)&&input==590u+id);
        unsigned parameter=0,type=0;std::memcpy(&parameter,variant,4);std::memcpy(&type,static_cast<const unsigned char*>(variant)+0x10,4);
        assert(parameter==(id==11u?777u:0u)&&type==(id==11u?2u:0u));
        assert(target==q.f->actor||target==q.f->weapon);
        q.order.push_back(1000u+id*10u+(target==q.f->weapon?1u:0u));
        if(id==10u&&!q.nested){
            q.nested=true;const ServerEntityVoidOutputAction action{504u,604u,0u,0u,14u,0.f};
            assert(q.queue_api->enqueue(q.queue_root,sizeof(q.queue_root),99u,20.f,0x8002u,0x8002u,&action,&q.queue_bindings,&q.queue_result));
        }
        return 1;
    };
    service_bind.release=[](void* p,unsigned token,void* row) noexcept -> int{
        auto& q=*static_cast<OutputFixture*>(p);assert(q.service_locked&&token&&token<=q.queue_allocated&&row==q.queue_rows[token-1]);
        q.order.push_back(2000u+token);std::memset(row,0xcc,0x38);return 1;
    };
    service_bind.missing=[](void* p,unsigned target,unsigned input,void* caller) noexcept -> int{
        auto& q=*static_cast<OutputFixture*>(p);assert(q.service_locked&&target==503u&&input==603u&&caller==q.f->actor);q.order.push_back(999u);return 1;
    };
    ServerEntityEventServiceResult sr{};int steps=0;outf.order.clear();
    auto run_service=[&](float now,bool step){
        return service->service(outf.queue_root,sizeof(outf.queue_root),root_token,now,step,&steps,&service_bind,&sr);
    };
    assert(run_service(30.f,true)&&sr.completed&&sr.step_stopped&&!sr.events&&outf.order.empty());
    assert(run_service(20.f,false)&&sr.completed&&sr.future_stopped&&!sr.events&&outf.order.empty());
    steps=1;assert(run_service(20.25f,true)&&sr.completed&&sr.step_stopped&&steps==0&&sr.events==1&&sr.named==2&&sr.direct==1&&sr.deliveries==3&&sr.removed==1);
    assert((outf.order==std::vector<unsigned>{900,1100,1101,1101,2001,901}));
    check_queue({5,2,3,4});assert(!outf.service_locked);
    outf.order.clear();assert(run_service(20.5f,false)&&sr.completed&&sr.future_stopped&&sr.events==2&&sr.named==1&&sr.direct==1&&sr.deliveries==2);
    assert((outf.order==std::vector<unsigned>{900,1140,2005,901,900,1111,2002,901}));check_queue({3,4});
    outf.order.clear();assert(run_service(20.75f,false)&&sr.completed&&sr.future_stopped&&sr.events==1&&sr.classname==1&&sr.deliveries==1);
    assert((outf.order==std::vector<unsigned>{900,1121,2003,901}));check_queue({4});
    outf.order.clear();assert(run_service(21.f,false)&&sr.completed&&sr.events==1&&sr.missing==1&&!sr.deliveries&&sr.removed==1);
    assert((outf.order==std::vector<unsigned>{900,999,2004,901}));check_queue({});
    assert(!outf.service_locked);
    // Live globals are reloaded after each released event, as at
    // 22155560 and 2215599f. Release can advance/retract the clock or
    // enable the original single-step gate before engine unlock.
    auto live_service=service_bind;live_service.live_now=&outf.live_clock;live_service.live_single_step=&outf.live_step;
    live_service.release=[](void* p,unsigned token,void* row) noexcept -> int{
        auto& q=*static_cast<OutputFixture*>(p);assert(q.service_locked&&token&&token<=q.queue_allocated&&row==q.queue_rows[token-1]);
        ++q.live_releases;
        if(q.live_releases==1){
            if(q.live_policy==1)q.live_clock=12.f;
            else if(q.live_policy==2){q.live_step=1;*q.live_budget=1;q.live_clock=12.f;}
            else if(q.live_policy==3)q.live_clock=8.f;
            else if(q.live_policy==4){q.live_step=1;*q.live_budget=0;}
        }
        return 1;
    };
    live_service.accept=[](void*,void*,unsigned,void*,void*,const void*,unsigned) noexcept -> int{return 1;};
    live_service.find_named=nullptr;live_service.find_class=nullptr;live_service.missing=nullptr;
    ServerEntityVoidOutputAction live_action{0u,590u,0u,0u,10u,0.f};
    outf.live_budget=&steps;
    auto prepare_live=[&](unsigned policy){
        reset_queue();outf.live_releases=0;outf.live_policy=policy;outf.live_step=0;outf.live_clock=10.f;steps=9;
        for(float deadline:{10.f,11.f,12.f}){
            assert(enqueue_named(live_action,deadline));
            f.put(outf.queue_rows[outf.queue_allocated-1],0x18,0x8002u);
        }
    };
    auto run_live=[&](){return service->service(outf.queue_root,sizeof(outf.queue_root),root_token,-99.f,false,&steps,&live_service,&sr);};
    prepare_live(1);assert(run_live()&&sr.events==3&&sr.clock_reads==3&&sr.step_reads==4&&!sr.steps_consumed&&steps==9&&!outf.service_locked);check_queue({});
    prepare_live(2);assert(run_live()&&sr.events==1&&sr.step_stopped&&sr.steps_consumed==1&&steps==0&&!outf.service_locked);check_queue({2,3});
    assert(run_live()&&sr.step_stopped&&!sr.events&&!sr.clock_reads&&sr.step_reads==1);
    outf.live_step=0;assert(run_live()&&sr.events==2&&!sr.steps_consumed);check_queue({});
    prepare_live(3);assert(run_live()&&sr.events==1&&sr.future_stopped&&sr.clock_reads==2);check_queue({2,3});
    outf.live_clock=12.f;assert(run_live()&&sr.events==2);check_queue({});
    prepare_live(4);assert(run_live()&&sr.events==1&&sr.step_stopped&&!sr.steps_consumed&&!steps);check_queue({2,3});
    reset_queue();outf.live_step=0;
    f.put(f.actor,0x378,saved_handle);f.put(f.actor,0xbdc,0u);
    // Datamap lookup, variant conversion and native field/handler dispatch.
    auto get_accept=reinterpret_cast<GetServerEntityAcceptInputApi>(GetProcAddress(server,server_entity_accept_input_export));
    assert(get_accept&&!GetProcAddress(client,server_entity_accept_input_export)&&!get_accept(0,sizeof(ServerEntityAcceptInputApi))&&!get_accept(1,0));
    const auto* accept=get_accept(1,sizeof(ServerEntityAcceptInputApi));assert(accept);
    struct InputFixture {
        F* f;
        alignas(4) unsigned char map[2][0x10]{},rows[2][8*0x34]{},variant[0x14]{};
        unsigned handlers{},diagnostics{},last_reason{},last_type{},last_value{},last_id{};
        ServerNpcActivePostAnimationBindings storage{};
        ServerEntityAcceptInputBindings bindings{};
        ServerEntityAcceptInputResult result{};
    } inp{&f};
    inp.storage.context=&inp;
    inp.storage.resolve=[](void* p,unsigned token,void** out,unsigned* n) noexcept -> int{
        auto& q=*static_cast<InputFixture*>(p);
        switch(token){
        case 0x22534674u:case 0x22534da4u:case 0x22535b28u:*out=q.map[0];*n=0x10;return 1;
        case 0x224dbd9cu:*out=q.map[1];*n=0x10;return 1;
        case 100u:case 101u:*out=q.rows[token-100];*n=sizeof(q.rows[0]);return 1;
        default:return 0;
        }
    };
    inp.bindings.storage=&inp.storage;inp.bindings.context=&inp;
    inp.bindings.entities.entries=f.slots;inp.bindings.entities.count=3;
    inp.bindings.name_equal=[](void*,unsigned a,unsigned b,bool* same) noexcept -> int{*same=a==b;return 1;};
    inp.bindings.handler=[](void* p,void* actor,unsigned token,const ServerEntityInputData* d) noexcept -> int{
        auto& q=*static_cast<InputFixture*>(p);assert(actor==q.f->actor&&token==0x220320b0u&&d->activator==q.f->weapon&&d->caller==q.f->actor);
        ++q.handlers;q.last_type=q.f->get<unsigned>(const_cast<unsigned char*>(d->variant),0x10);
        q.last_value=q.f->get<unsigned>(const_cast<unsigned char*>(d->variant),0);q.last_id=d->output_id;return 1;
    };
    inp.bindings.diagnostic=[](void* p,void*,void*,unsigned,unsigned reason,const void*) noexcept -> int{
        auto& q=*static_cast<InputFixture*>(p);++q.diagnostics;q.last_reason=reason;return 1;
    };
    inp.bindings.convert=[](void* p,void* v,unsigned type,bool* ok) noexcept -> int{
        auto& q=*static_cast<InputFixture*>(p);assert(type==5u&&q.f->get<unsigned>(v,0)==777u);
        q.f->put(v,0,-42);q.f->put(v,0x10,type);*ok=true;return 1;
    };
    f.put(inp.map[0],0,100u);f.put(inp.map[0],4,2);f.put(inp.map[0],0xc,0x224dbd9cu);
    f.put(inp.map[1],0,101u);f.put(inp.map[1],4,6);
    auto input_row=[&](unsigned m,unsigned i,unsigned name,unsigned type,unsigned flags,unsigned offset,unsigned handler){
        auto* row=inp.rows[m]+0x34*i;
        f.put(row,0,type);f.put(row,8,offset);f.put(row,0x12,static_cast<std::uint16_t>(flags));f.put(row,0x14,name);f.put(row,0x1c,handler);
    };
    // Non-input row with same external name must be skipped. Derived input
    // wins over inherited duplicate; inherited real Wake handler token.
    input_row(0,0,600u,5u,4u,0x9c,0u);
    input_row(0,1,601u,1u,12u,0x86c,0u);
    input_row(1,0,600u,5u,12u,0x9c,0u);
    input_row(1,1,601u,5u,12u,0x9c,0u);
    input_row(1,2,0x223db3c4u,0u,8u,0u,0x220320b0u);
    input_row(1,3,602u,6u,12u,0x870,0u);
    input_row(1,4,603u,2u,8u,0u,0x220320b0u);
    input_row(1,5,604u,13u,12u,0x1024,0u);
    auto run_input=[&](unsigned input,unsigned type,unsigned value){
        std::memset(inp.variant,0,sizeof(inp.variant));f.put(inp.variant,0,value);f.put(inp.variant,0xc,0x7001u);f.put(inp.variant,0x10,type);
        f.actor[0x1f0]=1;f.actor[0x200]=0;
        return accept->run(f.actor,sizeof(f.actor),ground_vt,input,f.weapon,f.actor,inp.variant,sizeof(inp.variant),77u,&inp.bindings,&inp.result);
    };
    assert(run_input(600u,2u,777u)&&inp.result.accepted&&inp.result.maps==2&&inp.result.row==0&&inp.result.converted&&inp.result.written==4&&f.get<int>(f.actor,0x9c)==-42&&f.actor[0x200]);
    assert(run_input(601u,5u,17u)&&inp.result.accepted&&inp.result.maps==1&&inp.result.row==1&&inp.result.actual_type==1&&f.get<float>(f.actor,0x86c)==17.f&&f.get<int>(f.actor,0x9c)==-42);
    const float minus=-3.75f;unsigned minus_word=0;std::memcpy(&minus_word,&minus,4);
    assert(run_input(600u,1u,minus_word)&&inp.result.accepted&&f.get<int>(f.actor,0x9c)==-3);
    assert(run_input(602u,5u,256u)&&inp.result.accepted&&inp.result.written==1&&f.actor[0x870]==1);
    assert(run_input(602u,1u,0u)&&inp.result.accepted&&!f.actor[0x870]);
    assert(run_input(604u,13u,0u)&&inp.result.accepted&&f.get<unsigned>(f.actor,0x1024)==0x7001u);
    assert(run_input(0x223db3c4u,2u,777u)&&inp.result.accepted&&inp.result.converted&&inp.handlers==1&&inp.last_type==0&&inp.last_value==0&&inp.last_id==77);
    assert(run_input(603u,0u,0u)&&inp.result.accepted&&!inp.result.converted&&inp.handlers==2&&inp.last_type==0);
    assert(run_input(600u,13u,0u)&&inp.result.completed&&!inp.result.accepted&&inp.result.reason==EntityInputBadConversion&&!f.actor[0x200]);
    assert(run_input(999u,0u,0u)&&inp.result.completed&&!inp.result.accepted&&inp.result.reason==EntityInputUnknown&&inp.diagnostics==2&&!f.actor[0x200]);
    f.put(inp.map[1],0xc,0x22534da4u);assert(!run_input(999u,0u,0u)&&!inp.result.completed&&!f.actor[0x200]);f.put(inp.map[1],0xc,0u);
    // Verify the three root getters for all eight known primary tables.
    for(unsigned vt:{0x2248df04u,0x224954a4u,0x2249aaa4u,0x224a416cu,0x2248c514u,0x2248e83cu,0x2249964cu,0x224a4a4cu}){
        assert(accept->run(f.actor,sizeof(f.actor),vt,601u,f.weapon,f.actor,inp.variant,sizeof(inp.variant),77u,&inp.bindings,&inp.result)&&inp.result.completed);
    }
    f.actor[0x870]=0;
    // Scripted source OnWake -> named event -> real inherited Wake input.
    // Original NPC input projection, not a sparse synthetic datamap.
    auto get_search=reinterpret_cast<GetServerEntitySearchApi>(GetProcAddress(server,server_entity_search_export));
    assert(get_search&&!GetProcAddress(client,server_entity_search_export)&&!get_search(0,sizeof(ServerEntitySearchApi))&&!get_search(1,0));
    const auto* entity_search=get_search(1,sizeof(ServerEntitySearchApi));assert(entity_search);
    struct ScriptWakeFixture {
        F* f;OutputFixture* output;void* target;
        const ServerEntityAcceptInputApi* accept;
        const ServerEntityEventServiceBindings* previous_service;
        const ServerEntitySearchApi* search_api;
        ServerEntitySearchBindings search{};
        ServerEntitySearchResult search_result{};
        ServerNpcActivePostAnimationBindings storage{};
        ServerEntityAcceptInputBindings input{};
        ServerEntityAcceptInputResult input_result{};
        ServerNpcWakeTransitionBindings wake{};
        ServerNpcWakeTransitionResult wake_result{};
    };
    static ScriptWakeFixture scripted{};scripted.f=&f;scripted.output=&outf;scripted.target=wt.member;scripted.accept=accept;scripted.previous_service=&service_bind;scripted.search_api=entity_search;
    scripted.storage.context=&scripted;
    sin_independent::EntityHandleEntry script_slots[3]{};
    script_slots[1].serial=7;script_slots[1].entity=reinterpret_cast<CBaseEntity*>(wt.member);
    script_slots[2].serial=8;script_slots[2].entity=reinterpret_cast<CBaseEntity*>(f.actor);
    script_slots[2].next=&script_slots[0];script_slots[0].next=&script_slots[1];
    f.put(f.actor,0xc0,501u);f.put(wt.member,0xc0,500u);f.put(wt.member,0x378,0x7001u);
    f.put(f.actor,0,ground_vt);f.put(wt.member,0,ground_vt);
    scripted.search.context=&scripted;scripted.search.entities.entries=script_slots;scripted.search.entities.count=3;scripted.search.head=&script_slots[2];
    scripted.search.extent=[](void* p,void* entity,unsigned* n) noexcept -> int{
        auto& q=*static_cast<ScriptWakeFixture*>(p);
        if(entity==q.f->actor)*n=sizeof(q.f->actor);else if(entity==q.target)*n=0xfd8;else return 0;return 1;
    };
    scripted.search.string_text=[](void*,unsigned token,const char** text,unsigned* n) noexcept -> int{
        switch(token){
        case 500u:*text="Recipient";break;case 501u:*text="Source";break;
        case 502u:*text="rECIPIENT";break;case 503u:*text="npc_grunt";break;
        case 504u:*text="NPC_GR*unused";break;case 505u:*text="*suffix";break;
        case 506u:*text="missing";break;case 507u:*text="!self";break;
        case 508u:*text="!AcTiVaToR";break;case 509u:*text="!player";break;
        case 510u:*text="!pvsplayer";break;case 511u:*text="!picker";break;
        case 512u:*text="!caller";break;
        default:return 0;
        }
        *n=static_cast<unsigned>(std::strlen(*text)+1);return 1;
    };
    scripted.input.storage=&scripted.storage;scripted.input.context=&scripted;
    scripted.input.entities.entries=script_slots;scripted.input.entities.count=3;
    scripted.input.name_equal=inp.bindings.name_equal;
    scripted.input.native_npc_datamaps=true;
    // Refuse every synthetic datamap resolution: the inherited Wake row
    // is supplied by the complete native NPC input projection instead.
    scripted.storage.resolve=[](void*,unsigned,void**,unsigned*) noexcept -> int{return 0;};
    scripted.input.wake=&scripted.wake;scripted.input.wake_result=&scripted.wake_result;
    // Neither a general input handler nor a hosted wake transition/output
    // is required for this unconnected recipient.
    assert(!scripted.input.handler&&!scripted.wake.output&&!scripted.wake.refresh_entity);
    auto script_service=service_bind;script_service.context=&scripted;script_service.entities=scripted.input.entities;
    script_service.resolve=[](void* p,unsigned token,void** out,unsigned* n) noexcept -> int{
        auto& q=*static_cast<ScriptWakeFixture*>(p);return q.previous_service->resolve(q.output,token,out,n);
    };
    script_service.lock=[](void* p,bool enter) noexcept -> int{
        auto& q=*static_cast<ScriptWakeFixture*>(p);return q.previous_service->lock(q.output,enter);
    };
    script_service.release=[](void* p,unsigned token,void* row) noexcept -> int{
        auto& q=*static_cast<ScriptWakeFixture*>(p);return q.previous_service->release(q.output,token,row);
    };
    script_service.find_named=nullptr;script_service.find_class=nullptr;script_service.accept=nullptr;script_service.missing=nullptr;
    script_service.native_search=&scripted.search;script_service.native_input=&scripted.input;
    reset_queue();seed_outputs();f.put(outf.rows[0],4,0x223db3c4u);f.put(outf.rows[0],0x18,0u);
    f.put(f.actor,0x378,0x8002u);f.put(f.actor,0x868,2);f.put(f.actor,0x78,0x20u);
    f.put(wt.member,0,ground_vt);f.put(wt.member,0x868,2);f.put(wt.member,0x78,0x20u);
    f.put(wt.member,0xbdc,0u);f.put(wt.member,0x16c,0u);wt.member[0x870]=0;wt.member[0x1f0]=1;wt.member[0x200]=0;
    // The composed route no longer uses the fixture's hosted output/queue
    // callback. Native transition supplies actor identity and queue time.
    ServerEntityNativeOutputQueueBindings direct_wake_queue{&outf.bindings,&outf.queue_bindings,outf.queue_root,sizeof(outf.queue_root),root_token};
    auto direct_queue_wake=connected;direct_queue_wake.output=nullptr;direct_queue_wake.output_queue=&direct_wake_queue;
    direct_queue_wake.entities=scripted.input.entities;direct_queue_wake.output_now=20.f;
    assert(transition_wake->run(f.actor,sizeof(f.actor),ground_vt,true,&direct_queue_wake,&wto)&&wto.completed&&wto.output_queue_calls==1&&wto.output_queue_actions==1&&outf.queue_allocated==1);
    assert(f.get<int>(f.actor,0x868)==0&&f.get<int>(wt.member,0x868)==2);
    assert(service->service(outf.queue_root,sizeof(outf.queue_root),root_token,20.f,false,&steps,&script_service,&sr)&&sr.future_stopped&&!sr.events);
    assert(service->service(outf.queue_root,sizeof(outf.queue_root),root_token,20.25f,false,&steps,&script_service,&sr)&&sr.completed&&sr.events==1&&sr.deliveries==1&&sr.removed==1);
    assert(sr.native_inputs==1&&!sr.rejected_inputs&&sr.native_named_searches==2&&!sr.native_class_searches&&!sr.foreign_inputs);
    assert(scripted.wake_result.completed&&scripted.wake_result.transitions==1&&scripted.wake_result.outputs==1&&scripted.wake_result.flags_changed==1);
    assert(f.get<int>(wt.member,0x868)==0&&f.get<unsigned>(wt.member,0x78)==0&&wt.member[0x200]&&!outf.service_locked);check_queue({});
    // A scripted Wake input uses AcceptInput time, not a stale binding clock.
    reset_queue();seed_outputs();f.put(outf.rows[0],4,0x223db3c4u);f.put(outf.rows[0],0x18,0u);
    f.put(f.actor,0x868,2);f.put(f.actor,0x78,0x20u);
    auto source_input=scripted.input;source_input.wake=&direct_queue_wake;source_input.wake_result=&wto;source_input.now=44.f;
    direct_queue_wake.output_now=12.f;
    unsigned char source_variant[0x14]{};ServerEntityAcceptInputResult source_result{};
    assert(accept->run(f.actor,sizeof(f.actor),ground_vt,0x223db3c4u,f.actor,f.actor,source_variant,sizeof(source_variant),0,&source_input,&source_result)&&source_result.native_wake&&wto.output_queue_actions==1);
    assert(f.get<float>(outf.queue_rows[0],0)==44.25f&&f.get<unsigned>(outf.queue_rows[0],0xc)==0x8002u&&f.get<unsigned>(outf.queue_rows[0],0x10)==0x8002u);
    reset_queue();f.put(f.actor,0x378,saved_handle);f.put(f.actor,0xbdc,0u);
    // Native input names compare bytes, not token identity. Decimal parsing
    // matches original 223a5673 including modulo-32-bit overflow.
    inp.bindings.name_equal=nullptr;inp.bindings.convert=nullptr;
    inp.bindings.string_text=[](void*,unsigned token,const char** text,unsigned* n) noexcept -> int{
        switch(token){
        case 600u:*text="SetHealth";break;case 1600u:*text="sEThEALTH";break;
        case 601u:*text="SetRadius";break;
        case 602u:*text="SetFlag";break;case 1602u:*text="SETFLAG";break;
        case 603u:*text="SetName";break;case 604u:*text="SetHandle";break;
        case 0x223db3c4u:*text="Wake";break;case 1700u:*text="wAkE";break;
        case 2000u:*text=" \t-42suffix";break;
        case 2001u:*text="+4294967296";break;
        case 2002u:*text="4294967297";break;
        case 2003u:*text="true";break;
        case 2004u:*text="-2147483649";break;
        case 2005u:*text="";break;
        case 2006u:*text="0x20";break;
        case 2999u:*text="Missing";break;
        case 3000u:{static const char bad[]{'x','y'};*text=bad;*n=sizeof(bad);return 1;}
        default:return 0;
        }
        *n=static_cast<unsigned>(std::strlen(*text)+1);return 1;
    };
    assert(run_input(1600u,2u,2000u)&&inp.result.accepted&&inp.result.converted&&f.get<int>(f.actor,0x9c)==-42);
    assert(run_input(1600u,2u,2001u)&&inp.result.accepted&&f.get<unsigned>(f.actor,0x9c)==0u);
    assert(run_input(1600u,2u,2002u)&&inp.result.accepted&&f.get<unsigned>(f.actor,0x9c)==1u);
    assert(run_input(1600u,2u,2004u)&&inp.result.accepted&&f.get<unsigned>(f.actor,0x9c)==0x7fffffffu);
    assert(run_input(1600u,2u,2005u)&&inp.result.accepted&&!f.get<unsigned>(f.actor,0x9c));
    assert(run_input(1600u,2u,0u)&&inp.result.accepted&&!f.get<unsigned>(f.actor,0x9c));
    assert(run_input(1600u,2u,2006u)&&inp.result.accepted&&!f.get<unsigned>(f.actor,0x9c));
    assert(run_input(1602u,2u,2002u)&&inp.result.accepted&&f.actor[0x870]==1);
    assert(run_input(1602u,2u,2003u)&&inp.result.accepted&&!f.actor[0x870]);
    const unsigned before_handlers=inp.handlers;
    assert(run_input(1700u,0u,0u)&&inp.result.accepted&&inp.handlers==before_handlers+1);
    assert(run_input(2999u,0u,0u)&&inp.result.completed&&!inp.result.accepted&&inp.result.reason==EntityInputUnknown);
    assert(!run_input(1600u,2u,3000u)&&!inp.result.completed&&!f.actor[0x200]);
    assert(!run_input(3000u,0u,0u)&&!inp.result.completed&&!f.actor[0x200]);
    // Original linked list order, not sorted slot indices, including the
    // null slot between source and recipient. Search aliases by actual text.
    f.put(f.actor,0x378,0x8002u);f.put(wt.member,0x378,0x7001u);
    f.put(f.actor,0xc,0u);f.put(wt.member,0xc,503u);
    auto& search_result=scripted.search_result;auto& search_bind=scripted.search;
    auto named=[&](void* previous,unsigned token,void* activator=nullptr,void* caller=nullptr){
        return entity_search->named(previous,token,activator,caller,&search_bind,&search_result);
    };
    auto by_class=[&](void* previous,unsigned token){return entity_search->classname(previous,token,&search_bind,&search_result);};
    assert(named(nullptr,502u)&&search_result.entity==wt.member&&search_result.visited==3&&search_result.empty_slots==1);
    assert(named(nullptr,505u)&&search_result.entity==f.actor&&search_result.visited==1);
    assert(named(f.actor,505u)&&search_result.entity==wt.member&&search_result.visited==2&&search_result.empty_slots==1);
    assert(named(wt.member,505u)&&!search_result.entity&&!search_result.visited);
    assert(named(nullptr,0u)&&!search_result.entity&&!search_result.visited);
    assert(by_class(nullptr,504u)&&search_result.entity==wt.member&&search_result.visited==3);
    assert(by_class(nullptr,505u)&&search_result.entity==f.actor); // null classname matches star
    assert(named(nullptr,507u,f.actor,wt.member)&&search_result.contextual&&search_result.entity==wt.member&&!search_result.visited);
    assert(named(nullptr,508u,f.actor,wt.member)&&search_result.entity==f.actor);
    assert(named(f.actor,508u,f.actor,wt.member)&&!search_result.entity&&!search_result.visited);
    search_bind.player_one=[](void* p,void** e) noexcept -> int{*e=static_cast<ScriptWakeFixture*>(p)->target;return 1;};
    search_bind.pvs_player=[](void* p,unsigned token,void** e) noexcept -> int{
        auto& q=*static_cast<ScriptWakeFixture*>(p);assert(token==123u||token==456u);*e=token==123u?q.target:q.f->actor;return 1;
    };
    search_bind.picker=[](void* p,void* player,void** e) noexcept -> int{
        auto& q=*static_cast<ScriptWakeFixture*>(p);assert(player==q.target);*e=q.f->actor;return 1;
    };
    assert(named(nullptr,509u)&&search_result.entity==wt.member);
    assert(named(nullptr,510u)&&search_result.entity==wt.member);
    f.put(f.actor,0x16c,123u);f.put(wt.member,0x16c,456u);
    assert(named(nullptr,510u,f.actor,wt.member)&&search_result.entity==f.actor);
    assert(named(nullptr,510u,f.actor)&&search_result.entity==wt.member);
    assert(named(nullptr,511u)&&search_result.entity==f.actor);
    assert(named(nullptr,512u)&&search_result.contextual&&!search_result.entity);
    assert(named(nullptr,506u)&&search_result.completed&&!search_result.entity);
    script_slots[1].next=&script_slots[2];assert(!named(nullptr,506u)&&!search_result.completed&&search_result.visited==3);script_slots[1].next=nullptr;
    f.put(wt.member,0x378,0x9001u);assert(!named(wt.member,505u)&&!search_result.completed);f.put(wt.member,0x378,0x7001u);
    f.put(f.actor,0x16c,0u);f.put(wt.member,0x16c,0u);f.put(f.actor,0x378,saved_handle);
    // Entity-valued input variants now use the native active-list search.
    // The input registry is authoritative, even if the supplied search
    // binding has no registry of its own.
    auto search_for_variant=search_bind;search_for_variant.entities={};
    inp.bindings.entities=scripted.input.entities;inp.bindings.search=&search_for_variant;
    f.put(f.actor,0x378,0x8002u);f.put(wt.member,0x378,0x7001u);
    assert(run_input(604u,2u,500u)&&inp.result.accepted&&inp.result.converted&&inp.result.actual_type==13&&f.get<unsigned>(f.actor,0x1024)==0x7001u);
    assert(run_input(604u,2u,502u)&&inp.result.accepted&&f.get<unsigned>(f.actor,0x1024)==0x7001u);
    for(unsigned token:{0u,506u,507u,508u}){
        assert(run_input(604u,2u,token)&&inp.result.accepted&&inp.result.actual_type==13&&f.get<unsigned>(f.actor,0x1024)==0xffffffffu);
    }
    // The original conversion passes null context but !player can still
    // resolve through the first-client engine service.
    assert(run_input(604u,2u,509u)&&inp.result.accepted&&f.get<unsigned>(f.actor,0x1024)==0x7001u);
    assert(run_input(603u,13u,0u)&&inp.result.accepted&&inp.result.converted&&inp.last_type==2&&inp.last_value==500u);
    f.put(wt.member,0xc0,0u);
    assert(run_input(603u,13u,123u)&&inp.result.accepted&&inp.last_type==2&&!inp.last_value);
    f.put(wt.member,0xc0,500u);
    script_slots[1].serial=9;
    assert(run_input(603u,13u,123u)&&inp.result.accepted&&inp.result.converted&&inp.result.actual_type==13&&inp.last_type==13&&inp.last_value==123u);
    script_slots[1].serial=7;
    f.put(f.actor,0x378,saved_handle);
    // Every input descriptor recovered from the original NPC maps is
    // reachable with no datamap backing storage. The next inherited map
    // (CBaseCombatCharacter) remains an explicit storage boundary.
    struct CatalogCase {unsigned vt,map,index,type,name,handler,maps;};
    const CatalogCase catalog[]{
        {ground_vt,0x224dbd9c,121,2,0x223db450,0x22031f90,2},
        {ground_vt,0x224dbd9c,122,5,0x223db434,0x22037a30,2},
        {ground_vt,0x224dbd9c,123,0,0x223db414,0x22031fe0,2},
        {ground_vt,0x224dbd9c,124,0,0x223db3f4,0x22031ff0,2},
        {ground_vt,0x224dbd9c,125,2,0x223db3d8,0x22032000,2},
        {ground_vt,0x224dbd9c,126,0,0x223db3c4,0x220320b0,2},
        {ground_vt,0x224dbd9c,127,2,0x223db3a0,0x2203f1f0,2},
        {ground_vt,0x224dbd9c,128,1,0x223db374,0x220320c0,2},
        {ground_vt,0x224dbd9c,129,0,0x223db360,0x220424a0,2},
        {ground_vt,0x224dbd9c,130,0,0x223db33c,0x2202e7d0,2},
        {ground_vt,0x224dbd9c,131,0,0x223db318,0x2202e7e0,2},
        {ground_vt,0x224dbd9c,132,0,0x223db2fc,0x22044d80,2},
        {ground_vt,0x224dbd9c,133,0,0x223db2d8,0x22042c00,2},
        {ground_vt,0x224dbd9c,134,2,0x223db2b8,0x22032120,2},
        {ground_vt,0x22534da4,31,0,0x22490f34,0x2235f260,1},
        {ground_vt,0x22534da4,32,0,0x22490f20,0x2235ddc0,1},
        {ground_vt,0x22534da4,33,0,0x22485b74,0x22357e30,1},
        {ground_vt,0x22534da4,34,0,0x22485b50,0x22357e40,1},
        {ground_vt,0x22534da4,35,2,0x223f5624,0x22357e50,1},
        {ground_vt,0x22534da4,36,6,0x22490eec,0x22357ea0,1},
        {vt,0x22535b28,19,2,0x22498948,0x22365fe0,1},
        {vt,0x22535b28,20,0,0x22498914,0x22363b80,1},
        {vt,0x22535b28,21,0,0x224988dc,0x22363580,1},
        {vt,0x22535b28,22,0,0x224988b8,0x22367470,1},
        {vt,0x22535b28,23,0,0x22498894,0x22366110,1},
        {vt,0x22535b28,24,0,0x2249886c,0x22363be0,1},
        {vt,0x22535b28,25,0,0x22498840,0x22363c40,1}
    };
    struct CatalogFixture {const CatalogCase* expected;unsigned calls{},fallback{};unsigned char parent[0x10]{};} catalog_f{};
    ServerNpcActivePostAnimationBindings catalog_storage{};catalog_storage.context=&catalog_f;
    catalog_storage.resolve=[](void* p,unsigned token,void** data,unsigned* n) noexcept -> int{
        auto& q=*static_cast<CatalogFixture*>(p);assert(token==0x2251f258u);++q.fallback;
        *data=q.parent;*n=sizeof(q.parent);return 1;
    };
    ServerEntityAcceptInputBindings catalog_b{};catalog_b.storage=&catalog_storage;catalog_b.context=&catalog_f;catalog_b.native_npc_datamaps=true;
    catalog_b.name_equal=[](void*,unsigned a,unsigned b,bool* same) noexcept -> int{*same=a==b;return 1;};
    catalog_b.handler=[](void* p,void*,unsigned handler,const ServerEntityInputData* d) noexcept -> int{
        auto& q=*static_cast<CatalogFixture*>(p);unsigned type=0;std::memcpy(&type,d->variant+0x10,4);
        assert(handler==q.expected->handler&&type==q.expected->type);++q.calls;return 1;
    };
    ServerEntityAcceptInputResult catalog_o{};
    for(const auto& item:catalog){
        catalog_f.expected=&item;std::memset(inp.variant,0,sizeof(inp.variant));f.put(inp.variant,0x10,item.type);
        assert(accept->run(f.actor,sizeof(f.actor),item.vt,item.name,f.weapon,f.actor,inp.variant,sizeof(inp.variant),0,&catalog_b,&catalog_o));
        assert(catalog_o.accepted&&catalog_o.native_maps==item.maps&&catalog_o.map==item.map&&catalog_o.row==item.index&&catalog_o.handler==item.handler&&catalog_o.expected_type==item.type);
    }
    assert(catalog_f.calls==27&&!catalog_f.fallback);
    // Elite has no local input rows; inherits all six Grunt descriptors.
    catalog_f.expected=&catalog[16];std::memset(inp.variant,0,sizeof(inp.variant));
    assert(accept->run(f.actor,sizeof(f.actor),0x2248df04u,catalog[16].name,f.weapon,f.actor,inp.variant,sizeof(inp.variant),0,&catalog_b,&catalog_o)&&catalog_o.accepted&&catalog_o.native_maps==2&&catalog_o.map==0x22534da4u);
    // Native projection uses its original literal name for alias matching.
    catalog_b.name_equal=nullptr;catalog_b.string_text=[](void*,unsigned token,const char** str,unsigned* n) noexcept -> int{
        assert(token==901u||token==902u);*str=token==901u?"sTaRtPaTrOlLiNg":"NotAnInput";*n=static_cast<unsigned>(std::strlen(*str)+1);return 1;
    };
    assert(accept->run(f.actor,sizeof(f.actor),ground_vt,901u,f.weapon,f.actor,inp.variant,sizeof(inp.variant),0,&catalog_b,&catalog_o)&&catalog_o.accepted&&catalog_o.row==33);
    assert(accept->run(f.actor,sizeof(f.actor),ground_vt,902u,f.weapon,f.actor,inp.variant,sizeof(inp.variant),0,&catalog_b,&catalog_o)&&catalog_o.completed&&!catalog_o.accepted&&catalog_o.reason==EntityInputUnknown&&catalog_o.native_maps==2&&catalog_f.fallback==1);
    // Ten recovered concrete NPC inputs without the general handler host.
    auto leaves=catalog_b;leaves.native_npc_handlers=true;leaves.handler=nullptr;leaves.now=31.f;
    leaves.name_equal=[](void*,unsigned a,unsigned b,bool* same) noexcept -> int{*same=a==b;return 1;};
    leaves.storage=&pb;f.actor[0x1f0]=1;
    auto run_leaf=[&](unsigned name,unsigned type=0,unsigned payload=0,unsigned cls=0,unsigned bytes=0){
        std::memset(inp.variant,0,sizeof(inp.variant));f.put(inp.variant,0,type==0?0u:payload);f.put(inp.variant,0x10,type);
        return accept->run(f.actor,bytes?bytes:sizeof(f.actor),cls?cls:ground_vt,name,f.weapon,f.actor,inp.variant,sizeof(inp.variant),0,&leaves,&catalog_o);
    };
    for(unsigned cls:{ground_vt,0x2248df04u,vt}){
        f.actor[0x968]=0;f.actor[0x200]=0;
        assert(run_leaf(0x223db33cu,0,0,cls)&&catalog_o.accepted&&catalog_o.native_handler&&!catalog_o.handler_dirty&&f.actor[0x968]==1&&f.actor[0x200]);
        assert(run_leaf(0x223db318u,0,0,cls)&&catalog_o.native_handler&&!f.actor[0x968]);
        f.actor[0xc65]=0;
        assert(run_leaf(0x22485b74u,0,0,cls)&&catalog_o.native_handler&&f.actor[0xc65]);
        assert(run_leaf(0x22485b50u,0,0,cls)&&catalog_o.native_handler&&!f.actor[0xc65]);
    }
    f.put(f.actor,0xb4,0x80000000u);
    assert(run_leaf(0x223db2fcu)&&catalog_o.native_handler&&catalog_o.handler_dirty==1&&f.get<unsigned>(f.actor,0xb4)==0x80000002u);
    assert(run_leaf(0x223db2fcu)&&catalog_o.native_handler&&!catalog_o.handler_dirty);
    assert(run_leaf(0x22490f34u)&&catalog_o.native_handler&&catalog_o.handler_dirty==1&&f.get<unsigned>(f.actor,0xb4)==0x80010002u);
    assert(run_leaf(0x22490f20u)&&catalog_o.native_handler&&catalog_o.handler_dirty==1&&f.get<unsigned>(f.actor,0xb4)==0x80000002u);
    assert(run_leaf(0x223db2d8u)&&catalog_o.native_handler&&catalog_o.handler_dirty==1&&f.get<unsigned>(f.actor,0xb4)==0x80000000u);
    // The original flag helper dirties external state before mutating.
    f.actor[0x1f0]=0;f.put(f.actor,0x16c,0x3333u);pf.dirty=0;
    assert(run_leaf(0x223db2fcu)&&catalog_o.handler_dirty==1&&pf.dirty==1&&f.get<unsigned>(f.actor,0xb4)==0x80000002u);
    f.put(f.actor,0x16c,0xdeadbeefu);
    assert(!run_leaf(0x223db2d8u)&&!catalog_o.completed&&!catalog_o.handler_dirty&&f.get<unsigned>(f.actor,0xb4)==0x80000002u);
    f.put(f.actor,0x16c,0u);f.actor[0x1f0]=1;
    for(unsigned value:{0u,1u,0xffu}){
        f.put(f.actor,0xc70,99.f);f.actor[0xc6c]=7;
        assert(run_leaf(0x22490eecu,6u,value)&&catalog_o.native_handler&&f.get<unsigned>(f.actor,0xc70)==0&&f.actor[0xc6c]==value);
    }
    f.put(f.actor,0xc70,99.f);
    assert(run_leaf(0x22490eecu,5u,256u)&&catalog_o.converted&&catalog_o.native_handler&&f.actor[0xc6c]==1&&!f.get<unsigned>(f.actor,0xc70));
    for(float duration:{2.5f,0.f,-3.f}){
        unsigned value=0;std::memcpy(&value,&duration,4);
        assert(run_leaf(0x223db374u,1u,value)&&catalog_o.native_handler&&f.get<float>(f.actor,0xc0c)==31.f+(duration>0?duration:10.f));
    }
    assert(run_leaf(0x223db374u,1u,0x7fc00000u)&&catalog_o.native_handler&&f.get<float>(f.actor,0xc0c)==41.f);
    assert(run_leaf(0x223db374u,5u,4u)&&catalog_o.converted&&f.get<float>(f.actor,0xc0c)==35.f);
    assert(!run_leaf(0x22485b74u,0,0,ground_vt,0xc65u)&&!catalog_o.completed);
    // Missing damage service fails only on the lowering branch.
    f.put(f.actor,0x9c,101);
    assert(!run_leaf(0x223db434u,5u,100u)&&catalog_o.matched&&!catalog_o.completed&&!catalog_o.native_handler);
    // Voice group setter preserves token identity and canonicalizes null or
    // empty strings, including the original void-to-string exception.
    leaves.string_text=[](void*,unsigned token,const char** value,unsigned* n) noexcept -> int{
        if(token==600u){*value="voice_group_alpha";*n=18;return 1;}
        if(token==601u){*value="";*n=1;return 1;}
        return 0;
    };
    assert(run_leaf(0x223db2b8u,2u,600u)&&catalog_o.native_handler&&f.get<unsigned>(f.actor,0x8e8)==600u);
    assert(run_leaf(0x223db2b8u,2u,601u)&&catalog_o.native_handler&&!f.get<unsigned>(f.actor,0x8e8));
    f.put(f.actor,0x8e8,600u);
    assert(run_leaf(0x223db2b8u)&&catalog_o.native_handler&&!f.get<unsigned>(f.actor,0x8e8));
    f.put(f.actor,0x8e8,600u);
    assert(run_leaf(0x223db2b8u,2u,0u)&&catalog_o.native_handler&&!f.get<unsigned>(f.actor,0x8e8));
    assert(!run_leaf(0x223db2b8u,2u,602u)&&!catalog_o.completed);
    // The complete Jetpack path flag extent reaches +1095, beyond the older
    // wake fixture. Use dedicated storage and a live generation registry.
    alignas(4) unsigned char path_actor[0x1098]{},path_entity[8]{};
    sin_independent::EntityHandleEntry path_slots[2]{};
    path_slots[1].serial=7;path_slots[1].entity=reinterpret_cast<CBaseEntity*>(path_entity);
    struct PathInputFixture {void* actor;unsigned calls{},last{};} path_f{path_actor};
    auto path_b=leaves;path_b.context=&path_f;
    path_b.entities.entries=path_slots;path_b.entities.count=2;
    path_b.path_warning=[](void* p,void* actor,unsigned warning) noexcept -> int{
        auto& q=*static_cast<PathInputFixture*>(p);assert(actor==q.actor);++q.calls;q.last=warning;return 1;
    };
    path_actor[0x1f0]=1;
    std::memset(inp.variant,0,sizeof(inp.variant));
    auto path_input=[&](unsigned name,unsigned bytes=0){
        return accept->run(path_actor,bytes?bytes:sizeof(path_actor),vt,name,f.weapon,f.actor,inp.variant,sizeof(inp.variant),0,&path_b,&catalog_o);
    };
    const unsigned path_inputs[]{0x22498914u,0x2249886cu,0x22498840u};
    const unsigned path_warnings[]{0x224982e8u,0x22498324u,0x22498358u};
    for(unsigned handle:{0xffffffffu,0x9001u,0x7000u}){
        f.put(path_actor,0xff4,handle);
        for(unsigned i=0;i<3;++i){
            path_actor[0x1084]=2;path_actor[0x1085]=3;path_actor[0x1095]=4;path_actor[0x200]=0;
            assert(path_input(path_inputs[i])&&catalog_o.native_handler&&catalog_o.path_missing==1&&path_f.last==path_warnings[i]);
            assert(path_actor[0x1084]==2&&path_actor[0x1085]==3&&path_actor[0x1095]==4&&path_actor[0x200]);
        }
        assert(path_input(0x224988dcu)&&catalog_o.native_handler&&!catalog_o.path_missing&&!path_actor[0x1084]&&path_actor[0x1085]==3&&path_actor[0x1095]==4);
    }
    assert(path_f.calls==9);
    f.put(path_actor,0xff4,0x7001u);
    for(unsigned flags:{0u,0x400000u}){
        f.put(path_actor,0xb4,flags);
        path_actor[0x1084]=0;path_actor[0x1085]=9;path_actor[0x1095]=5;
        assert(path_input(0x22498914u)&&catalog_o.native_handler&&!catalog_o.path_missing&&path_actor[0x1084]==1&&path_actor[0x1085]==(flags?1:9)&&path_actor[0x1095]==5);
        assert(path_input(0x224988dcu)&&!path_actor[0x1084]&&path_actor[0x1095]==5);
        path_actor[0x1085]=9;
        assert(path_input(0x2249886cu)&&path_actor[0x1084]==1&&path_actor[0x1085]==(flags?1:9)&&path_actor[0x1095]==1);
        assert(path_input(0x22498840u)&&path_actor[0x1084]==1&&!path_actor[0x1095]);
    }
    path_actor[0x1084]=0;path_actor[0x1085]=9;
    assert(!path_input(0x2249886cu,0x1088u)&&!catalog_o.completed&&!path_actor[0x1084]&&path_actor[0x1085]==9);
    assert(path_f.calls==9);
    // Original SetSquad: capacity, case-insensitive manager lookup, leave,
    // move, creation and full-list policy. Memory/peer effects are explicit.
    struct SquadInputFixture {
        F* f;void* peer;void* weapon;
        alignas(4) unsigned char groups[3][0x78]{};
        unsigned head{},allocated{},interned{},notices{},copies{},warnings{},last_warning{},weapon_caps{},cap_calls{};
    } sq{&f,wt.member,f.weapon};
    constexpr unsigned group_a=0x1000u,group_b=0x2000u,group_c=0x3000u;
    ServerNpcActivePostAnimationBindings sq_storage{};sq_storage.context=&sq;
    sq_storage.resolve=[](void* p,unsigned token,void** out,unsigned* n) noexcept -> int{
        auto& q=*static_cast<SquadInputFixture*>(p);
        if(token!=0x1000u&&token!=0x2000u&&!(token==0x3000u&&q.allocated))return 0;
        *out=q.groups[token/0x1000u-1];*n=0x78;return 1;
    };
    ServerNpcInputSquadBindings sq_services{};sq_services.context=&sq;sq_services.head=&sq.head;
    sq_services.weapon_capabilities=[](void* p,void* w,unsigned* caps) noexcept -> int{
        auto& q=*static_cast<SquadInputFixture*>(p);assert(w==q.weapon);++q.cap_calls;*caps=q.weapon_caps;return 1;
    };
    sq_services.allocate=[](void* p,unsigned* token,void** data,unsigned* n) noexcept -> int{
        auto& q=*static_cast<SquadInputFixture*>(p);assert(!q.allocated);++q.allocated;
        *token=0x3000u;*data=q.groups[2];*n=sizeof(q.groups[2]);std::memset(*data,0xcc,*n);return 1;
    };
    sq_services.intern=[](void* p,unsigned name,unsigned* canonical) noexcept -> int{
        auto& q=*static_cast<SquadInputFixture*>(p);assert(name==705u);++q.interned;*canonical=706u;return 1;
    };
    sq_services.removed_notice=[](void* p,void* peer,void* actor) noexcept -> int{
        auto& q=*static_cast<SquadInputFixture*>(p);assert(peer==q.peer&&actor==q.f->actor);
        assert(q.f->get<unsigned>(actor,0x9c4)==0x1000u&&q.f->get<int>(q.groups[0],0x48)==1);
        ++q.notices;return 1;
    };
    sq_services.copy_leader_memory=[](void* p,void* actor,void* leader) noexcept -> int{
        auto& q=*static_cast<SquadInputFixture*>(p);assert(actor==q.f->actor&&leader==q.peer);
        const unsigned token=q.f->get<unsigned>(actor,0x9c4);assert(token==0x1000u||token==0x2000u);
        assert(q.f->get<unsigned>(actor,0x9c8)==(token==0x1000u?701u:703u));
        ++q.copies;return 1;
    };
    sq_services.warning=[](void* p,void* actor,unsigned token) noexcept -> int{
        auto& q=*static_cast<SquadInputFixture*>(p);assert(actor==q.f->actor);++q.warnings;q.last_warning=token;return 1;
    };
    sin_independent::EntityHandleEntry sq_slots[3]{};
    sq_slots[0].serial=6;sq_slots[0].entity=reinterpret_cast<CBaseEntity*>(f.weapon);
    sq_slots[1].serial=7;sq_slots[1].entity=reinterpret_cast<CBaseEntity*>(wt.member);
    sq_slots[2].serial=8;sq_slots[2].entity=reinterpret_cast<CBaseEntity*>(f.actor);
    auto squad_b=leaves;squad_b.storage=&sq_storage;squad_b.squad=&sq_services;squad_b.name_equal=nullptr;
    squad_b.entities.entries=sq_slots;squad_b.entities.count=3;
    squad_b.string_text=[](void*,unsigned token,const char** text,unsigned* n) noexcept -> int{
        switch(token){
        case 0x223db3d8u:*text="SetSquad";break;
        case 701u:*text="Alpha";break;case 702u:*text="aLPHA";break;
        case 703u:*text="Beta";break;case 704u:*text="BETA";break;
        case 705u:*text="Gamma";break;case 706u:*text="Gamma";break;
        default:return 0;
        }*n=static_cast<unsigned>(std::strlen(*text)+1);return 1;
    };
    auto set_squad=[&](unsigned token,unsigned type=2u){
        std::memset(inp.variant,0,sizeof(inp.variant));f.put(inp.variant,0,token);f.put(inp.variant,0x10,type);
        return accept->run(f.actor,sizeof(f.actor),ground_vt,0x223db3d8u,f.weapon,f.actor,inp.variant,sizeof(inp.variant),0,&squad_b,&catalog_o);
    };
    f.put(f.actor,0x378,0x8002u);f.put(f.actor,0x78c,0xffffffffu);f.put(f.actor,0x8c,0);f.put(f.actor,0x8ec,0u);
    f.put(f.actor,0x9c4,0u);f.put(f.actor,0x9c8,123u);f.actor[0x1f0]=1;
    sq.head=group_a;f.put(sq.groups[0],0,group_b);f.put(sq.groups[0],4,701u);f.put(sq.groups[1],4,703u);
    f.put(sq.groups[0],8,0x7001u);f.put(sq.groups[0],0x48,1);
    f.put(sq.groups[1],8,0x7001u);f.put(sq.groups[1],0x48,1);
    assert(set_squad(702u)&&catalog_o.native_handler&&catalog_o.squad_denied==1&&sq.last_warning==0x223d9bc0u&&f.get<unsigned>(f.actor,0x9c8)==123u);
    f.put(f.actor,0x78c,0x6000u);sq.weapon_caps=0x4000000u;
    assert(set_squad(702u)&&catalog_o.native_handler&&catalog_o.squad_added==1&&catalog_o.squad_memory==1&&sq.cap_calls==1&&sq.copies==1);
    assert(f.get<unsigned>(f.actor,0x9c4)==group_a&&f.get<unsigned>(f.actor,0x9c8)==701u&&f.get<int>(sq.groups[0],0x48)==2&&f.get<unsigned>(sq.groups[0],12)==0x8002u);
    assert(set_squad(702u)&&!catalog_o.squad_added&&!catalog_o.squad_removed&&f.get<unsigned>(f.actor,0x9c8)==702u);
    assert(set_squad(0u,0u)&&catalog_o.squad_removed==1&&catalog_o.squad_notices==1&&sq.notices==1&&!f.get<unsigned>(f.actor,0x9c4)&&!f.get<unsigned>(f.actor,0x9c8));
    f.put(f.actor,0x78c,0xffffffffu);f.put(f.actor,0x8ec,0x4000000u);
    assert(set_squad(701u)&&catalog_o.squad_added&&sq.copies==2);
    assert(set_squad(704u)&&catalog_o.squad_visited==2&&catalog_o.squad_removed==1&&catalog_o.squad_added==1&&!catalog_o.squad_notices&&sq.copies==3);
    assert(f.get<int>(sq.groups[0],0x48)==1&&f.get<int>(sq.groups[1],0x48)==2&&f.get<unsigned>(f.actor,0x9c8)==703u);
    assert(set_squad(705u)&&catalog_o.squad_created==1&&catalog_o.squad_removed==1&&catalog_o.squad_added==1&&!catalog_o.squad_memory);
    assert(sq.head==group_c&&sq.allocated==1&&sq.interned==1&&f.get<unsigned>(sq.groups[2],0)==group_a&&f.get<unsigned>(sq.groups[2],4)==706u);
    assert(f.get<int>(sq.groups[2],0x48)==1&&f.get<unsigned>(sq.groups[2],8)==0x8002u&&f.get<unsigned>(sq.groups[2],0x4c)==group_c+8&&f.get<unsigned>(sq.groups[2],0x58)==0xffffffffu);
    // A full destination evicts the final row before append, as original.
    for(unsigned i=0;i<16;++i)f.put(sq.groups[1],8+4*i,i==0?0x7001u:0xffffffffu);
    f.put(sq.groups[1],0x48,16);
    assert(set_squad(703u)&&catalog_o.squad_full==1&&catalog_o.squad_removed==1&&catalog_o.squad_added==1&&sq.last_warning==0x223ee798u&&f.get<int>(sq.groups[1],0x48)==16&&f.get<unsigned>(sq.groups[1],8+15*4)==0x8002u);
    // Dead members skip list insertion; the outer input still sets +9c4.
    f.put(f.actor,0x8c,1);
    assert(set_squad(701u)&&!catalog_o.squad_added&&!catalog_o.squad_removed&&f.get<unsigned>(f.actor,0x9c4)==group_a&&f.get<int>(sq.groups[0],0x48)==1);
    f.put(f.actor,0x8c,0);f.put(f.actor,0x9c4,0u);f.put(f.actor,0x9c8,0u);f.put(f.actor,0x378,saved_handle);
    // ChangePathTrack shares native target search and generation validation;
    // the real path helper clears the cached progress and refreshes nav.
    struct JetLifecycleFixture {
        F* f;void* actor;void* left;void* right;
        unsigned resets{},prepared{},warnings{},last_warning{};
        std::vector<void*> removed;
    } jl{&f,path_actor,f.actor,wt.member};
    ServerNpcInputJetBindings jet_inputs_b{};jet_inputs_b.context=&jl;
    jet_inputs_b.reset_navigation=[](void* p,void* a,unsigned nav) noexcept -> int{
        auto& q=*static_cast<JetLifecycleFixture*>(p);assert(a==q.actor&&nav==1234u);
        assert(q.f->get<unsigned>(a,0xff4)==0x7001u&&q.f->get<unsigned>(a,0x1088)==0x7001u&&q.f->get<unsigned>(a,0x108c)==0x7001u);
        assert(q.f->get<unsigned>(a,0x1090)==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(q.right))&&!q.f->get<std::uint8_t>(a,0xff8));
        ++q.resets;return 1;
    };
    jet_inputs_b.prepare_thrust=[](void* p,void* a) noexcept -> int{
        auto& q=*static_cast<JetLifecycleFixture*>(p);assert(a==q.actor);
        ++q.prepared;q.f->put(a,0xfe4,0x8002u);q.f->put(a,0xfe8,0x7001u);return 1;
    };
    jet_inputs_b.remove_thrust=[](void* p,void* entity) noexcept -> int{
        auto& q=*static_cast<JetLifecycleFixture*>(p);assert(entity==q.left||entity==q.right);
        assert(q.f->get<unsigned>(q.actor,entity==q.right?0xfe8:0xfe4)==(entity==q.right?0x7001u:0x8002u));
        q.removed.push_back(entity);return 1;
    };
    auto jl_b=path_b;jl_b.context=&jl;jl_b.jet=&jet_inputs_b;jl_b.search=&search_bind;
    jl_b.entities=scripted.input.entities;
    jl_b.path_warning=[](void* p,void* a,unsigned token) noexcept -> int{
        auto& q=*static_cast<JetLifecycleFixture*>(p);assert(a==q.actor&&token==0x224984f8u);
        ++q.warnings;q.last_warning=token;return 1;
    };
    f.put(f.actor,0x378,0x8002u);f.put(wt.member,0x378,0x7001u);
    f.put(f.actor,0xc0,501u);f.put(wt.member,0xc0,500u);
    f.put(path_actor,0x8f8,1234u);
    auto jet_input=[&](unsigned name,unsigned type=0,unsigned token=0){
        std::memset(inp.variant,0,sizeof(inp.variant));f.put(inp.variant,0,token);f.put(inp.variant,0x10,type);
        return accept->run(path_actor,sizeof(path_actor),vt,name,f.weapon,f.actor,inp.variant,sizeof(inp.variant),0,&jl_b,&catalog_o);
    };
    for(unsigned name:{500u,502u}){
        path_actor[0xff8]=1;
        assert(jet_input(0x22498948u,2u,name)&&catalog_o.native_handler&&catalog_o.path_changed==1&&catalog_o.path_reset==1&&!catalog_o.path_missing);
        assert(f.get<unsigned>(path_actor,0xff4)==0x7001u);
    }
    for(unsigned name:{506u,0u}){
        path_actor[0xff8]=1;
        assert(jet_input(0x22498948u,2u,name)&&catalog_o.native_handler&&catalog_o.path_changed==1&&catalog_o.path_missing==1&&!catalog_o.path_reset);
        assert(f.get<unsigned>(path_actor,0xff4)==0xffffffffu&&f.get<unsigned>(path_actor,0x1088)==0xffffffffu&&f.get<unsigned>(path_actor,0x108c)==0xffffffffu&&!f.get<unsigned>(path_actor,0x1090)&&!path_actor[0xff8]);
    }
    assert(jet_input(0x22498948u)&&catalog_o.path_missing&&jl.warnings==3&&jl.resets==2);
    // ActivateJets returns if either existing thrust handle is live.
    // DeactivateJets ignores stale rows and removes right then left.
    for(unsigned left:{0xffffffffu,0x9002u,0x8002u})for(unsigned right:{0xffffffffu,0x9001u,0x7001u}){
        f.put(path_actor,0xfe4,left);f.put(path_actor,0xfe8,right);
        const bool reused=left==0x8002u||right==0x7001u;
        const unsigned before=jl.prepared;
        assert(jet_input(0x224988b8u)&&catalog_o.native_handler&&catalog_o.thrust_reused==reused&&catalog_o.thrust_prepared==!reused);
        assert(jl.prepared==before+(!reused));
        if(reused)assert(f.get<unsigned>(path_actor,0xfe4)==left&&f.get<unsigned>(path_actor,0xfe8)==right);
        // Restore original pair for independent deactivation test.
        f.put(path_actor,0xfe4,left);f.put(path_actor,0xfe8,right);jl.removed.clear();
        assert(jet_input(0x22498894u)&&catalog_o.native_handler&&catalog_o.thrust_removed==(left==0x8002u)+(right==0x7001u));
        std::vector<void*> expected;
        if(right==0x7001u)expected.push_back(jl.right);
        if(left==0x8002u)expected.push_back(jl.left);
        assert(jl.removed==expected);
        assert(f.get<unsigned>(path_actor,0xfe4)==(left==0x8002u?0xffffffffu:left)&&f.get<unsigned>(path_actor,0xfe8)==(right==0x7001u?0xffffffffu:right));
    }
    assert(jl.prepared==4);
    jl_b.jet=nullptr;f.put(path_actor,0xfe4,0x8002u);f.put(path_actor,0xfe8,0xffffffffu);
    assert(jet_input(0x224988b8u)&&catalog_o.thrust_reused);
    assert(!jet_input(0x22498894u)&&!catalog_o.completed&&f.get<unsigned>(path_actor,0xfe4)==0x8002u);
    f.put(f.actor,0x378,saved_handle);
    // Actual embedded rappel behavior registration and two private IDs.
    struct RappelInputFixture {
        F* f;
        alignas(4) unsigned char behavior[0x48]{},range[0x14]{},owner[0x48]{};
        unsigned detaches{};
    } rappel_f{&f};
    ServerNpcActivePostAnimationBindings rappel_storage{};rappel_storage.context=&rappel_f;
    rappel_storage.resolve=[](void* p,unsigned token,void** data,unsigned* bytes) noexcept -> int{
        auto& q=*static_cast<RappelInputFixture*>(p);
        if(token==0x2254c648u){*data=q.behavior;*bytes=sizeof(q.behavior);return 1;}
        if(token==0x9876u){*data=q.range;*bytes=sizeof(q.range);return 1;}
        return 0;
    };
    ServerNpcConditionMapBindings rappel_owner_map{};rappel_owner_map.context=&rappel_f;
    rappel_owner_map.descriptor=[](void* p,void* actor,const void** data,unsigned* bytes) noexcept -> int{
        auto& q=*static_cast<RappelInputFixture*>(p);assert(actor==q.f->actor);
        *data=q.owner;*bytes=sizeof(q.owner);return 1;
    };
    ServerNpcInputRappelBindings rappel_services{};rappel_services.context=&rappel_f;rappel_services.actor_conditions=&rappel_owner_map;
    rappel_services.detach=[](void* p,void* actor) noexcept -> int{
        auto& q=*static_cast<RappelInputFixture*>(p);assert(actor==q.f->actor);
        assert(q.f->get<unsigned>(actor,0x7e4)&0x100u);
        assert(q.f->get<float>(actor,0xd14)==-1.f); // snapshot follows detach
        q.f->put(actor,0xb8,0u);q.f->put(actor,0x2bc,42.f);++q.detaches;return 1;
    };
    auto rappel_b=leaves;rappel_b.rappel=&rappel_services;rappel_b.storage=&rappel_storage;
    f.put(rappel_f.behavior,0x34,npc_global_condition_base+200);
    f.put(rappel_f.behavior,0x38,100000);f.put(rappel_f.behavior,0x3c,100000);
    f.put(rappel_f.behavior,0x44,0x9876u);
    f.put(rappel_f.range,0,npc_global_condition_base+201);f.put(rappel_f.range,4,100001);f.put(rappel_f.range,8,100001);
    f.put(f.actor,0xcfc,0x223f956cu);f.put(f.actor,0xd00,static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.actor)));
    f.put(f.actor,0x7e4,0u);f.put(f.actor,0x2bc,1.f);f.put(f.actor,0x2c0,2.f);f.put(f.actor,0x2c4,3.f);
    f.put(f.actor,0xd14,-1.f);f.put(f.actor,0xd18,-1.f);f.put(f.actor,0xd1c,-1.f);
    f.put(f.actor,0xb8,0x800u);f.actor[0x1f0]=1;f.actor[0x200]=0;
    std::memset(inp.variant,0,sizeof(inp.variant));
    auto rappel_input=[&](unsigned name,unsigned cls=0,unsigned bytes=0){
        return accept->run(f.actor,bytes?bytes:sizeof(f.actor),cls?cls:ground_vt,name,f.weapon,f.actor,inp.variant,sizeof(inp.variant),0,&rappel_b,&catalog_o);
    };
    assert(rappel_input(0x223db414u)&&catalog_o.native_handler&&catalog_o.rappel_mapped&&catalog_o.rappel_condition&&catalog_o.rappel_detached&&catalog_o.rappel_snapshot&&rappel_f.detaches==1);
    assert(f.get<unsigned>(f.actor,0x7e4)==0x100u&&f.get<float>(f.actor,0xd14)==42.f&&f.get<float>(f.actor,0xd18)==2.f&&f.get<float>(f.actor,0xd1c)==3.f&&f.actor[0x200]);
    f.put(f.actor,0x2bc,99.f);f.put(f.actor,0xb8,0x800u);
    assert(rappel_input(0x223db3f4u,0x2248df04u)&&catalog_o.native_handler&&catalog_o.rappel_condition&&!catalog_o.rappel_detached&&!catalog_o.rappel_snapshot);
    assert(f.get<unsigned>(f.actor,0x7e4)==0x300u&&f.get<float>(f.actor,0xd14)==42.f&&f.get<unsigned>(f.actor,0xb8)==0x800u&&rappel_f.detaches==1);
    f.put(f.actor,0xb8,0u);
    assert(rappel_input(0x223db414u,vt)&&catalog_o.rappel_snapshot&&!catalog_o.rappel_detached&&f.get<float>(f.actor,0xd14)==99.f);
    // Unmapped private ID is an original successful no-op for the bank,
    // not a reason to skip the BeginRappel position snapshot.
    f.put(rappel_f.behavior,0x38,9999);f.put(rappel_f.behavior,0x44,0u);
    f.put(f.actor,0x7e4,0u);f.put(f.actor,0x2bc,7.f);
    assert(rappel_input(0x223db414u)&&catalog_o.native_handler&&!catalog_o.rappel_mapped&&!catalog_o.rappel_condition&&catalog_o.rappel_snapshot&&f.get<float>(f.actor,0xd14)==7.f&&!f.get<unsigned>(f.actor,0x7e4));
    f.put(f.actor,0xcfc,0u);
    assert(!rappel_input(0x223db414u)&&!catalog_o.completed);f.put(f.actor,0xcfc,0x223f956cu);
    f.put(f.actor,0xd00,0u);
    assert(!rappel_input(0x223db414u)&&!catalog_o.completed);
    f.put(f.actor,0xd00,static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.actor)));
    assert(!rappel_input(0x223db414u,ground_vt,0xd1fu)&&!catalog_o.completed);
    // Native SetHealth increase uses maximum and damageability gates.
    auto health_b=leaves;health_b.context=&f;
    health_b.health_damage=[](void* p,void* actor,const void* record,unsigned bytes) noexcept -> int{
        auto& q=*static_cast<F*>(p);assert(actor==q.actor&&bytes==0x48);
        auto* info=const_cast<void*>(record);
        for(unsigned off=0;off<0x24;off+=4)assert(q.get<unsigned>(info,off)==0);
        assert(q.get<unsigned>(info,0x24)==0x8002u&&q.get<unsigned>(info,0x28)==0x8002u);
        assert(q.get<float>(info,0x2c)==q.get<float>(info,0x30)&&q.get<unsigned>(info,0x34)==0x7f7fffffu);
        assert(!q.get<unsigned>(info,0x38)&&!q.get<unsigned>(info,0x3c)&&q.get<unsigned>(info,0x40)==0xffffffffu&&!q.get<unsigned>(info,0x44));
        q.trace.push_back(880);return 1;
    };
    auto set_health=[&](int value,unsigned cls){
        std::memset(inp.variant,0,sizeof(inp.variant));f.put(inp.variant,0,value);f.put(inp.variant,0x10,5u);
        return accept->run(f.actor,sizeof(f.actor),cls,0x223db434u,f.weapon,f.actor,inp.variant,sizeof(inp.variant),0,&health_b,&catalog_o);
    };
    f.put(f.actor,0x378,0x8002u);f.actor[0x1f0]=1;
    for(unsigned cls:{ground_vt,0x2248df04u,vt}){
        f.put(f.actor,0x98,100);f.put(f.actor,0x9c,40);f.put(f.actor,0x94,2);
        f.put(f.actor,0x16c,0u);
        assert(set_health(70,cls)&&catalog_o.native_handler&&catalog_o.health_blocked&&f.get<int>(f.actor,0x9c)==40);
        f.put(f.actor,0x16c,333u);f.put(f.actor,0x94,1);
        assert(set_health(70,cls)&&catalog_o.health_blocked&&f.get<int>(f.actor,0x9c)==40);
        f.put(f.actor,0x94,2);
        assert(set_health(70,cls)&&catalog_o.health_healed&&f.get<int>(f.actor,0x9c)==70);
        assert(set_health(130,cls)&&catalog_o.health_healed&&f.get<int>(f.actor,0x9c)==100);
        assert(set_health(130,cls)&&catalog_o.health_blocked&&f.get<int>(f.actor,0x9c)==100);
        assert(set_health(100,cls)&&!catalog_o.health_healed&&!catalog_o.health_damage&&!catalog_o.health_blocked);
        f.trace.clear();
        assert(set_health(20,cls)&&catalog_o.health_damage==1&&f.get<int>(f.actor,0x9c)==100&&(f.trace==std::vector<int>{880}));
    }
    f.put(f.actor,0x16c,0u);f.put(f.actor,0x378,saved_handle);f.trace.clear();
    // Native relationship parser and original 16-byte vector rows.
    struct RelFixture {
        F* f;
        alignas(4) unsigned char rows[8*16]{};
        unsigned grows{},warnings{},last_warning{},classes{};
    } rel{&f};
    ServerNpcActivePostAnimationBindings rel_storage{};rel_storage.context=&rel;
    rel_storage.resolve=[](void* p,unsigned token,void** data,unsigned* bytes) noexcept -> int{
        auto& q=*static_cast<RelFixture*>(p);if(token!=0x4444u)return 0;
        *data=q.rows;*bytes=sizeof(q.rows);return 1;
    };
    ServerNpcInputRelationshipBindings rel_services{};rel_services.context=&rel;
    rel_services.grow=[](void* p,void* actor,unsigned need) noexcept -> int{
        auto& q=*static_cast<RelFixture*>(p);assert(actor==q.f->actor&&need<=8);
        ++q.grows;q.f->put(actor,0x638,0x4444u);q.f->put(actor,0x63c,8);return 1;
    };
    rel_services.classify=[](void* p,const char* name,unsigned* cls,bool* known) noexcept -> int{
        auto& q=*static_cast<RelFixture*>(p);++q.classes;*known=std::strcmp(name,"npc_other")==0;*cls=*known?19u:0u;return 1;
    };
    rel_services.warning=[](void* p,void* actor,unsigned token,const char*) noexcept -> int{
        auto& q=*static_cast<RelFixture*>(p);assert(actor==q.f->actor);++q.warnings;q.last_warning=token;return 1;
    };
    auto rel_b=leaves;rel_b.relationship=&rel_services;rel_b.storage=&rel_storage;rel_b.search=&search_bind;
    rel_b.entities=scripted.input.entities;
    rel_b.string_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept -> int{
        switch(token){
        case 800u:*text="Recipient D_HT 7";break;
        case 801u:*text="rECIPIENT d_li";break;
        case 802u:*text="player D_FR 3 npc_other D_NU -2";break;
        case 803u:*text="npc_other D_HT";break;
        case 804u:*text="Unknown D_NU 0";break;
        case 805u:*text="Recipient bad 1";break;
        case 806u:*text="Recipient";break;
        case 807u:*text="* D_FR 9";break;
        case 808u:*text="Recipient D_NU 2";break;
        default:return 0;
        }*bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
    };
    f.put(f.actor,0x378,0x8002u);f.put(wt.member,0x378,0x7001u);
    f.put(f.actor,0xc0,501u);f.put(wt.member,0xc0,500u);
    f.put(f.actor,0x638,0u);f.put(f.actor,0x63c,0);f.put(f.actor,0x644,0);f.put(f.actor,0x648,0u);
    auto relationship_input=[&](unsigned token,unsigned type=2u){
        std::memset(inp.variant,0,sizeof(inp.variant));f.put(inp.variant,0,token);f.put(inp.variant,0x10,type);
        return accept->run(f.actor,sizeof(f.actor),ground_vt,0x223db450u,f.weapon,f.actor,inp.variant,sizeof(inp.variant),0,&rel_b,&catalog_o);
    };
    assert(relationship_input(800u)&&catalog_o.native_handler&&catalog_o.relationship_added==1&&catalog_o.relationship_entities==1&&rel.grows==1);
    assert(f.get<unsigned>(rel.rows,0)==0x7001u&&f.get<unsigned>(rel.rows,4)==0&&f.get<unsigned>(rel.rows,8)==1&&f.get<int>(rel.rows,12)==7);
    assert(relationship_input(801u)&&catalog_o.relationship_updated==1&&!catalog_o.relationship_added&&f.get<unsigned>(rel.rows,8)==3&&f.get<int>(rel.rows,12)==7);
    assert(relationship_input(802u)&&catalog_o.relationship_specs==2&&catalog_o.relationship_classes==2&&catalog_o.relationship_added==2);
    assert(f.get<unsigned>(rel.rows+16,0)==0xffffffffu&&f.get<unsigned>(rel.rows+16,4)==1&&f.get<unsigned>(rel.rows+16,8)==2&&f.get<int>(rel.rows+16,12)==3);
    assert(f.get<unsigned>(rel.rows+32,4)==19&&f.get<unsigned>(rel.rows+32,8)==4&&f.get<int>(rel.rows+32,12)==-2);
    assert(relationship_input(803u)&&catalog_o.relationship_updated==1&&f.get<unsigned>(rel.rows+32,8)==1&&f.get<int>(rel.rows+32,12)==-2);
    assert(relationship_input(804u)&&catalog_o.relationship_warnings==1&&rel.last_warning==0x223d9910u);
    assert(relationship_input(805u)&&catalog_o.relationship_warnings==1&&rel.last_warning==0x223d98d0u);
    assert(relationship_input(806u)&&catalog_o.relationship_warnings==1&&rel.last_warning==0x223d98a8u);
    assert(relationship_input(807u)&&catalog_o.relationship_entities==2&&catalog_o.relationship_added==1&&catalog_o.relationship_updated==1);
    assert(f.get<unsigned>(rel.rows+48,0)==0x8002u&&f.get<unsigned>(rel.rows+48,8)==2&&f.get<int>(rel.rows+48,12)==9);
    assert(f.get<int>(rel.rows,12)==9&&f.get<int>(f.actor,0x644)==4&&f.get<unsigned>(f.actor,0x648)==0x4444u);
    // A stale entry does not alias the newly live target with same slot.
    f.put(rel.rows,0,0x9001u);
    assert(relationship_input(808u)&&catalog_o.relationship_added==1&&f.get<int>(f.actor,0x644)==5&&f.get<unsigned>(rel.rows+64,0)==0x7001u);
    assert(relationship_input(0u,0u)&&catalog_o.native_handler&&!catalog_o.relationship_specs);
    f.put(f.actor,0x378,saved_handle);
    // ForgetEntity drives the actual native enemy-memory tree and setter.
    auto get_memory_control=reinterpret_cast<GetServerNpcEnemyMemoryControlApi>(GetProcAddress(server,server_npc_enemy_memory_control_export));
    auto get_memory_tree=reinterpret_cast<GetServerNpcEnemyMemoryTreeApi>(GetProcAddress(server,server_npc_enemy_memory_tree_export));
    assert(get_memory_control&&get_memory_tree);
    const auto* forget_control=get_memory_control(1,sizeof(ServerNpcEnemyMemoryControlApi));
    const auto* forget_tree=get_memory_tree(1,sizeof(ServerNpcEnemyMemoryTreeApi));
    assert(forget_control&&forget_tree);
    struct ForgetFixture {
        F* f;void* target;
        alignas(4) unsigned char memory[0x40]{},nodes[8*12]{},descriptor[0x48]{};
        unsigned warnings{},released{},last{};
    } forget_f{&f,wt.member};
    ServerNpcInputForgetBindings forget_services{};forget_services.context=&forget_f;forget_services.developer=true;
    forget_services.warning=[](void* p,unsigned token) noexcept -> int{
        auto& q=*static_cast<ForgetFixture*>(p);assert(token==0x223da448u);++q.warnings;return 1;
    };
    forget_services.memory.storage.context=&forget_f;
    forget_services.memory.storage.storage=[](void* p,unsigned token,void** data,unsigned* bytes) noexcept -> int{
        auto& q=*static_cast<ForgetFixture*>(p);
        if(token==0x9910u){*data=q.memory;*bytes=sizeof(q.memory);return 1;}
        if(token==0x9920u){*data=q.nodes;*bytes=sizeof(q.nodes);return 1;}
        return 0;
    };
    forget_services.memory.context=&forget_f;
    forget_services.memory.entity_key=[](void* p,void* target,unsigned* key) noexcept -> int{
        auto& q=*static_cast<ForgetFixture*>(p);if(target==q.target)*key=0x8100u;else if(target==q.f->actor)*key=0x8200u;else return 0;return 1;
    };
    forget_services.memory.release=[](void* p,unsigned token) noexcept -> int{
        auto& q=*static_cast<ForgetFixture*>(p);assert(token==0x9100u||token==0x9200u);
        if(token==0x9100u)assert(q.f->get<unsigned>(q.f->actor,0x8a0)==0xffffffffu&&q.f->get<int>(q.f->actor,0x858)==2);
        ++q.released;q.last=token;return 1;
    };
    forget_services.enemy.conditions.context=&forget_f;
    forget_services.enemy.conditions.descriptor=[](void* p,void* a,const void** data,unsigned* bytes) noexcept -> int{
        auto& q=*static_cast<ForgetFixture*>(p);assert(a==q.f->actor);*data=q.descriptor;*bytes=sizeof(q.descriptor);return 1;
    };
    f.put(forget_f.descriptor,0x34,npc_global_condition_base);f.put(forget_f.descriptor,0x38,0);f.put(forget_f.descriptor,0x3c,63);
    const float initial_memory[3]{};
    assert(forget_control->initialize(forget_f.memory,sizeof(forget_f.memory),0x9920u,8,initial_memory,&forget_services.memory.storage));
    ServerNpcEnemyMemoryTreeResult forget_result{};
    assert(forget_tree->insert(forget_f.memory,sizeof(forget_f.memory),0x8100u,0x9100u,&forget_services.memory.storage,&forget_result));
    assert(forget_tree->insert(forget_f.memory,sizeof(forget_f.memory),0x8200u,0x9200u,&forget_services.memory.storage,&forget_result));
    auto forget_b=leaves;forget_b.forget=&forget_services;forget_b.search=&search_bind;forget_b.entities=scripted.input.entities;forget_b.now=55.f;
    forget_b.string_text=search_bind.string_text;forget_b.context=search_bind.context;
    f.put(f.actor,0x378,0x8002u);f.put(wt.member,0x378,0x7001u);
    f.put(f.actor,0xc0,501u);f.put(wt.member,0xc0,500u);
    f.put(f.actor,0x984,0x9910u);f.put(f.actor,0x8a0,0x7001u);f.put(f.actor,0x858,3);f.put(f.actor,0x9c4,0u);
    f.put(f.actor,0x7cc,0xffffffffu);f.put(f.actor,0x7d0,0xffffffffu);f.actor[0x8b0]=1;
    auto forget_input=[&](unsigned token){
        std::memset(inp.variant,0,sizeof(inp.variant));f.put(inp.variant,0,token);f.put(inp.variant,0x10,2u);
        return accept->run(f.actor,sizeof(f.actor),ground_vt,0x223db3a0u,f.weapon,f.actor,inp.variant,sizeof(inp.variant),0,&forget_b,&catalog_o);
    };
    assert(forget_input(506u)&&catalog_o.native_handler&&!catalog_o.forget_found&&!forget_f.released);
    assert(forget_input(501u)&&catalog_o.forget_found&&catalog_o.forget_removed&&!catalog_o.forget_cleared&&forget_f.last==0x9200u);
    assert(f.get<unsigned>(f.actor,0x8a0)==0x7001u&&f.get<int>(f.actor,0x858)==3);
    assert(forget_input(502u)&&catalog_o.forget_found&&catalog_o.forget_cleared&&catalog_o.forget_removed&&forget_f.last==0x9100u);
    assert(f.get<unsigned>(f.actor,0x8a0)==0xffffffffu&&f.get<float>(f.actor,0x8a4)==55.f&&f.get<float>(f.actor,0xbf8)==-1.f&&!f.actor[0x8b0]);
    assert(f.get<unsigned>(f.actor,0x7cc)&(1u<<21));assert(!(f.get<unsigned>(f.actor,0x7cc)&(1u<<25)));
    assert(forget_tree->validate(forget_f.memory,sizeof(forget_f.memory),&forget_services.memory.storage,&forget_result)&&forget_result.count==0);
    assert(forget_input(500u)&&catalog_o.forget_found&&!catalog_o.forget_removed&&!catalog_o.forget_cleared&&forget_f.released==2);
    f.put(f.actor,0x378,saved_handle);
    // Original Assault behavior: native named search, rally priority and
    // squared distance tie-break, reservation, linked assault point and reset.
    struct AssaultFixture {
        F* f;
        alignas(4) unsigned char points[5][0x3c0]{};
        unsigned warnings{},last_warning{},casts{},detaches{};
        alignas(4) unsigned char conditions[0x48]{},schedules[0x18]{};
        unsigned behavior_list[1]{};
        bool ready=true;
        unsigned shared_calls{};
    } assault_f{&f};
    sin_independent::EntityHandleEntry assault_slots[6]{};
    for(unsigned i=0;i<5;++i){
        assault_slots[i].serial=1;assault_slots[i].entity=reinterpret_cast<CBaseEntity*>(assault_f.points[i]);
        assault_slots[i].next=&assault_slots[i+1];
        f.put(assault_f.points[i],0x378,0x1000u+i);
        f.put(assault_f.points[i],0xc0,i<4?901u:902u);
        f.put(assault_f.points[i],0x3bc,0xffffffffu);
        f.put(assault_f.points[i],0x394,902u);
    }
    assault_slots[5].serial=2;assault_slots[5].entity=reinterpret_cast<CBaseEntity*>(f.actor);
    f.put(f.actor,0x378,0x2005u);f.put(f.actor,0xc0,0u);
    f.put(f.actor,0xdb8,0x223f5544u);f.put(f.actor,0xdbc,static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.actor)));
    f.put(f.actor,0x2bc,0.f);f.put(f.actor,0x2c0,0.f);f.put(f.actor,0x2c4,0.f);f.put(f.actor,0xb8,0u);
    // First reserved candidate is ignored. Priority 5 beats priority 1;
    // closest among equal priority wins. Exact distance ties retain first.
    f.put(assault_f.points[0],0x3bc,0x2005u);f.put(assault_f.points[0],0x3a0,99);
    f.put(assault_f.points[1],0x3a0,1);f.put(assault_f.points[1],0x2bc,1.f);
    f.put(assault_f.points[2],0x3a0,5);f.put(assault_f.points[2],0x2bc,10.f);
    f.put(assault_f.points[3],0x3a0,5);f.put(assault_f.points[3],0x2bc,4.f);
    ServerEntitySearchBindings assault_search{};assault_search.context=&assault_f;assault_search.head=assault_slots;
    assault_search.extent=[](void* p,void* e,unsigned* bytes) noexcept -> int{
        auto& q=*static_cast<AssaultFixture*>(p);
        if(e==q.f->actor){*bytes=sizeof(q.f->actor);return 1;}
        for(auto& row:q.points)if(e==row){*bytes=sizeof(row);return 1;}
        return 0;
    };
    assault_search.foreign_handle=[](void* p,void* e,unsigned* handle) noexcept -> int{
        auto& q=*static_cast<AssaultFixture*>(p);*handle=q.f->get<unsigned>(e,0x378);return 1;
    };
    assault_search.string_text=[](void*,unsigned token,const char** text,unsigned* n) noexcept -> int{
        switch(token){case 901u:*text="Rally";break;case 902u:*text="AssaultPoint";break;case 903u:*text="rALLY";break;case 904u:*text="Missing";break;default:return 0;}
        *n=static_cast<unsigned>(std::strlen(*text)+1);return 1;
    };
    ServerNpcInputAssaultBindings assault_services{};assault_services.context=&assault_f;
    assault_services.point_type=[](void* p,void* e,bool rally,bool* yes) noexcept -> int{
        auto& q=*static_cast<AssaultFixture*>(p);++q.casts;*yes=false;
        for(unsigned i=0;i<5;++i)if(e==q.points[i])*yes=rally?i<4:i==4;return 1;
    };
    assault_services.detach=[](void* p,void* e) noexcept -> int{
        auto& q=*static_cast<AssaultFixture*>(p);++q.detaches;q.f->put(e,0xb8,0u);return 1;
    };
    assault_services.warning=[](void* p,void* e,unsigned token) noexcept -> int{
        auto& q=*static_cast<AssaultFixture*>(p);assert(e==q.f->actor);++q.warnings;q.last_warning=token;return 1;
    };
    auto assault_b=leaves;assault_b.context=&assault_f;assault_b.assault=&assault_services;
    assault_b.search=&assault_search;assault_b.entities.entries=assault_slots;assault_b.entities.count=6;
    auto assault_input=[&](unsigned name){
        std::memset(inp.variant,0,sizeof(inp.variant));f.put(inp.variant,0,name);f.put(inp.variant,0x10,2u);
        return accept->run(f.actor,sizeof(f.actor),ground_vt,0x223f5624u,f.weapon,f.actor,inp.variant,sizeof(inp.variant),0,&assault_b,&catalog_o);
    };
    f.put(f.actor,0x798,123u);f.put(f.actor,0x82c,0u);f.actor[0xddc]=1;f.actor[0xdde]=1;
    assert(assault_input(903u)&&catalog_o.native_handler&&catalog_o.assault_candidates==4&&catalog_o.assault_reserved&&catalog_o.assault_selected&&catalog_o.assault_linked&&catalog_o.assault_reset);
    assert(f.get<unsigned>(assault_f.points[3],0x3bc)==0x2005u&&f.get<unsigned>(f.actor,0xdd0)==0x1003u&&f.get<unsigned>(f.actor,0xdcc)==0x1004u&&f.get<unsigned>(f.actor,0xdd4)==3);
    assert(!f.get<unsigned>(f.actor,0x798)&&f.get<unsigned>(f.actor,0x82c)==0xffffffffu&&!f.actor[0xddc]&&!f.actor[0xdde]);
    f.put(assault_f.points[2],0x394,904u);
    assert(assault_input(901u)&&catalog_o.assault_selected&&catalog_o.assault_missing&&!catalog_o.assault_linked&&catalog_o.assault_reset&&assault_f.last_warning==0x223f3bc0u);
    assert(f.get<unsigned>(f.actor,0xdd0)==0x1002u&&!f.get<unsigned>(f.actor,0xdd4)&&f.get<unsigned>(f.actor,0xdcc)==0xffffffffu);
    assert(assault_input(901u)&&catalog_o.assault_selected&&catalog_o.assault_linked&&f.get<unsigned>(f.actor,0xdd0)==0x1001u);
    f.put(f.actor,0x798,321u);
    assert(assault_input(901u)&&catalog_o.assault_missing&&!catalog_o.assault_selected&&!catalog_o.assault_reset&&assault_f.last_warning==0x223f3cd0u);
    assert(f.get<unsigned>(f.actor,0x798)==321u);
    // One fresh InputAssault initializes the very same actor subsequently
    // consumed by native behavior admission and schedule arbitration.
    f.put(assault_f.points[3],0x3bc,0xffffffffu);
    f.put(f.actor,0xc14,0x2249aa40u);
    const unsigned actor_token=static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.actor));
    f.put(f.actor,0xdc4,actor_token+0xc14u);f.put(f.actor,0xc18,0u);
    f.put(f.actor,0xc1c,0x6611u);f.put(f.actor,0xc28,1);assault_f.behavior_list[0]=actor_token+0xdb8u;
    f.put(f.actor,0xde4,0.f);f.put(f.actor,0x8a0,0xffffffffu);f.put(f.actor,0x9c4,0u);
    for(unsigned i=0;i<8;++i)f.put(f.actor,0x7cc+4*i,0u);
    assert(assault_input(901u)&&catalog_o.assault_linked&&catalog_o.assault_reset&&f.get<unsigned>(f.actor,0xdd0)==0x1003u);
    auto get_arb=reinterpret_cast<GetServerNpcGruntAssaultArbitrationApi>(GetProcAddress(server,server_npc_grunt_assault_arbitration_export));
    assert(get_arb);const auto* arb=get_arb(1,sizeof(ServerNpcGruntAssaultArbitrationApi));assert(arb);
    ServerNpcGruntAssaultScheduleBindings assault_schedule{};auto& assault_admission=assault_schedule.admission;
    assault_admission.entries=assault_slots;assault_admission.count=6;assault_admission.actor_vtable=ground_vt;assault_admission.actor_token=actor_token;
    assault_admission.context=&assault_f;assault_admission.extent=assault_search.extent;
    assault_admission.conditions.context=&assault_f;
    assault_admission.conditions.descriptor=[](void* p,void* actor,const void** desc,unsigned* bytes) noexcept -> int{
        auto& q=*static_cast<AssaultFixture*>(p);assert(actor==q.f->actor);*desc=q.conditions;*bytes=sizeof(q.conditions);return 1;
    };
    assault_admission.owner_ready=[](void* p,void* actor,bool* yes) noexcept -> int{
        auto& q=*static_cast<AssaultFixture*>(p);assert(actor==q.f->actor);*yes=q.ready;return 1;
    };
    f.put(assault_f.conditions,0x34,npc_global_condition_base);f.put(assault_f.conditions,0x38,0);f.put(assault_f.conditions,0x3c,100);
    f.put(assault_f.schedules,4,npc_global_condition_base+300);f.put(assault_f.schedules,8,100000);
    f.put(assault_f.schedules,12,100009);f.put(assault_f.schedules,16,npc_global_condition_base+309);
    std::uint8_t guard=0;ServerNpcGruntAssaultActivationBindings activation{};activation.schedule=&assault_schedule;activation.activation_guard=&guard;
    ServerNpcGruntAssaultPolicyBindings assault_policy{};assault_policy.admission=assault_admission;
    assault_policy.behavior_schedules.context=&assault_f;
    assault_policy.behavior_schedules.descriptor=[](void* p,void* component,const void** desc,unsigned* bytes) noexcept -> int{
        auto& q=*static_cast<AssaultFixture*>(p);assert(component==q.f->actor+0xdb8);*desc=q.schedules;*bytes=sizeof(q.schedules);return 1;
    };
    ServerNpcGruntAssaultArbitrationBindings assault_arb{};assault_arb.activation=&activation;assault_arb.policy=&assault_policy;assault_arb.context=&assault_f;
    assault_arb.list=[](void* p,unsigned token,const void** list,unsigned* bytes) noexcept -> int{
        auto& q=*static_cast<AssaultFixture*>(p);assert(token==0x6611u);*list=q.behavior_list;*bytes=sizeof(q.behavior_list);return 1;
    };
    assault_arb.shared_select=[](void* p,void* actor,int* id) noexcept -> int{
        auto& q=*static_cast<AssaultFixture*>(p);assert(actor==q.f->actor);++q.shared_calls;*id=49;return 1;
    };
    ServerNpcGruntAssaultArbitrationResult arbitration{};
    assert(arb->select(f.actor,sizeof(f.actor),91.f,true,&assault_arb,&arbitration)&&arbitration.completed&&arbitration.selected&&arbitration.admission.admitted);
    assert(arbitration.transition.changed&&arbitration.transition.guard_set&&arbitration.transition.guard_cleared&&!guard);
    assert(arbitration.native_select&&arbitration.local_schedule==100000&&arbitration.private_id&&arbitration.mapped&&arbitration.schedule==npc_global_condition_base+300);
    assert(f.get<unsigned>(f.actor,0xc18)==actor_token+0xdb8u&&f.get<unsigned>(assault_f.points[3],0x3bc)==0x2005u);
    // The same behavior now fails native readiness and deactivates, releasing
    // the reservation placed by InputAssault. No hosted behavior selection.
    assault_f.ready=false;
    assert(arb->select(f.actor,sizeof(f.actor),92.f,true,&assault_arb,&arbitration)&&!arbitration.selected&&arbitration.transition.native_departure&&arbitration.transition.departure.reservation_released);
    assert(arbitration.shared_select&&arbitration.schedule==49&&assault_f.shared_calls==1&&!f.get<unsigned>(f.actor,0xc18)&&f.get<unsigned>(assault_f.points[3],0x3bc)==0xffffffffu);
    f.put(f.actor,0x378,saved_handle);
    // Break lifecycle keeps engine collision and fragment work external.
    // Native request fields, order and deferred removal are verified.
    struct BreakFixture {F* f;unsigned physics{},refreshes{};std::vector<unsigned> order;} break_f{&f};
    ServerNpcInputBreakBindings break_services{};break_services.context=&break_f;
    break_services.non_solid=[](void* p,void* a,unsigned flags) noexcept -> int{
        auto& q=*static_cast<BreakFixture*>(p);assert(a==q.f->actor&&flags==0x44u&&!q.f->get<int>(a,0x94));
        q.order.push_back(1);return 1;
    };
    break_services.refresh_transform=[](void* p,void* a,unsigned original) noexcept -> int{
        auto& q=*static_cast<BreakFixture*>(p);assert(a==q.f->actor);
        assert(original==0x2213d750u||original==0x2213fa60u);++q.refreshes;
        q.f->put(a,0xb8,q.f->get<unsigned>(a,0xb8)&~(original==0x2213d750u?0x1000u:0x800u));q.order.push_back(2);return 1;
    };
    break_services.physics_snapshot=[](void* p,unsigned token,float* v,float* av,float* origin,float* angles) noexcept -> int{
        auto& q=*static_cast<BreakFixture*>(p);assert(token==0x7777u);++q.physics;q.order.push_back(3);
        for(unsigned i=0;i<3;++i){v[i]=float(10+i);av[i]=float(20+i);origin[i]=-1.f;angles[i]=-2.f;}
        return 1;
    };
    break_services.effect=[](void* p,const ServerNpcBreakEffectRequest* req) noexcept -> int{
        auto& q=*static_cast<BreakFixture*>(p);assert(req->actor==q.f->actor);
        assert(req->model==42u&&req->skin==6u&&req->scale==100.f&&req->limit==-1&&!req->ignore_limit);
        assert(req->origin[0]==30.f&&req->angles[0]==40.f&&req->collision_group==(req->physics?7u:4u));
        assert(req->velocity[0]==(req->physics?10.f:1.f)&&req->angular_velocity[0]==(req->physics?20.f:2.f));
        q.order.push_back(4);return 1;
    };
    break_services.defer_remove=[](void* p,void* a) noexcept -> int{
        auto& q=*static_cast<BreakFixture*>(p);assert(a==q.f->actor&&!q.f->get<std::uint8_t>(a,0x200));
        q.order.push_back(5);return 1;
    };
    auto break_b=leaves;break_b.break_input=&break_services;
    std::memset(inp.variant,0,sizeof(inp.variant));
    auto break_run=[&](){return accept->run(f.actor,sizeof(f.actor),ground_vt,0x223db360u,f.weapon,f.actor,inp.variant,sizeof(inp.variant),0,&break_b,&catalog_o);};
    f.put(f.actor,0x94,2);f.put(f.actor,0x120,std::uint16_t{0x40});f.put(f.actor,0x218,0u);f.put(f.actor,0x214,0u);
    f.put(f.actor,0x38,42u);f.put(f.actor,0x62c,6u);f.put(f.actor,0x250,1.f);f.put(f.actor,0x25c,2.f);
    f.put(f.actor,0x2bc,30.f);f.put(f.actor,0x354,40.f);f.put(f.actor,0xb8,0x1800u);f.actor[0x200]=0;
    assert(break_run()&&catalog_o.native_handler&&catalog_o.break_damage_disabled&&catalog_o.break_non_solid&&catalog_o.break_effect&&catalog_o.break_removed&&!catalog_o.break_physics_snapshot);
    assert((break_f.order==std::vector<unsigned>{1,2,2,4,5})&&f.actor[0x200]&&break_f.refreshes==2);
    break_f.order.clear();f.actor[0x200]=0;f.put(f.actor,0x218,0x7777u);f.put(f.actor,0x214,7u);
    assert(break_run()&&catalog_o.native_handler&&!catalog_o.break_damage_disabled&&catalog_o.break_physics_snapshot&&catalog_o.break_removed);
    assert((break_f.order==std::vector<unsigned>{1,3,4,5})&&break_f.physics==1&&f.actor[0x200]);
    break_services.defer_remove=nullptr;break_f.order.clear();f.put(f.actor,0x94,2);
    assert(!break_run()&&!catalog_o.completed&&f.get<int>(f.actor,0x94)==2&&break_f.order.empty());
    f.put(f.actor,0x218,0u);
    // Fresh env_rockettrail pair: original writes, partial factory outcomes
    // and reuse. The factory publishes serial handles in the native registry.
    struct FreshThrustFixture {
        F* f;void* actor;
        alignas(4) unsigned char trails[2][0x3e0]{};
        unsigned handles[2]{},calls{},lifetimes{},parents{},indices{};
        std::vector<unsigned> order;
    } fresh_f{&f,path_actor};
    sin_independent::EntityHandleEntry fresh_slots[3]{};
    for(unsigned i=0;i<2;++i){
        fresh_slots[i+1].serial=7+i;fresh_slots[i+1].entity=reinterpret_cast<CBaseEntity*>(fresh_f.trails[i]);
    }
    ServerNpcJetThrustBindings fresh_services{};fresh_services.context=&fresh_f;
    fresh_services.create=[](void* p,unsigned classname,unsigned* handle) noexcept -> int{
        auto& q=*static_cast<FreshThrustFixture*>(p);assert(classname==0x223d3248u&&q.calls<2);
        *handle=q.handles[q.calls];q.order.push_back(++q.calls);return 1;
    };
    fresh_services.extent=[](void* p,void* e,unsigned* n) noexcept -> int{
        auto& q=*static_cast<FreshThrustFixture*>(p);
        if(e!=q.trails[0]&&e!=q.trails[1])return 0;*n=sizeof(q.trails[0]);return 1;
    };
    fresh_services.lifetime=[](void* p,void* e,float duration) noexcept -> int{
        auto& q=*static_cast<FreshThrustFixture*>(p);assert(q.calls==2&&duration==500.f);
        const unsigned side=e==q.trails[0]?0u:1u;assert(e==q.trails[side]);
        assert(q.f->get<unsigned>(q.actor,0xfe4)==0x7001u&&q.f->get<unsigned>(q.actor,0xfe8)==0x8002u);
        assert(q.f->get<float>(e,0x3b0)==10.f&&q.f->get<float>(e,0x3b4)==1.f&&q.f->get<float>(e,0x3c4)==8.f&&q.f->get<float>(e,0x3c8)==20.f);
        assert(q.f->get<float>(e,0x3cc)==10.f&&q.f->get<float>(e,0x3bc)==15.f&&q.f->get<float>(e,0x3c0)==25.f&&q.f->get<float>(e,0x3dc)==.5f);
        for(unsigned i=0;i<3;++i){assert(q.f->get<float>(e,0x394+4*i)==.25f);assert(q.f->get<float>(e,0x3a0+4*i)==0.f);}
        ++q.lifetimes;q.order.push_back(10+side);return 1;
    };
    fresh_services.attach_parent=[](void* p,void* e,void* a,unsigned off) noexcept -> int{
        auto& q=*static_cast<FreshThrustFixture*>(p);assert(a==q.actor&&off==0x1028u);
        assert(q.lifetimes==q.parents+1);++q.parents;q.order.push_back(e==q.trails[0]?20u:21u);return 1;
    };
    fresh_services.attachment_index=[](void* p,void* a,unsigned off,unsigned name,unsigned* index) noexcept -> int{
        auto& q=*static_cast<FreshThrustFixture*>(p);assert(a==q.actor&&off==0x1028u&&q.parents==q.indices+1);
        assert(name==(q.indices?0x22498830u:0x22498838u));
        *index=11u+q.indices;q.order.push_back(30u+q.indices);++q.indices;return 1;
    };
    auto fresh_jet=jet_inputs_b;fresh_jet.prepare_thrust=nullptr;fresh_jet.fresh=&fresh_services;
    auto fresh_b=path_b;fresh_b.jet=&fresh_jet;fresh_b.entities.entries=fresh_slots;fresh_b.entities.count=3;
    auto fresh_run=[&](){
        std::memset(inp.variant,0,sizeof(inp.variant));
        return accept->run(path_actor,sizeof(path_actor),vt,0x224988b8u,f.weapon,f.actor,inp.variant,sizeof(inp.variant),0,&fresh_b,&catalog_o);
    };
    for(unsigned left:{0xffffffffu,0x9001u,0x7001u})for(unsigned right:{0xffffffffu,0x9002u,0x8002u}){
        fresh_f.calls=fresh_f.lifetimes=fresh_f.parents=fresh_f.indices=0;fresh_f.order.clear();
        fresh_f.handles[0]=left;fresh_f.handles[1]=right;
        f.put(path_actor,0xfe4,0xffffffffu);f.put(path_actor,0xfe8,0xffffffffu);
        for(auto& trail:fresh_f.trails){
            std::memset(trail,0,sizeof(trail));trail[0x1f0]=1;
            f.put(trail,0x3a0,2.f);f.put(trail,0x3a4,2.f);f.put(trail,0x3a8,2.f);
        }
        const bool complete=left==0x7001u&&right==0x8002u;
        assert(fresh_run()&&catalog_o.native_handler&&catalog_o.thrust_prepared&&fresh_f.calls==2&&catalog_o.thrust.partial==!complete);
        assert(f.get<unsigned>(path_actor,0xfe4)==left&&f.get<unsigned>(path_actor,0xfe8)==right);
        assert(catalog_o.thrust.configured==(complete?2u:0u)&&catalog_o.thrust.writes==(complete?30u:0u));
        if(complete){
            assert((fresh_f.order==std::vector<unsigned>{1,2,10,20,30,11,21,31}));
            assert(f.get<unsigned>(fresh_f.trails[0],0x3d4)==11u&&f.get<unsigned>(fresh_f.trails[1],0x3d4)==12u);
            assert(fresh_f.trails[0][0x200]&&fresh_f.trails[1][0x200]);
        }else assert((fresh_f.order==std::vector<unsigned>{1,2}));
        if(left==0x7001u||right==0x8002u){
            assert(fresh_run()&&catalog_o.thrust_reused&&fresh_f.calls==2);
        }
    }
    // Already initialized fields, including negative zero, must not dirty.
    f.put(path_actor,0xfe4,0xffffffffu);f.put(path_actor,0xfe8,0xffffffffu);
    fresh_f.calls=fresh_f.lifetimes=fresh_f.parents=fresh_f.indices=0;fresh_f.order.clear();
    for(auto& trail:fresh_f.trails){trail[0x200]=0;f.put(trail,0x3a0,-0.f);}
    assert(fresh_run()&&catalog_o.thrust.configured==2&&!catalog_o.thrust.writes);
    assert(!fresh_f.trails[0][0x200]&&!fresh_f.trails[1][0x200]&&f.get<unsigned>(fresh_f.trails[0],0x3a0)==0x80000000u);
    // Same fresh helper is consumed by the actual Jetpack wake prefix,
    // with no legacy jet_prepare callback.
    sin_independent::EntityHandleEntry fresh_wake_slots[4]{};
    fresh_wake_slots[1]=fresh_slots[1];fresh_wake_slots[2]=fresh_slots[2];
    fresh_wake_slots[3].serial=9;fresh_wake_slots[3].entity=reinterpret_cast<CBaseEntity*>(wt.member);
    auto fresh_wake=wtb;fresh_wake.entities.entries=fresh_wake_slots;fresh_wake.entities.count=4;
    fresh_wake.jet_prepare=nullptr;fresh_wake.fresh_thrust=&fresh_services;
    fresh_wake.output=nullptr;fresh_wake.squad_next=nullptr;
    f.put(path_actor,0xfe4,0xffffffffu);f.put(path_actor,0xfe8,0xffffffffu);
    f.put(path_actor,0x1024,0x9003u);f.put(path_actor,0x218,123u);
    f.put(path_actor,0x868,2);f.put(path_actor,0x78,0x20u);f.put(path_actor,0xbdc,0u);path_actor[0x870]=0;
    f.put(wt.member,0,ground_vt);f.put(wt.member,0x868,2);f.put(wt.member,0xbdc,0u);wt.member[0x870]=0;
    fresh_f.calls=fresh_f.lifetimes=fresh_f.parents=fresh_f.indices=0;fresh_f.order.clear();f.trace.clear();
    assert(transition_wake->run(path_actor,sizeof(path_actor),vt,true,&fresh_wake,&wto)&&wto.completed&&wto.jet_prepared&&wto.jet_linked&&wto.jet_attachment&&wto.transitions==2);
    assert(wto.thrust.created==2&&wto.thrust.configured==2&&!wto.thrust.writes&&f.get<int>(path_actor,0x868)==0&&f.get<int>(wt.member,0x868)==0);
    assert((fresh_f.order==std::vector<unsigned>{1,2,10,20,30,11,21,31}));
    f.trace.clear();
    // Native squad join reports each live leader memory in tree order.
    // Leader and recipient own separate indexed trees and record pools.
    struct SquadMemoryFixture {
        F* f;void* leader;
        alignas(4) unsigned char owners[2][0x40]{},nodes[2][8*12]{},source[4][0x38]{},dest[4][0x38]{};
        unsigned allocated{},released{};
    } sqm{&f,wt.member};
    ServerNpcGruntMemoryReportBindings squad_report{};
    squad_report.mutation.context=&sqm;squad_report.mutation.storage.context=&sqm;
    squad_report.mutation.storage.storage=[](void* p,unsigned token,void** out,unsigned* bytes) noexcept -> int{
        auto& q=*static_cast<SquadMemoryFixture*>(p);
        if(token>=0xa100u&&token<0xa102u){*out=q.owners[token-0xa100u];*bytes=0x40;return 1;}
        if(token>=0xa200u&&token<0xa202u){*out=q.nodes[token-0xa200u];*bytes=sizeof(q.nodes[0]);return 1;}
        if(token>=0xa300u&&token<0xa304u){*out=q.source[token-0xa300u];*bytes=0x38;return 1;}
        if(token>=0xa400u&&token<0xa400u+q.allocated){*out=q.dest[token-0xa400u];*bytes=0x38;return 1;}
        return 0;
    };
    squad_report.mutation.allocate=[](void* p,unsigned need,unsigned* token,void** data,unsigned* bytes) noexcept -> int{
        auto& q=*static_cast<SquadMemoryFixture*>(p);assert(need==0x38&&q.allocated<4);
        *token=0xa400u+q.allocated;*data=q.dest[q.allocated++];*bytes=0x38;return 1;
    };
    squad_report.mutation.release=[](void* p,unsigned) noexcept -> int{++static_cast<SquadMemoryFixture*>(p)->released;return 1;};
    squad_report.mutation.entity_key=[](void* p,void* e,unsigned* key) noexcept -> int{
        auto& q=*static_cast<SquadMemoryFixture*>(p);if(e==q.f->weapon)*key=0x8100u;else if(e==q.f->actor)*key=0x8400u;else return 0;return 1;
    };
    squad_report.mutation.entity_handle=[](void* p,void* e,unsigned* handle) noexcept -> int{
        auto& q=*static_cast<SquadMemoryFixture*>(p);if(e==q.f->weapon)*handle=0x6000u;else if(e==q.f->actor)*handle=0x8002u;else return 0;return 1;
    };
    for(unsigned i=0;i<2;++i)assert(forget_control->initialize(sqm.owners[i],0x40,0xa200u+i,8,initial_memory,&squad_report.mutation.storage));
    const unsigned source_keys[]{0x8400u,0x8200u,0x8100u,0x8300u};
    const unsigned source_handles[]{0x8002u,0x9000u,0x6000u,0xffffffffu};
    for(unsigned i=0;i<4;++i){
        f.put(sqm.source[i],0,source_handles[i]);f.put(sqm.source[i],4,float(10+i));f.put(sqm.source[i],8,2.f);f.put(sqm.source[i],12,3.f);
        assert(forget_tree->insert(sqm.owners[0],0x40,source_keys[i],0xa300u+i,&squad_report.mutation.storage,&forget_result));
    }
    sq.head=group_a;f.put(sq.groups[0],0,0u);f.put(sq.groups[0],0x48,1);f.put(sq.groups[0],8,0x7001u);
    f.put(f.actor,0x378,0x8002u);f.put(f.actor,0x9c4,0u);f.put(f.actor,0x9c8,0u);
    f.put(f.actor,0x8c,0);f.put(f.actor,0x8ec,0x4000000u);f.put(f.actor,0x78c,0xffffffffu);
    f.put(f.actor,0x984,0xa101u);f.put(wt.member,0x984,0xa100u);f.put(wt.member,0,ground_vt);
    f.put(f.actor,0x850,1);f.put(f.actor,0x85c,123u);f.put(f.actor,0x864,456u);f.actor[0xb6]&=~1u;
    sq_services.copy_leader_memory=nullptr;sq_services.native_memory=&squad_report;
    sq_services.leader_extent=[](void* p,void* leader,unsigned* bytes) noexcept -> int{
        auto& q=*static_cast<SquadInputFixture*>(p);assert(leader==q.peer);*bytes=0xfd8u;return 1;
    };
    squad_b.now=80.f;
    assert(set_squad(701u)&&catalog_o.native_handler&&catalog_o.squad_added&&catalog_o.squad_memory);
    assert(catalog_o.squad_memory_visited==4&&catalog_o.squad_memory_stale==2&&catalog_o.squad_memory_reported==2&&catalog_o.squad_memory_created==2&&!catalog_o.squad_memory_updated);
    assert(sqm.allocated==2&&!sqm.released&&f.get<unsigned>(sqm.dest[0],0)==0x6000u&&f.get<float>(sqm.dest[0],4)==12.f);
    assert(f.get<unsigned>(sqm.dest[1],0)==0x8002u&&f.get<float>(sqm.dest[1],4)==10.f);
    assert(f.get<float>(sqm.dest[0],0x30)==-std::numeric_limits<float>::max()&&!f.get<unsigned>(f.actor,0x85c)&&!f.get<unsigned>(f.actor,0x864));
    assert(forget_tree->validate(sqm.owners[1],0x40,&squad_report.mutation.storage,&forget_result)&&forget_result.count==2);
    assert(forget_tree->validate(sqm.owners[0],0x40,&squad_report.mutation.storage,&forget_result)&&forget_result.count==4);
    assert(set_squad(701u)&&!catalog_o.squad_added&&!catalog_o.squad_memory_reported&&sqm.allocated==2);
    f.put(f.actor,0x9c4,0u);f.put(f.actor,0x9c8,0u);f.put(f.actor,0x378,saved_handle);
    // SetSquad now calls the same native removal and survivor route used
    // by the independent death path. No hosted removed_notice is needed.
    ServerNpcGruntSquadLeaveBindings leave_native{};
    leave_native.context=&sq;leave_native.sentence.speech.context=&sq;
    leave_native.sentence.speech.squad_storage=sq_storage.resolve;
    leave_native.sentence.speech.random_float=[](void*,float lo,float hi,float* result) noexcept -> int{
        assert(lo==1.5f&&hi==2.f);*result=1.75f;return 1;
    };
    leave_native.sentence.extent=[](void* p,void* entity,unsigned* bytes) noexcept -> int{
        auto& q=*static_cast<SquadInputFixture*>(p);
        if(entity==q.peer){*bytes=0xfd8u;return 1;}if(entity==q.f->actor){*bytes=sizeof(q.f->actor);return 1;}return 0;
    };
    leave_native.sentence.entity_rejected=[](void*,void*,bool* rejected) noexcept -> int{*rejected=false;return 1;};
    leave_native.member_class=[](void* p,void* member,unsigned* vtable,unsigned* bytes) noexcept -> int{
        auto& q=*static_cast<SquadInputFixture*>(p);assert(member==q.peer);*vtable=0x2249aaa4u;*bytes=0xfd8u;return 1;
    };
    sq_services.native_leave=&leave_native;sq_services.removed_notice=nullptr;
    auto seed_leave=[&](int health,unsigned flags){
        f.put(f.actor,0x378,0x8002u);f.put(f.actor,0x9c4,group_a);f.put(f.actor,0x9c8,701u);f.put(f.actor,0x9c,health);
        f.put(wt.member,0x9c4,group_a);f.put(wt.member,0x204,1u);f.put(wt.member,0xbf,std::uint8_t(flags));
        f.put(wt.member,0x850,1);f.put(wt.member,0xb4,2u);
        f.put(wt.member,0xc04,0.f);f.put(wt.member,0xc08,-1);
        f.put(sq.groups[0],0x48,3);f.put(sq.groups[0],8,0x7001u);f.put(sq.groups[0],12,0x8002u);f.put(sq.groups[0],16,0x9001u);
        f.put(sq.groups[0],0x50,0.f);f.put(sq.groups[0],0x54,-1);
    };
    seed_leave(20,0);
    assert(set_squad(0u,0u)&&catalog_o.squad_removed==1&&catalog_o.squad_native_notices==1&&catalog_o.squad_stale_notices==1&&catalog_o.squad_leave.last.survivor.removed_positive);
    assert(!f.get<unsigned>(f.actor,0x9c4)&&!f.get<unsigned>(f.actor,0x9c8)&&f.get<int>(sq.groups[0],0x48)==2&&f.get<unsigned>(sq.groups[0],12)==0x9001u);
    assert(!f.get<float>(wt.member,0xc04));
    seed_leave(0,8);
    assert(set_squad(0u,0u)&&catalog_o.squad_native_notices==1&&catalog_o.squad_leave.last.survivor.flag_blocked&&!f.get<float>(wt.member,0xc04));
    seed_leave(0,0);
    assert(set_squad(0u,0u)&&catalog_o.squad_native_notices==1&&catalog_o.squad_leave.last.survivor.last_member);
    assert(!catalog_o.squad_leave.last.survivor.sentence.mode_allowed&&catalog_o.squad_leave.last.survivor.final_mark.owner_written&&catalog_o.squad_leave.last.survivor.final_mark.squad_written);
    assert(f.get<float>(wt.member,0xc04)==81.75f&&f.get<float>(sq.groups[0],0x50)==81.75f);
    f.put(f.actor,0x378,saved_handle);
    // Scripted health lowering through the real life-dispatch entry.
    // Type zero is not a damaging corpse/dying event; the dispatcher
    // legitimately returns without changing health in those states.
    struct SelfDamageFixture {
        F* f;void* parent;
        alignas(4) unsigned char rows[3*12]{},mods[3][12]{};
        bool admitted=true;unsigned admissions{},queued{},centers{},normalizes{},casts{},bones{},deaths{};
        float direction[3]{};std::vector<unsigned> outputs;
        alignas(4) unsigned char death_descriptor[0x48]{};
        std::vector<unsigned> death_order;
        unsigned jet_ticks{};
        int expected_class=11;
        float think_clock{},think_scheduled{};
    } hd{&f,wt.member};
    ServerNpcActivePostAnimationBindings hd_storage{};hd_storage.context=&hd;
    hd_storage.resolve=[](void* p,unsigned token,void** out,unsigned* bytes) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);
        if(token==0xb100u){*out=q.rows;*bytes=sizeof(q.rows);return 1;}
        if(token>=0xb200u&&token<0xb203u){*out=q.mods[token-0xb200u];*bytes=sizeof(q.mods[0]);return 1;}
        return 0;
    };
    static ServerNpcGruntLifeDispatchBindings hd_life{};
    static ServerNpcGruntLifeDispatchResult hd_life_result{};
    auto& hd_identity=hd_life.combat.alive.chain.detail.damage.base.chain.prefix.damage.reaction.sentence;
    assert(!hd_identity.entries&&!hd_identity.speech.original_vtable); // authoritative input binding
    ServerNpcInputHealthDispatchBindings hd_services{};hd_services.context=&hd;
    hd_services.life=&hd_life;hd_services.life_result=&hd_life_result;
    hd_services.parent_admit=[](void* p,void* parent,const void* info,bool* accepted) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(parent==q.parent);
        auto* d=const_cast<void*>(info);assert(q.f->get<float>(d,0x2c)==20.f&&q.f->get<unsigned>(d,0x38)==0);
        *accepted=q.admitted;++q.admissions;return 1;
    };
    hd_services.enqueue=[](void* p,void* actor,const void* info,unsigned bytes) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(actor==q.f->actor&&bytes==0x48);
        auto* d=const_cast<void*>(info);assert(q.f->get<float>(d,0x2c)==20.f&&q.f->get<float>(d,0x30)==20.f);
        ++q.queued;return 1;
    };
    auto hd_b=leaves;hd_b.storage=&hd_storage;hd_b.native_health=&hd_services;hd_b.entities=scripted.input.entities;hd_b.now=90.f;
    for(unsigned i=0;i<3;++i){
        f.put(hd.rows,12*i,0xb200u+i);f.put(hd.rows,12*i+8,i==2?0xffffffffu:i+1);
        f.put(hd.mods[i],0,i==0?2.f:i==1?.5f:3.f);hd.mods[i][8]=i==1;
    }
    f.put(f.actor,0x378,0x8002u);f.put(wt.member,0x378,0x7001u);
    f.put(f.actor,0xc4,0xb100u);f.put(f.actor,0xd0,0u);f.put(f.actor,0xa4,0x7001u);f.put(f.actor,0xe8,0xffffffffu);
    f.put(f.actor,0x98,100);f.put(f.actor,0x9c,100);f.put(f.actor,0x8c,1);f.put(f.actor,0x94,2);
    f.put(f.actor,0x16c,0u);f.actor[0x1f0]=1;f.actor[0xa28]=0;
    auto hd_run=[&](int desired=80,unsigned vt=0x2249aaa4u,unsigned bytes=0x1088u){
        std::memset(inp.variant,0,sizeof(inp.variant));f.put(inp.variant,0,desired);f.put(inp.variant,0x10,5u);
        return accept->run(f.actor,bytes,vt,0x223db434u,f.weapon,f.actor,inp.variant,sizeof(inp.variant),0,&hd_b,&catalog_o);
    };
    hd.admitted=false;
    assert(hd_run()&&catalog_o.health_admission_blocked&&f.actor[0xa28]&&!catalog_o.health_life_called&&!catalog_o.health_multiplier_rows);
    hd.admitted=true;hd_services.defer_damage=true;
    assert(hd_run()&&catalog_o.health_deferred&&hd.queued==1&&!catalog_o.health_life_called&&!catalog_o.health_multiplier_rows);
    hd_services.defer_damage=false;
    assert(hd_run()&&catalog_o.health_life_called&&catalog_o.health_multiplier_rows==6&&catalog_o.health_outgoing_scale==6.f&&catalog_o.health_incoming_scale==.5f&&catalog_o.health_scaled_amount==60.f);
    assert(hd_life_result.completed&&hd_life_result.route==NpcDamageDying&&hd_life_result.scaled_amount==60.f&&hd_life_result.native_result==1&&f.get<int>(f.actor,0x9c)==100);
    f.put(f.actor,0x94,0);f.put(f.actor,0xa4,0xffffffffu);f.put(f.actor,0xd0,0xffffffffu);
    assert(hd_run()&&catalog_o.health_life_called&&!catalog_o.health_multiplier_rows&&catalog_o.health_scaled_amount==20.f&&hd_life_result.mode_blocked);
    f.put(f.actor,0xd0,0u);f.put(hd.rows,32,0u);
    assert(!hd_run()&&!catalog_o.completed&&!catalog_o.health_life_called);
    assert(hd.admissions==3&&hd.queued==1);
    // A live NPC now traverses native damage override, fractional health,
    // damage outputs and the native death dispatch decision. Engine-owned
    // bone/center queries and the class-specific death service remain explicit.
    auto& alive_hd=hd_life.combat.alive;
    alive_hd.life.context=&hd;alive_hd.life.shared_direction=hd.direction;
    alive_hd.life.center=[](void* p,void* entity,float* result) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(entity==q.f->actor);
        ++q.centers;result[0]=result[1]=result[2]=0.f;return 1;
    };
    alive_hd.life.normalize=[](void* p,float* result) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(result[0]==0.f&&result[1]==0.f&&result[2]==-10.f);
        ++q.normalizes;result[2]=-1.f;return 1;
    };
    auto& detail_hd=alive_hd.chain.detail;
    detail_hd.damage.context=&hd;
    detail_hd.damage.cast_grunt=[](void* p,void* entity,void** cast,unsigned* extent) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(entity==q.f->actor);++q.casts;*cast=entity;*extent=sizeof(q.f->actor);return 1;
    };
    detail_hd.bone.context=&hd;
    detail_hd.bone.index=[](void* p,void* entity,const char* name,int* index) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(entity==q.f->actor);
        assert(!std::strcmp(name,q.bones%2?"ValveBiped.Bip01_L_Calf":"ValveBiped.Bip01_R_Calf"));
        ++q.bones;*index=-1;return 1;
    };
    auto& output_hd=detail_hd.damage.base.chain.outputs;output_hd.context=&hd;output_hd.max_clients=2;
    output_hd.output=[](void* p,void* entity,unsigned off) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(entity==q.f->actor);q.outputs.push_back(off);return 1;
    };
    output_hd.is_player=[](void* p,void* entity,bool* yes) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(entity==q.f->actor);*yes=false;return 1;
    };
    output_hd.cast_npc=[](void* p,void* entity,void** cast,unsigned* extent) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(entity==q.f->actor);*cast=entity;*extent=sizeof(q.f->actor);return 1;
    };
    hd_life.context=&hd;
    hd_life.death=[](void* p,void* entity,void* record,unsigned vt) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(entity==q.f->actor&&vt==0x2249aaa4u);
        assert(q.f->get<int>(entity,0x9c)==-80&&q.f->get<float>(record,0x2c)==120.f&&q.f->get<float>(record,0x30)==40.f);
        assert(!q.f->get<unsigned>(record,0x38)&&q.f->get<unsigned>(record,0x28)==0x8002u);
        ++q.deaths;return 1;
    };
    f.put(hd.rows,32,0xffffffffu);f.put(f.actor,0xd0,0u);f.put(f.actor,0x94,2);f.put(f.actor,0x8c,0);
    f.put(f.actor,0x9c,100);f.put(f.actor,0x98,100);f.put(f.actor,0x634,0.f);
    f.put(f.actor,0xcac,0.f);f.put(f.actor,0xc18,0u);f.put(f.actor,0x874,0u);f.put(f.actor,0x868,0);
    f.put(f.actor,0x9c4,0u);f.put(f.actor,0xe8,0xffffffffu);f.put(f.actor,0x218,0u);
    f.put(f.actor,0xbc,0x10000000u);f.actor[0xb6]=0;f.actor[0xdb1]=0;
    f.put(f.actor,0x994,89.f);
    assert(hd_run()&&catalog_o.health_life_called&&hd_life_result.route==NpcDamageAlive&&hd_life_result.native_result==1);
    assert(hd_life_result.alive.alive.life.health_before==100&&hd_life_result.alive.alive.life.health_after==40&&f.get<int>(f.actor,0x9c)==40);
    assert(!hd_life_result.death_called&&hd.centers==2&&hd.normalizes==1&&hd.casts==1&&hd.bones==2&&hd.direction[2]==-1.f);
    assert((hd.outputs==std::vector<unsigned>{0xa48u,0xa78u}));
    hd.outputs.clear();
    assert(hd_run(0)&&catalog_o.health_scaled_amount==120.f&&hd_life_result.death_called&&hd_life_result.post_death_noop&&hd.deaths==1);
    assert(f.get<int>(f.actor,0x9c)==-80&&hd_life_result.alive.alive.life.health_after==-80&&hd.centers==4&&hd.normalizes==2&&hd.bones==4);
    assert((hd.outputs==std::vector<unsigned>{0xa48u,0xa78u}));
    // Scripted health damage -> native output action lists -> delayed
    // named queue -> actual native inputs on a second Grunt.
    struct HealthQueueFixture {
        F* f;void* recipient;
        const ServerEntityAcceptInputApi* accept;
        const ServerEntitySearchApi* search;
        const ServerEntitySearchBindings* lookup;
        ServerEntityAcceptInputBindings input;
        alignas(4) unsigned char root[0x38]{},actions[2][0x1c]{},events[2][0x38]{};
        unsigned allocated{},released_actions{},released_events{},delivered{},locks{};
        std::vector<unsigned> ids;
    } hq{&f,wt.member,accept,entity_search,&search_bind,leaves};
    hq.input.entities=scripted.input.entities;
    auto hq_resolve=[](void* p,unsigned token,void** data,unsigned* bytes) noexcept -> int{
        auto& q=*static_cast<HealthQueueFixture*>(p);
        if(token>=0x7100u&&token<0x7102u){*data=q.actions[token-0x7100u];*bytes=0x1c;return 1;}
        if(token>=0x7200u&&token<0x7200u+q.allocated){*data=q.events[token-0x7200u];*bytes=0x38;return 1;}
        return 0;
    };
    ServerEntityVoidOutputBindings hq_actions{};hq_actions.context=&hq;hq_actions.resolve=hq_resolve;
    hq_actions.release=[](void* p,unsigned token,void* row) noexcept -> int{
        auto& q=*static_cast<HealthQueueFixture*>(p);assert(token==0x7100u&&row==q.actions[0]);++q.released_actions;return 1;
    };
    ServerEntityNamedEventQueueBindings hq_queue{};hq_queue.context=&hq;hq_queue.resolve=hq_resolve;
    hq_queue.allocate=[](void* p,unsigned* token,void** data,unsigned* bytes) noexcept -> int{
        auto& q=*static_cast<HealthQueueFixture*>(p);assert(q.allocated<2);*token=0x7200u+q.allocated;*data=q.events[q.allocated++];*bytes=0x38;return 1;
    };
    hq_queue.string_nonempty=[](void*,unsigned token,bool* nonempty) noexcept -> int{
        assert(token==500u||token==0x22485b74u||token==0x223db33cu);*nonempty=true;return 1;
    };
    ServerNpcHealthOutputQueueBindings hq_bind{&hq_actions,&hq_queue,hq.root,sizeof(hq.root),0x7000u};
    hd_services.outputs=&hq_bind;output_hd.output=nullptr;output_hd.is_player=nullptr;output_hd.cast_npc=nullptr;
    for(unsigned i=0;i<2;++i){
        f.put(hq.actions[i],0,500u);f.put(hq.actions[i],4,i?0x22485b74u:0x223db33cu);
        f.put(hq.actions[i],0xc,i?.125f:.25f);f.put(hq.actions[i],0x10,i?0xffffffffu:1u);f.put(hq.actions[i],0x14,10u+i);
    }
    f.put(f.actor,0xa5c,0x7100u);f.put(f.actor,0xa8c,0x7101u);f.actor[0xa8]=0;
    f.put(f.actor,0x9c,100);f.put(f.actor,0x994,89.f);f.put(f.actor,0xc0,501u);f.put(wt.member,0xc0,500u);
    wt.member[0xc65]=wt.member[0x968]=0;wt.member[0x1f0]=1;wt.member[0x200]=0;f.put(wt.member,0x16c,0u);
    assert(hd_run()&&catalog_o.health_outputs==2&&catalog_o.health_output_actions==2&&catalog_o.health_output_enqueued==2&&catalog_o.health_output_removed==1);
    assert(f.get<int>(f.actor,0x9c)==40&&hq.allocated==2&&hq.released_actions==1&&!f.get<unsigned>(f.actor,0xa5c)&&f.get<unsigned>(f.actor,0xa8c)==0x7101u);
    assert(f.get<unsigned>(hq.root,0x30)==0x7201u&&f.get<unsigned>(hq.events[1],0x30)==0x7200u);
    for(unsigned i=0;i<2;++i){
        assert(f.get<unsigned>(hq.events[i],0xc)==0x8002u&&f.get<unsigned>(hq.events[i],0x10)==0x8002u);
        assert(f.get<unsigned>(hq.events[i],0x14)==10u+i&&f.get<float>(hq.events[i],0)==90.f+(i?.125f:.25f));
    }
    ServerEntityEventServiceBindings hq_service{};hq_service.context=&hq;hq_service.entities=scripted.input.entities;hq_service.resolve=hq_resolve;
    hq_service.lock=[](void* p,bool enter) noexcept -> int{
        auto& q=*static_cast<HealthQueueFixture*>(p);if(enter){assert(!q.locks);++q.locks;}else{assert(q.locks==1);--q.locks;}return 1;
    };
    hq_service.release=[](void* p,unsigned token,void* row) noexcept -> int{
        auto& q=*static_cast<HealthQueueFixture*>(p);assert(token>=0x7200u&&token<0x7202u&&row==q.events[token-0x7200u]);++q.released_events;return 1;
    };
    hq_service.native_search=hq.lookup;hq_service.native_input=&hq.input;
    f.put(wt.member,0,ground_vt);
    assert(service->service(hq.root,sizeof(hq.root),0x7000u,90.f,false,&steps,&hq_service,&sr)&&sr.future_stopped&&!sr.native_inputs);
    assert(service->service(hq.root,sizeof(hq.root),0x7000u,90.125f,false,&steps,&hq_service,&sr)&&sr.events==1&&sr.deliveries==1&&sr.native_inputs==1&&!sr.rejected_inputs&&sr.native_named_searches==2);
    assert(wt.member[0xc65]==1&&!wt.member[0x968]&&wt.member[0x200]);
    assert(service->service(hq.root,sizeof(hq.root),0x7000u,90.25f,false,&steps,&hq_service,&sr)&&sr.events==1&&sr.deliveries==1&&sr.native_inputs==1&&!sr.rejected_inputs&&sr.native_named_searches==2);
    assert(wt.member[0xc65]&&wt.member[0x968]&&hq.released_events==2&&!hq.locks&&!f.get<unsigned>(hq.root,0x30));
    // Native name misses fall back to classname, and rejected input
    // return values still consume the event (retail ignores the virtual result).
    std::memset(hq.root,0,sizeof(hq.root));hq.allocated=0;hq.released_events=0;
    f.put(f.actor,0xc,503u);f.put(wt.member,0xc,503u);
    f.put(f.actor,0,ground_vt);f.put(wt.member,0,ground_vt);
    hq_queue.string_nonempty=[](void*,unsigned,bool* yes) noexcept -> int{*yes=true;return 1;};
    hq.input.diagnostic=nullptr;
    // Unknown input must walk the inherited map, not unrelated dirty storage.
    hq.input.storage=&catalog_storage;
    const unsigned rejected_parent_reads=catalog_f.fallback;
    ServerEntityVoidOutputAction rejected_action{503u,0xfeedu,0u,0u,66u,0.f};
    ServerEntityNamedEventQueueResult rejected_queue{};
    assert(outf.queue_api->enqueue(hq.root,sizeof(hq.root),0x7000u,93.f,0x8002u,0x8002u,&rejected_action,&hq_queue,&rejected_queue));
    assert(service->service(hq.root,sizeof(hq.root),0x7000u,93.f,false,&steps,&hq_service,&sr)&&sr.events==1&&sr.classname==2&&sr.native_inputs==2&&sr.rejected_inputs==2&&sr.native_named_searches==1&&sr.native_class_searches==3&&!sr.missing&&hq.released_events==1);
    assert(catalog_f.fallback==rejected_parent_reads+2);
    hq.input.storage=leaves.storage;
    // Direct target uses the same native route without any name lookup.
    ServerEntityVoidOutputAction direct_action{0u,0x223db318u,0u,0u,67u,0.f};
    assert(outf.queue_api->enqueue(hq.root,sizeof(hq.root),0x7000u,94.f,0x8002u,0x8002u,&direct_action,&hq_queue,&rejected_queue));
    f.put(hq.events[1],0x18,0x7001u);
    assert(service->service(hq.root,sizeof(hq.root),0x7000u,94.f,false,&steps,&hq_service,&sr)&&sr.direct==1&&sr.native_inputs==1&&!sr.rejected_inputs&&!sr.native_named_searches&&!sr.native_class_searches&&!wt.member[0x968]);
    f.put(f.actor,0xa8c,0u);
    // The actual ground death composition consumes the same scripted
    // record, writes life state and executes removal/condition logic.
    // Model selection, loot policy and engine base death remain explicit.
    static ServerNpcGruntGroundDeathBindings hd_ground{};
    static ServerNpcGruntGroundLifeResult hd_ground_result{};
    hd_ground.context=&hd;
    hd_ground.loot_enabled=[](void* p,bool* enabled) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(q.f->get<int>(q.f->actor,0xc68)==q.expected_class);q.death_order.push_back(1);*enabled=false;return 1;
    };
    auto& script_death_binding=hd_ground.base.combat.death;
    script_death_binding.prepare.context=&hd;
    script_death_binding.prepare.model_available=[](void* p,void* a,bool* yes) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(a==q.f->actor);q.death_order.push_back(2);*yes=false;return 1;
    };
    script_death_binding.event.context=&hd;
    auto& script_death_prefix=script_death_binding.event.prefix;
    script_death_prefix.schedules.context=&hd;
    script_death_prefix.schedules.descriptor=[](void* p,void* a,const void** desc,unsigned* bytes) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(a==q.f->actor);*desc=q.death_descriptor;*bytes=sizeof(q.death_descriptor);return 1;
    };
    f.put(hd.death_descriptor,0x34,npc_global_condition_base);f.put(hd.death_descriptor,0x38,0);f.put(hd.death_descriptor,0x3c,100);
    script_death_prefix.removal.removal.context=&hd;
    script_death_prefix.removal.removal.event=[](void* p,void* a,void* attacker) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(a==q.f->actor&&attacker==a&&q.f->get<int>(a,0x8c)==1);q.death_order.push_back(3);return 1;
    };
    script_death_prefix.removal.removal.finalize=[](void* p,void* a) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(a==q.f->actor&&q.f->get<std::uint8_t>(a,0xa44)==1);q.death_order.push_back(4);return 1;
    };
    script_death_binding.event.base_death=[](void* p,void* a,void* record) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(a==q.f->actor&&q.f->get<int>(a,0x9c)==-200&&q.f->get<float>(record,0x2c)==300.f&&q.f->get<float>(record,0x30)==100.f);
        q.death_order.push_back(5);return 1;
    };
    script_death_binding.event.activity_lookup=[](void* p,void* a,int activity,int* result) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(a==q.f->actor&&activity==0x18);q.death_order.push_back(6);*result=42;return 1;
    };
    hd_services.ground_death=&hd_ground;hd_services.ground_result=&hd_ground_result;hd_life.death=nullptr;
    f.put(f.actor,0x9c,100);f.put(f.actor,0x8c,0);f.put(f.actor,0x94,2);f.put(f.actor,0x994,89.f);
    f.put(f.actor,0x798,0u);f.put(f.actor,0x850,1);f.put(f.actor,0x858,1);f.put(f.actor,0x868,0);
    f.put(f.actor,0xa18,0xffffffffu);f.put(f.actor,0x970,0xffffffffu);f.put(f.actor,0x9c4,0u);
    f.put(f.actor,0xc18,0u);f.put(f.actor,0xc28,0);f.put(f.actor,0x7cc,0u);
    f.actor[0xa44]=0;f.actor[0xcb1]=0;f.actor[0x200]=0;
    alignas(4) unsigned char live_death_seed[sizeof(f.actor)];std::memcpy(live_death_seed,f.actor,sizeof(f.actor));
    assert(hd_run(0)&&catalog_o.health_ground_route&&catalog_o.health_ground_death&&catalog_o.health_life_called&&hd_ground_result.ground_called&&hd_ground_result.ground.completed);
    auto& death_event=hd_ground_result.ground.base.combat.death.event;
    assert(hd_ground_result.ground.attacker_class==11&&death_event.completed&&death_event.prefix.life_changed&&death_event.prefix.removal.removal.finalized&&death_event.state_written&&death_event.ideal_written);
    assert(f.get<int>(f.actor,0x8c)==1&&f.get<int>(f.actor,0x850)==7&&f.get<int>(f.actor,0x858)==7&&f.get<float>(f.actor,0x854)==90.f);
    assert(f.actor[0xa44]&&f.actor[0x200]&&(f.get<unsigned>(f.actor,0x7cc)&(1u<<16)));
    assert((hd.death_order==std::vector<unsigned>{1,2,3,4,5,6})&&hd.deaths==1&&hd_life_result.death_called);
    // All five subclass tables select the existing class-aware life entry.
    // Mode zero deliberately stops before lethal class-specific effects;
    // those bodies have their own loaded Phase 1228/1225 regressions.
    static ServerNpcGruntEliteLifeBindings hd_subclass;
    static ServerNpcGruntEliteLifeResult hd_subclass_result;
    hd_subclass.combat.combat.ground=hd_ground;
    hd_services.subclass_death=&hd_subclass;hd_services.subclass_result=&hd_subclass_result;
    auto& sub_life=hd_subclass_result.combat.combat.life;
    f.put(f.actor,0x94,0);f.put(f.actor,0x9c,100);
    for(unsigned vt:{0x2248df04u,0x2248c514u,0x2248e83cu,0x2249964cu,0x224a4a4cu}){
        assert(hd_run(80,vt)&&catalog_o.health_life_called&&!catalog_o.health_ground_route&&!catalog_o.health_subclass_death);
        assert(catalog_o.health_elite_route==(vt==0x2248df04u)&&catalog_o.health_jet_route==(vt!=0x2248df04u));
        assert(sub_life.completed&&sub_life.mode_blocked&&hd_life_result.mode_blocked&&f.get<int>(f.actor,0x9c)==100);
    }
    hd_services.subclass_result=nullptr;
    assert(!hd_run(80,0x2248df04u)&&!catalog_o.health_life_called);
    hd_services.subclass_result=&hd_subclass_result;
    assert(!hd_run(80,0x2248df04u,0xdb2u)&&!catalog_o.health_life_called);
    assert(!hd_run(80,0x2248c514u,0xfe0u)&&!catalog_o.health_life_called);
    hd_services.life_result=nullptr;
    assert(hd_run(80,0x2248df04u)&&sub_life.mode_blocked&&catalog_o.health_elite_route);
    hd_services.life_result=&sub_life; // nested result alias is supported
    assert(hd_run(80,0x2248c514u)&&sub_life.mode_blocked&&catalog_o.health_jet_route);
    hd_services.life_result=&hd_life_result;
    // Lethal Elite input uses the already reconstructed effect wrapper,
    // then shared death and the gated post helper, including the original
    // constant +d0 self classification without a host callback.
    auto reset_subclass=[&](unsigned vt){
        std::memcpy(f.actor,live_death_seed,sizeof(f.actor));f.put(f.actor,0,vt);
        f.put(f.actor,0xfdc,0xffffffffu);f.put(f.actor,0xfe4,0xffffffffu);f.put(f.actor,0xfe8,0xffffffffu);
        f.put(f.actor,0x1028,0xffffffffu);f.put(f.actor,0x1008,0u);f.actor[0xc44]=1;
        f.put(f.actor,0xb8,f.get<unsigned>(f.actor,0xb8)&~0x800u);
        hd.expected_class=(vt==0x2248df04u||vt==0x2248c514u||vt==0x2248e83cu||vt==0x2249aaa4u)?11:6;
        hd.death_order.clear();hd.jet_ticks=0;
    };
    auto& sub_ground=hd_subclass.combat.combat.ground;
    assert(!sub_ground.classification);
    auto& elite=hd_subclass.elite;elite.context=&hd;
    elite.effect_begin=[](void* p,void* a) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(a==q.f->actor&&q.f->get<int>(a,0xc38)==0&&q.f->get<int>(a,0x8c)==0);
        q.death_order.push_back(10);return 1;
    };
    elite.effects=[](void* p,void* a) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(a==q.f->actor);q.death_order.push_back(11);return 1;
    };
    elite.rule=[](void* p,bool* yes) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(q.f->get<int>(q.f->actor,0x850)==7);q.death_order.push_back(12);*yes=false;return 1;
    };
    elite.effect_end=[](void* p,void* a) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(a==q.f->actor);q.death_order.push_back(13);return 1;
    };
    reset_subclass(0x2248df04u);f.put(f.actor,0xc38,4);
    assert(hd_run(0,0x2248df04u)&&catalog_o.health_elite_route&&catalog_o.health_subclass_death&&sub_life.death_called);
    const auto& elite_result=hd_subclass_result.elite;
    assert(elite_result.completed&&elite_result.prior_grenades==4&&elite_result.grenades_cleared&&elite_result.effect_opened&&elite_result.effects_called&&elite_result.base_called&&elite_result.rule_checked&&!elite_result.rule_allowed&&elite_result.effect_closed);
    assert((hd.death_order==std::vector<unsigned>{10,11,1,2,3,4,5,6,12,13})&&hd_subclass_result.combat.combat.ground.attacker_class==11);
    assert(f.get<int>(f.actor,0x9c)==-200&&f.get<int>(f.actor,0x8c)==1&&f.get<int>(f.actor,0x850)==7&&hd.deaths==1);
    // Lethal Jetpack mode 4 enters the delayed route without calling
    // shared terminal death; the original damage copy is retained at +102c.
    auto& jet_mode=hd_subclass.combat.combat.jet;jet_mode.context=&hd;
    jet_mode.random_int=[](void* p,int lo,int hi,int* v) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(lo==0&&hi==100);q.death_order.push_back(20);*v=30;return 1;
    };
    jet_mode.effect_rule=[](void* p,void* a,bool* yes) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(a==q.f->actor&&q.f->get<int>(a,0x1004)==4);q.death_order.push_back(21);*yes=false;return 1;
    };
    jet_mode.force_mode3=[](void* p,bool* yes) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);q.death_order.push_back(22);*yes=false;return 1;
    };
    auto& jet_branch=hd_subclass.combat.branch;jet_branch.context=&hd;
    jet_branch.powerdown=[](void* p,void* a,const char* cue) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(a==q.f->actor&&!std::strcmp(cue,"NPC_Jetpack.Powerdown"));q.death_order.push_back(23);return 1;
    };
    jet_branch.ignite=[](void* p,void* a,float time,int flags,void* x,void* y,float radius,int last) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(a==q.f->actor&&time==1.f&&flags==1&&!x&&!y&&radius==25.f&&last==1);q.death_order.push_back(24);return 1;
    };
    jet_branch.ragdoll=[](void* p,void* a,void* damage,const float* force) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(a==q.f->actor&&q.f->get<int>(a,0x8c)==1&&q.f->get<float>(damage,0x2c)==300.f&&!force[0]&&!force[1]&&!force[2]);q.f->put(a,0x1028,0x7001u);q.death_order.push_back(25);return 1;
    };
    jet_branch.time=[](void* p,float* now) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);q.death_order.push_back(26);*now=90.f+float(q.jet_ticks++)*.125f;return 1;
    };
    jet_branch.install_think=[](void* p,void* a,unsigned token) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(a==q.f->actor&&token==0x22365760u&&q.f->get<float>(a,0xffc)==90.f);q.death_order.push_back(27);return 1;
    };
    jet_branch.schedule=[](void* p,void* a,float now) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(a==q.f->actor&&now==90.125f);q.death_order.push_back(28);return 1;
    };
    // Force the original immediate branch on all Jetpack tables; the
    // same SetHealth attacker must classify as 11,11,6,6 respectively.
    const auto delayed_mode=jet_mode.force_mode3;
    jet_mode.force_mode3=[](void* p,bool* yes) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);q.death_order.push_back(22);*yes=true;return 1;
    };
    jet_branch.remove=[](void* p,void* entity) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(!entity);q.death_order.push_back(37);return 1;
    };
    jet_branch.effect_begin=[](void* p,void* a) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(a==q.f->actor);q.death_order.push_back(38);return 1;
    };
    jet_branch.immediate_effect=[](void* p,void* a) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(a==q.f->actor);q.death_order.push_back(39);return 1;
    };
    jet_branch.effect_end=[](void* p,void* a) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(a==q.f->actor);q.death_order.push_back(40);return 1;
    };
    auto& sub_prefix=sub_ground.base.combat.death.event.prefix;sub_prefix.context=&hd;
    sub_prefix.jet_release=[](void* p,unsigned token) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(!token);q.death_order.push_back(34);return 1;
    };
    for(unsigned vt:{0x2248c514u,0x2248e83cu,0x2249964cu,0x224a4a4cu}){
        reset_subclass(vt);f.actor[0xbf]&=static_cast<unsigned char>(~8u);
        assert(hd_run(0,vt)&&catalog_o.health_jet_route&&catalog_o.health_subclass_death);
        const auto& mode=hd_subclass_result.combat.combat.jet;
        const auto& branch=hd_subclass_result.combat.branch;
        const auto& death=hd_subclass_result.combat.combat.ground;
        assert(mode.forced&&mode.final_mode==3&&branch.completed&&branch.base_called&&branch.effect_ended);
        assert(death.completed&&death.attacker_class==hd.expected_class&&f.get<int>(f.actor,0xc68)==hd.expected_class);
        assert(f.actor[0xa44]&&f.get<int>(f.actor,0x850)==7&&f.get<int>(f.actor,0x9c)==-200);
        assert((hd.death_order==std::vector<unsigned>{20,21,22,37,38,39,1,2,3,4,34,5,6,40}));
    }
    jet_mode.force_mode3=delayed_mode;
    for(unsigned vt:{0x2248c514u,0x2248e83cu,0x2249964cu,0x224a4a4cu}){
        reset_subclass(vt);f.actor[0xbf]&=static_cast<unsigned char>(~8u);
        assert(hd_run(0,vt)&&catalog_o.health_jet_route&&catalog_o.health_subclass_death&&sub_life.death_called);
        const auto& mode=hd_subclass_result.combat.combat.jet;const auto& branch=hd_subclass_result.combat.branch;
        assert(mode.completed&&mode.copied&&mode.sample==30&&mode.final_mode==4&&mode.tail_called);
        assert(branch.completed&&branch.mode==4&&branch.cleanup&&branch.life_changed&&branch.ragdoll&&branch.installed&&branch.scheduled_called&&!branch.base_called);
        assert(!hd_subclass_result.combat.combat.ground_called&&f.get<int>(f.actor,0x9c)==-200&&f.get<int>(f.actor,0x8c)==1&&!f.actor[0xa44]);
        assert(f.get<float>(f.actor,0x1058)==300.f&&f.get<float>(f.actor,0x105c)==100.f&&f.get<unsigned>(f.actor,0x1054)==0x8002u);
        assert((hd.death_order==std::vector<unsigned>{20,21,22,23,24,25,26,27,26,28})&&hd.jet_ticks==2);
    }
    // Continue the same delayed death from the scripted input, rather
    // than inventing a second synthetic damage record for DeathThink.
    auto get_think=reinterpret_cast<GetServerNpcJetpackDeathThinkApi>(GetProcAddress(server,server_npc_jetpack_death_think_export));
    assert(get_think&&!GetProcAddress(client,server_npc_jetpack_death_think_export));
    const auto* death_think=get_think(1,sizeof(ServerNpcJetpackDeathThinkApi));assert(death_think);
    static ServerNpcJetpackDeathThinkBindings think{};
    static ServerNpcJetpackDeathThinkResult think_result{};
    think.ground=sub_ground;think.context=&hd;
    // Standalone DeathThink still accepts an arbitrary attacker; the
    // script-specific classification is not assumed by that older entry.
    think.ground.classification=[](void* p,void* e,int* v) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(e==q.f->actor);*v=q.expected_class;return 1;
    };
    auto& ts=think.ground.base.combat.death.event.prefix.removal.removal.leave.sentence;
    ts.entries=hd_b.entities.entries;ts.count=hd_b.entities.count;
    ts.speech.original_vtable=0x224a4a4cu;
    think.ground.base.combat.death.event.prefix.context=&hd;
    think.ground.base.combat.death.event.prefix.jet_release=[](void* p,unsigned token) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(!token);q.death_order.push_back(34);return 1;
    };
    think.time=[](void* p,float* time) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);*time=q.think_clock;return 1;
    };
    think.schedule=[](void* p,void* a,float time) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(a==q.f->actor);q.think_scheduled=time;q.death_order.push_back(36);return 1;
    };
    think.effect_begin=[](void* p,void* a) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(a==q.f->actor);q.death_order.push_back(30);return 1;
    };
    think.terminal_effect=[](void* p,void* a,void* subject,bool mode4) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(a==q.f->actor&&subject==q.parent&&mode4);q.death_order.push_back(31);return 1;
    };
    think.remove=[](void* p,void* subject) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(subject==q.parent);q.death_order.push_back(32);return 1;
    };
    think.effect_end=[](void* p,void* a) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(a==q.f->actor);q.death_order.push_back(33);return 1;
    };
    think.install_think=[](void* p,void* a,unsigned token) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(a==q.f->actor&&token==0x221c0d40u&&q.f->get<int>(a,0x1004)==4);q.death_order.push_back(35);return 1;
    };
    f.put(f.actor,0x1008,0u);f.put(f.actor,0x1000,0xffffffffu);
    hd.death_order.clear();hd.think_clock=92.f;
    assert(death_think->run(f.actor,sizeof(f.actor),0x8002u,&think,&think_result)&&think_result.completed&&think_result.elapsed==2.0&&!think_result.expired&&!think_result.death_called);
    assert((hd.death_order==std::vector<unsigned>{36})&&!f.actor[0xa44]&&hd.think_scheduled==92.1f);
    hd.death_order.clear();hd.think_clock=92.125f;
    assert(!death_think->run(f.actor,sizeof(f.actor),0x9002u,&think,&think_result)&&hd.death_order.empty());
    assert(death_think->run(f.actor,sizeof(f.actor),0x8002u,&think,&think_result)&&think_result.completed&&think_result.expired&&think_result.terminal_mode==4&&think_result.death_called&&think_result.ground.completed);
    assert(think_result.primary_removed&&think_result.effect_closed&&think_result.installed&&think_result.mode_written&&think_result.scheduled_called);
    assert(f.get<unsigned>(f.actor,0x1028)==0xffffffffu&&f.get<int>(f.actor,0x1004)==3&&f.actor[0xa44]);
    assert(f.get<int>(f.actor,0x9c)==-200&&f.get<int>(f.actor,0x850)==7&&f.get<float>(f.actor,0x854)==92.125f);
    assert((hd.death_order==std::vector<unsigned>{30,31,32,33,35,1,2,3,4,34,5,6,36}));
    assert(hd.think_scheduled==static_cast<float>(double(92.125f)+double(.1f))&&hd.deaths==1);
    // Exercise the recovered primary context scheduler end-to-end:
    // install an identity token, write due tick, dispatch death and finally
    // dispatch its replacement removal token. No raw PE call or manual
    // invocation of DeathThink from this portion of the regression.
    auto get_primary=reinterpret_cast<GetServerNpcJetpackPrimaryThinkApi>(GetProcAddress(server,server_npc_jetpack_primary_think_export));
    assert(get_primary&&!GetProcAddress(client,server_npc_jetpack_primary_think_export)&&!get_primary(0,sizeof(ServerNpcJetpackPrimaryThinkApi))&&!get_primary(1,0));
    const auto* primary=get_primary(1,sizeof(ServerNpcJetpackPrimaryThinkApi));assert(primary);
    ServerNpcJetpackPrimaryThinkBindings primary_b{};
    ServerNpcJetpackPrimaryThinkResult primary_o{};
    primary_b.entities=hd_b.entities;primary_b.context=&hd;primary_b.clock=think.time;
    primary_b.cadence.storage=&hd_storage;primary_b.cadence.context=&hd;
    primary_b.cadence.notify_think_flags=[](void* p,void* a) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(a==q.f->actor);
        q.death_order.push_back(q.f->get<unsigned>(a,0xb8)&0x400000u?41:42);return 1;
    };
    primary_b.record=[](void* p,void* a,int tick,float time,int scheduled) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(a==q.f->actor&&tick>=scheduled&&scheduled==q.f->get<int>(a,0x74));
        const float expected=scheduled*.125f;assert(time==(expected>q.think_clock?q.think_clock:expected));
        q.death_order.push_back(43);return 1;
    };
    primary_b.scope=[](void* p,void* a,bool begin) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(a==q.f->actor);
        if(begin)assert(q.f->get<int>(a,0x74)==-1);
        q.death_order.push_back(begin?44:45);return 1;
    };
    primary_b.remove=[](void* p,void* a) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(a==q.f->actor&&q.f->get<int>(a,0x9c)<=0);
        q.f->put(a,0xb8,q.f->get<unsigned>(a,0xb8)|1u);q.death_order.push_back(46);return 1;
    };
    primary_b.positive_health_warning=[](void* p,void* a) noexcept -> int{
        auto& q=*static_cast<SelfDamageFixture*>(p);assert(a==q.f->actor&&q.f->get<int>(a,0x9c)==0);q.death_order.push_back(47);return 1;
    };
    // Intentionally omit hosted installation/scheduling and classifier in
    // the bound death-think route; the primary dispatcher supplies them.
    think.ground.classification=nullptr;think.install_think=nullptr;think.schedule=nullptr;
    primary_b.death=&think;primary_b.death_result=&think_result;
    constexpr unsigned primary_vt=0x224a4a4cu;
    // Initial scheduling is now owned by the same native tick fields.
    // Old branch callbacks are deliberately absent; no manual install or
    // set_default call bridges SetHealth to the primary dispatcher.
    ServerNpcInputJetThinkScheduleBindings initial_think{&primary_b.cadence,.125f};
    hd_services.jet_think=&initial_think;
    jet_branch.install_think=nullptr;jet_branch.schedule=nullptr;
    reset_subclass(primary_vt);f.actor[0xbf]&=static_cast<unsigned char>(~8u);
    f.put(f.actor,0x1000,0xffffffffu);f.put(f.actor,0x50,0);f.put(f.actor,0xb8,0x400000u);
    f.put(f.actor,0x74,-1);f.put(f.actor,0x34,-1);f.put(f.actor,8,0u);
    initial_think.interval=0.f;
    assert(!hd_run(0,primary_vt)&&f.get<int>(f.actor,0x9c)==100&&hd.death_order.empty()&&!f.get<unsigned>(f.actor,8));
    initial_think.interval=.125f;
    assert(hd_run(0,primary_vt)&&catalog_o.health_jet_route&&catalog_o.health_jet_think_installed==1&&catalog_o.health_jet_think_scheduled==1);
    assert(catalog_o.health_jet_cadence.completed&&catalog_o.health_jet_cadence.tick==721&&catalog_o.health_jet_cadence.flags_changed);
    assert(f.get<unsigned>(f.actor,8)==0x22365760u&&f.get<int>(f.actor,0x74)==721&&!(f.get<unsigned>(f.actor,0xb8)&0x400000u));
    assert((hd.death_order==std::vector<unsigned>{20,21,22,23,24,25,26,26,42})&&hd.jet_ticks==2);
    hd_services.jet_think=nullptr;
    hd.death_order.clear();hd.think_clock=90.f;
    auto primary_run=[&](int tick,unsigned handle=0x8002u){return primary->run(f.actor,sizeof(f.actor),primary_vt,handle,tick,.125f,&primary_b,&primary_o);};
    assert(primary_run(720)&&primary_o.not_due&&primary_o.native_result==1&&hd.death_order.empty());
    assert(!primary_run(721,0x9002u)&&hd.death_order.empty());
    f.put(f.actor,0xb8,0x400000u);assert(primary_run(721)&&primary_o.no_think&&hd.death_order.empty());f.put(f.actor,0xb8,0u);
    f.put(f.actor,8,0xdeadbeefu);assert(!primary_run(721)&&f.get<int>(f.actor,0x74)==721&&hd.death_order.empty());
    assert(primary->install(f.actor,sizeof(f.actor),primary_vt,0x22365760u));
    hd.think_clock=92.f;
    assert(primary_run(736)&&primary_o.completed&&primary_o.death_called&&primary_o.rescheduled&&!primary_o.installed&&!think_result.expired);
    assert(primary_o.due_tick==721&&primary_o.last_tick==736&&f.get<int>(f.actor,0x34)==736&&f.get<int>(f.actor,0x74)==737&&primary_o.native_result==1);
    assert((hd.death_order==std::vector<unsigned>{43,41,44,42,45}));
    hd.death_order.clear();hd.think_clock=92.125f;
    assert(primary_run(737)&&primary_o.death_called&&primary_o.installed&&primary_o.rescheduled&&think_result.expired&&think_result.death_called);
    assert(f.get<unsigned>(f.actor,8)==0x221c0d40u&&f.get<int>(f.actor,0x74)==738&&f.get<int>(f.actor,0x34)==737&&think_result.ground.attacker_class==6);
    assert((hd.death_order==std::vector<unsigned>{43,41,44,30,31,32,33,1,2,3,4,34,5,6,42,45}));
    hd.death_order.clear();hd.think_clock=92.25f;
    assert(primary_run(738)&&primary_o.completed&&primary_o.removed&&!primary_o.native_result&&!primary_o.health_zeroed);
    assert(f.get<int>(f.actor,0x74)==-1&&f.get<int>(f.actor,0x34)==738&&(f.get<unsigned>(f.actor,0xb8)&0x400001u)==0x400001u);
    assert((hd.death_order==std::vector<unsigned>{43,41,44,46,45}));
    // The same removal leaf zeroes unexpected positive health before the warning.
    f.put(f.actor,0xb8,0u);f.put(f.actor,0x74,739);f.put(f.actor,0x9c,5);
    hd.death_order.clear();hd.think_clock=92.375f;
    assert(primary_run(739)&&primary_o.removed&&primary_o.health_zeroed&&!primary_o.native_result);
    assert((hd.death_order==std::vector<unsigned>{43,41,44,47,46,45}));
    std::memcpy(f.actor,live_death_seed,sizeof(f.actor));
    assert(!hd_subclass.combat.combat.life.combat.alive.chain.detail.damage.base.chain.prefix.damage.reaction.sentence.entries);
    assert(!hd_identity.entries&&!hd_identity.speech.original_vtable);
    f.put(f.actor,0x378,saved_handle);f.put(f.actor,0xd0,0xffffffffu);
    // Stale generation fails before advancement.
    seed_frame();assert(dict->clear(&events));
    assert(!grunt->run(f.actor,sizeof(f.actor),&in,&gb,&gout)&&f.trace.empty());
    FreeLibrary(server);FreeLibrary(client);std::puts("PASS native NPC request through weapon reset and active synchronized animation tick");
}
