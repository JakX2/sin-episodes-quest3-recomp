#pragma once
#include "server_native_npc_map_host.h"
#include "native_use_observation.h"
#include "native_npc_spawn.h"
#include <cassert>
#include <vector>
#include <string>
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
    npc::TalkCooldownState cooldown{};
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
    NpcBspShell(const npc::JessicaApi& a,const npc::CountInterrogationApi& c,const npc::CountSchedulerApi& s,const npc::GenericActorApi& g,const npc::NpcSpawnApi& sp,const npc::ResponsePolicyApi& rp,const npc::ExpeditorLifecycleApi& lc):lifecycle(lc),spawn(sp),response(rp),j(a),count(c),sched(s),actor(g){
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
        auto& jd=dialogue.jessica;
        jd.context=this;jd.clock=b.count.clock;jd.criteria=dialogue.count.criteria;
        jd.cooldown=&cooldown;jd.semaphore=&sem;jd.talk.context=this;
        jd.talk.manager=[](void*) noexcept -> int{return 1;};
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
            float time=0;std::memcpy(&time,f.greeting_row+0x14,4);return time==f.now;
        };
        ib.context=this;
        ib.speech_manager=[](void*) noexcept -> int{return 1;};
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
        ib.reset_manager=[](void* p,float delta,int arg) noexcept -> int{auto& f=*static_cast<NpcBspShell*>(p);assert(delta==-1.f&&!arg);++f.interaction_resets;return 1;};
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
    bool parse_key(EntityRef e,unsigned type,std::string_view v,std::uint32_t& out)override{
        assert(e.index==5&&type==5);if(fail_parse)return false;
        if(v=="12")out=12;else if(v=="247")out=247;else return false;return true;
    }
};
inline void check_npc_bsp(const npc::NpcMapApi& api,const npc::JessicaApi& j,const npc::CountInterrogationApi& count,
    const npc::CountSchedulerApi& sched,const npc::GenericActorApi& actor,const npc::UseObservationApi& use,const npc::NpcSpawnApi& spawn,const npc::JessicaInteractionApi& interaction,const npc::ResponsePolicyApi& response,const npc::ExpeditorLifecycleApi& lifecycle,const npc::NpcMapDialogueApi& dialogue,const std::vector<std::uint8_t>& bsp){
    NpcBspShell f(j,count,sched,actor,spawn,response,lifecycle);
    NativeNpcMapHost host(f,f,api,f.session,&interaction,&dialogue);f.host=&host;sm::SourceMapRuntime runtime(host);
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
    float greeting_time=0;std::memcpy(&greeting_time,f.greeting_row+0x14,4);assert(greeting_time==5);
    // Test existing-entry seed path: no insertion and preserves response word.
    f.put(f.greeting_row,0x18,f.ptr(f.owned_greeting_response));
    f.now=6;assert(response.seed(f.jessica_expeditor,sizeof(f.jessica_expeditor),&f.history,"TLK_HELLO")==1);
    assert(f.greeting_inserts==1&&f.greeting_notifies==2&&f.greeting_clocks==3&&f.word(f.greeting_row,0x18)==f.ptr(f.owned_greeting_response));
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
    monitor[0x300]=1;assert(use.npc_tick(C,&n)==1&&gate.delivered==1&&f.speeches==1&&f.word(C,0x1014)==1);
    f.put(J,0x850,1); // original priority-zero companion state prerequisite
    f.dialogue_missing=true;assert(host.observe(je,monitor,0x5e9)==NpcMapResult::Rejected&&f.speeches==1);
    f.dialogue_missing=false;
    assert(host.observe(EntityRef{je.index,je.generation+1},monitor,0x5e9)==NpcMapResult::Rejected);
    gate.ref=je;assert(use.npc_tick(J,&n)==1&&f.speeches==2);
    assert(f.modifier_calls==2&&f.criteria_ends==2&&f.dialogue_criteria==2);
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
    NpcBspShell bad(j,count,sched,actor,spawn,response,lifecycle);bad.fail_parse=true;NativeNpcMapHost bh(bad,bad,api,bad.session);bad.host=&bh;
    sm::SourceMapRuntime rollback(bh);assert(rollback.load_bsp(bsp.data(),bsp.size()).code==sm::SourceMapLoadCode::HostKeyValueFailed&&!rollback.loaded()&&!bh.attached_count());
    assert(bad.symbol_constructs==bad.symbol_destroys&&bad.symbol_constructs==2&&bad.count_exp.stage==npc::ExpeditorStage::Retired&&bad.jessica_exp.stage==npc::ExpeditorStage::Retired);
}
}
