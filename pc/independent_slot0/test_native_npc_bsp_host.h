#pragma once
#include "server_native_npc_map_host.h"
#include "server_scene_cache_owner.h"
#include "native_use_observation.h"
#include "native_npc_spawn.h"
#include "test_retail_dialogue_source.h"
#include <cassert>
#include <vector>
#include <string>
#include <fstream>
#include <cstdlib>
namespace sin_port::map_host::test {
namespace sm=sin_compat::map;
struct NpcBspShell final:sm::ISourceMapEntityHost,INpcMapShells {
    struct Slot {
        alignas(4) unsigned char storage[0x1458]{};
        npc::HologramControl control{storage,sizeof(storage)};
        EntityRef ref{};bool live=false;
    };
    Slot slots[6]{};
    alignas(4) unsigned char player[0x1230]{};
    std::uint32_t behaviors[3]{};
    alignas(4) unsigned char expeditor[0x7c]{},jessica_expeditor[0x7c]{};
    npc::CountClassInitBindings ci{};npc::JessicaSpawnBindings js{};npc::JcProfileBindings cp{};
    const npc::ExpeditorLifecycleApi& lifecycle;
    npc::ExpeditorControl count_exp{expeditor,sizeof(expeditor)},jessica_exp{jessica_expeditor,sizeof(jessica_expeditor)};
    npc::ExpeditorTables tables{0x11110000,0x22220000,0x33330000,0x44440000};
    npc::ExpeditorLifecycleBindings life{};
    npc::SpeechSemaphorePair sem{};
    alignas(4) unsigned char owned_greeting_response[0x124]{};
    unsigned symbol_constructs=0,symbol_destroys=0,history_destroys=0,row_frees=0;
    const npc::NpcSpawnApi& spawn;
    npc::NpcMapBindings b{};
    npc::JessicaInteractionBindings ib{};
    npc::NpcMapDialogueBindings dialogue{};
    npc::SceneManagerSession scene_session{};npc::IntegratedNpcMapDialogueBindings scene{};
    bool scene_missing=false,generated_missing=false;
    npc::NativeGeneratedResponseBindings generated_scene{};
    npc::ScriptActorSpeechBindings script_speech{};
    bool script_missing=false;int script_result=1;unsigned script_calls=0;
    void* observed_store_owner=nullptr;void* observed_store_system=nullptr;
    const npc::ResponseSceneFilterApi* observed_filter_api=nullptr;
    npc::TalkCooldownState cooldown{};
    const npc::TalkManagerApi& manager_api;
    const npc::TalkManagerRuntimeApi& manager_runtime;
    const npc::TalkCatalogApi& catalog;
    alignas(4) unsigned char manager_object[npc::talk_manager_bytes]{};
    npc::TalkManagerControl manager_control{manager_object,sizeof(manager_object)};
    npc::TalkManagerSession manager_session{};
    npc::TalkManagerTables manager_tables{0x55550000,0x66660000,0x77770000};
    npc::TalkManagerRuntimeBindings manager{};
    unsigned manager_creates=0,manager_inserts=0,manager_destroys=0,manager_propagates=0;
    bool manager_missing=false;
    alignas(4) unsigned char selected_response[0x124]{};
    const char* selected_concept=nullptr;
    unsigned modifier_calls=0,criteria_ends=0,other_observes=0,dialogue_criteria=0;
    bool dialogue_missing=false;
    int response_system=0,criteria_storage=0;
    const npc::ResponsePolicyApi& response;
    npc::ResponseHistoryBindings history{};
    alignas(4) unsigned char greeting_row[0x1c]{};
    int greeting_index=-1;unsigned greeting_inserts=0,greeting_notifies=0,greeting_clocks=0;
    float now=5;
    NpcMapResult (*interaction_thunk)(NativeNpcMapHost&,EntityRef,unsigned*)=nullptr;
    unsigned interaction_calls=0,interaction_resets=0;
    bool interaction_used=false,interaction_fail=false;
    npc::NpcMapSession session{};
    const npc::JessicaApi& j;
    const npc::CountInterrogationApi& count;
    const npc::CountSchedulerApi& sched;
    const npc::GenericActorApi& actor;
    NativeNpcMapHost* host=nullptr;
    std::vector<std::string> keys;
    unsigned next=0,spawned=0,activated=0,removed=0,conditions=0,holsters=0,speeches=0,actor_observes=0,changes=0;
    bool fail_parse=false,fail_remove=false,fail_condition=false;
    NpcBspShell(const npc::JessicaApi& a,const npc::CountInterrogationApi& c,const npc::CountSchedulerApi& s,const npc::GenericActorApi& g,const npc::NpcSpawnApi& sp,const npc::ResponsePolicyApi& rp,const npc::ExpeditorLifecycleApi& lc,const npc::TalkManagerApi& ma,const npc::TalkManagerRuntimeApi& mr,const npc::TalkCatalogApi& cat)
        :lifecycle(lc),spawn(sp),manager_api(ma),manager_runtime(mr),catalog(cat),response(rp),j(a),count(c),sched(s),actor(g){
        manager.session=&manager_session;manager.manager.context=this;manager.cooldown.context=this;
        manager.manager.base_construct=[](void* p,void* n,int arg) noexcept -> int{auto& f=*static_cast<NpcBspShell*>(p);return n==f.manager_object&&arg==1;};
        manager.manager.create=[](void* p,npc::TalkManagerSession* s,const char* name,int index) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert(s==&f.manager_session&&!std::strcmp(name,"ai_ally_speech_manager")&&index==-1);++f.manager_creates;
            return f.manager_api.construct(s,&f.manager_control,&f.manager_tables,&f.manager.manager)==1;
        };
        manager.manager.register_created=[](void* p,void* n) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert(n==f.manager_object);return f.manager_api.populate(&f.manager_session,&f.manager_control,&f.manager.manager)==1;
        };
        manager.manager.insert_deadline=[](void* p,void* tree,const char* name,float initial) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert(tree==f.manager_object+0x3a0&&f.manager_inserts<48&&initial==-1&&!std::strcmp(name,f.catalog.at(f.manager_inserts)->name));++f.manager_inserts;return 1;
        };
        manager.manager.free_tree=[](void*,std::uint32_t) noexcept -> int{assert(false);return 0;};
        manager.manager.base_destroy=[](void* p,void* n) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert(n==f.manager_object&&!f.manager_session.instance);++f.manager_destroys;return 1;
        };
        manager.cooldown.clock=[](void* p,float* out) noexcept -> int{*out=static_cast<NpcBspShell*>(p)->now;return 1;};
        manager.cooldown.random=[](void*,float low,float high,float* out) noexcept -> int{
            if(low==5&&high==10){*out=7;return 1;}
            assert(low==10&&high==20);*out=15;return 1;
        };
        manager.cooldown.concept_deadline=[](void*,const char* name,float** out) noexcept -> int{assert(!std::strcmp(name,"TLK_PLLOOKING"));*out=nullptr;return 1;};
        manager.cooldown.propagate=[](void* p,void* n,const char* name) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert(n==f.slots[2].storage&&!std::strcmp(name,"TLK_HELLO"));++f.manager_propagates;return 1;
        };
        manager.cooldown.cooldown_disabled=[](void*,unsigned* out) noexcept -> int{*out=0;return 1;};
        life.context=this;
        life.symbol_construct=[](void* p,void* s,int a,int b,unsigned c) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert((s==f.expeditor+0x34||s==f.jessica_expeditor+0x34)&&!a&&!b&&c==1);
            ++f.symbol_constructs;return 1;
        };
        life.symbol_destroy=[](void* p,void* s) noexcept -> int{auto& f=*static_cast<NpcBspShell*>(p);assert(s==f.expeditor+0x34||s==f.jessica_expeditor+0x34);++f.symbol_destroys;return 1;};
        life.row=[](void* p,void* e,unsigned i,void** row,unsigned* bytes) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert(e==f.jessica_expeditor&&i==0&&f.greeting_index==0);*row=f.greeting_row;*bytes=sizeof(f.greeting_row);return 1;};
        life.destroy_response=[](void* p,std::uint32_t v) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert(v==f.ptr(f.owned_greeting_response)&&!f.history_destroys);++f.history_destroys;return 1;};
        life.classify=[](void* p,void* n,int a,unsigned* out) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert((n==f.slots[2].storage||n==f.slots[3].storage)&&!a);*out=n==f.slots[2].storage;return 1;};
        life.resolve=[](void* p,std::uint32_t h,void** out) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);*out=h==0x123004?f.slots[2].storage:h==0x1004?f.slots[3].storage:nullptr;return 1;};
        life.free_rows=[](void* p,std::uint32_t v) noexcept -> int{auto& f=*static_cast<NpcBspShell*>(p);assert(v==f.ptr(f.greeting_row));++f.row_frees;return 1;};
        b.jessica.context=this;b.scheduler.context=this;b.count.context=this;b.attention.context=this;b.actor.context=this;
        b.jessica.condition=[](void* p,void* n,npc::JessicaCondition op,unsigned id,unsigned*) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert(n==f.slots[2].storage&&op==npc::JessicaCondition::Set&&id==0x47);++f.conditions;return !f.fail_condition;
        };
        b.jessica.resolve_target=[](void* p,std::uint32_t h,void** out) noexcept -> int{auto& f=*static_cast<NpcBspShell*>(p);assert(h==0x7001);*out=f.player;return 1;};
        b.jessica.holster=[](void* p,void* w,int arg) noexcept -> int{auto& f=*static_cast<NpcBspShell*>(p);assert(w==f.player&&!arg);++f.holsters;return 1;};
        b.scheduler.append=[](void* p,void* n,std::uint32_t behavior) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert(n==f.slots[3].storage&&behavior==f.ptr(f.slots[3].storage+0x1000));
            f.behaviors[0]=behavior;f.put(n,0xc1c,f.ptr(f.behaviors));f.put(n,0xc28,1);return 1;
        };
        b.scheduler.register_base=[](void* p,void* n) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);f.behaviors[1]=0x12345678;f.behaviors[2]=0x87654321;f.put(n,0xc28,3);return 1;
        };
        b.scheduler.eligible=[](void*,std::uint32_t,unsigned* out) noexcept -> int{*out=0;return 1;};
        b.scheduler.begin=[](void* p,std::uint32_t behavior) noexcept -> int{auto& f=*static_cast<NpcBspShell*>(p);return behavior==f.behaviors[0];};
        b.scheduler.gather=[](void* p,std::uint32_t behavior) noexcept -> int{auto& f=*static_cast<NpcBspShell*>(p);assert(f.session.scheduler.gathering);return behavior==f.behaviors[0];};
        b.scheduler.end=b.scheduler.begin;
        b.scheduler.reset=[](void*,void*) noexcept -> int{return 1;};
        b.scheduler.changed=[](void* p,void*,std::uint32_t prior,std::uint32_t selected) noexcept -> int{auto& f=*static_cast<NpcBspShell*>(p);assert(prior!=selected);++f.changes;return 1;};
        b.scheduler.schedule=[](void*,std::uint32_t,int* out) noexcept -> int{*out=100000;return 1;};
        b.scheduler.is_local=[](void*,std::uint32_t,int,unsigned*) noexcept -> int{return 0;};
        b.scheduler.translate=[](void*,std::uint32_t,int id,int* out) noexcept -> int{assert(id==100000);*out=500;return 1;};
        b.scheduler.base_schedule=[](void*,void*,int* out) noexcept -> int{*out=99;return 1;};
        b.count.resolve=[](void* p,std::uint32_t h,void** out) noexcept -> int{auto& f=*static_cast<NpcBspShell*>(p);assert(h==0x1004);*out=f.slots[3].storage;return 1;};
        b.count.clock=[](void*,float* out) noexcept -> int{*out=5;return 1;};
        b.count.expeditor=[](void* p,void* n,void** out) noexcept -> int{auto& f=*static_cast<NpcBspShell*>(p);assert(n==f.slots[3].storage);*out=f.count_exp.stage==npc::ExpeditorStage::Live?f.expeditor:nullptr;return 1;};
        b.count.speaking=[](void*,void*,unsigned* out) noexcept -> int{*out=0;return 1;};
        b.count.criteria=[](void* p,const void* rec,float now,npc::CountCriteria*) noexcept -> int{auto& f=*static_cast<NpcBspShell*>(p);assert(rec==f.player+0xc40+0x18&&now==5);return 1;};
        b.count.speak=[](void* p,void*,const char* name,const npc::CountCriteria*,unsigned* played) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert(!std::strcmp(name,"TLK_INTERROGATE_COCKY"));++f.speeches;*played=0;return 1;
        };
        b.scheduler.observe_other=[](void* p,std::uint32_t active,const void* obs,unsigned bytes) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert(active==0x12345678&&obs==f.player+0xc40&&bytes>=0x5e9&&f.slots[3].control.busy);
            ++f.other_observes;return 1;
        };
        // Both real observation adapters run the selection wrapper using
        // their actual constructed expeditors. Synthetic rule engine
        // returns no match; Phase788 separately verifies playback.
        npc::ResponseSpeakBindings speak{};
        speak.context=this;speak.selection.context=this;
        speak.selection.system=[](void* p,void* exp,void** out) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert(exp==f.expeditor||exp==f.jessica_expeditor);*out=&f.response_system;return 1;
        };
        speak.selection.begin_criteria=[](void* p,void** out) noexcept -> int{*out=&static_cast<NpcBspShell*>(p)->criteria_storage;return 1;};
        speak.selection.add=[](void* p,void* c,const char* key,const char* name,float w) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert(c==&f.criteria_storage&&!std::strcmp(key,"concept")&&w==5.f);
            f.selected_concept=name;return 1;
        };
        speak.selection.modifiers=[](void* p,void* c,const char* mods,float w) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert(c==&f.criteria_storage&&!std::strcmp(mods,"region:head")&&w==5.f);++f.modifier_calls;return 1;
        };
        speak.selection.npc_criteria=[](void*,void*,void*) noexcept -> int{return 1;};
        speak.selection.player_criteria=[](void*,void*,int index) noexcept -> int{return index==1;};
        speak.selection.allocate=[](void* p,unsigned n,void** out,unsigned* extent) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert(n==sizeof(f.selected_response));*out=f.selected_response;*extent=n;return 1;
        };
        speak.selection.find=[](void* p,void* system,void* c,void* response,void* exp,unsigned* found) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert(system==&f.response_system&&c==&f.criteria_storage&&response==f.selected_response&&f.selected_concept);
            if(exp==f.expeditor)assert(!std::strcmp(f.selected_concept,"TLK_INTERROGATE_COCKY")&&f.slots[3].control.busy);
            else assert(exp==f.jessica_expeditor&&!std::strcmp(f.selected_concept,"TLK_PLLOOKING")&&f.slots[2].control.busy);
            ++f.speeches;*found=0;return 1;
        };
        speak.selection.draw=[](void*,int,int,int*) noexcept -> int{assert(false);return 0;};
        speak.selection.destroy=[](void*,void*) noexcept -> int{assert(false);return 0;};
        speak.selection.end_criteria=[](void* p,void* c) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert(c==&f.criteria_storage);++f.criteria_ends;return 1;
        };
        speak.trace_selected=[](void*,void*,const char*) noexcept -> int{assert(false);return 0;};
        dialogue.count.context=this;dialogue.count.resolve=b.count.resolve;dialogue.count.clock=b.count.clock;
        dialogue.count.expeditor=[](void* p,void* n,void** out,unsigned* bytes) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert(n==f.slots[3].storage&&f.count_exp.stage==npc::ExpeditorStage::Live);*out=f.expeditor;*bytes=sizeof(f.expeditor);return 1;
        };
        dialogue.count.criteria=[](void* p,const void* rec,float now,npc::CountCriteria* out) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert(rec==f.player+0xc40+0x18&&now==5);++f.dialogue_criteria;
            std::strcpy(reinterpret_cast<char*>(out->bytes),"region:head");return 1;
        };
        dialogue.count.speak=speak;
        scene.response.manager=&scene_session;scene.lifecycle.manager=&scene_session;
        scene.response.owner=[](void*,void*,void**) noexcept -> int{assert(false);return 0;};
        scene.response.retain_sound=[](void*,const char*,const char**) noexcept -> int{assert(false);return 0;};
        generated_scene.response=scene.response;generated_scene.generated.lifecycle=scene.lifecycle;
        auto& jd=dialogue.jessica;
        jd.context=this;jd.clock=manager.cooldown.clock;
        jd.criteria=[](void* p,const void* rec,float now,npc::CountCriteria* out) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert(rec==f.player+0xc40+0x18&&now==f.now);++f.dialogue_criteria;
            std::strcpy(reinterpret_cast<char*>(out->bytes),"region:head");return 1;
        };
        cooldown.global[0]=1000; // deliberately stale, managed path must ignore
        jd.cooldown=&cooldown;jd.semaphore=&sem;jd.talk.context=this;
        jd.talk.manager=nullptr; // managed path uses actual native singleton
        jd.talk.resolve=[](void* p,std::uint32_t h,void** out,unsigned* extent) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert(h==0||h==0x123004);*out=h?f.player:nullptr;*extent=sizeof(f.player);return 1;
        };
        jd.talk.expeditor=[](void* p,void* n,void** out,unsigned* bytes) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert(n==f.slots[2].storage&&f.jessica_exp.stage==npc::ExpeditorStage::Live);*out=f.jessica_expeditor;*bytes=sizeof(f.jessica_expeditor);return 1;
        };
        jd.talk.player=[](void* p,void** out) noexcept -> int{*out=static_cast<NpcBspShell*>(p)->player;return 1;};
        jd.talk.within_range=[](void*,void*,void*,unsigned* out) noexcept -> int{*out=1;return 1;};
        jd.talk.suppressed=[](void*,void*,int arg,unsigned* out) noexcept -> int{assert(arg==1);*out=0;return 1;};
        jd.talk.cooldown.context=this;jd.talk.cooldown.clock=b.count.clock;
        jd.talk.cooldown.concept_deadline=[](void*,const char*,float** out) noexcept -> int{*out=nullptr;return 1;};
        jd.talk.semaphore.context=this;jd.talk.semaphore.uses=[](void*,void*,unsigned* out) noexcept -> int{*out=0;return 1;};
        jd.talk.semaphore.classify=[](void*,void*,int,unsigned*) noexcept -> int{assert(false);return 0;};
        jd.talk.semaphore.clock=b.count.clock;
        jd.talk.semaphore.resolve=[](void*,std::uint32_t,void**) noexcept -> int{assert(false);return 0;};
        jd.talk.repeat.context=this;
        jd.talk.repeat.lookup=[](void*,void*,const char* name,int* out) noexcept -> int{assert(!std::strcmp(name,"TLK_PLLOOKING"));*out=-1;return 1;};
        jd.talk.repeat.entry=[](void*,void*,int,void**,unsigned*) noexcept -> int{assert(false);return 0;};
        jd.talk.repeat.resolve=[](void*,std::uint32_t,void**,unsigned*) noexcept -> int{assert(false);return 0;};
        jd.talk.repeat.interval.random=[](void*,float,float,float*) noexcept -> int{assert(false);return 0;};
        jd.talk.repeat.clock=b.count.clock;jd.talk.speak=speak;
        b.attention.clock=b.count.clock;
        b.attention.speak_criteria=[](void* p,void* n,const void* rec,float now,const char* name) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert(n==f.slots[2].storage&&rec==f.player+0xc40+0x18&&now==5&&!std::strcmp(name,"TLK_PLLOOKING"));++f.speeches;return 1;
        };
        b.actor.health_dirty=[](void*,void*) noexcept -> int{return 1;};
        b.actor.behavior_observe=[](void* p,std::uint32_t behavior,const void* obs) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert(behavior==0x11111111&&obs==f.player+0xc40);++f.actor_observes;return 1;
        };
        ci.context=this;ci.behaviors=b.scheduler;
        ci.base_postconstruct=[](void*,void*,std::uint32_t name) noexcept -> int{return name==42;};
        ci.base_create=[](void*,void*,unsigned* accepted) noexcept -> int{*accepted=1;return 1;};
        ci.create_expeditor=[](void* p,void* npc,void** out,unsigned* bytes) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert(npc==f.slots[3].storage&&f.word(npc,0xc28)==3);if(f.lifecycle.construct(&f.count_exp,npc,&f.tables,&f.life)!=1)return 0;
            *out=f.expeditor;*bytes=sizeof(f.expeditor);return 1;
        };
        cp.context=this;cp.health_dirty=b.actor.health_dirty;
        cp.npc_initialize=[](void* p,void* npc) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert(npc==f.slots[3].storage&&f.word(npc,0x9c)==8&&f.word(npc,0xc28)==3&&f.word(npc,0xffc)==f.ptr(f.expeditor));return 1;
        };
        js.context=this;
        js.base_spawn=[](void*,void*) noexcept -> int{return 1;};
        js.resolve_parent=[](void*,std::uint32_t,void**) noexcept -> int{return 0;};
        js.parented_setup=[](void*,void*,unsigned,unsigned) noexcept -> int{return 0;};
        js.restore_unparented=[](void*,void*) noexcept -> int{return 1;};
        js.health_dirty=b.actor.health_dirty;
        js.npc_initialize=[](void* p,void* npc) noexcept -> int{auto& f=*static_cast<NpcBspShell*>(p);return npc==f.slots[2].storage&&f.word(npc,0x9c)==80;};
        js.install_interaction=[](void* p,void* n,unsigned off,unsigned thunk) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);
            if(n!=f.slots[2].storage||off!=0x80||thunk!=0x22361e40)return 0;
            f.interaction_thunk=[](NativeNpcMapHost& h,EntityRef e,unsigned* out){return h.interaction(e,out);};
            unsigned result=99;
            assert(f.interaction_thunk(*f.host,f.slots[2].ref,&result)==NpcMapResult::Rejected&&result==99);
            return 1;
        };
        history.context=this;
        history.lookup=[](void* p,void* e,const char* name,int* index) noexcept -> int{auto& f=*static_cast<NpcBspShell*>(p);assert(e==f.jessica_expeditor&&!std::strcmp(name,"TLK_HELLO"));*index=f.greeting_index;return 1;};
        history.insert=[](void* p,void* e,const char* name,float initial,int* index) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert(e==f.jessica_expeditor&&!std::strcmp(name,"TLK_HELLO")&&f.greeting_index==-1&&initial==f.now);
            ++f.greeting_inserts;f.greeting_index=*index=0;
            f.put(f.greeting_row,0,0xffffffffu);f.put(f.greeting_row,4,0xffffffffu);
            f.put(e,0x14,f.ptr(f.greeting_row));f.put(e,0x18,1);f.put(e,0x20,0);f.put(e,0x24,1);f.put(e,0x2c,1);
            std::memcpy(f.greeting_row+0x14,&initial,4);return 1;
        };
        history.entry=[](void* p,void* e,int i,void** row,unsigned* bytes) noexcept -> int{auto& f=*static_cast<NpcBspShell*>(p);assert(e==f.jessica_expeditor&&i==0);*row=f.greeting_row;*bytes=sizeof(f.greeting_row);return 1;};
        history.clock=[](void* p,float* out) noexcept -> int{auto& f=*static_cast<NpcBspShell*>(p);++f.greeting_clocks;*out=f.now;return 1;};
        history.notify=[](void* p,void* e,const char* name) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert(e==f.jessica_expeditor&&!std::strcmp(name,"TLK_HELLO"));++f.greeting_notifies;
            float time=0;std::memcpy(&time,f.greeting_row+0x14,4);
            return time==f.now&&f.manager_runtime.notify(&f.manager,f.slots[2].storage,sizeof(f.slots[2].storage),e,sizeof(f.jessica_expeditor),name)==1;
        };
        ib.context=this;
        ib.speech_manager=[](void* p) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);void* out=nullptr;return f.manager_api.ensure(&f.manager_session,&f.manager.manager,&out)==1&&out==f.manager_object;
        };
        ib.resolve=[](void*,std::uint32_t h,unsigned* out) noexcept -> int{*out=h==0x123004;return 1;};
        ib.expeditor=[](void* p,void* n,void** out) noexcept -> int{auto& f=*static_cast<NpcBspShell*>(p);assert(n==f.slots[2].storage);*out=f.jessica_expeditor;return 1;};
        ib.eligible=[](void*,void*,void*,unsigned* out) noexcept -> int{*out=1;return 1;};
        ib.timing=[](void* p,void* exp,unsigned* out) noexcept -> int{auto& f=*static_cast<NpcBspShell*>(p);return f.response.timing(exp,sizeof(f.jessica_expeditor),npc::ResponseTiming::Urgent,f.now,out)==1;};
        ib.player=[](void* p,void** out) noexcept -> int{*out=static_cast<NpcBspShell*>(p)->player;return 1;};
        ib.within_range=[](void* p,void* n,void* pl,unsigned* out) noexcept -> int{auto& f=*static_cast<NpcBspShell*>(p);assert(n==f.slots[2].storage&&pl==f.player);*out=1;return 1;};
        ib.suppressed=[](void*,void*,int arg,unsigned* out) noexcept -> int{assert(!arg);*out=0;return 1;};
        ib.speak=[](void* p,void* n,const char* name,int arg,unsigned* out) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert(n==f.slots[2].storage&&!arg);++f.interaction_calls;
            if(!std::strcmp(name,"TLK_USE"))*out=f.interaction_used;
            else {float timestamp=0;std::memcpy(&timestamp,f.greeting_row+0x14,4);assert(!std::strcmp(name,timestamp!=-1.f?"TLK_IDLE":"TLK_HELLO"));*out=1;}
            return !f.interaction_fail;
        };
        ib.has_concept=[](void* p,void* exp,const char* name,unsigned* out) noexcept -> int{auto& f=*static_cast<NpcBspShell*>(p);assert(exp==f.jessica_expeditor&&!std::strcmp(name,"TLK_HELLO"));return f.response.has_concept(exp,sizeof(f.jessica_expeditor),&f.history,name,out)==1;};
        ib.reset_manager=[](void* p,float delta,int arg) noexcept -> int{auto& f=*static_cast<NpcBspShell*>(p);assert(delta==-1.f&&!arg);++f.interaction_resets;return f.manager_runtime.reset_interaction(&f.manager)==1;};
        js.expeditor=[](void* p,void*,void** out) noexcept -> int{*out=static_cast<NpcBspShell*>(p)->jessica_expeditor;return 1;};
        js.seed_concept=[](void* p,void* exp,const char* name,unsigned a,unsigned b) noexcept -> int{auto& f=*static_cast<NpcBspShell*>(p);return exp==f.jessica_expeditor&&!std::strcmp(name,"TLK_HELLO")&&!a&&b==1&&f.response.seed(exp,sizeof(f.jessica_expeditor),&f.history,name)==1;};
    }
    static unsigned ptr(void* p){return static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(p));}
    static void put(void* p,unsigned off,unsigned w){std::memcpy(static_cast<unsigned char*>(p)+off,&w,4);}
    static unsigned word(void* p,unsigned off){unsigned w=0;std::memcpy(&w,static_cast<unsigned char*>(p)+off,4);return w;}
    Slot* get(EntityRef e){return is_live(e)?&slots[e.index-1]:nullptr;}
    bool is_live(EntityRef e)const override{return e.index>=1&&e.index<=6&&slots[e.index-1].live&&slots[e.index-1].ref==e;}
    bool create_entity(std::string_view name,EntityRef& e)override{
        if(next>=6)return false;auto& s=slots[next];e={++next,1};s.ref=e;s.live=true;
        if(name=="npc_jessica"){
            put(s.storage,0xf0,0xffffffffu);
            // Host fixture invokes the shared zero-allocation constructor.
            // This is not a claim of recovered full Jessica PostConstructor.
            if(lifecycle.construct(&jessica_exp,s.storage,&tables,&life)!=1)return false;
            put(s.storage,0xffc,ptr(jessica_expeditor));put(jessica_expeditor,0xc,ptr(s.storage+0xdfc));
        }
        if(name=="npc_count"){
            unsigned accepted=0;
            return count.initialize(&s.control)==1&&spawn.count_class_init(&s.control,&ci,42,&accepted)==1&&accepted;
        }
        return true;
    }
    bool key_value(EntityRef e,std::string_view k,std::string_view v)override{
        if(!is_live(e))return false;keys.emplace_back(std::string(k)+"="+std::string(v));return true;
    }
    bool dispatch_spawn(EntityRef e)override{
        auto* s=get(e);if(!s)return false;++spawned;
        // Fixture invokes known native slices at its engine boundaries.
        if(e.index==3){assert(host->input(e,"HolsterWeapon",5,0)==NpcMapResult::Rejected);return spawn.jessica(&s->control,&js)==1;}
        if(e.index==4)return spawn.count_profile(&s->control,&cp)==1;
        if(e.index==5)return actor.spawn_profile(&s->control,&b.actor)==1;
        return true;
    }
    bool activate(EntityRef e)override{if(!is_live(e))return false;++activated;return true;}
    bool remove_entity(EntityRef e)override{
        auto* s=get(e);if(!s||(fail_remove&&e.index==4))return false;
        auto* exp=e.index==3?&jessica_exp:e.index==4?&count_exp:nullptr;
        if(exp&&exp->stage==npc::ExpeditorStage::Live){
            if(lifecycle.destroy(exp,s->storage,&tables,&sem,&life)!=1)return false;
            put(s->storage,0xffc,0);
        }
        if(exp&&exp->stage==npc::ExpeditorStage::Failed)return false;
        s->live=false;++removed;return true;
    }
    npc::HologramControl* control(EntityRef e)override{auto* s=get(e);return s?&s->control:nullptr;}
    const npc::NpcMapBindings* bindings(EntityRef e)override{return is_live(e)?&b:nullptr;}
    const npc::JessicaInteractionBindings* interaction_bindings(EntityRef e)override{return is_live(e)&&e.index==3?&ib:nullptr;}
    const npc::NpcMapDialogueBindings* dialogue_bindings(EntityRef e)override{return !dialogue_missing&&is_live(e)&&(e.index==3||e.index==4)?&dialogue:nullptr;}
    const npc::TalkManagerRuntimeBindings* manager_bindings(EntityRef e)override{return !manager_missing&&is_live(e)&&e.index==3?&manager:nullptr;}
    const npc::IntegratedNpcMapDialogueBindings* scene_bindings(EntityRef e)override{return !scene_missing&&is_live(e)&&(e.index==3||e.index==4)?&scene:nullptr;}
    const npc::NativeGeneratedResponseBindings* generated_scene_bindings(EntityRef e)override{return !generated_missing&&is_live(e)&&(e.index==3||e.index==4)?&generated_scene:nullptr;}
    const npc::ScriptActorSpeechBindings* script_speech_bindings(EntityRef e)override{return !script_missing&&is_live(e)&&(e.index==3||e.index==4)?&script_speech:nullptr;}
    float manager_deadline(){npc::TalkCooldownState s{};assert(manager_api.read_cooldowns(&manager_session,&s)==1);return s.global[0];}
    bool parse_key(EntityRef e,unsigned type,std::string_view v,std::uint32_t& out)override{
        assert(e.index==5&&type==5);if(fail_parse)return false;
        if(v=="12")out=12;else if(v=="247")out=247;else return false;return true;
    }
};
inline void check_npc_bsp(const npc::NpcMapApi& api,const npc::JessicaApi& j,const npc::CountInterrogationApi& count,
    const npc::CountSchedulerApi& sched,const npc::GenericActorApi& actor,const npc::UseObservationApi& use,const npc::NpcSpawnApi& spawn,const npc::JessicaInteractionApi& interaction,const npc::ResponsePolicyApi& response,const npc::ExpeditorLifecycleApi& lifecycle,const npc::NpcMapDialogueApi& dialogue,const npc::ManagedJessicaObservationApi& managed,const npc::TalkManagerApi& ma,const npc::TalkManagerRuntimeApi& mr,const npc::TalkCatalogApi& cat,const std::vector<std::uint8_t>& bsp,const npc::IntegratedNpcMapDialogueApi* integrated=nullptr,const npc::NativeGeneratedMapDialogueApi* generated=nullptr){
    NpcBspShell f(j,count,sched,actor,spawn,response,lifecycle,ma,mr,cat);
    NativeNpcMapHost host(f,f,api,f.session,&interaction,&dialogue,&managed,integrated,generated);f.host=&host;sm::SourceMapRuntime runtime(host);
    const auto loaded=runtime.load_bsp(bsp.data(),bsp.size());
    assert(loaded.ok()&&loaded.entities_created==6&&f.spawned==6&&f.activated==6&&host.attached_count()==4);
    const auto relay=runtime.entities()[1],je=runtime.entities()[2],co=runtime.entities()[3],ga=runtime.entities()[4],jc=runtime.entities()[5];
    auto* J=f.slots[2].storage;auto* C=f.slots[3].storage;auto* G=f.slots[4].storage;
    assert(f.word(J,0x1450)==0xffffffffu&&f.word(J,0x1454)==0xffffffffu&&f.word(J,0x9c)==80&&f.word(C,0x9c)==8&&f.word(C,0xc28)==3&&f.word(f.expeditor,0xc)==f.ptr(C+0xdfc)&&f.word(G,0x9c)==12&&f.word(G,0x614)==247);
    assert(f.symbol_constructs==2&&f.count_exp.stage==npc::ExpeditorStage::Live&&f.jessica_exp.stage==npc::ExpeditorStage::Live);
    assert(f.word(f.expeditor,4)==f.ptr(C)&&f.word(f.jessica_expeditor,4)==f.ptr(J)&&f.word(f.expeditor,0x74)==100&&f.word(f.jessica_expeditor,0x74)==100);
    assert(f.keys.size()==8&&f.keys[0]=="targetname=relay"&&f.keys[1]=="OnTrigger=count,InterrogationStart,,0,-1"&&f.keys[2]=="OnTrigger=jessica,ThrowItemAtPlayer,weapon_magnum,0,-1"&&f.keys[3]=="targetname=jessica"&&f.keys[4]=="targetname=count"&&f.keys[5]=="targetname=actor"&&f.keys[6]=="model=fixture.mdl"&&f.keys[7]=="targetname=jc");
    assert(host.input(relay,"InterrogationStart",0,0)==NpcMapResult::Unhandled&&host.input(jc,"HolsterWeapon",5,0)==NpcMapResult::Unhandled);
    assert(f.greeting_index==0&&f.greeting_inserts==1&&f.greeting_notifies==1&&f.greeting_clocks==2);
    assert(f.manager_creates==1&&f.manager_inserts==48&&f.manager_propagates==1&&f.manager_deadline()==12);
    float greeting_time=0;std::memcpy(&greeting_time,f.greeting_row+0x14,4);assert(greeting_time==5);
    // Test existing-entry seed path: no insertion and preserves response word.
    f.put(f.greeting_row,0x18,f.ptr(f.owned_greeting_response));
    f.now=6;assert(response.seed(f.jessica_expeditor,sizeof(f.jessica_expeditor),&f.history,"TLK_HELLO")==1);
    assert(f.greeting_inserts==1&&f.greeting_notifies==2&&f.greeting_clocks==3&&f.word(f.greeting_row,0x18)==f.ptr(f.owned_greeting_response));
    assert(f.manager_deadline()==13&&f.manager_propagates==2);
    unsigned interaction_result=99;
    assert(f.interaction_thunk&&host.interaction(co,&interaction_result)==NpcMapResult::Unhandled&&interaction_result==99);
    assert(host.interaction(EntityRef{je.index,je.generation+1},&interaction_result)==NpcMapResult::Rejected);
    f.put(J,0x16c,0x123004);
    assert(f.interaction_thunk(host,je,&interaction_result)==NpcMapResult::Applied&&interaction_result==1&&f.interaction_calls==2);
    // Spawn seed selects IDLE; the original -1 sentinel selects HELLO.
    const float unspoken=-1.f;std::memcpy(f.greeting_row+0x14,&unspoken,4);
    std::memcpy(f.jessica_expeditor+0x78,&f.now,4);
    assert(f.interaction_thunk(host,je,&interaction_result)==NpcMapResult::Applied&&!interaction_result&&f.interaction_calls==2);
    f.put(f.jessica_expeditor,0x78,0);
    const float future=7.f;std::memcpy(f.jessica_expeditor+0x6c,&future,4);
    assert(f.interaction_thunk(host,je,&interaction_result)==NpcMapResult::Applied&&!interaction_result&&f.interaction_calls==2);
    assert(response.timing(f.jessica_expeditor,sizeof(f.jessica_expeditor),npc::ResponseTiming::Normal,f.now,&interaction_result)==1&&interaction_result==1);
    std::memcpy(f.jessica_expeditor+0x6c,&f.now,4); // equality allowed in second predicate
    assert(f.interaction_thunk(host,je,&interaction_result)==NpcMapResult::Applied&&interaction_result==1&&f.interaction_calls==4);
    // Zero is a valid spoken timestamp (only -1 is unspoken).
    f.put(f.greeting_row,0x14,0);assert(response.has_concept(f.jessica_expeditor,sizeof(f.jessica_expeditor),&f.history,"TLK_HELLO",&interaction_result)==1&&interaction_result==1);
    f.interaction_used=true;
    assert(f.interaction_thunk(host,je,&interaction_result)==NpcMapResult::Applied&&!interaction_result&&f.interaction_calls==5&&f.interaction_resets==1);
    assert(f.manager_deadline()==21&&f.manager_creates==1);
    f.interaction_fail=true;interaction_result=99;
    assert(f.interaction_thunk(host,je,&interaction_result)==NpcMapResult::Failed&&interaction_result==99&&f.slots[2].control.blocked);
    assert(host.interaction(je,&interaction_result)==NpcMapResult::Rejected&&interaction.acknowledge(&f.slots[2].control));
    f.interaction_fail=false;

    assert(host.input(je,"SetPrimaryAmmoCountToThrow",5,9)==NpcMapResult::Applied&&f.word(J,0x1450)==9);
    assert(host.input(je,"SetSecondaryAmmoCountToThrow",2,9)==NpcMapResult::Applied&&!f.word(J,0x1454));
    assert(host.input(je,"throwitematplayer",2,17)==NpcMapResult::Applied&&f.conditions==1&&f.word(J,0x144c)==17);
    f.put(J,0x78c,0x7001);assert(host.input(je,"HolsterWeapon",5,0)==NpcMapResult::Applied&&f.holsters==1);
    assert(host.input(ga,"PlayerCollide",6,0x103)==NpcMapResult::Applied&&G[0x100c]==3);
    assert(host.input(ga,"PlayerCollide",5,1)==NpcMapResult::Applied&&!G[0x100c]);
    int schedule=0;
    assert(host.select_count(co,&schedule)==NpcMapResult::Applied&&schedule==99);
    assert(host.input(co,"InterrogationStart",0,0)==NpcMapResult::Applied&&C[0x1010]);
    assert(host.select_count(co,&schedule)==NpcMapResult::Applied&&schedule==500&&f.changes==1);
    auto* monitor=f.player+0xc40;f.put(monitor,0x14,0x1004);std::strcpy(reinterpret_cast<char*>(monitor+0x2d8),"head");
    std::strcpy(reinterpret_cast<char*>(monitor+0x18+0x260),"npc_count");
    f.put(G,0xc18,0x11111111);
    struct Gate {NpcBspShell* f;EntityRef ref;unsigned delivered=0;};
    Gate gate{&f,co};
    npc::NpcObservationBindings n{};n.context=&gate;
    n.max_clients=[](void*,int* out) noexcept -> int{*out=1;return 1;};
    n.single_player=[](void* p,void** out,unsigned* bytes) noexcept -> int{auto& x=*static_cast<Gate*>(p);*out=x.f->player;*bytes=sizeof(x.f->player);return 1;};
    n.observe=[](void* p,void* npc,const void* obs,unsigned bytes) noexcept -> int{
        auto& x=*static_cast<Gate*>(p);assert(npc==x.f->slots[x.ref.index-1].storage);++x.delivered;
        return x.f->host->observe(x.ref,obs,bytes)==NpcMapResult::Applied;
    };
    assert(use.npc_tick(C,&n)==1&&!gate.delivered);
    monitor[0x300]=1;
    if(generated){
        assert(integrated); // explicitly verify no fallback to available older path
        f.generated_missing=true;
        assert(host.observe(co,monitor,0x5e9)==NpcMapResult::Rejected&&!f.speeches&&!f.word(C,0x1014));
        f.generated_missing=false;
        npc::SceneManagerSession wrong{};f.generated_scene.generated.lifecycle.manager=&wrong;
        assert(host.observe(co,monitor,0x5e9)==NpcMapResult::Rejected&&!f.speeches&&!f.word(C,0x1014));
        f.generated_scene.generated.lifecycle.manager=&f.scene_session;
        f.scene_missing=true; // native path must not require old scene bindings
    }else if(integrated){
        f.scene_missing=true;assert(host.observe(co,monitor,0x5e9)==NpcMapResult::Rejected&&!f.speeches&&!f.word(C,0x1014));
        f.scene_missing=false;
        npc::SceneManagerSession wrong{};f.scene.lifecycle.manager=&wrong;
        assert(host.observe(co,monitor,0x5e9)==NpcMapResult::Rejected&&!f.speeches&&!f.word(C,0x1014));
        f.scene.lifecycle.manager=&f.scene_session;
    }
    assert(use.npc_tick(C,&n)==1&&gate.delivered==1&&f.speeches==1&&f.word(C,0x1014)==1);
    f.put(J,0x850,1); // original priority-zero companion state prerequisite
    f.dialogue_missing=true;assert(host.observe(je,monitor,0x5e9)==NpcMapResult::Rejected&&f.speeches==1);
    f.dialogue_missing=false;
    assert(host.observe(EntityRef{je.index,je.generation+1},monitor,0x5e9)==NpcMapResult::Rejected);
    f.manager_missing=true;assert(host.observe(je,monitor,0x5e9)==NpcMapResult::Rejected&&f.speeches==1);
    f.manager_missing=false;
    gate.ref=je;assert(use.npc_tick(J,&n)==1&&f.speeches==1&&f.dialogue_criteria==2); // spawn/use cooldown
    f.now=21;assert(use.npc_tick(J,&n)==1&&f.speeches==2);
    assert(f.modifier_calls==2&&f.criteria_ends==2&&f.dialogue_criteria==3&&f.manager_deadline()==21&&f.cooldown.global[0]==1000);
    f.put(C,0xc18,0x12345678);assert(host.observe(co,monitor,0x5e9)==NpcMapResult::Applied&&f.other_observes==1&&f.speeches==2);
    f.put(C,0xc18,f.ptr(C+0x1000));
    gate.ref=ga;assert(use.npc_tick(G,&n)==1&&f.actor_observes==1);
    assert(host.observe(jc,monitor,0x5e9)==NpcMapResult::Unhandled);
    assert(host.input(co,"InterrogationStop",0,0)==NpcMapResult::Applied&&!C[0x1010]);
    assert(host.select_count(co,&schedule)==NpcMapResult::Applied&&schedule==99&&f.changes==2);
    gate.ref=co;assert(use.npc_tick(C,&n)==1&&f.speeches==2);
    f.fail_condition=true;assert(host.input(je,"ThrowItemAtPlayer",2,8)==NpcMapResult::Failed&&f.slots[2].control.blocked);
    assert(host.input(je,"HolsterWeapon",5,0)==NpcMapResult::Rejected);
    f.sem.classified={10,0x123004};f.sem.other={20,0x1004};
    f.fail_remove=true;assert(runtime.unload().code==sm::SourceMapLoadCode::PartialUnload&&host.attached_count()==1);
    assert(f.jessica_exp.stage==npc::ExpeditorStage::Retired&&f.count_exp.stage==npc::ExpeditorStage::Live&&f.symbol_destroys==1&&f.history_destroys==1&&f.row_frees==1);
    assert(f.sem.classified.owner==0xffffffffu&&f.sem.other.owner==0x1004);
    f.fail_remove=false;assert(runtime.unload().code==sm::SourceMapLoadCode::Unloaded&&!host.attached_count());
    assert(f.count_exp.stage==npc::ExpeditorStage::Retired&&f.symbol_destroys==2&&f.history_destroys==1&&f.sem.other.owner==0xffffffffu);
    assert(host.input(co,"InterrogationStart",0,0)==NpcMapResult::Rejected);
    assert(host.observe(je,monitor,0x5e9)==NpcMapResult::Rejected);
    interaction_result=99;assert(f.interaction_thunk(host,je,&interaction_result)==NpcMapResult::Rejected&&interaction_result==99);
    // Manager lifetime belongs to the engine session, not an arbitrary NPC.
    assert(ma.destroy(&f.manager_session,&f.manager_control,&f.manager_tables,&f.manager.manager)==1&&f.manager_destroys==1);
    if(generated){
        // Route-only test: the loaded script-map algorithm itself is
        // exercised separately by check_native_response_criteria.
        NpcBspShell r(j,count,sched,actor,spawn,response,lifecycle,ma,mr,cat);
        r.script_speech.context=&r;
        npc::ScriptNpcMapDialogueApi script{npc::version,sizeof(npc::ScriptNpcMapDialogueApi)};
        script.observe=[](npc::NpcMapKind kind,npc::HologramControl* c,npc::NpcMapSession* session,const npc::NpcMapBindings* b,
            const npc::NpcMapDialogueBindings* d,const npc::TalkManagerRuntimeBindings* manager,const npc::ScriptActorSpeechBindings* speech,
            const void* record,unsigned bytes) noexcept -> int {
            auto& r=*static_cast<NpcBspShell*>(speech->context);
            assert(session==&r.session&&b==&r.b&&d==&r.dialogue&&speech==&r.script_speech&&record&&bytes>=0x5e9);
            if(kind==npc::NpcMapKind::Jessica)assert(c==&r.slots[2].control&&manager==&r.manager);
            else assert(kind==npc::NpcMapKind::Count&&c==&r.slots[3].control&&!manager);
            ++r.script_calls;return r.script_result;
        };
        NativeNpcMapHost rh(r,r,api,r.session,&interaction,&dialogue,&managed,integrated,generated,&script);
        r.host=&rh;sm::SourceMapRuntime rt(rh);assert(rt.load_bsp(bsp.data(),bsp.size()).ok());
        const auto jref=rt.entities()[2],cref=rt.entities()[3];
        alignas(4) unsigned char observation[0x5ec]{};
        r.script_missing=true;
        assert(rh.observe(cref,observation,sizeof(observation))==NpcMapResult::Rejected&&!r.script_calls);
        r.script_missing=false;r.manager_missing=true;
        assert(rh.observe(jref,observation,sizeof(observation))==NpcMapResult::Rejected&&!r.script_calls);
        r.manager_missing=false;r.dialogue_missing=true;
        assert(rh.observe(cref,observation,sizeof(observation))==NpcMapResult::Rejected&&!r.script_calls);
        r.dialogue_missing=false;r.generated_missing=true;r.scene_missing=true;
        assert(rh.observe(EntityRef{cref.index,cref.generation+1},observation,sizeof(observation))==NpcMapResult::Rejected&&!r.script_calls);
        script.size=0;assert(rh.observe(cref,observation,sizeof(observation))==NpcMapResult::Rejected&&!r.script_calls);
        script.size=sizeof(script);
        assert(rh.observe(cref,observation,sizeof(observation))==NpcMapResult::Applied&&r.script_calls==1);
        assert(rh.observe(jref,observation,sizeof(observation))==NpcMapResult::Applied&&r.script_calls==2);
        r.script_result=-1;
        assert(rh.observe(jref,observation,sizeof(observation))==NpcMapResult::Failed&&r.script_calls==3&&!r.speeches);
        assert(rt.unload().code==sm::SourceMapLoadCode::Unloaded&&!rh.attached_count());
        assert(rh.observe(cref,observation,sizeof(observation))==NpcMapResult::Rejected&&r.script_calls==3);
        assert(ma.destroy(&r.manager_session,&r.manager_control,&r.manager_tables,&r.manager.manager)==1);
        // Session lifetime/route fixture: a fake finder isolates map-host
        // ownership, while the loaded finder itself has separate tests.
        static unsigned token=0,registry=0,creates=0,releases=0;
        creates=releases=0;
        npc::ScriptResponseFinderApi finder{npc::version,sizeof(npc::ScriptResponseFinderApi)};
        finder.create=[](const char* name,const char* data,unsigned bytes,const npc::ScriptResponseInclude*,npc::ScriptResponseStore* out) noexcept -> int{
            assert(!std::strcmp(name,"talker/response_rules.txt")&&!out->owner&&!out->system);
            ++creates;if(!data||bytes!=4)return -1;
            *out={&token,&registry};return 1;};
        finder.release=[](npc::ScriptResponseStore* out) noexcept -> int{
            assert(out->owner==&token&&out->system==&registry);++releases;*out={};return 1;};
        finder.find=[](void*,void*,void*,void*,void*,unsigned*) noexcept -> int{assert(false);return 0;};
        NpcBspShell sr(j,count,sched,actor,spawn,response,lifecycle,ma,mr,cat);
        sr.script_speech.context=&sr;
        script.observe=[](npc::NpcMapKind kind,npc::HologramControl*,npc::NpcMapSession* session,const npc::NpcMapBindings*,
            const npc::NpcMapDialogueBindings*,const npc::TalkManagerRuntimeBindings*,const npc::ScriptActorSpeechBindings* speech,
            const void* record,unsigned bytes) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(speech->context);
            assert((kind==npc::NpcMapKind::Jessica||kind==npc::NpcMapKind::Count)&&session==&f.session&&record&&bytes>=0x5e9);
            assert(speech!=&f.script_speech&&speech->selection.finder.store.owner==&token&&speech->selection.finder.store.system==&registry);
            assert(f.host->close_script_session()==0&&!f.host->unbind_script_scene_filter());
            const auto& filter=speech->selection.finder.host.scene_filter;
            assert(filter.context==&f&&filter.storage&&filter.resolve_name&&filter.metadata);
            void* row=nullptr;unsigned n=0;
            assert(filter.metadata(filter.context,"fixture.vcd",&row,&n)==1&&row==&f.script_result&&n==sizeof(f.script_result));
            assert(!f.script_speech.selection.finder.host.scene_filter.metadata);
            ++f.script_calls;return 1;};
        NativeNpcMapHost sh(sr,sr,api,sr.session,&interaction,&dialogue,&managed,integrated,generated,&script,&finder);
        sr.host=&sh;sm::SourceMapRuntime st(sh);
        npc::ResponseSceneFilterBindings shared{};shared.context=&sr;
        assert(!sh.bind_script_scene_filter(shared));
        shared.storage=[](void*,std::uint32_t,void**,unsigned*) noexcept -> int{assert(false);return 0;};
        shared.resolve_name=[](void*,void*,const char*,char*,unsigned) noexcept -> int{assert(false);return 0;};
        shared.metadata=[](void* p,const char* name,void** out,unsigned* n) noexcept -> int{
            auto& f=*static_cast<NpcBspShell*>(p);assert(!std::strcmp(name,"fixture.vcd"));
            *out=&f.script_result;*n=sizeof(f.script_result);return 1;};
        assert(sh.bind_script_scene_filter(shared)&&!sh.bind_script_scene_filter(shared));
        EntityRef uncreated{};
        assert(!sh.create_entity("npc_count",uncreated)&&!sh.script_session_open()&&!creates);
        assert(sh.open_script_session("bad",3)==-1&&!sh.script_session_open()&&creates==1);
        assert(sh.open_script_session("data",4)==1&&sh.script_session_open()&&creates==2);
        assert(!sh.unbind_script_scene_filter()&&!sh.bind_script_scene_filter(shared));
        assert(sh.open_script_session("data",4)==0&&creates==2);
        assert(st.load_bsp(bsp.data(),bsp.size()).ok());
        const auto sj=st.entities()[2],sc=st.entities()[3];
        assert(sh.close_script_session()==0&&!releases);
        sr.script_speech.selection.finder.store={&registry,&token};
        assert(sh.observe(sc,observation,sizeof(observation))==NpcMapResult::Rejected&&!sr.script_calls);
        sr.script_speech.selection.finder.store={};
        sr.script_speech.selection.finder.host.scene_filter=shared;
        assert(sh.observe(sc,observation,sizeof(observation))==NpcMapResult::Rejected&&!sr.script_calls);
        sr.script_speech.selection.finder.host.scene_filter={};
        assert(sh.observe(sc,observation,sizeof(observation))==NpcMapResult::Applied&&sr.script_calls==1);
        assert(sh.observe(sj,observation,sizeof(observation))==NpcMapResult::Applied&&sr.script_calls==2);
        assert(!sr.script_speech.selection.finder.store.owner);
        sr.fail_remove=true;assert(st.unload().code==sm::SourceMapLoadCode::PartialUnload&&sh.close_script_session()==0&&!releases);
        sr.fail_remove=false;assert(st.unload().code==sm::SourceMapLoadCode::Unloaded);
        assert(sh.close_script_session()==1&&!sh.script_session_open()&&releases==1);
        assert(sh.close_script_session()==0&&releases==1);
        assert(sh.open_script_session("data",4)==1&&creates==3&&sh.close_script_session()==1&&releases==2);
        assert(sh.unbind_script_scene_filter()&&!sh.unbind_script_scene_filter());
        assert(sh.bind_script_scene_filter(shared)&&sh.unbind_script_scene_filter());
        assert(ma.destroy(&sr.manager_session,&sr.manager_control,&sr.manager_tables,&sr.manager.manager)==1);
    }
    NpcBspShell bad(j,count,sched,actor,spawn,response,lifecycle,ma,mr,cat);bad.fail_parse=true;NativeNpcMapHost bh(bad,bad,api,bad.session);bad.host=&bh;
    sm::SourceMapRuntime rollback(bh);assert(rollback.load_bsp(bsp.data(),bsp.size()).code==sm::SourceMapLoadCode::HostKeyValueFailed&&!rollback.loaded()&&!bh.attached_count());
    assert(!bad.manager_session.instance&&!bad.manager_creates);
    assert(bad.symbol_constructs==bad.symbol_destroys&&bad.symbol_constructs==2&&bad.count_exp.stage==npc::ExpeditorStage::Retired&&bad.jessica_exp.stage==npc::ExpeditorStage::Retired);
}

