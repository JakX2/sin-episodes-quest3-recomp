#include "native_server_npc_run_animation.h"
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
using namespace sin_port::native_weapon_tables;
struct F {
    alignas(4) unsigned char actor[0xe0c]{},rows[0x80]{};
    ServerStudioEvent base[1]{{.55f,20,0,"base",0}},overlay[1]{{.1f,21,0,"overlay",0}};
    alignas(4) unsigned char studio[0x180]{};
    ServerStudioEvent registered[2]{};
    bool descriptor_available=true;
    std::vector<int> trace;
    bool available=true;int choice=-1;
    template<class T> void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T> T get(void* p,unsigned off){T v{};std::memcpy(&v,static_cast<unsigned char*>(p)+off,sizeof(v));return v;}
    static F& self(void* p){return *static_cast<F*>(p);}
    static std::uint32_t word(void* p){return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(p));}
    void clocks(){put(actor,0x28,9.875f);put(actor,0x2c,10.f);put(actor,0x3e8,.5f);put(actor,0x398,.5f);put(rows,0xc,0.f);put(rows,0x38,0.f);}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto get=reinterpret_cast<GetServerNpcRunAnimationApi>(GetProcAddress(server,server_npc_run_animation_export));
    auto go=reinterpret_cast<GetServerAnimationLayerOwnerApi>(GetProcAddress(server,server_animation_layer_owner_export));
    assert(get&&go&&!GetProcAddress(client,server_npc_run_animation_export)&&!get(0,sizeof(ServerNpcRunAnimationApi))&&!get(1,0));
    auto* api=get(1,sizeof(ServerNpcRunAnimationApi));auto* owner=go(1,sizeof(ServerAnimationLayerOwnerApi));assert(api&&owner);
    F f;ServerNpcRunAnimationBindings b{};b.context=&f;
    b.model_available=[](void* p,void* a) noexcept -> int{auto& f=F::self(p);assert(a==f.actor);f.trace.push_back(1);return f.available;};
    b.debug_update=[](void* p,void* a,std::uint32_t c) noexcept{auto& f=F::self(p);assert(a==f.actor&&c==0x1234);f.trace.push_back(4);};
    b.select_sequence=[](void* p,void* a,int activity,int current,bool,int* out) noexcept -> int{auto& f=F::self(p);assert(a==f.actor&&activity==5&&current==9);f.trace.push_back(5);*out=f.choice;return 1;};
    b.set_sequence=[](void* p,void* a,int seq) noexcept -> int{auto& f=F::self(p);assert(a==f.actor&&seq==11);f.trace.push_back(6);f.put(a,0x3ec,seq);f.put(a,0x3e8,0.f);f.put(a,0x398,0.f);return 1;};
    auto& adv=b.advance;adv.context=&f;adv.base.context=&f;adv.layers.context=&f;
    adv.layers.vectors.context=&f;
    adv.layers.vectors.storage=[](void* p,std::uint32_t w,void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);if(w!=F::word(f.rows))return 0;*out=f.rows;*n=sizeof(f.rows);return 1;};
    adv.layers.vectors.resize=[](void*,std::uint32_t,unsigned,void**,unsigned*) noexcept -> int{return 0;};
    adv.base.vectors=adv.layers.vectors;
    adv.base.duration=[](void*,void*,int seq,double* out) noexcept -> int{assert(seq==9);*out=2.;return 1;};
    adv.base.finish_threshold=[](void*,void*,int seq,double* out) noexcept -> int{assert(seq==9);*out=.9;return 1;};
    adv.base.movement_distance=[](void*,void*,int seq,double* out) noexcept -> int{assert(seq==9);*out=120.;return 1;};
    adv.base.invalidate=[](void* p,void*,unsigned flags) noexcept -> int{assert(flags==8);F::self(p).trace.push_back(2);return 1;};
    adv.base.cache=[](void*,void*,std::uint32_t,float) noexcept -> int{return 1;};
    adv.layers.cycle_rate=[](void* p,void*,int seq,double* out) noexcept -> int{assert(seq==7);F::self(p).trace.push_back(3);*out=1.;return 1;};
    b.events.context=&f;b.events.storage=adv.layers.vectors.storage;
    b.events.sequence=[](void* p,void* a,int seq,ServerAnimationEventSequence* out) noexcept -> int{auto& f=F::self(p);assert(a==f.actor);if(seq==11){*out={};return 1;}assert(seq==7||seq==9);*out={seq==9?f.base:f.overlay,1,1,0};return 1;};
    b.events.cycle_rate=[](void*,void*,int,double* out) noexcept -> int{*out=1.;return 1;};
    b.events.emit=[](void* p,void* a,int layer,const ServerAnimationEvent* e) noexcept{auto& f=F::self(p);assert(a==f.actor&&e->source==a);assert((layer==-1&&e->event==20)||(layer==0&&e->event==21));f.trace.push_back(e->event);};
    f.put(f.actor,0x494,F::word(f.rows));f.put(f.actor,0x498,2);f.put(f.actor,0x4a0,2);f.put(f.actor,0x4a4,F::word(f.rows));
    f.put(f.rows,0x34,15);f.put(f.rows+0x40,0x34,15);int layer=-1;
    assert(owner->allocate(f.actor,sizeof(f.actor),7,0,10,1,&adv.layers.vectors,&layer)&&layer==0);
    f.put(f.actor,0x3c0,2.f);f.put(f.actor,0x3ec,9);f.actor[0x1f0]=1;f.put(f.actor,0xe08,7u);
    f.put(f.actor,0x8f8,0x1234u);f.put(f.actor,0x850,4);f.put(f.actor,0x878,1);f.put(f.actor,0x87c,5);f.put(f.actor,0x884,9);
    f.clocks();ServerNpcRunAnimationInput in{10.125f,false,true};ServerNpcRunAnimationResult out{};
    f.available=false;assert(api->run(f.actor,sizeof(f.actor),&in,&b,&out)&&!out.advanced&&!out.events_dispatched&&(f.trace==std::vector<int>{1}));
    f.available=true;f.trace.clear();
    assert(api->run(f.actor,sizeof(f.actor),&in,&b,&out)&&out.advanced&&out.events_dispatched&&!out.selection_attempted&&out.events.emitted==2);
    assert((f.trace==std::vector<int>{1,2,3,4,20,21})&&f.get<unsigned>(f.actor,0xe08)==7);
    f.clocks();f.trace.clear();f.actor[0x3e0]=1;f.actor[0x3e1]=1;f.put(f.actor,0x850,0);
    in.clear_npc_flags=true;
    assert(api->run(f.actor,sizeof(f.actor),&in,&b,&out)&&out.selection_attempted&&!out.sequence_set&&out.events.emitted==2&&f.get<unsigned>(f.actor,0xe08)==4);
    assert((f.trace==std::vector<int>{1,2,3,4,5,20,21}));
    f.clocks();f.trace.clear();f.choice=11;
    assert(api->run(f.actor,sizeof(f.actor),&in,&b,&out)&&out.sequence_set&&out.selected==11&&out.events.emitted==1);
    assert((f.trace==std::vector<int>{1,2,3,4,5,6,21})&&f.get<int>(f.actor,0x3ec)==11);

    auto gn=reinterpret_cast<GetServerNpcNativeSequenceFrameApi>(GetProcAddress(server,server_npc_native_sequence_frame_export));
    assert(gn&&!GetProcAddress(client,server_npc_native_sequence_frame_export)&&!gn(0,sizeof(ServerNpcNativeSequenceFrameApi))&&!gn(1,0));
    auto* native=gn(1,sizeof(ServerNpcNativeSequenceFrameApi));assert(native);
    struct ChoiceFixture {
        F* f;
        ServerSequenceChoiceEntry entries[12]{};
    } choice{&f};
    choice.entries[9]={5,1};choice.entries[11]={5,4};
    ServerNpcNativeSequenceBindings nb{};
    nb.frame=b;nb.frame.select_sequence=nullptr;nb.frame.set_sequence=nullptr;
    nb.context=&choice;nb.choice.context=&choice;
    nb.choices=[](void* p,void* a,const ServerSequenceChoiceEntry** e,unsigned* n) noexcept -> int {
        auto& c=*static_cast<ChoiceFixture*>(p);assert(a==c.f->actor);c.f->trace.push_back(30);*e=c.entries;*n=12;return 1;
    };
    nb.choice.random=[](void* p,int lo,int hi,int seq,int* out) noexcept -> int {
        auto& c=*static_cast<ChoiceFixture*>(p);assert(lo==0&&((seq==9&&hi==0)||(seq==11&&hi==4)));
        c.f->trace.push_back(31);*out=0;return 1;
    };
    nb.reset.context=&f;
    nb.reset.duration=[](void* p,void* a,int seq,double* out) noexcept -> int {
        auto& f=F::self(p);assert(a==f.actor&&seq==11);f.trace.push_back(32);*out=2.;return 1;};
    nb.reset.distance=[](void* p,void*,int seq,double* out) noexcept -> int {assert(seq==11);F::self(p).trace.push_back(33);*out=80.;return 1;};
    nb.reset.flags=[](void* p,void*,int seq,unsigned* out) noexcept -> int{assert(seq==11);F::self(p).trace.push_back(34);*out=0;return 1;};
    nb.reset.register_events=[](void* p,void* a,int seq) noexcept -> int {
        auto& f=F::self(p);assert(a==f.actor&&seq==11&&f.get<float>(a,0x398)==0.f&&f.get<unsigned>(a,0x464)==0);
        f.trace.push_back(35);return 1;};
    f.clocks();f.trace.clear();f.put(f.actor,0x3ec,9);f.put(f.actor,0x3c0,2.f);
    f.actor[0x3e0]=1;f.actor[0x3e1]=1;f.put(f.actor,0x464,7u);f.put(f.actor,0x468,5u);
    ServerNpcNativeSequenceFrameResult nr{};
    assert(native->run(f.actor,sizeof(f.actor),&in,&nb,&nr)&&nr.frame.selected==11&&nr.frame.sequence_set&&nr.reset.reset&&nr.reset.sequence_changed&&!nr.reset.cycle_changed);
    assert((f.trace==std::vector<int>{1,2,3,4,30,31,31,32,33,34,35,21}));
    assert(f.get<float>(f.actor,0x3e8)==.625f&&f.get<float>(f.actor,0x394)==40.f&&f.get<float>(f.actor,0x3c0)==1.f);
    assert(f.get<unsigned>(f.actor,0x464)==0&&f.get<unsigned>(f.actor,0x468)==6&&nr.frame.events.emitted==1);
    // Nonlooping selection uses greatest weight without RNG, and the
    // old nonlooping flag causes native cycle zero before reset.
    f.clocks();f.trace.clear();f.put(f.actor,0x3ec,9);f.put(f.actor,0x3c0,2.f);
    f.actor[0x3e0]=1;f.actor[0x3e1]=0;f.put(f.actor,0x464,7u);
    assert(native->run(f.actor,sizeof(f.actor),&in,&nb,&nr)&&nr.frame.selected==11&&nr.reset.reset&&nr.reset.cycle_changed);
    assert((f.trace==std::vector<int>{1,2,3,4,30,32,33,34,35,21}));
    assert(f.get<float>(f.actor,0x3e8)==0.f);

    // No host selection, setter or event-registration implementation.
    auto gd=reinterpret_cast<GetServerEventDictionaryApi>(GetProcAddress(server,server_event_dictionary_export));
    auto gf=reinterpret_cast<GetServerNpcDictionaryFrameApi>(GetProcAddress(server,server_npc_dictionary_frame_export));
    auto gs=reinterpret_cast<GetServerSequenceDictionaryApi>(GetProcAddress(server,server_sequence_dictionary_export));
    assert(gd&&gf&&gs&&!GetProcAddress(client,server_npc_dictionary_frame_export)&&!GetProcAddress(client,server_sequence_dictionary_export));
    auto* dict=gd(1,sizeof(ServerEventDictionaryApi));auto* frame=gf(1,sizeof(ServerNpcDictionaryFrameApi));auto* setter=gs(1,sizeof(ServerSequenceDictionaryApi));
    assert(dict&&frame&&setter&&!gf(0,sizeof(ServerNpcDictionaryFrameApi))&&!gs(1,0));
    ServerEventDictionaryEntry dict_rows[32]{};ServerEventDictionary registry{dict_rows,32,0,0,0};
    assert(dict->seed(&registry));
    f.put(f.studio,0x18,2);f.put(f.studio,0x1c,0x40);
    ServerStudioEvent named[2]{{.25f,-1,0x400,"",0xc0},{.375f,-1,0x400,"",0x90}};
    std::memcpy(f.studio+0x40,named,sizeof(named));std::strcpy(reinterpret_cast<char*>(f.studio+0x100),"AE_CL_PLAYSOUND");
    std::strcpy(reinterpret_cast<char*>(f.studio+0x120),"COMBINE_AE_ALTFIRE");
    ServerNpcDictionaryFrameBindings db{};db.sequence=nb;db.sequence.reset.register_events=nullptr;
    db.dictionary.context=&f;db.dictionary.dictionary=&registry;
    db.dictionary.descriptor=[](void* p,void* a,int seq,void** storage,unsigned* bytes,unsigned* offset) noexcept -> int {
        auto& f=F::self(p);assert(a==f.actor&&seq==11&&f.get<float>(a,0x398)==0.f&&f.get<unsigned>(a,0x464)==0);
        f.trace.push_back(36);if(!f.descriptor_available)return 0;
        *storage=f.studio;*bytes=sizeof(f.studio);*offset=0;return 1;
    };
    db.sequence.frame.events.sequence=[](void* p,void* a,int seq,ServerAnimationEventSequence* out) noexcept -> int {
        auto& f=F::self(p);assert(a==f.actor);
        if(seq==11){std::memcpy(f.registered,f.studio+0x40,sizeof(f.registered));*out={f.registered,2,0,0};}
        else{assert(seq==7);*out={f.overlay,1,1,0};}
        return 1;
    };
    db.sequence.frame.events.emit=[](void* p,void* a,int layer,const ServerAnimationEvent* e) noexcept {
        auto& f=F::self(p);assert(a==f.actor&&e->source==a&&((layer==-1&&e->event==24)||(layer==0&&e->event==21)));
        f.trace.push_back(e->event);
    };
    f.clocks();f.trace.clear();f.put(f.actor,0x3ec,9);f.put(f.actor,0x3c0,2.f);
    f.actor[0x3e0]=1;f.actor[0x3e1]=1;f.put(f.actor,0x464,7u);
    ServerNpcDictionaryFrameResult dr{};
    assert(frame->run(f.actor,sizeof(f.actor),&in,&db,&dr)&&dr.frame.selected==11&&dr.sequence.reset.reset&&dr.sequence.descriptor_available&&dr.sequence.registration.created==1&&dr.sequence.registration.existing==1&&dr.frame.events.emitted==2);
    assert((f.trace==std::vector<int>{1,2,3,4,30,31,31,32,33,34,36,24,21}));
    int assigned=-1;assert(dict->lookup(&registry,"COMBINE_AE_ALTFIRE",&assigned)&&assigned==24);
    assert(f.registered[0].type==0x410&&f.registered[1].event==assigned);
    // Original reset tail early returns when studio metadata unavailable.
    f.descriptor_available=false;f.trace.clear();f.put(f.actor,0x464,7u);
    ServerSequenceDictionaryResult sr{};
    assert(setter->reset(f.actor,sizeof(f.actor),&db.sequence.reset,&db.dictionary,&sr)&&sr.reset.reset&&!sr.descriptor_available&&registry.count==25);
    assert((f.trace==std::vector<int>{32,33,34,36}));

    // Native sequence selection now consumes unresolved activity names.
    auto gaf=reinterpret_cast<GetServerNpcActivityFrameApi>(GetProcAddress(server,server_npc_activity_frame_export));
    auto gad=reinterpret_cast<GetServerActivityDictionaryApi>(GetProcAddress(server,server_activity_dictionary_export));
    assert(gaf&&gad&&!GetProcAddress(client,server_npc_activity_frame_export)&&!gaf(0,sizeof(ServerNpcActivityFrameApi))&&!gaf(1,0));
    auto* activity_frame=gaf(1,sizeof(ServerNpcActivityFrameApi));auto* activity_dict=gad(1,sizeof(ServerActivityDictionaryApi));assert(activity_frame&&activity_dict);
    ServerEventDictionaryEntry activity_rows[8]{};
    ServerActivityDictionary activities{{activity_rows,8,0,3,0},-1};
    assert(activity_dict->shared(&activities,"ACT_TEST_IDLE",5));
    struct MetadataFixture {
        F* f;
        alignas(4) unsigned char storage[11][0x80]{};
        ServerActivityDescriptor desc[12]{};
        std::uint32_t generations[2]{3,3};
        ServerActivityGenerationNode groups[2]{{generations,-1},{generations+1,0}};
        ServerActivityCompositeView view{desc,12,groups,2};
    } meta{&f};
    for(unsigned i=0;i<11;++i){
        meta.desc[i]={meta.storage[i],sizeof(meta.storage[i]),0};
        f.put(meta.storage[i],8,0x40);
        f.put(meta.storage[i],0x10,-1);
    }
    std::strcpy(reinterpret_cast<char*>(meta.storage[9]+0x40),"ACT_TEST_IDLE");
    f.put(meta.storage[9],0x14,-1);
    meta.desc[11]={f.studio,sizeof(f.studio),0};
    f.put(f.studio,8,0x150);f.put(f.studio,0x10,-1);f.put(f.studio,0x14,4);
    std::strcpy(reinterpret_cast<char*>(f.studio+0x150),"ACT_TEST_IDLE");
    f.studio[0xd]=0;
    ServerNpcActivityFrameBindings ab{};ab.frame=db;ab.frame.sequence.choices=nullptr;
    ab.activities=&activities;ab.context=&meta;
    ab.metadata=[](void* p,void* a,const ServerActivityCompositeView** v) noexcept -> int {
        auto& m=*static_cast<MetadataFixture*>(p);assert(a==m.f->actor);m.f->trace.push_back(37);*v=&m.view;return 1;
    };
    ab.frame.sequence.frame.events.sequence=[](void* p,void* a,int seq,ServerAnimationEventSequence* out) noexcept -> int {
        auto& f=F::self(p);assert(a==f.actor);
        if(seq==11){std::memcpy(f.registered,f.studio+0x40,sizeof(f.registered));*out={f.registered,2,0,0};}
        else{assert(seq==7||seq==9);*out={seq==9?f.base:f.overlay,1,1,0};}return 1;
    };
    ab.frame.sequence.frame.events.emit=[](void* p,void* a,int layer,const ServerAnimationEvent* e) noexcept {
        auto& f=F::self(p);assert(a==f.actor&&e->source==a&&((layer==-1&&(e->event==20||e->event==24))||(layer==0&&e->event==21)));
        f.trace.push_back(e->event);
    };
    f.descriptor_available=true;
    f.clocks();f.trace.clear();f.put(f.actor,0x3ec,9);f.put(f.actor,0x3c0,2.f);
    f.actor[0x3e0]=1;f.actor[0x3e1]=1;f.put(f.actor,0x464,7u);
    ServerNpcActivityFrameResult ar{};
    assert(activity_frame->run(f.actor,sizeof(f.actor),&in,&ab,&ar)&&ar.frame.frame.selected==9&&ar.frame.frame.sequence_set&&!ar.frame.sequence.reset.reset);
    assert(ar.choice.scanned==10&&ar.choice.lazy_registrations==10&&!ar.choice.refresh.refreshed&&f.studio[0xd]==0);
    assert((f.trace==std::vector<int>{1,2,3,4,37,20,21}));
    // Force stale group: full registration must precede weighted selection,
    // then the native setter and event iteration use the same registry.
    meta.generations[1]=2;f.put(meta.storage[9],0x14,1);
    f.clocks();f.trace.clear();f.put(f.actor,0x3ec,9);f.put(f.actor,0x3c0,2.f);
    f.actor[0x3e0]=1;f.actor[0x3e1]=1;f.put(f.actor,0x464,7u);
    assert(activity_frame->run(f.actor,sizeof(f.actor),&in,&ab,&ar)&&ar.frame.frame.selected==11&&ar.frame.sequence.reset.reset);
    assert(ar.choice.refresh.refreshed&&ar.choice.refresh.visited==12&&ar.choice.refresh.events_existing==2&&ar.choice.scanned==12&&!ar.choice.lazy_registrations);
    assert(meta.generations[0]==3&&meta.generations[1]==3&&f.get<int>(f.studio,0x10)==5&&ar.frame.frame.events.emitted==2);
    assert((f.trace==std::vector<int>{1,2,3,4,37,31,31,32,33,34,36,24,21}));
    FreeLibrary(server);FreeLibrary(client);
    std::puts("PASS native NPC RunAnimation metadata gate, advancement, idle selection and server events");
}
