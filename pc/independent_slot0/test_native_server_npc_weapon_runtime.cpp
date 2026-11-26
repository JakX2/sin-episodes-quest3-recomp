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
    struct PostFixture {F* f;alignas(4) unsigned char nav[0x28]{},path[0x48]{};unsigned dirty{};} pf{&f};
    f.put(pf.nav,0x24,0x2222u);f.put(pf.path,0x44,2u);
    ServerNpcActivePostAnimationBindings pb{};pb.context=&pf;
    pb.resolve=[](void* p,std::uint32_t token,void** data,unsigned* n) noexcept -> int {
        auto& q=*static_cast<PostFixture*>(p);
        if(token==0x1111u){*data=q.nav;*n=sizeof(q.nav);return 1;}
        if(token==0x2222u){*data=q.path;*n=sizeof(q.path);return 1;}
        if(token==0x3333u){*data=&q.dirty;*n=4;return 1;}return 0;
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
    f.put(f.weapon,0x4ec,0);f.put(f.weapon,0x4f0,-1);f.put(f.weapon,0x580,0.f);
    seed_frame();assert(outer_api->jet(f.actor,sizeof(f.actor),&in,.05f,&jet_bind,&outer,&jet_out,&po,&oo)&&oo.completed&&oo.impact_called&&oo.active_called&&!oo.think_called&&po.completed&&jet_out.animation.fixed_resets==1);
    assert(f.get<int>(f.weapon,0x4ec)==31&&oo.finish.next_time==static_cast<float>(double(in.now)+.1)&&oo.finish.deadline==20.f);
    assert(f.trace.front()==217&&f.trace[1]==214&&f.trace[2]==215&&f.trace.back()==214);
    // Disabled branch resets navigation timestamp; no animation or pose work.
    f.trace.clear();outer.admission.global_flags=1;warning_deadline=100.f;
    assert(outer_api->jet(f.actor,sizeof(f.actor),&in,.05f,&jet_bind,&outer,&jet_out,&po,&oo)&&oo.completed&&!oo.active_called&&oo.nav_sentinel_written&&oo.admission.pause_called);
    assert(f.get<unsigned>(f.actor,0x910)==0x7f7fffffu&&!jet_out.animation_called&&!po.completed);
    assert((f.trace==std::vector<int>{214,215,211,214}));
    // Sleeping actor performs wake service and skips active scene.
    f.trace.clear();f.put(f.actor,0x868,2);
    assert(outer_api->jet(f.actor,sizeof(f.actor),&in,.05f,&jet_bind,&outer,&jet_out,&po,&oo)&&oo.completed&&oo.wake_called&&oo.sleep_bypassed&&!oo.active_called&&oo.finish.mode==4);
    assert((f.trace==std::vector<int>{214,215,216,214}));
    f.trace.clear();f.put(f.actor,0x868,0);f.put(f.actor,0x85c,0);outer.active_scene=false;
    assert(outer_api->jet(f.actor,sizeof(f.actor),&in,.05f,&jet_bind,&outer,&jet_out,&po,&oo)&&oo.completed&&oo.scene_bypassed&&!oo.active_called);
    assert((f.trace==std::vector<int>{214,215,214}));
    // Stale generation fails before advancement.
    seed_frame();assert(dict->clear(&events));
    assert(!grunt->run(f.actor,sizeof(f.actor),&in,&gb,&gout)&&f.trace.empty());
    FreeLibrary(server);FreeLibrary(client);std::puts("PASS native NPC request through weapon reset and active synchronized animation tick");
}