inline void check_loaded_script_registry_bsp(const npc::NpcMapApi& api,const npc::JessicaApi& j,
    const npc::CountInterrogationApi& count,const npc::CountSchedulerApi& sched,const npc::GenericActorApi& actor,
    const npc::NpcSpawnApi& spawn,const npc::JessicaInteractionApi& interaction,const npc::ResponsePolicyApi& response,
    const npc::ExpeditorLifecycleApi& lifecycle,const npc::NpcMapDialogueApi& dialogue,const npc::ManagedJessicaObservationApi& managed,
    const npc::TalkManagerApi& ma,const npc::TalkManagerRuntimeApi& mr,const npc::TalkCatalogApi& cat,
    const std::vector<std::uint8_t>& bsp,const npc::ScriptResponseFinderApi& finder,
    const npc::SceneMetadataIndexApi& scene_index,const npc::SceneResponseServicesApi& scene_services,
    const npc::ResponseSceneFilterApi& scene_filter,const char* private_dir=nullptr){
    NpcBspShell shell(j,count,sched,actor,spawn,response,lifecycle,ma,mr,cat);
    shell.script_speech.context=&shell;shell.observed_filter_api=&scene_filter;
    struct CacheFixture {
        NpcBspShell& shell;
        unsigned stamps=0,refreshes=0,owners=0;
        static CacheFixture& get(void* p){return *static_cast<CacheFixture*>(p);}
    } cache{shell};
    SceneCacheOwner::Leaves leaves{};leaves.metadata_vtable=0x12345678;auto& h=leaves.external;h.context=&cache;
    leaves.filename=[](void*,const char* name,std::uint32_t* out) noexcept -> int{
        if(std::strcmp(name,"scenes/fixture.vcd"))return 0;*out=1;return 1;};
    leaves.release_vector=[](void*,std::uint32_t) noexcept -> int{assert(false);return 0;};
    h.storage=[](void* p,std::uint32_t word,void** out,unsigned* n) noexcept -> int{
        auto& f=CacheFixture::get(p);for(unsigned i:{2u,3u}){
            auto& slot=f.shell.slots[i];if(slot.live&&word==f.shell.ptr(slot.storage)){++f.owners;*out=slot.storage;*n=sizeof(slot.storage);return 1;}
        }return 0;};
    h.model_name=[](void*,void*,const char** out) noexcept -> int{*out=nullptr;return 1;};
    h.expand=[](void*,const char*,const char* name,char* out,unsigned n) noexcept -> int{
        if(std::strlen(name)>=n)return 0;std::strcpy(out,name);return 1;};
    h.stamp=[](void* p,const char*,unsigned mode,std::uint32_t* out) noexcept -> int{
        if(mode!=1)return 0;++CacheFixture::get(p).stamps;*out=5;return 1;};
    h.refresh=[](void* p,void* metadata,const char* name) noexcept -> int{
        auto& f=CacheFixture::get(p);if(std::strcmp(name,"scenes/fixture.vcd"))return 0;
        ++f.refreshes;float duration=3.f;std::memcpy(static_cast<unsigned char*>(metadata)+4,&duration,4);return 1;};
    SceneCacheOwner owner(scene_index,leaves);npc::ResponseSceneFilterBindings shared{};
    assert(owner.ready()&&scene_services.adapt(&owner.bindings(),&shared)==1);
    // Route observer intentionally does not execute speech. The registry
    // itself is constructed and destroyed by the actual loaded DLL.
    npc::ScriptNpcMapDialogueApi probe{npc::version,sizeof(npc::ScriptNpcMapDialogueApi)};
    probe.observe=[](npc::NpcMapKind kind,npc::HologramControl* c,npc::NpcMapSession* session,const npc::NpcMapBindings* b,
        const npc::NpcMapDialogueBindings* d,const npc::TalkManagerRuntimeBindings* m,const npc::ScriptActorSpeechBindings* speech,
        const void*,unsigned) noexcept -> int{
        auto& f=*static_cast<NpcBspShell*>(speech->context);
        assert(session==&f.session&&b==&f.b&&d==&f.dialogue&&speech!=&f.script_speech);
        if(kind==npc::NpcMapKind::Jessica)assert(c==&f.slots[2].control&&m==&f.manager);
        else assert(kind==npc::NpcMapKind::Count&&c==&f.slots[3].control&&!m);
        const auto& store=speech->selection.finder.store;
        assert(store.owner&&store.system);
        if(!f.script_calls){f.observed_store_owner=store.owner;f.observed_store_system=store.system;}
        else assert(store.owner==f.observed_store_owner&&store.system==f.observed_store_system);
        unsigned allowed=99;auto* exp=kind==npc::NpcMapKind::Jessica?f.jessica_expeditor:f.expeditor;
        assert(f.observed_filter_api&&f.observed_filter_api->allow(exp,sizeof(f.expeditor),&speech->selection.finder.host.scene_filter,
            3,"scenes/fixture.vcd",&allowed)==1&&allowed==1);
        ++f.script_calls;return 1;
    };
    NativeNpcMapHost host(shell,shell,api,shell.session,&interaction,&dialogue,&managed,nullptr,nullptr,&probe,&finder);
    shell.host=&host;assert(host.bind_script_scene_filter(shared));
    const char root[]=R"(criterion idle concept TLK_IDLE required
#include "talker/fixture.txt"
rule idle_rule
{
 criteria idle
 response lines
}
)";
    npc::ScriptResponseInclude inc{};
    inc.load=[](void*,const char* name,const char** data,unsigned* bytes) noexcept -> int{
        if(std::strcmp(name,"talker/fixture.txt"))return 0;
        static const char included[]="response lines speak fixture_line\n";
        *data=included;*bytes=sizeof(included)-1;return 1;
    };
    assert(host.open_script_session(root,sizeof(root)-1)==-1&&!host.script_session_open());
    sin_port::test_support::RetailDialogueSource retail{};
    std::string retail_root;
    if(private_dir&&*private_dir){
        retail.directory=private_dir;
        assert(retail.read("talker/response_rules.txt",retail_root));
        inc={&retail,&sin_port::test_support::RetailDialogueSource::include};
        assert(host.open_script_session(retail_root.data(),static_cast<unsigned>(retail_root.size()),&inc)==1);
        assert(retail.reads>1);
        // The loaded registry must not retain the caller's input spans.
        retail_root.clear();retail.buffer.clear();
    }else assert(host.open_script_session(root,sizeof(root)-1,&inc)==1);
    assert(host.script_session_open());
    sm::SourceMapRuntime runtime(host);
    assert(runtime.load_bsp(bsp.data(),bsp.size()).ok());
    const auto je=runtime.entities()[2],co=runtime.entities()[3];
    alignas(4) unsigned char observation[0x5ec]{};
    assert(host.observe(je,observation,sizeof(observation))==NpcMapResult::Applied);
    assert(host.observe(co,observation,sizeof(observation))==NpcMapResult::Applied&&shell.script_calls==2);
    assert(cache.owners==2&&cache.refreshes==1&&cache.stamps==1&&owner.size()==1);
    assert(host.close_script_session()==0&&host.script_session_open()&&!host.unbind_script_scene_filter());
    assert(runtime.unload().code==sm::SourceMapLoadCode::Unloaded);
    assert(host.close_script_session()==1&&!host.script_session_open());
    assert(ma.destroy(&shell.manager_session,&shell.manager_control,&shell.manager_tables,&shell.manager.manager)==1);
    // Reopen independently after complete teardown (no stale store survives).
    npc::ScriptResponseInclude synthetic{};
    synthetic.load=[](void*,const char*,const char** data,unsigned* bytes) noexcept -> int{
        static const char included[]="response lines speak fixture_line\n";
        *data=included;*bytes=sizeof(included)-1;return 1;
    };
    assert(host.open_script_session(root,sizeof(root)-1,&synthetic)==1&&host.close_script_session()==1);
    assert(owner.size()==1&&host.unbind_script_scene_filter()&&owner.clear()&&!owner.size());
}
}
