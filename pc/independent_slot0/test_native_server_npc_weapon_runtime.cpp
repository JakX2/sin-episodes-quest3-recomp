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
    // The CAI_BaseNPC row count/index and tokens are original; other entries
    // are zeroed in this deliberately sparse single-input fixture.
    struct ScriptWakeFixture {
        F* f;OutputFixture* output;void* target;
        const ServerEntityAcceptInputApi* accept;
        const ServerEntityEventServiceBindings* previous_service;
        alignas(4) unsigned char grunt_map[0x10]{},base_map[0x10]{},grunt_rows[39*0x34]{},base_rows[139*0x34]{};
        ServerNpcActivePostAnimationBindings storage{};
        ServerEntityAcceptInputBindings input{};
        ServerEntityAcceptInputResult input_result{};
        ServerNpcWakeTransitionBindings wake{};
        ServerNpcWakeTransitionResult wake_result{};
    };
    static ScriptWakeFixture scripted{};scripted.f=&f;scripted.output=&outf;scripted.target=wt.member;scripted.accept=accept;scripted.previous_service=&service_bind;
    f.put(scripted.grunt_map,0,0x22534e8cu);f.put(scripted.grunt_map,4,39);f.put(scripted.grunt_map,0xc,0x224dbd9cu);
    f.put(scripted.base_map,0,0x224dc27cu);f.put(scripted.base_map,4,139);
    auto* wake_row=scripted.base_rows+126*0x34;
    f.put(wake_row,0,0u);f.put(wake_row,4,0x223db3ccu);f.put(wake_row,0x12,std::uint16_t{8});f.put(wake_row,0x14,0x223db3c4u);f.put(wake_row,0x1c,0x220320b0u);
    scripted.storage.context=&scripted;
    scripted.storage.resolve=[](void* p,unsigned token,void** out,unsigned* n) noexcept -> int{
        auto& q=*static_cast<ScriptWakeFixture*>(p);
        switch(token){
        case 0x22534da4u:*out=q.grunt_map;*n=sizeof(q.grunt_map);return 1;
        case 0x224dbd9cu:*out=q.base_map;*n=sizeof(q.base_map);return 1;
        case 0x22534e8cu:*out=q.grunt_rows;*n=sizeof(q.grunt_rows);return 1;
        case 0x224dc27cu:*out=q.base_rows;*n=sizeof(q.base_rows);return 1;
        default:return 0;
        }
    };
    sin_independent::EntityHandleEntry script_slots[3]{};
    script_slots[1].serial=7;script_slots[1].entity=reinterpret_cast<CBaseEntity*>(wt.member);
    script_slots[2].serial=8;script_slots[2].entity=reinterpret_cast<CBaseEntity*>(f.actor);
    scripted.input.storage=&scripted.storage;scripted.input.context=&scripted;
    scripted.input.entities.entries=script_slots;scripted.input.entities.count=3;
    scripted.input.name_equal=inp.bindings.name_equal;
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
    script_service.find_named=[](void* p,void* previous,unsigned name,void* activator,void* caller,void** entity) noexcept -> int{
        auto& q=*static_cast<ScriptWakeFixture*>(p);assert(name==500u&&activator==q.f->actor&&caller==q.f->actor);
        *entity=previous?nullptr:q.target;return 1;
    };
    script_service.find_class=nullptr;script_service.missing=nullptr;
    script_service.accept=[](void* p,void* target,unsigned input,void* activator,void* caller,const void* variant,unsigned id) noexcept -> int{
        auto& q=*static_cast<ScriptWakeFixture*>(p);assert(target==q.target&&input==0x223db3c4u&&id==10u);
        return q.accept->run(target,0xfd8u,q.f->get<unsigned>(target,0),input,activator,caller,variant,0x14,id,&q.input,&q.input_result);
    };
    reset_queue();seed_outputs();f.put(outf.rows[0],4,0x223db3c4u);f.put(outf.rows[0],0x18,0u);
    f.put(f.actor,0x378,0x8002u);f.put(f.actor,0x868,2);f.put(f.actor,0x78,0x20u);
    f.put(wt.member,0,ground_vt);f.put(wt.member,0x868,2);f.put(wt.member,0x78,0x20u);
    f.put(wt.member,0xbdc,0u);f.put(wt.member,0x16c,0u);wt.member[0x870]=0;wt.member[0x1f0]=1;wt.member[0x200]=0;
    assert(transition_wake->run(f.actor,sizeof(f.actor),ground_vt,true,&connected,&wto)&&wto.completed&&outf.result.queued==1&&outf.queue_allocated==1);
    assert(f.get<int>(f.actor,0x868)==0&&f.get<int>(wt.member,0x868)==2);
    assert(service->service(outf.queue_root,sizeof(outf.queue_root),root_token,20.f,false,&steps,&script_service,&sr)&&sr.future_stopped&&!sr.events);
    assert(service->service(outf.queue_root,sizeof(outf.queue_root),root_token,20.25f,false,&steps,&script_service,&sr)&&sr.completed&&sr.events==1&&sr.deliveries==1&&sr.removed==1);
    assert(scripted.input_result.accepted&&scripted.input_result.native_wake&&scripted.input_result.map==0x224dbd9cu&&scripted.input_result.row==126&&scripted.input_result.handler==0x220320b0u);
    assert(scripted.wake_result.completed&&scripted.wake_result.transitions==1&&scripted.wake_result.outputs==1&&scripted.wake_result.flags_changed==1);
    assert(f.get<int>(wt.member,0x868)==0&&f.get<unsigned>(wt.member,0x78)==0&&wt.member[0x200]&&!outf.service_locked);check_queue({});
    f.put(f.actor,0x378,saved_handle);f.put(f.actor,0xbdc,0u);
    // Stale generation fails before advancement.
    seed_frame();assert(dict->clear(&events));
    assert(!grunt->run(f.actor,sizeof(f.actor),&in,&gb,&gout)&&f.trace.empty());
    FreeLibrary(server);FreeLibrary(client);std::puts("PASS native NPC request through weapon reset and active synchronized animation tick");
}
