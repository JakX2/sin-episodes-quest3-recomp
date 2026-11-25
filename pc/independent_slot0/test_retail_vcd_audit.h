#pragma once
#include "test_native_npc_scene_playback.h"
#include "response_rule_scoring.h"
#include "native_server_event_intensity.h"
#include "native_server_facial_records.h"
#include "native_server_composed_facial_think.h"
#include "native_server_named_facial_think.h"
#include "native_server_cached_facial_think.h"
#include "native_server_npc_weapon_scene.h"
#include "native_server_animation_scene.h"
#include "native_weapon_datatables.h"
#include "native_client_composed_facial.h"
#include "native_server_flex_spline.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace sin_port::native_weapon_tables::test {
// Owner-side PE32 harness. The grammar, constructors, track ownership and
// finalization are loaded from the replacement server. The allocator,
// numeric conversion and scale-symbol insertion remain explicit host leaves.
// No original binary or retail text is required for the synthetic CI case.
struct VcdAuditCounts {
    unsigned actors=0,channels=0,events=0,tracks=0,samples=0,scales=0,missing=0;
    unsigned expressions=0,flex_events=0,facial_ramps=0,intensity_evals=0,spline_evals=0;
    void add(const VcdAuditCounts& x) noexcept {
        actors+=x.actors;channels+=x.channels;events+=x.events;tracks+=x.tracks;
        samples+=x.samples;scales+=x.scales;missing+=x.missing;
        expressions+=x.expressions;flex_events+=x.flex_events;facial_ramps+=x.facial_ramps;
        intensity_evals+=x.intensity_evals;spline_evals+=x.spline_evals;
    }
};
struct VcdAuditArena {
    using F=ScenePlaybackFixture;
    using Kind=ChoreographyFlexAllocation;
    const ChoreographyConstructApi& constructors;
    const ChoreographySymbolApi& symbol_api;
    const ChoreographyFlexOwnershipApi& tracks;
    const ChoreographyDestructionApi& destruction;
    const ChoreographyScaleTreeApi& scale_tree_api;
    ChoreographyScaleTreeBindings scale_tree{};
    std::map<std::uint32_t,unsigned> live;
    const ChoreographySymbolInternApi& intern_api;
    ChoreographySymbolInternBindings intern_bindings{};
    const ChoreographyVectorApi& vectors;
    ChoreographyVectorBindings vector_bindings{};
    bool fail_resize=false;
    std::uint32_t counter=0;
    ChoreographyScaleSentinel scale_sentinel{};
    ChoreographySymbolSentinel symbol_sentinel{};
    ChoreographyFlexOwnershipBindings ownership{};
    PortableVcdFinalizedGrammarBindings bindings{};
    ChoreographyDestructionBindings retire{};
    unsigned missing=0,unexpected=0;
    const ServerEventIntensityApi* intensity=nullptr;
    const ServerFlexSplineApi* spline=nullptr;
    char last_token[128]{};
    static VcdAuditArena& self(void* p) noexcept {return *static_cast<VcdAuditArena*>(p);}
    ~VcdAuditArena(){for(const auto& x:live)std::free(reinterpret_cast<void*>(static_cast<std::uintptr_t>(x.first)));}
    VcdAuditArena(const VcdAuditArena&)=delete;
    VcdAuditArena& operator=(const VcdAuditArena&)=delete;

    static int __cdecl allocate(void* p,unsigned n,void** out,unsigned* bytes) noexcept {
        auto& a=self(p);*out=nullptr;*bytes=0;
        if(!n||n>16*1024*1024||a.live.size()>=100000)return 0;
        void* v=std::calloc(1,n);if(!v)return 0;
        try {
            if(!a.live.emplace(F::ptr(v),n).second){std::free(v);return 0;}
        }catch(...){std::free(v);return 0;}
        *out=v;*bytes=n;return 1;
    }
    static int __cdecl storage(void* p,std::uint32_t w,void** out,unsigned* bytes) noexcept {
        auto& a=self(p);const auto it=a.live.find(w);*out=nullptr;*bytes=0;
        if(it==a.live.end())return 0;
        *out=reinterpret_cast<void*>(static_cast<std::uintptr_t>(w));*bytes=it->second;return 1;
    }
    static int __cdecl release(void* p,std::uint32_t w) noexcept {
        if(!w)return 1;
        auto& a=self(p);const auto it=a.live.find(w);if(it==a.live.end())return 0;
        a.live.erase(it);std::free(reinterpret_cast<void*>(static_cast<std::uintptr_t>(w)));return 1;
    }
    static int __cdecl resize(void* p,std::uint32_t old,unsigned bytes,void** out,unsigned* extent) noexcept {
        auto& a=self(p);*out=nullptr;*extent=0;
        if(a.fail_resize)return 0;
        void* source=nullptr;unsigned previous=0;
        if(old&&(!storage(p,old,&source,&previous)||previous>bytes))return 0;
        if(!allocate(p,bytes,out,extent))return 0;
        if(old){std::memcpy(*out,source,previous);release(p,old);}
        return 1;
    }
    static int __cdecl grow(void* p,void* v,unsigned extra,unsigned stride) noexcept {
        auto& a=self(p);return a.vectors.grow_vector(&a.vector_bindings,v,extra,stride);
    }
    static int __cdecl grow_memory(void* p,void* v,unsigned extra,unsigned stride) noexcept {
        auto& a=self(p);return a.vectors.grow_memory(&a.vector_bindings,v,extra,stride);
    }
    static int __cdecl number(void*,const char* s,float* out) noexcept {
        char* end=nullptr;*out=std::strtof(s,&end);return end!=s&&!*end;
    }
    VcdAuditArena(const ChoreographyConstructApi& c,const ChoreographySymbolApi& s,
        const ChoreographyFlexOwnershipApi& t,const ChoreographyDestructionApi& d,const ChoreographyScaleTreeApi& st,const ChoreographySymbolInternApi& si,const ChoreographyVectorApi& va)
        :constructors(c),symbol_api(s),tracks(t),destruction(d),scale_tree_api(st),intern_api(si),vectors(va){
        vector_bindings.context=this;vector_bindings.storage=&storage;vector_bindings.resize=&resize;
        static_assert(sizeof(void*)==4,"VCD audit requires PE32 pointers");
        auto& grammar=bindings.grammar.curves.grammar;
        auto& events=grammar.events;auto& actors=events.actors;auto& text=actors.text;
        events.context=actors.context=text.context=actors.allocation.context=this;
        text.tokenizer_word=1; // non-dereferenced parser identity in portable path
        text.unexpected=[](void* p,const char* token) noexcept -> int {
            auto& a=self(p);++a.unexpected;std::strncpy(a.last_token,token,sizeof(a.last_token)-1);return 0;
        };
        auto& alloc=actors.allocation;alloc.event_counter=&counter;alloc.allocate=&allocate;
        alloc.release_unpublished=[](void* p,void* obj) noexcept -> int{return release(p,F::ptr(obj));};
        alloc.append_parsed=[](void* p,void* root,GeneratedSceneChild k,void* child) noexcept -> int {
            auto& a=self(p);return a.vectors.append_parsed(&a.vector_bindings,root,k,child);
        };
        actors.append_channel=[](void* p,void* actor,void* channel) noexcept -> int {
            auto& a=self(p);return a.vectors.append_channel(&a.vector_bindings,actor,channel);
        };
        events.append_event=[](void* p,void* channel,void* event) noexcept -> int {
            auto& a=self(p);return a.vectors.append_event(&a.vector_bindings,channel,event);
        };
        events.number=&number;
        // Null overrides select original-derived native flex refresh and
        // legacy three-sample ramp conversion in the finalized grammar.
        events.end_changed=nullptr;events.legacy_ramp=nullptr;
        alloc.parsed.context=this;alloc.parsed.compare_word=0x22112550;
        alloc.parsed.symbol_construct=[](void* p,void* table,int grow,int initial,unsigned insensitive) noexcept -> int{
            return initial==0&&insensitive==1&&self(p).symbol_api.construct_empty(table,0x34,grow,0x2228e9b0)==1;
        };
        grammar.tags.context=this;grammar.tags.storage=&storage;grammar.tags.grow=&grow;
        auto& scales=bindings.grammar.curves.scales;
        scales.context=this;scales.sentinel=&scale_sentinel;scales.storage=&storage;
        scales.integer=[](void*,const char* s,int* out) noexcept -> int{
            char* end=nullptr;const long value=std::strtol(s,&end,10);if(end==s||*end)return 0;*out=static_cast<int>(value);return 1;
        };
        scales.intern=[](void* p,void* table,const char* key,std::uint16_t* out) noexcept -> int{
            auto& a=self(p);return a.intern_api.intern(&a.intern_bindings,table,key,out);
        };
        scale_tree.context=this;scale_tree.sentinel=&scale_sentinel;scale_tree.storage=&storage;
        scale_tree.grow_memory=&grow_memory;
        intern_bindings.context=this;intern_bindings.sentinel=&symbol_sentinel;intern_bindings.insensitive=1;
        intern_bindings.storage=&storage;intern_bindings.grow_memory=scale_tree.grow_memory;
        intern_bindings.allocate=&allocate;intern_bindings.release_unpublished=&release;
        scales.insert=[](void* p,void* tree,std::uint16_t symbol,std::uint32_t* out) noexcept -> int{
            auto& a=self(p);return a.scale_tree_api.insert(&a.scale_tree,tree,symbol,out);
        };
        ownership.context=this;ownership.vtable=0x22400590; // stored, never invoked
        ownership.allocate=[](void* p,unsigned n,Kind,void** out,unsigned* extent) noexcept -> int{return allocate(p,n,out,extent);};
        ownership.release=[](void* p,std::uint32_t w,Kind) noexcept -> int{return release(p,w);};
        ownership.storage=&storage;ownership.grow=&grow;
        auto& flex=bindings.grammar.flex;flex.context=&ownership;flex.create=tracks.create;flex.delete_aux=tracks.destroy;
        flex.samples.context=this;flex.samples.storage=&storage;flex.samples.grow=&grow;flex.samples.number=&number;
        bindings.finalize.context=this;bindings.finalize.storage=&storage;
        bindings.finalize.missing_tag=[](void* p,void*,const char*,const char*,const char*) noexcept -> int{++self(p).missing;return 1;};
        retire.context=this;retire.storage=&storage;
        retire.release=[](void* p,std::uint32_t w,ChoreographyRelease) noexcept -> int{return release(p,w);};
        retire.delete_aux=[](void* p,std::uint32_t w,unsigned flags) noexcept -> int{
            auto& a=self(p);return a.tracks.destroy(&a.ownership,w,flags);
        };
        retire.symbol_destroy=[](void* p,void* table) noexcept -> int{
            auto& a=self(p);ChoreographySymbolBindings b{};
            b.context=p;b.sentinel=&a.symbol_sentinel;b.storage=&storage;
            b.release=[](void* p,std::uint32_t w,ChoreographySymbolRelease) noexcept -> int{return release(p,w);};
            return a.symbol_api.destroy(table,0x34,&b)==1;
        };
    }
    bool rows(void* object,unsigned off,unsigned stride,void*& data,unsigned& n) noexcept {
        n=F::word(object,off+0xc);data=nullptr;
        if(n>4096||n>F::word(object,off+4))return false;
        unsigned extent=0;return !n||(storage(this,F::word(object,off),&data,&extent)&&extent>=n*stride);
    }
    bool child(void* list,unsigned i,unsigned need,void*& obj) noexcept {
        unsigned n=0;return storage(this,F::word(list,i*4),&obj,&n)&&n>=need;
    }
    static float read_float(const void* p,unsigned off) noexcept {
        float v=0;std::memcpy(&v,static_cast<const unsigned char*>(p)+off,4);return v;
    }
    bool inspect_facial(void* event,VcdAuditCounts& totals) noexcept {
        const auto type=F::word(event,0);
        if(type!=2&&type!=10)return true;
        if(type==2)++totals.expressions;else ++totals.flex_events;
        if(F::word(event,0x1a8))++totals.facial_ramps;
        if(!intensity&&!spline)return true;
        const float start=read_float(event,0x184),end=read_float(event,0x188);
        if(!std::isfinite(start)||!std::isfinite(end)||(end!=-1&&end<start))return false;
        const float duration=end==-1?0.f:static_cast<float>(static_cast<double>(end)-start);
        const float times[]={start,static_cast<float>(static_cast<double>(start)+duration*.5),end==-1?start:end};
        if(intensity){
            for(float time:times){
                float value=0;
                if(!intensity->event(event,0x3c0,time,this,&storage,&value)||!std::isfinite(value)||value<0||value>1){
                    std::fprintf(stderr,"FACIAL intensity failed type=%u time=%g\n",type,time);return false;
                }
                ++totals.intensity_evals;
            }
        }
        if(type!=10||end==-1||!spline)return true;
        void* tracks=nullptr;unsigned count=0;
        if(!rows(event,0x30c,4,tracks,count))return false;
        for(unsigned i=0;i<count;++i){
            void* track=nullptr;
            if(!child(tracks,i,0x50,track))return false;
            const auto* raw=static_cast<const unsigned char*>(track);
            const float low=read_float(track,0xc),high=read_float(track,0x10);
            for(unsigned bank=0;bank<(raw[9]?2u:1u);++bank){
                void* points=nullptr;unsigned n=0;
                if(!rows(track,0x14+bank*0x14,12,points,n))return false;
                for(float time:{0.f,duration*.5f,duration}){
                    float value=0;
                    if(!spline->sample(static_cast<const ServerFlexSample*>(points),n,low,high,duration,time,bank,&value)||
                       !std::isfinite(value)||(n&&(value<0||value>1))){
                        std::fprintf(stderr,"FACIAL spline failed track=%u bank=%u time=%g\n",i,bank,time);return false;
                    }
                    ++totals.spline_evals;
                }
            }
        }
        return true;
    }
    bool inspect(void* root,VcdAuditCounts& totals) noexcept {
        void *actors=nullptr,*channels=nullptr,*events=nullptr;
        if(!rows(root,0x14,4,actors,totals.actors)||!rows(root,0x28,4,channels,totals.channels)||
           !rows(root,0,4,events,totals.events))return false;
        unsigned linked_channels=0,linked_events=0,root_events=0;
        for(unsigned i=0;i<totals.actors;++i){
            void *a=nullptr,*links=nullptr;unsigned n=0;
            if(!child(actors,i,0x118,a)||!rows(a,0x100,4,links,n))return false;
            linked_channels+=n;
            for(unsigned j=0;j<n;++j){
                void *c=nullptr,*elinks=nullptr;unsigned en=0;
                if(!child(links,j,0x9c,c)||F::word(c,0)!=F::ptr(a)||!rows(c,0x84,4,elinks,en))return false;
                linked_events+=en;
                for(unsigned k=0;k<en;++k){
                    void* e=nullptr;
                    if(!child(elinks,k,0x3c0,e)||F::word(e,0x2b4)!=F::ptr(a)||F::word(e,0x2b8)!=F::ptr(c))return false;
                }
            }
        }
        if(linked_channels!=totals.channels)return false;
        for(unsigned i=0;i<totals.events;++i){
            void* e=nullptr;if(!child(events,i,0x3c0,e)||F::word(e,0x330)!=F::ptr(root)||!inspect_facial(e,totals))return false;
            if(!F::word(e,0x2b8))++root_events;
            void* list=nullptr;unsigned n=0;
            if(!rows(e,0x30c,4,list,n))return false;
            totals.tracks+=n;
            for(unsigned j=0;j<n;++j){
                void* t=nullptr;if(!child(list,j,0x50,t)||F::word(t,0x4c)!=F::ptr(e))return false;
                for(unsigned bank=0;bank<2;++bank){
                    void* samples=nullptr;unsigned count=0;
                    if(!rows(t,0x14+bank*0x14,12,samples,count))return false;
                    totals.samples+=count;
                }
            }
        }
        totals.scales=F::word(root,0x154);totals.missing=missing;
        return linked_events+root_events==totals.events&&totals.events==counter;
    }
    NativeVcdRuntimeBindings runtime() noexcept {
        NativeVcdRuntimeBindings b{};b.context=this;b.storage=&storage;b.allocate_object=&allocate;
        b.resize_memory=&resize;b.release_object=&release;b.release_memory=&release;
        b.number=&number;b.integer=bindings.grammar.curves.scales.integer;
        b.unexpected=bindings.grammar.curves.grammar.events.actors.text.unexpected;b.missing_tag=bindings.finalize.missing_tag;
        b.event_counter=&counter;b.scale_sentinel=&scale_sentinel;b.symbol_sentinel=&symbol_sentinel;
        b.scale_comparator=0x22112550;b.symbol_comparator=0x2228e9b0;b.flex_vtable=0x22400590;b.tokenizer_word=1;b.diagnostic_word=2;
        return b;
    }
    bool parse_runtime(const NativeVcdRuntimeApi& api,const char* name,const char* data,unsigned n,VcdAuditCounts& out) noexcept {
        auto b=runtime();std::uint32_t rootword=0;
        const int status=api.parse(nullptr,name,data,n,&b,&rootword);
        void* root=nullptr;unsigned extent=0;
        const bool inspected=status==1&&storage(this,rootword,&root,&extent)&&inspect(root,out);
        const int destroyed=rootword?api.destroy(rootword,&b):0;
        if(!inspected||destroyed!=1||!live.empty()){
            std::fprintf(stderr,"VCD FAIL %s parse=%d inspect=%d destroy=%d live=%u unexpected=%u token=%s\n",
                name,status,inspected?1:0,destroyed,static_cast<unsigned>(live.size()),unexpected,last_token);
            return false;
        }
        return true;
    }
    bool parse(const PortableVcdFinalizedGrammarApi& grammar,const char* name,const char* data,unsigned n,VcdAuditCounts& out) noexcept {
        void* root=nullptr;unsigned extent=0;
        if(!allocate(this,0x220,&root,&extent))return false;
        if(constructors.parsed(root,extent,nullptr,&bindings.grammar.curves.grammar.events.actors.allocation.parsed)!=1)return false;
        const int status=grammar.parse(root,extent,name,data,n,&bindings);
        const bool inspected=status==1&&inspect(root,out);
        const int destroyed=destruction.parsed(root,extent,&retire);
        if(!inspected||destroyed!=1||!live.empty()){
            std::fprintf(stderr,"VCD FAIL %s parse=%d inspect=%d destroy=%d live=%u unexpected=%u token=%s\n",
                name,status,inspected?1:0,destroyed,static_cast<unsigned>(live.size()),unexpected,last_token);
            return false;
        }
        return true;
    }
};
// Exercise the composed runtime through the named NPC engine boundary,
// with real parsed graphs instead of manually populated parsed objects.
struct VcdOwnedPlaybackFixture {
    using F=ScenePlaybackFixture;
    VcdAuditArena& arena;
    const ChoreographyStartupSoundApi& startup_sound;
    unsigned warmed=0;
    NativeNamedPlaybackBindings n{};
    SceneManagerSession manager{};
    alignas(4) unsigned char scene[0x5ec]{},npc[0x604]{};
    unsigned phase=0,locks=0,unlocks=0,sounds=0,stops=0;
    unsigned locked=0,cache_index=0;
    std::uint32_t root_word=0,child_word=0;
    inline static constexpr char root_text[]=R"VCD(actor Jessica {
 channel Voice {
  event speak greeting {
   time 0 2
   param Jessica.Greeting
  }
  event subscene response {
   time 2 4
   param "Scenes/Child"
  }
 }
}
scalesettings { RampTool 75 }
)VCD";
    inline static constexpr char child_text[]=R"VCD(event speak reply {
 time 0 1
 param Jessica.Reply
}
)VCD";
    static VcdOwnedPlaybackFixture& self(void* p) noexcept {return *static_cast<VcdOwnedPlaybackFixture*>(p);}
    static int __cdecl resolve(void* p,std::uint32_t word,void** out,unsigned* extent) noexcept {
        return self(p).arena.storage(&self(p).arena,word,out,extent);
    }
    static int __cdecl event(void* p,void* root,unsigned i,void** out,unsigned* extent) noexcept {
        auto& f=self(p);void* rows=nullptr;unsigned bytes=0;
        if(i>=F::word(root,0xc)||!resolve(p,F::word(root,0),&rows,&bytes)||bytes<(i+1)*4)return 0;
        return resolve(p,F::word(rows,i*4),out,extent);
    }
    static int __cdecl actor(void* p,void* scene,unsigned i,void** out,unsigned* extent) noexcept {
        auto& f=self(p);if(scene!=f.scene||i)return 0;
        *out=f.npc;*extent=sizeof(f.npc);return 1;
    }
    static int __cdecl grow(void* p,void* vector,unsigned extra) noexcept {
        auto& a=self(p).arena;return a.vectors.grow_vector(&a.vector_bindings,vector,extra,4);
    }
    explicit VcdOwnedPlaybackFixture(VcdAuditArena& a,const ChoreographyStartupSoundApi& s):arena(a),startup_sound(s){
        n.context=this;n.precache.context=this;
        auto& load=n.precache.load;load.context=this;load.cache.context=this;
        load.set_extension=[](void*,char* text,const char* ext,unsigned cap) noexcept -> int{
            const auto len=std::strlen(text),n=std::strlen(ext);
            if(len+n>=cap)return 0;std::memcpy(text+len,ext,n+1);return 1;
        };
        load.missing=[](void*,const char*) noexcept -> int{assert(false);return 0;};
        load.cache.ensure=[](void* p,const char* key) noexcept -> int{
            auto& f=self(p);
            if(!std::strcmp(key,"scenes\\root.vcd"))f.cache_index=1;
            else if(!std::strcmp(key,"scenes\\child.vcd"))f.cache_index=2;
            else return 0;
            return !f.locked;
        };
        load.cache.find=[](void* p,const char*,std::uint32_t* out) noexcept -> int{*out=self(p).cache_index;return 1;};
        load.cache.lock=[](void* p,std::uint32_t idx,const void** out,unsigned* extent) noexcept -> int{
            auto& f=self(p);if(f.locked||idx!=f.cache_index||idx<1||idx>2)return 0;
            f.locked=idx;++f.locks;
            *out=idx==1?root_text:child_text;*extent=idx==1?sizeof(root_text)-1:sizeof(child_text)-1;return 1;
        };
        load.cache.reload=[](void*,const char*,std::uint32_t,std::uint32_t*) noexcept -> int{assert(false);return 0;};
        load.cache.unlock=[](void* p,std::uint32_t idx) noexcept -> int{
            auto& f=self(p);if(idx!=f.locked)return 0;f.locked=0;++f.unlocks;return 1;
        };
        n.precache.text=[](void* p,std::uint32_t w,const char** out) noexcept -> int{
            auto& f=self(p);if(w!=F::ptr(f.scene+0x568))return 0;
            *out=reinterpret_cast<char*>(f.scene+0x568);return 1;
        };
        n.precache.precache_sound=[](void* p,const char* sound) noexcept -> int{
            auto& f=self(p);assert(f.phase==2&&!f.locked&&f.sounds<2);
            assert(!std::strcmp(sound,f.sounds?"Jessica.Reply":"Jessica.Greeting"));++f.sounds;return 1;
        };
        auto& q=n.playback;q.context=this;
        q.create=[](void* p,const char* cls,void** out,unsigned* extent) noexcept -> int{
            auto& f=self(p);assert(!f.phase&&!std::strcmp(cls,"instanced_scripted_scene"));
            f.scene[0x5e8]=1;*out=f.scene;*extent=sizeof(f.scene);return 1;
        };
        q.resolve_name=[](void* p,void* npc,const char* name,char* out,unsigned cap) noexcept -> int{
            auto& f=self(p);assert(npc==f.npc&&!std::strcmp(name,"Jessica")&&cap>=12);
            std::strcpy(out,"Scenes/Root");return 1;
        };
        q.actor_handle=[](void* p,void* npc,std::uint32_t* out) noexcept -> int{
            assert(npc==self(p).npc);*out=0x321;return 1;
        };
        q.set_delay=[](void* p,void* scene,float delay) noexcept -> int{
            auto& f=self(p);assert(scene==f.scene&&!f.phase&&delay==.5f&&F::word(scene,0x55c)==0x321);
            f.phase=1;return 1;
        };
        n.register_with_spawn=[](void* p,void* scene,void* context,NamedSpawnCallback spawn) noexcept -> int{
            auto& f=self(p);assert(scene==f.scene&&f.phase==1);f.phase=2;
            if(!spawn(context,scene))return 0;f.phase=3;return 1;
        };
        q.missing=[](void*,const char*,unsigned) noexcept -> int{assert(false);return 0;};
        q.resolve_parsed=&resolve;q.duration.context=this;q.duration.event=&event;
        auto& life=n.lifecycle;life.context=this;life.parsed=&resolve;life.manager=&manager;
        life.base_activate=[](void* p,void* scene) noexcept -> int{
            auto& f=self(p);assert(scene==f.scene&&f.phase==3&&f.sounds==2&&f.locks==2&&f.unlocks==2);
            f.root_word=F::word(scene,0x40c);void* root=nullptr;void* child_event=nullptr;unsigned extent=0;
            assert(resolve(p,f.root_word,&root,&extent)&&F::word(root,0xc)==2&&F::word(root,0x20)==1);
            assert(event(p,root,1,&child_event,&extent)&&F::word(child_event,0)==11);
            f.child_word=F::word(child_event,0x324);void* child=nullptr;
            assert(resolve(p,f.child_word,&child,&extent)&&static_cast<unsigned char*>(child)[0x124]&&F::word(child,0xc)==1);
            f.phase=4;return 1;
        };
        life.base_remove=[](void* p,void* scene) noexcept -> int{
            auto& f=self(p);assert(scene==f.scene&&f.phase==7&&!F::word(scene,0x40c));
            assert(!F::word(f.npc,0x5f8)&&!F::word(f.npc,0x5c4)&&f.stops==2);
            f.phase=8;return 1;
        };
        life.runtime.context=this;
        life.runtime.resolve=[](void*,std::uint32_t,void**,unsigned*) noexcept -> int{assert(false);return 0;};
        life.runtime.create=[](void*,const char* cls,int edict,void** out) noexcept -> int{
            assert(!std::strcmp(cls,"scene_manager")&&edict==-1);*out=nullptr;return 1;
        };
        life.runtime.packed_handle=[](void*,void*,std::uint32_t*) noexcept -> int{assert(false);return 0;};
        life.runtime.spawn=[](void*,void*) noexcept -> int{assert(false);return 0;};
        auto& stop=n.stop;stop.context=this;stop.parsed=&resolve;stop.actor=&actor;stop.events=q.duration;
        stop.records.context=this;stop.records.storage=&resolve;stop.records.grow=&grow;
        stop.records.stop_record=[](void* p,void* npc,void* record,int zero,int arg) noexcept -> int{
            auto& f=self(p);assert(npc==f.npc&&f.phase==7&&!zero&&!arg&&f.stops<2);
            assert(F::word(record,4)==(f.stops?f.child_word:f.root_word));++f.stops;return 1;
        };
        auto& start=n.startup;start.context=this;start.parsed=&resolve;start.actor=&actor;
        start.is_npc=[](void* p,void* npc,unsigned* out) noexcept -> int{assert(npc==self(p).npc);*out=1;return 1;};
        start.speaking=[](void*,void*,unsigned* out) noexcept -> int{*out=0;return 1;};
        start.refresh=[](void* p,void* scene) noexcept -> int{
            auto& f=self(p);assert(scene==f.scene&&f.phase==4&&f.scene[0x3d8]&&F::word(f.npc,0x5f8)==1);
            f.phase=5;return 1;
        };
        start.output=[](void* p,void* out,void* activator,void* caller,float delay) noexcept -> int{
            auto& f=self(p);assert(f.phase==5&&out==f.scene+0x41c&&activator==f.scene&&caller==f.scene&&!delay);
            f.phase=6;return 1;
        };
        start.startup_events=[](void* p,void* scene,std::uint32_t root) noexcept -> int{
            auto& f=self(p);assert(f.phase==6&&scene==f.scene&&root==f.root_word);
            ChoreographyStartupSoundBindings sounds{};
            sounds.context=&f;sounds.storage=&resolve;
            sounds.warm_sound=[](void* p,const char* sound) noexcept -> int{
                auto& f=self(p);assert(f.phase==6&&!std::strcmp(sound,"Jessica.Greeting"));++f.warmed;return 1;
            };
            assert(f.startup_sound.warm(&sounds,scene,root)==1&&f.warmed==1);
            // Synthetic injection solely to test retirement, not real startup dispatch.
            void* rows=nullptr;unsigned extent=0;
            if(!f.arena.allocate(&f.arena,0x80,&rows,&extent))return 0;
            F::put(rows,4,f.root_word);F::put(rows,0x44,f.child_word);
            F::put(f.npc,0x5b8,F::ptr(rows));F::put(f.npc,0x5bc,2);
            F::put(f.npc,0x5c4,2);F::put(f.npc,0x5c8,F::ptr(rows));
            f.phase=7;return 1;
        };
        start.reset.context=this;start.reset.events=q.duration;start.reset.storage=&resolve;start.reset.grow=&grow;
    }
    void run(const NativeVcdRuntimeApi& api){
        auto raw=arena.runtime();float duration=-1;
        assert(api.scene(npc,"Jessica",.5f,&n,&raw,&duration)==1&&duration==4.f&&phase==7);
        assert(F::word(scene,0x40c)==root_word&&root_word&&child_word&&F::word(npc,0x5f8)==1);
        assert(api.remove(scene,sizeof(scene),&n.lifecycle,&n.stop,&raw)==1&&phase==8&&stops==2);
        void* out=nullptr;unsigned bytes=0;
        assert(!resolve(this,root_word,&out,&bytes)&&!resolve(this,child_word,&out,&bytes));
        // Actor buffers remain engine-owned after removing a scene.
        assert(arena.live.size()==2&&arena.release(&arena,F::word(npc,0x5b8))&&arena.release(&arena,F::word(npc,0x5ec))&&arena.live.empty());
    }
};
// Real grammar-created pause, tracked condition and speech events. The
// dispatch observer is deliberately not the audio engine.
inline void check_native_choreography_clock_graph(VcdAuditArena& arena,const NativeVcdRuntimeApi& runtime,const NativeChoreographyClockApi& clock){
    using F=ScenePlaybackFixture;
    const char data[]=R"VCD(event section pause
{
 time 1 -1
}
actor Jessica
{
 channel First
 {
  event gesture motion
  {
   time 0 4
   resumecondition
  }
 }
 channel Voice
 {
  event speak line
  {
   time 1.05 3
   param Jessica.Line
  }
 }
}
)VCD";
    alignas(4) unsigned char scene[0x5ec]{};
    auto b=arena.runtime();std::uint32_t rootword=0;
    assert(runtime.parse(scene+0x410,"clock.vcd",data,sizeof(data)-1,&b,&rootword)==1&&rootword);
    void* root=nullptr;unsigned extent=0;
    assert(arena.storage(&arena,rootword,&root,&extent)&&extent>=0x220);
    struct Trace {void* root;unsigned count=0,slots[32]{},types[32]{};} trace{root};
    NativeChoreographyClockBindings tick{};tick.vectors=arena.vector_bindings;tick.release_memory=&VcdAuditArena::release;
    tick.context=&trace;tick.scene=scene;tick.scene_bytes=sizeof(scene);
    tick.other_event=[](void* p,std::uint32_t cb,unsigned slot,float,void* root,void* event) noexcept -> int{
        auto& t=*static_cast<Trace*>(p);assert(root==t.root&&cb&&t.count<32);
        t.slots[t.count]=slot;t.types[t.count++]=F::word(event,0);return 1;
    };
    F::fl(root,0x90,.1f);
    assert(clock.reset(root,extent,1,0,0,&tick)==1&&F::word(root,0x70)==1&&F::word(root,0x48)==1);
    // Zero-anchored reset; start the tracked gesture, then cross the
    // point pause at 1. The pending speech child remains unique.
    assert(clock.simulate(root,extent,.5f,&tick)==1);
    assert(clock.simulate(root,extent,1.f,&tick)==1);
    assert(F::word(root,0x5c)==1&&F::word(root,0x21c)==0);
    void *events=nullptr,*pause=nullptr;unsigned n=0;
    assert(arena.storage(&arena,F::word(root,0),&events,&n)&&n>=12);
    assert(arena.storage(&arena,F::word(events,0),&pause,&n)&&F::word(pause,0x3b0)==1);
    // Resume consumes a native snapshot and replays the attached speech.
    const auto before=trace.count;
    assert(clock.resume(root,extent,&tick)==1&&trace.count==before+1&&trace.types[before]==5&&trace.slots[before]==0);
    assert(F::word(root,0x21c)==0xffffffffu&&!F::word(root,0x5c));
    assert(runtime.destroy(rootword,&b)==1&&arena.live.empty());
}
// Exercise real parser -> clock -> native speech callback -> native
// preparation, language, caption and actor record lifecycle. Only the
// external engine effects are synthetic, not the callback dispatch.

inline void check_native_response_history_tree(VcdAuditArena& a,const ResponseHistoryTreeApi& api,
    const ResponsePolicyApi& policy){
    using F=ScenePlaybackFixture;
    alignas(4) unsigned char exp[0x7c]{};
    F::put(exp,0x20,0xffffffffu);F::put(exp,0x28,0xffffffffu);
    assert(a.symbol_api.construct_empty(exp+0x34,0x34,0,0x2228e9b0)==1);
    struct State{float now=10;unsigned notifications=0;} state;
    ResponseHistoryTreeBindings b{};b.memory=a.vector_bindings;b.symbols=a.intern_bindings;b.context=&state;
    b.clock=[](void* p,float* out) noexcept -> int{*out=static_cast<State*>(p)->now;return 1;};
    b.notify=[](void* p,void*,const char*) noexcept -> int{++static_cast<State*>(p)->notifications;return 1;};
    ResponseHistoryBindings h{};assert(api.adapt(&b,&h));
    auto lookup=[&](const char* name){int i=-2;assert(h.lookup(h.context,exp,name,&i));return i;};
    auto seed=[&](const char* name){assert(policy.seed(exp,sizeof(exp),&h,name)==1);};
    auto entry=[&](int i){void* row=nullptr;unsigned n=0;assert(h.entry(h.context,exp,i,&row,&n)&&n==28);return row;};
    unsigned has=99;assert(policy.has_concept(exp,sizeof(exp),&h,"TLK_HELLO",&has)==1&&!has);
    assert(!F::word(exp,0x24)&&((F::word(exp+0x34,0x10)&0xffffu)==0));
    assert(lookup("tlk_hello")==-1);
    assert(lookup("Zed")==-1&&lookup("Alpha")==-1&&lookup("Mid")==-1);
    seed("Alpha");seed("TLK_HELLO");seed("Zed");seed("Mid");
    assert(F::word(exp,0x24)==4&&F::word(exp,0x2c)==4&&F::word(exp,0x18)==4&&state.notifications==4);
    assert(lookup("alpha")==0&&lookup("tlk_hello")==1&&lookup("ZED")==2&&lookup("Mid")==3);
    auto* hello=entry(1);assert(VcdAuditArena::read_float(hello,0x14)==10);
    F::put(hello,0x18,123u);state.now=20;seed("tlk_HELLO");
    assert(F::word(entry(1),0x18)==123&&VcdAuditArena::read_float(entry(1),0x14)==20&&F::word(exp,0x24)==4);
    int i=-2;assert(!h.insert(h.context,exp,"TLK_HELLO",30,&i)&&F::word(exp,0x24)==4);
    assert(lookup("Fifth")==-1);a.fail_resize=true;
    const auto root=F::word(exp,0x20),backing=F::word(exp,0x14);
    assert(!h.insert(h.context,exp,"Fifth",30,&i)&&F::word(exp,0x20)==root&&F::word(exp,0x14)==backing&&F::word(exp,0x24)==4);
    a.fail_resize=false;seed("Fifth");assert(F::word(exp,0x18)==8&&lookup("Fifth")==4);
    void* rows=nullptr;unsigned bytes=0;assert(a.storage(&a,F::word(exp,0x14),&rows,&bytes));
    for(unsigned j=0;j<5;++j){auto* row=static_cast<unsigned char*>(rows)+j*28;F::put(row,0,j);F::put(row,4,j==4?0xffffffffu:j+1);F::put(row,24,0u);}
    F::put(exp,0x20,0xffffffffu);F::put(exp,0x24,0u);F::put(exp,0x28,0u);
    const auto retained=F::word(exp,0x14);
    seed("Mid");assert(lookup("Mid")==0&&F::word(exp,0x28)==1&&F::word(exp,0x2c)==5&&F::word(exp,0x14)==retained);
    void* invalid=nullptr;unsigned n=0;assert(!h.entry(h.context,exp,1,&invalid,&n));
    assert(a.retire.symbol_destroy(&a,exp+0x34)&&a.release(&a,retained)&&a.live.empty());
}


inline void check_managed_response_history(VcdAuditArena& a,const ResponseHistoryManagedApi& api,
    const ResponsePolicyApi& policy,const TalkManagerApi& manager,const TalkCatalogApi& catalog){
    using F=ScenePlaybackFixture;
    ManagedTalkFixture f(manager,catalog);auto& p=f.policy;auto& m=f.manager;
    F::put(f.exp,0x20,0xffffffffu);F::put(f.exp,0x28,0xffffffffu);
    assert(a.symbol_api.construct_empty(f.exp+0x34,0x34,0,0x2228e9b0)==1);
    struct Context {
        ManagedTalkFixture& f;
        SceneSpeechActorKind kind=SceneSpeechActorKind::Jessica;
        unsigned clocks=0;
    } context{f};
    ResponseHistoryManagedBindings b{};
    b.tree.memory=a.vector_bindings;b.tree.symbols=a.intern_bindings;b.tree.context=&context;
    b.tree.clock=[](void* p,float* out) noexcept -> int{
        auto& c=*static_cast<Context*>(p);++c.clocks;*out=c.f.policy.now;return 1;};
    b.manager=f.b;b.context=&context;
    b.storage=[](void* p,std::uint32_t word,void** out,unsigned* n) noexcept -> int{
        auto& f=static_cast<Context*>(p)->f;
        if(word!=F::ptr(f.policy.npc))return 0;
        *out=f.policy.npc;*n=sizeof(f.policy.npc);return 1;};
    b.kind=[](void* p,void* actor,SceneSpeechActorKind* kind) noexcept -> int{
        auto& c=*static_cast<Context*>(p);assert(actor==c.f.policy.npc);*kind=c.kind;return 1;};
    ResponseHistoryBindings h{};assert(api.adapt(&b,&h));
    p.lo=5;p.hi=10;p.draw=7;
    assert(policy.seed(f.exp,sizeof(f.exp),&h,"TLK_HELLO")==1);
    assert(context.clocks==2&&m.creations==1&&m.inserts==48&&VcdAuditArena::read_float(m.object,0x394)==17.f);
    p.expect({4,5,2,1});
    unsigned has=0;assert(policy.has_concept(f.exp,sizeof(f.exp),&h,"tlk_hello",&has)==1&&has);
    int idx=-1;void* row=nullptr;unsigned n=0;
    assert(h.lookup(h.context,f.exp,"TLK_HELLO",&idx)&&idx==0&&h.entry(h.context,f.exp,idx,&row,&n)&&n==28);
    assert(VcdAuditArena::read_float(row,0x14)==10.f&&!F::word(row,0x18));
    // Non-Jessica sink does not touch manager or propagate, but native
    // concept history is still committed by the caller.
    context.kind=SceneSpeechActorKind::JC;p.now=20;
    assert(policy.seed(f.exp,sizeof(f.exp),&h,"TLK_HELLO")==1&&context.clocks==3);
    assert(VcdAuditArena::read_float(row,0x14)==20.f&&VcdAuditArena::read_float(m.object,0x394)==17.f&&!p.count);
    context.kind=SceneSpeechActorKind::Jessica;p.now=25;p.fail=4;
    assert(policy.seed(f.exp,sizeof(f.exp),&h,"TLK_HELLO")==-1);
    assert(VcdAuditArena::read_float(row,0x14)==25.f&&VcdAuditArena::read_float(m.object,0x394)==17.f);
    p.expect({4});p.fail=0;
    assert(manager.destroy(&m.session,&m.control,&m.tables,&m.b)==1);
    assert(a.retire.symbol_destroy(&a,f.exp+0x34)&&a.release(&a,F::word(f.exp,0x14))&&a.live.empty());
}


inline void check_native_talk_deadline_tree(VcdAuditArena& a,const TalkDeadlineTreeApi& tree,
    const TalkManagerApi& manager,const TalkCatalogApi& catalog){
    using F=ScenePlaybackFixture;
    struct Context {
        VcdAuditArena& a;const TalkDeadlineTreeApi& tree;const TalkCatalogApi& catalog;
        TalkDeadlineTreeBindings cfg{};unsigned visits=0;
    } c{a,tree,catalog};
    TalkManagerFixture m(manager,catalog);
    c.cfg.memory=a.vector_bindings;c.cfg.session=&m.session;c.cfg.context=&c;
    c.cfg.intern=[](void* p,const char* key,const char** out) noexcept -> int{
        auto& c=*static_cast<Context*>(p);const auto* d=c.catalog.lookup(key);
        if(!d)return 0;*out=d->name;return 1;};
    // Manager construction and lifecycle remain the original native API.
    assert(manager.construct(&m.session,&m.control,&m.tables,&m.b)==1);
    TalkManagerBindings b{};b.context=&c;
    b.insert_deadline=[](void* p,void* object,const char* name,float initial) noexcept -> int{
        auto& c=*static_cast<Context*>(p);assert(initial==-1&&c.visits<48&&!std::strcmp(name,c.catalog.at(c.visits)->name));
        ++c.visits;return c.tree.insert(&c.cfg,object,name,initial);};
    assert(manager.populate(&m.session,&m.control,&b)==1&&c.visits==48);
    assert(F::word(m.object,0x3a8)==48&&m.object[0x3b1]==48&&m.object[0x3b3]==48);
    assert(m.object[0x3b2]==0xff&&F::word(m.object,0x3b4)==F::word(m.object,0x3a4));
    auto find=[&](const char* key){float* result=nullptr;assert(tree.deadline(&c.cfg,key,&result));return result;};
    for(unsigned i=0;i<48;++i){auto* v=find(catalog.at(i)->name);assert(v&&*v==-1);}
    assert(!find("missing")&&!find(nullptr)&&!find(""));
    float* hello=find("TLK_HELLO");assert(hello==find("tlk_hello"));*hello=17.f;
    assert(*find("TLK_IDLE")==-1.f&&*find("tlk_hello")==17.f);
    void* rows=nullptr;unsigned bytes=0;assert(a.storage(&a,F::word(m.object,0x3a4),&rows,&bytes)&&bytes>=48*12);
    unsigned duplicates=0;
    for(unsigned i=0;i<48;++i){
        const auto* key=reinterpret_cast<const char*>(static_cast<std::uintptr_t>(F::word(rows,i*12+4)));
        if(!std::strcmp(key,"TLK_ENEMY_DEAD"))++duplicates;
    }
    assert(duplicates==2);
    // Failed resize at 48 leaves all existing deadlines and counts unchanged.
    a.fail_resize=true;assert(!tree.insert(&c.cfg,m.object+0x3a0,"TLK_HELLO",-1));
    assert(m.object[0x3b1]==48&&m.object[0x3b3]==48&&find("TLK_HELLO")==hello&&*hello==17);
    a.fail_resize=false;
    b.free_tree=[](void* p,std::uint32_t word) noexcept -> int{return static_cast<Context*>(p)->a.release(&static_cast<Context*>(p)->a,word);};
    b.base_destroy=[](void*,void*) noexcept -> int{return 1;};
    assert(manager.destroy(&m.session,&m.control,&m.tables,&b)==1&&a.live.empty());
    float* invalid=nullptr;assert(!tree.deadline(&c.cfg,"TLK_HELLO",&invalid));
}


inline void check_native_talk_deadline_runtime(VcdAuditArena& a,const TalkDeadlineRuntimeApi& api,
    const TalkManagerRuntimeApi& runtime,const TalkManagerApi& manager,const TalkCatalogApi& catalog){
    using F=ScenePlaybackFixture;
    ManagedTalkFixture f(manager,catalog);auto& p=f.policy;auto& m=f.manager;
    TalkDeadlineRuntimeBindings cfg{};cfg.tree.memory=a.vector_bindings;cfg.tree.session=&m.session;
    cfg.tree.context=const_cast<TalkCatalogApi*>(&catalog);
    cfg.tree.intern=[](void* p,const char* name,const char** out) noexcept -> int{
        const auto* d=static_cast<const TalkCatalogApi*>(p)->lookup(name);if(!d)return 0;*out=d->name;return 1;};
    cfg.release=&VcdAuditArena::release;cfg.host=f.b;
    cfg.host.manager.insert_deadline=nullptr;cfg.host.manager.free_tree=nullptr;
    cfg.host.cooldown.concept_deadline=nullptr;
    TalkManagerRuntimeBindings adapted{};assert(api.adapt(&cfg,&adapted));
    // Factory and registration callback refer to the adapted bindings.
    m.b=adapted.manager;f.b=adapted;
    auto notify=[&](){return runtime.notify(&f.b,p.npc,sizeof(p.npc),f.exp,sizeof(f.exp),p.concept_name);};
    p.lo=5;p.hi=10;p.draw=7;
    assert(notify()==1&&m.creations==1&&!m.inserts&&m.object[0x3b1]==48);
    p.expect({4,5,2,1});
    assert(VcdAuditArena::read_float(m.object,0x394)==17.f);
    p.concept_name="TLK_PLPUSH";p.lo=p.hi=p.draw=15;
    assert(notify()==1&&m.creations==1&&p.deadline==0);p.expect({5,2,1});
    float* deadline=nullptr;assert(f.b.cooldown.concept_deadline(f.b.cooldown.context,"tlk_plpush",&deadline)&&deadline&&*deadline==25);
    unsigned ready=99;assert(catalog.concept_ready(&f.b.cooldown,"TLK_PLPUSH",&ready)==1&&!ready);p.expect({1});
    p.now=25;assert(catalog.concept_ready(&f.b.cooldown,"TLK_PLPUSH",&ready)==1&&ready);p.expect({1});
    assert(catalog.concept_ready(&f.b.cooldown,"missing",&ready)==1&&ready&&!p.count);
    p.concept_name="TLK_DANGER";p.lo=p.hi=p.draw=5;
    assert(notify()==1);p.expect({5,2,1});
    float* danger=nullptr;assert(f.b.cooldown.concept_deadline(f.b.cooldown.context,"TLK_DANGER",&danger)&&danger&&danger!=deadline&&*danger==30&&*deadline==25);
    assert(manager.destroy(&m.session,&m.control,&m.tables,&m.b)==1&&m.destroys==1&&a.live.empty());
}


inline void check_native_talk_propagation_history(VcdAuditArena& a,const TalkNativePropagationApi& api,
    const ResponseHistoryTreeApi& tree,const TalkCatalogApi& catalog){
    using F=ScenePlaybackFixture;
    struct Fixture {
        alignas(4) unsigned char npc[0x1458]{},nearby[3][0x1458]{},exp[0x7c]{};
        unsigned counts=0,candidates=0,refreshes=0,visible=0,access=0;
        float now=10;bool hidden=false;TalkCooldownState global{};
        static Fixture& self(void* p){return *static_cast<Fixture*>(p);}
    } f;
    F::put(f.npc,0xb8,0x800);F::put(f.nearby[0],0xb8,0x800);
    F::fl(f.nearby[0],0x2bc,999);F::fl(f.nearby[1],0x2bc,1000);F::fl(f.nearby[2],0x2bc,100);
    F::put(f.nearby[0],0xffc,F::ptr(f.exp));F::put(f.exp,4,F::ptr(f.nearby[0]));
    F::put(f.exp,0x20,0xffffffffu);F::put(f.exp,0x28,0xffffffffu);
    assert(a.symbol_api.construct_empty(f.exp+0x34,0x34,0,0x2228e9b0)==1);
    ResponseHistoryTreeBindings hb{};hb.memory=a.vector_bindings;hb.symbols=a.intern_bindings;hb.context=&f;
    hb.clock=[](void* p,float* out) noexcept -> int{*out=Fixture::self(p).now;return 1;};
    hb.notify=[](void*,void*,const char*) noexcept -> int{assert(false);return 0;};
    TalkNativePropagationCooldownBindings cfg{};auto& b=cfg.propagation;
    assert(tree.adapt(&hb,&b.history));b.context=&f;
    b.extent=[](void* p,void* actor,unsigned* out) noexcept -> int{
        auto& f=Fixture::self(p);assert(actor==f.npc||actor==f.nearby[0]);*out=sizeof(f.npc);return 1;};
    b.storage=[](void* p,std::uint32_t word,void** out,unsigned* n) noexcept -> int{
        auto& f=Fixture::self(p);++f.access;if(word!=F::ptr(f.exp))return 0;*out=f.exp;*n=sizeof(f.exp);return 1;};
    auto& e=b.enumeration;e.context=&f;
    e.list=[](void* p,void** out) noexcept -> int{*out=p;return 1;};
    e.count=[](void* p,int* out) noexcept -> int{++Fixture::self(p).counts;*out=5;return 1;};
    e.candidate=[](void* p,void* token,unsigned i,void** out,unsigned* n) noexcept -> int{
        auto& f=Fixture::self(p);assert(token==p&&i<5);++f.candidates;*out=i==0?nullptr:i==1?f.npc:f.nearby[i-2];*n=sizeof(f.npc);return 1;};
    e.refresh=[](void* p,void* actor) noexcept -> int{auto& f=Fixture::self(p);assert(actor==f.npc||actor==f.nearby[0]);++f.refreshes;F::put(actor,0xb8,0u);return 1;};
    e.visible=[](void* p,void* source,void* actor,unsigned mask,void* blocker,unsigned* out) noexcept -> int{
        auto& f=Fixture::self(p);assert(source==f.npc&&(actor==f.nearby[0]||actor==f.nearby[2])&&mask==0x4081&&!blocker);
        ++f.visible;*out=actor==f.nearby[0]&&!f.hidden;return 1;};
    cfg.host.context=&f;cfg.host.clock=hb.clock;
    cfg.host.random=[](void*,float lo,float hi,float* out) noexcept -> int{assert(lo==5&&hi==10);*out=7;return 1;};
    cfg.host.concept_deadline=[](void*,const char*,float**) noexcept -> int{assert(false);return 0;};
    cfg.host.cooldown_disabled=[](void*,unsigned* out) noexcept -> int{*out=0;return 1;};
    TalkCooldownBindings cooldown{};assert(api.adapt(&cfg,&cooldown));
    auto note=[&](){return catalog.note(f.npc,sizeof(f.npc),&f.global,&cooldown,"TLK_HELLO");};
    assert(note()==1&&f.counts==6&&f.candidates==5&&f.refreshes==2&&f.visible==2&&f.access==1&&f.global.global[0]==17);
    auto& h=b.history;int idx=-1;void* row=nullptr;unsigned n=0;
    assert(h.lookup(h.context,f.exp,"tlk_hello",&idx)&&idx==0&&h.entry(h.context,f.exp,idx,&row,&n)&&n==28);
    assert(VcdAuditArena::read_float(row,0x14)==10&&F::word(f.exp,0x24)==1);
    F::put(row,0x18,123u);f.now=20;
    assert(note()==1&&VcdAuditArena::read_float(row,0x14)==20&&F::word(row,0x18)==123&&f.access==2&&f.global.global[0]==27);
    f.hidden=true;f.now=30;
    assert(note()==1&&VcdAuditArena::read_float(row,0x14)==20&&f.access==2&&f.global.global[0]==37);
    f.hidden=false;F::put(f.exp,4,0u);
    assert(note()==-1&&f.access==3&&f.global.global[0]==37&&VcdAuditArena::read_float(row,0x14)==20);
    F::put(row,0x18,0u);
    assert(a.retire.symbol_destroy(&a,f.exp+0x34)&&a.release(&a,F::word(f.exp,0x14))&&a.live.empty());
}


inline void check_native_response_criteria(VcdAuditArena& a,const ResponseCriteriaTreeApi& api,
    const ResponseCriteriaSelectionApi* integrated=nullptr,const ResponseSelectApi* select=nullptr,
    const ResponseSceneFilterApi* filter=nullptr,const NpcResponseCriteriaApi* npcCriteria=nullptr,const PlayerResponseCriteriaApi* playerCriteria=nullptr,const BaseResponseCriteriaApi* baseCriteria=nullptr,const ActorResponseSelectionApi* actorSelection=nullptr,const ResponseCriteriaRemovalApi* removal=nullptr,const ResponseRuleRankingApi* ranking=nullptr){
    using F=ScenePlaybackFixture;
    struct State {
        struct Child{char name[32]{},value[64]{};float weight=0;} children[48]{};
        int root=0;unsigned count=0,created=0,destroyed=0;
        static State& self(void* p){return *static_cast<State*>(p);}
    } f;
    ResponseCriteriaTreeBindings b{};b.memory=a.vector_bindings;b.symbols=a.intern_bindings;
    b.symbol_retire.context=&a;b.symbol_retire.sentinel=&a.symbol_sentinel;b.symbol_retire.storage=&VcdAuditArena::storage;
    b.symbol_retire.release=[](void* p,std::uint32_t w,ChoreographySymbolRelease) noexcept -> int{return VcdAuditArena::release(p,w);};
    b.compare_word=0x2205b1f0;b.symbol_compare_word=0x2228e9b0;b.context=&f;b.release_rows=&VcdAuditArena::release;
    b.create_root=[](void* p,const char* name,void** out) noexcept -> int{
        auto& f=State::self(p);assert(!std::strcmp(name,"CriteriaSet"));++f.created;f.count=0;*out=&f.root;return 1;};
    b.find_child=[](void* p,void* root,const char* name,int create,void** out) noexcept -> int{
        auto& f=State::self(p);assert(root==&f.root&&(create==0||create==1));
        for(unsigned i=0;i<f.count;++i)if(f.children[i].name[0]&&!_stricmp(f.children[i].name,name)){*out=&f.children[i];return 1;}
        if(!create){*out=nullptr;return 1;}
        assert(f.count<48&&std::strlen(name)<32);auto& c=f.children[f.count++];std::strcpy(c.name,name);*out=&c;return 1;};
    b.set_string=[](void*,void* child,const char* field,const char* value) noexcept -> int{
        assert(!std::strcmp(field,"value")&&std::strlen(value)<64);std::strcpy(static_cast<State::Child*>(child)->value,value);return 1;};
    b.set_float=[](void*,void* child,const char* field,float value) noexcept -> int{
        assert(!std::strcmp(field,"weight"));static_cast<State::Child*>(child)->weight=value;return 1;};
    b.destroy_root=[](void* p,void* root) noexcept -> int{auto& f=State::self(p);assert(root==&f.root);++f.destroyed;return 1;};
    alignas(4) unsigned char c[response_criteria_bytes]{};
    assert(api.construct(c,sizeof(c),&b)==1&&f.created==1&&F::word(c,0x14)==0xffffffffu&&F::word(c,0x1c)==0xffffffffu);
    assert(api.add(&b,c,"concept","TLK_IDLE",5)&&api.add(&b,c,"part","head",5)&&api.add(&b,c,"PART","torso",2));
    assert(F::word(c,0x18)==3&&F::word(c,0x20)==3&&F::word(c,0xc)==4&&f.count==2);
    int index=-1;std::uint32_t child=0;
    assert(api.find(c,sizeof(c),&b,"part",&index,&child)==1&&index>=0&&child==F::ptr(&f.children[1]));
    assert(!std::strcmp(f.children[1].value,"torso")&&f.children[1].weight==2);
    void* rows=nullptr;unsigned extent=0;assert(a.storage(&a,F::word(c,8),&rows,&extent)&&extent>=72);
    assert(F::word(rows,24+20)==child&&F::word(rows,48+20)==child);
    assert(api.find(c,sizeof(c),&b,"missing",&index,&child)==1&&index==-1&&!child);
    assert(api.add(&b,c,"state","alert",3)&&F::word(c,0x18)==4);
    a.fail_resize=true;assert(!api.add(&b,c,"extra","x",1)&&F::word(c,0x18)==4);
    a.fail_resize=false;assert(api.add(&b,c,"extra","y",1)&&F::word(c,0xc)==8);
    assert(api.destroy(c,sizeof(c),&b)==1&&f.destroyed==1&&a.live.empty());
    assert(api.destroy(c,sizeof(c),&b)==0);
    if(!integrated)return;
    assert(select);
    struct Selection {
        State& kv;const ResponseCriteriaTreeApi& tree;ResponseCriteriaTreeBindings& b;
        alignas(4) unsigned char exp[0x7c]{},response[0x124]{};
        int system=0,npc=0;unsigned finds=0,allocations=0,warnings=0,destroys=0,scene_lookups=0;
        const ResponseSceneFilterApi* filter=nullptr;ResponseSceneFilterBindings filter_bindings{};
        alignas(4) unsigned char metadata[8]{};bool missing=false;
        bool no_system=false;
        static Selection& self(void* p){return *static_cast<Selection*>(p);}
    } fsel{f,api,b};fsel.filter=filter;
    F::put(fsel.exp,4,F::ptr(&fsel.npc));
    if(filter){
        auto& fb=fsel.filter_bindings;fb.context=&fsel;
        fb.storage=[](void* p,std::uint32_t word,void** out,unsigned* n) noexcept -> int{
            auto& s=Selection::self(p);assert(word==F::ptr(&s.npc));*out=&s.npc;*n=sizeof(s.npc);return 1;};
        fb.resolve_name=[](void* p,void* npc,const char* name,char* dest,unsigned n) noexcept -> int{
            auto& s=Selection::self(p);assert(npc==&s.npc&&n==256&&!std::strcmp(name,"scenes/test.vcd"));
            std::strcpy(dest,"scenes/resolved.vcd");return 1;};
        fb.metadata=[](void* p,const char* name,void** out,unsigned* n) noexcept -> int{
            auto& s=Selection::self(p);assert(!std::strcmp(name,"scenes/resolved.vcd"));++s.scene_lookups;
            *out=s.missing?nullptr:s.metadata;*n=sizeof(s.metadata);return 1;};
        F::fl(fsel.metadata,4,2.5f);
    }
    ResponseCriteriaSelectionSession session{c,sizeof(c)};
    ResponseCriteriaSelectionBindings cfg{};cfg.criteria=b;cfg.session=&session;
    auto& h=cfg.host;h.context=&fsel;
    h.system=[](void* p,void* exp,void** out) noexcept -> int{
        auto& s=Selection::self(p);assert(exp==s.exp);*out=s.no_system?nullptr:&s.system;return 1;};
    h.npc_criteria=[](void* p,void*,void* c) noexcept -> int{
        auto& s=Selection::self(p);return s.tree.add(&s.b,c,"npc","Jessica",1);};
    h.player_criteria=[](void* p,void* c,int i) noexcept -> int{
        auto& s=Selection::self(p);assert(i==1);return s.tree.add(&s.b,c,"player","Blade",1);};
    h.allocate=[](void* p,unsigned n,void** out,unsigned* extent) noexcept -> int{
        auto& s=Selection::self(p);assert(n==0x124);++s.allocations;*out=s.response;*extent=sizeof(s.response);return 1;};
    h.find=[](void* p,void* system,void* c,void* response,void* exp,unsigned* found) noexcept -> int{
        auto& s=Selection::self(p);assert(system==&s.system&&response==s.response&&exp==s.exp);
        assert(F::word(c,0x18)==6&&s.kv.count==5);
        int idx=-1;std::uint32_t child=0;
        assert(s.tree.find(c,response_criteria_bytes,&s.b,"part",&idx,&child)==1&&idx>=0&&child==F::ptr(&s.kv.children[1]));
        assert(!std::strcmp(s.kv.children[1].value,"torso")&&s.kv.children[1].weight==5.f);
        assert(!std::strcmp(s.kv.children[0].value,"TLK_IDLE")&&!std::strcmp(s.kv.children[3].value,"Jessica")&&!std::strcmp(s.kv.children[4].value,"Blade"));
        ++s.finds;
        if(s.filter){
            // Exercise the original filter at the finder boundary, before
            // publishing a selected response. Candidate ranking remains a
            // synthetic fixture, not an implementation of the rule engine.
            unsigned allowed=99;
            if(s.filter->allow(exp,sizeof(s.exp),&s.filter_bindings,3,"scenes/test.vcd",&allowed)!=1)return 0;
            if(allowed){F::put(response,0,3u);std::strcpy(reinterpret_cast<char*>(s.response+4),"scenes/test.vcd");}
            else{
                assert(s.filter->allow(exp,sizeof(s.exp),nullptr,1,nullptr,&allowed)==1&&allowed);
                F::put(response,0,1u);std::strcpy(reinterpret_cast<char*>(s.response+4),"fallback");
            }
        }else{s.response[4]='x';s.response[5]=0;}
        *found=1;return 1;};
    h.draw=[](void*,int,int,int*) noexcept -> int{assert(false);return 0;};
    h.destroy=[](void* p,void* response) noexcept -> int{auto& s=Selection::self(p);assert(response==s.response);++s.destroys;return 1;};
    cfg.warning=[](void* p,const char*) noexcept -> int{++Selection::self(p).warnings;return 1;};
    ResponseSelectionBindings adapted{};assert(integrated->adapt(&cfg,&adapted));
    void* out=nullptr;unsigned n=0;
    auto run=[&](const char* mods){return select->select(fsel.exp,sizeof(fsel.exp),&adapted,"TLK_IDLE",mods,&out,&n);};
    assert(run("part:head,PART:torso,state:alert")==1&&out==fsel.response&&n==0x124);
    assert(!session.busy&&f.created==2&&f.destroyed==2&&fsel.finds==1&&fsel.allocations==1&&a.live.empty());
    out=&fsel;n=99;assert(run("broken")==-1&&out==&fsel&&n==99&&fsel.warnings==1);
    assert(!session.busy&&f.created==3&&f.destroyed==3&&fsel.allocations==1&&a.live.empty());
    fsel.no_system=true;assert(run(nullptr)==1&&!out&&!n&&f.created==3);
    void* active=nullptr;assert(adapted.begin_criteria(adapted.context,&active)&&active==c&&session.busy);
    void* second=nullptr;assert(!adapted.begin_criteria(adapted.context,&second)&&!second);
    assert(adapted.end_criteria(adapted.context,active)&&!session.busy&&a.live.empty());
    if(filter){
        assert(F::word(fsel.response,0)==3&&fsel.scene_lookups==1&&!std::strcmp(reinterpret_cast<char*>(fsel.response+4),"scenes/test.vcd"));
        fsel.no_system=false;F::fl(fsel.metadata,4,0);
        assert(run("part:head,PART:torso,state:alert")==1&&out==fsel.response&&F::word(fsel.response,0)==1);
        assert(!std::strcmp(reinterpret_cast<char*>(fsel.response+4),"fallback")&&fsel.scene_lookups==2&&a.live.empty());
        fsel.missing=true;F::fl(fsel.metadata,4,2.5f);
        assert(run("part:head,PART:torso,state:alert")==1&&F::word(fsel.response,0)==1&&fsel.scene_lookups==3&&a.live.empty());
    }
    if(npcCriteria){
        struct Npc {
            const ResponseCriteriaTreeApi& tree;ResponseCriteriaTreeBindings& b;
            alignas(4) unsigned char actor[0x9a4]{},player[0x2c8]{};
            unsigned enemies=0,refreshes=0,local=0,views=0,traces=0,clocks=0;
            bool hostile=true,multi=false,view=true;
            static Npc& self(void* p){return *static_cast<Npc*>(p);}
        } npc{api,b};
        NpcResponseCriteriaBindings h{};h.context=&npc;
        h.add=[](void* p,void* c,const char* k,const char* v,float w) noexcept -> int{auto& n=Npc::self(p);return n.tree.add(&n.b,c,k,v,w);};
        h.activity=[](void*,int id,const char** out) noexcept -> int{assert(id==42);*out="ACT_IDLE";return 1;};
        h.enemy=[](void* p,void* actor,unsigned* present,const char** name) noexcept -> int{auto& n=Npc::self(p);assert(actor==n.actor);++n.enemies;*present=n.hostile;*name="npc_sintekgrunt";return 1;};
        h.clock=[](void* p,float* out) noexcept -> int{++Npc::self(p).clocks;*out=12.5f;return 1;};
        h.velocity=[](void*,void*,float* xyz) noexcept -> int{xyz[0]=3;xyz[1]=4;xyz[2]=0;return 1;};
        h.weapon=[](void*,void*,unsigned* present,const char** name) noexcept -> int{*present=0;*name=nullptr;return 1;};
        h.max_clients=[](void* p,int* out) noexcept -> int{*out=Npc::self(p).multi?2:1;return 1;};
        h.local_player=[](void* p,void** out,unsigned* n) noexcept -> int{auto& s=Npc::self(p);++s.local;*out=s.player;*n=sizeof(s.player);return 1;};
        h.refresh=[](void* p,void* ent) noexcept -> int{auto& s=Npc::self(p);++s.refreshes;assert(ent==s.actor||ent==s.player);F::put(ent,0xb8,0u);if(ent==s.player){F::fl(ent,0x2bc,6);F::fl(ent,0x2c0,8);}return 1;};
        h.condition=[](void*,void*,int id,unsigned* out) noexcept -> int{assert(id==0x1f);*out=1;return 1;};
        h.in_view=[](void* p,void* player,void* actor,unsigned* out) noexcept -> int{auto& s=Npc::self(p);assert(player==s.player&&actor==s.actor);++s.views;*out=s.view;return 1;};
        h.visible=[](void* p,void*,void*,unsigned mask,unsigned* out) noexcept -> int{assert(mask==0x4081);++Npc::self(p).traces;*out=1;return 1;};
        F::put(npc.actor,0x878,42u);F::put(npc.actor,0x850,3u);
        F::put(npc.actor,0xb8,0x800u);F::put(npc.player,0xb8,0x800u);
        auto expect=[&](const char* key,const char* value){
            int idx=-1;std::uint32_t word=0;assert(api.find(c,sizeof(c),&b,key,&idx,&word)==1&&idx>=0&&word);
            auto* child=reinterpret_cast<State::Child*>(static_cast<std::uintptr_t>(word));
            assert(!std::strcmp(child->value,value)&&child->weight==1.f);};
        assert(api.construct(c,sizeof(c),&b)==1);
        assert(npcCriteria->append(npc.actor,sizeof(npc.actor),c,&h)==1&&f.count==9);
        expect("activity","ACT_IDLE");expect("npcstate","[NPCState::Combat]");expect("enemy","npc_sintekgrunt");
        expect("timesincecombat","-1");expect("speed","5.000");expect("weapon","none");
        expect("distancetoplayer","10.000000");expect("seeplayer","1");expect("seenbyplayer","1");
        assert(npc.enemies==2&&npc.refreshes==2&&npc.local==1&&npc.views==1&&npc.traces==1&&!npc.clocks);
        assert(api.destroy(c,sizeof(c),&b)==1&&a.live.empty());
        npc.hostile=false;npc.multi=true;F::put(npc.actor,0x850,7u);
        assert(api.construct(c,sizeof(c),&b)==1);
        assert(npcCriteria->append(npc.actor,sizeof(npc.actor),c,&h)==1&&f.count==7);
        expect("timesincecombat","999999.0");expect("distancetoplayer","16384");expect("seenbyplayer","0");
        assert(npc.enemies==3&&npc.local==1&&npc.views==1&&npc.traces==1&&!npc.clocks);
        assert(api.destroy(c,sizeof(c),&b)==1&&a.live.empty());
        npc.multi=false;npc.view=false;F::fl(npc.actor,0x9a0,10.f);
        assert(api.construct(c,sizeof(c),&b)==1);
        assert(npcCriteria->append(npc.actor,sizeof(npc.actor),c,&h)==1);
        expect("timesincecombat","2.500000");expect("seenbyplayer","0");
        assert(npc.clocks==1&&npc.views==2&&npc.traces==1);
        assert(npcCriteria->append(npc.actor,0x9a3,c,&h)==0);
        assert(api.destroy(c,sizeof(c),&b)==1&&a.live.empty());
    }
    if(playerCriteria){
        struct Player {
            const ResponseCriteriaTreeApi& tree;ResponseCriteriaTreeBindings& b;
            alignas(4) unsigned char actor[0x980]{},rows[16]{};
            unsigned refreshes=0,globals=0;int global_index=-1;
            static Player& self(void* p){return *static_cast<Player*>(p);}
        } player{api,b};
        PlayerResponseCriteriaBindings h{};h.context=&player;h.contexts.context=&player;
        h.contexts.add=[](void* p,void* c,const char* k,const char* v,float w) noexcept -> int{auto& s=Player::self(p);return s.tree.add(&s.b,c,k,v,w);};
        h.contexts.storage=[](void* p,std::uint32_t w,void** out,unsigned* n) noexcept -> int{auto& s=Player::self(p);assert(w==F::ptr(s.rows));*out=s.rows;*n=sizeof(s.rows);return 1;};
        h.contexts.string=[](void*,std::uint32_t w,const char** out) noexcept -> int{
            if(w==1)*out="injured";else if(w==2)*out="yes";else if(w==3)*out="mood";else return 0;return 1;};
        h.weapon=[](void*,void*,unsigned* present,const char** name) noexcept -> int{*present=1;*name="weapon_magnum";return 1;};
        h.activity=[](void*,int id,const char** out) noexcept -> int{assert(id==77);*out="ACT_RUN";return 1;};
        h.refresh_velocity=[](void* p,void* actor) noexcept -> int{auto& s=Player::self(p);assert(actor==s.actor);++s.refreshes;F::put(actor,0xb8,0u);F::fl(actor,0x250,3);F::fl(actor,0x254,4);return 1;};
        h.global_find=[](void* p,const char* key,int* out) noexcept -> int{auto& s=Player::self(p);assert(!std::strcmp(key,"gordon_precriminal"));++s.globals;*out=s.global_index;return 1;};
        F::put(player.actor,0x98,100u);F::put(player.actor,0x9c,25u);F::put(player.actor,0x97c,77u);F::put(player.actor,0xb8,0x1000u);
        F::put(player.actor,0x5c,F::ptr(player.rows));F::put(player.actor,0x68,2u);
        F::put(player.rows,0,1u);F::put(player.rows,4,2u);F::put(player.rows,8,3u);
        auto expect=[&](const char* key,const char* value){
            int idx=-1;std::uint32_t word=0;assert(api.find(c,sizeof(c),&b,key,&idx,&word)==1&&idx>=0&&word);
            auto* child=reinterpret_cast<State::Child*>(static_cast<std::uintptr_t>(word));
            assert(!std::strcmp(child->value,value)&&child->weight==1.f);};
        assert(api.construct(c,sizeof(c),&b)==1);
        assert(playerCriteria->append(player.actor,sizeof(player.actor),c,1,&h)==1&&f.count==8);
        expect("playerhealth","25");expect("playerhealthfrac","0.250");expect("playerweapon","weapon_magnum");
        expect("playeractivity","ACT_RUN");expect("playerspeed","5.000");expect("playerinjured","yes");expect("playermood","");
        expect("gordon_precriminal","0");assert(player.refreshes==1&&player.globals==1);
        assert(api.destroy(c,sizeof(c),&b)==1&&a.live.empty());
        F::put(player.actor,0x98,0u);player.global_index=4;
        assert(api.construct(c,sizeof(c),&b)==1);
        assert(playerCriteria->append(player.actor,sizeof(player.actor),c,1,&h)==1&&f.count==7);
        expect("playerhealthfrac","0.000");assert(player.refreshes==1&&player.globals==2);
        assert(api.destroy(c,sizeof(c),&b)==1&&a.live.empty());
        assert(api.construct(c,sizeof(c),&b)==1);
        assert(playerCriteria->contexts(player.actor,sizeof(player.actor),c,"",&h.contexts)==1&&f.count==2);
        expect("injured","yes");expect("mood","");
        assert(playerCriteria->append(player.actor,0x97f,c,0,&h)==0);
        assert(api.destroy(c,sizeof(c),&b)==1&&a.live.empty());
    }
    if(baseCriteria){
        struct Base {
            const ResponseCriteriaTreeApi& tree;ResponseCriteriaTreeBindings& b;
            alignas(4) unsigned char entity[0xc4]{},world[0xc4]{},rows[8]{},worldrows[8]{};
            unsigned counts=0,globals=0;bool no_world=false,shrink=false;
            static Base& self(void* p){return *static_cast<Base*>(p);}
        } s{api,b};
        BaseResponseCriteriaBindings h{};h.context=&s;h.contexts.context=&s;
        h.contexts.add=[](void* p,void* c,const char* k,const char* v,float w) noexcept -> int{auto& s=Base::self(p);return s.tree.add(&s.b,c,k,v,w);};
        h.contexts.storage=[](void* p,std::uint32_t w,void** out,unsigned* n) noexcept -> int{
            auto& s=Base::self(p);assert(w==F::ptr(s.rows)||w==F::ptr(s.worldrows));*out=w==F::ptr(s.rows)?s.rows:s.worldrows;*n=8;return 1;};
        h.contexts.string=[](void*,std::uint32_t w,const char** out) noexcept -> int{
            switch(w){case 1:*out="npc_jessica";break;case 2:*out="jessica";break;case 3:*out="mood";break;case 4:*out="alert";break;case 5:*out="chapter";break;case 6:*out="three";break;default:return 0;}return 1;};
        h.map_name=[](void*,const char** out) noexcept -> int{*out="se1_test";return 1;};
        h.global_count=[](void* p,int* out) noexcept -> int{auto& s=Base::self(p);++s.counts;*out=s.shrink&&s.globals?0:2;return 1;};
        h.global_row=[](void* p,int i,const char** name,int* state) noexcept -> int{auto& s=Base::self(p);assert(i==0||i==1);++s.globals;*name=i?"gate_open":"mission_started";*state=i?2:1;return 1;};
        h.world=[](void* p,void** out,unsigned* n) noexcept -> int{auto& s=Base::self(p);*out=s.no_world?nullptr:s.world;*n=sizeof(s.world);return 1;};
        F::put(s.entity,0xc,1);F::put(s.entity,0xc0,2);F::put(s.entity,0x98,80);F::put(s.entity,0x9c,20);
        F::put(s.entity,0x5c,F::ptr(s.rows));F::put(s.entity,0x68,1);F::put(s.rows,0,3);F::put(s.rows,4,4);
        F::put(s.world,0x5c,F::ptr(s.worldrows));F::put(s.world,0x68,1);F::put(s.worldrows,0,5);F::put(s.worldrows,4,6);
        auto expect=[&](const char* key,const char* value){
            int idx=-1;std::uint32_t word=0;assert(api.find(c,sizeof(c),&b,key,&idx,&word)==1&&idx>=0&&word);
            auto* child=reinterpret_cast<State::Child*>(static_cast<std::uintptr_t>(word));
            assert(!std::strcmp(child->value,value)&&child->weight==1.f);};
        assert(api.construct(c,sizeof(c),&b)==1&&baseCriteria->append(s.entity,sizeof(s.entity),c,&h)==1&&f.count==9);
        expect("map","se1_test");expect("classname","npc_jessica");expect("name","jessica");expect("health","20");expect("healthfrac","0.250");
        expect("mission_started","1");expect("gate_open","2");expect("mood","alert");expect("worldchapter","three");
        assert(s.counts==3&&s.globals==2&&api.destroy(c,sizeof(c),&b)==1&&a.live.empty());
        s.globals=0;s.shrink=true;s.no_world=true;F::put(s.entity,0x98,0);F::put(s.entity,0xc0,0);
        assert(api.construct(c,sizeof(c),&b)==1&&baseCriteria->append(s.entity,sizeof(s.entity),c,&h)==1&&f.count==7);
        expect("name","");expect("healthfrac","0.000");expect("mission_started","1");
        assert(s.globals==1&&api.destroy(c,sizeof(c),&b)==1&&a.live.empty());
    }
    if(actorSelection){
        struct Actors {
            State& kv;const ResponseCriteriaTreeApi& tree;ResponseCriteriaTreeBindings& b;
            alignas(4) unsigned char exp[0x7c]{},npc[0x1160]{},player[0x980]{},target[0xc4]{},row[8]{},response[0x124]{},enemy[0xec]{},owner[0xc4]{};
            unsigned family=1,resolves=0,finds=0,allocations=0,players=0;int system=0;
            bool absent=false,stale=false,fail_player=false,hostile=false,fire=false,match=false;
            unsigned enemy_calls=0,owner_calls=0,identities=0,retired=0,fire_checks=0;
            static Actors& self(void* p){return *static_cast<Actors*>(p);}
        } s{f,api,b};
        ActorResponseSelectionBindings cfg{};cfg.context=&s;cfg.selection.criteria=b;cfg.selection.session=&session;
        auto& sh=cfg.selection.host;sh.context=&s;
        sh.system=[](void* p,void*,void** out) noexcept -> int{*out=&Actors::self(p).system;return 1;};
        sh.allocate=[](void* p,unsigned n,void** out,unsigned* extent) noexcept -> int{auto& s=Actors::self(p);assert(n==0x124);++s.allocations;*out=s.response;*extent=sizeof(s.response);return 1;};
        sh.find=[](void* p,void*,void* c,void* r,void*,unsigned* found) noexcept -> int{
            auto& s=Actors::self(p);++s.finds;
            auto expect=[&](const char* key,const char* value,float weight=1.f){
                int i=-1;std::uint32_t word=0;assert(s.tree.find(c,response_criteria_bytes,&s.b,key,&i,&word)==1);
                if(!value){assert(i==-1&&!word);return;}
                assert(i>=0&&word);auto* v=reinterpret_cast<State::Child*>(static_cast<std::uintptr_t>(word));
                assert(!std::strcmp(v->value,value)&&v->weight==weight);
            };
            const unsigned count=15+(s.family&&!s.stale?1:0)+(s.absent?0:6)+(s.hostile?1:0)+(s.family==2&&s.fire?1:0);
            const unsigned replaced=s.family==2&&s.hostile&&s.match?1:0;
            assert(s.kv.count==count+replaced&&F::word(c,0x18)==count+1);
            expect("concept","TLK_IDLE",5);expect("part","torso",5);expect("map","se1_test");expect("classname","npc_count");
            expect("health","40");expect("activity","ACT_IDLE");expect("timesincecombat",s.hostile?"-1":"999999.0");
            expect("enemy",s.hostile?(replaced?"func_tankmortar":"npc_grenade_frag"):nullptr);
            expect("hurt_by_fire",s.family==2&&s.fire?"1":nullptr);
            expect("target_mood",s.family&&!s.stale?"ready":nullptr);expect("playerhealth",s.absent?nullptr:"75");
            expect("gordon_precriminal",s.absent?nullptr:"0");
            auto* raw=static_cast<unsigned char*>(r);raw[4]='x';raw[5]=0;*found=1;return 1;
        };
        sh.draw=[](void*,int,int,int*) noexcept -> int{assert(false);return 0;};
        sh.destroy=[](void*,void*) noexcept -> int{return 1;};
        cfg.actor=[](void* p,std::uint32_t word,void** out,unsigned* n,unsigned* family) noexcept -> int{auto& s=Actors::self(p);assert(word==F::ptr(s.npc));*out=s.npc;*n=sizeof(s.npc);*family=s.family;return 1;};
        cfg.target=[](void* p,std::uint32_t handle,void** out,unsigned* n) noexcept -> int{auto& s=Actors::self(p);if(handle==0x6002){++s.owner_calls;*out=s.owner;*n=sizeof(s.owner);return 1;}assert(handle==0x5001);++s.resolves;*out=s.stale?nullptr:s.target;*n=sizeof(s.target);return 1;};
        cfg.player_at=[](void* p,int i,void** out,unsigned* n) noexcept -> int{auto& s=Actors::self(p);assert(i==1);++s.players;if(s.fail_player)return 0;*out=s.absent?nullptr:s.player;*n=sizeof(s.player);return 1;};
        auto& bh=cfg.base;bh.context=&s;bh.contexts.context=&s;
        bh.contexts.add=[](void* p,void* c,const char* k,const char* v,float w) noexcept -> int{auto& s=Actors::self(p);return s.tree.add(&s.b,c,k,v,w);};
        bh.contexts.storage=[](void* p,std::uint32_t word,void** out,unsigned* n) noexcept -> int{auto& s=Actors::self(p);assert(word==F::ptr(s.row));*out=s.row;*n=8;return 1;};
        bh.contexts.string=[](void*,std::uint32_t w,const char** out) noexcept -> int{switch(w){case 1:*out="npc_count";break;case 2:*out="target_mood";break;case 3:*out="ready";break;case 4:case 5:*out="func_tankmortar";break;default:return 0;}return 1;};
        bh.map_name=[](void*,const char** out) noexcept -> int{*out="se1_test";return 1;};
        bh.global_count=[](void*,int* n) noexcept -> int{*n=0;return 1;};
        bh.global_row=[](void*,int,const char**,int*) noexcept -> int{assert(false);return 0;};
        bh.world=[](void*,void** out,unsigned* n) noexcept -> int{*out=nullptr;*n=0;return 1;};
        auto& nh=cfg.npc;nh.context=&s;nh.add=bh.contexts.add;
        nh.activity=[](void*,int,const char** out) noexcept -> int{*out="ACT_IDLE";return 1;};
        nh.enemy=[](void* p,void*,unsigned* present,const char** name) noexcept -> int{auto& s=Actors::self(p);*present=s.hostile;*name=s.hostile?"npc_grenade_frag":nullptr;return 1;};
        nh.clock=[](void*,float*) noexcept -> int{assert(false);return 0;};
        nh.velocity=[](void*,void*,float* xyz) noexcept -> int{xyz[0]=xyz[1]=xyz[2]=0;return 1;};
        nh.weapon=[](void*,void*,unsigned* present,const char** name) noexcept -> int{*present=0;*name=nullptr;return 1;};
        nh.max_clients=[](void*,int* n) noexcept -> int{*n=2;return 1;};
        nh.local_player=[](void*,void**,unsigned*) noexcept -> int{assert(false);return 0;};
        nh.refresh=[](void*,void*) noexcept -> int{assert(false);return 0;};
        nh.condition=[](void* p,void*,int id,unsigned* out) noexcept -> int{auto& s=Actors::self(p);if(id==0x45){++s.fire_checks;*out=s.fire;}else{assert(id==0x1f);*out=0;}return 1;};
        nh.in_view=[](void*,void*,void*,unsigned*) noexcept -> int{assert(false);return 0;};
        nh.visible=[](void*,void*,void*,unsigned,unsigned*) noexcept -> int{assert(false);return 0;};
        auto& ph=cfg.player;ph.context=&s;ph.contexts=bh.contexts;ph.weapon=nh.weapon;ph.activity=nh.activity;ph.refresh_velocity=nh.refresh;
        ph.global_find=[](void*,const char*,int* out) noexcept -> int{*out=-1;return 1;};
        auto& j=cfg.jessica;j.context=&s;
        j.enemy_object=[](void* p,void* npc,void** out,unsigned* n) noexcept -> int{auto& s=Actors::self(p);assert(npc==s.npc);++s.enemy_calls;*out=s.hostile?s.enemy:nullptr;*n=sizeof(s.enemy);return 1;};
        j.mortar_identity=[](void* p,std::uint32_t* m,std::uint32_t* empty) noexcept -> int{++Actors::self(p).identities;*m=4;*empty=99;return 1;};
        j.retire_child=[](void* p,void* root,void* child) noexcept -> int{auto& f=State::self(p);assert(root==&f.root);auto* v=static_cast<State::Child*>(child);assert(!std::strcmp(v->name,"enemy"));v->name[0]=0;return 1;};
        F::put(s.enemy,0xe8,0x6002);F::put(s.owner,0xc,4);
        F::put(s.exp,4,F::ptr(s.npc));F::put(s.npc,0xc,1);F::put(s.npc,0x9c,40);F::put(s.npc,0x98,80);
        F::put(s.npc,0x115c,0x5001);F::put(s.target,0x5c,F::ptr(s.row));F::put(s.target,0x68,1);F::put(s.row,0,2);F::put(s.row,4,3);
        F::put(s.player,0x9c,75);F::put(s.player,0x98,100);
        auto run=[&](){return actorSelection->select(s.exp,sizeof(s.exp),&cfg,"TLK_IDLE","part:head,PART:torso",&out,&n);};
        assert(run()==1&&out==s.response&&n==0x124&&s.resolves==2&&s.finds==1&&!session.busy&&a.live.empty());
        s.stale=true;s.absent=true;
        assert(run()==1&&s.resolves==3&&s.finds==2&&!session.busy&&a.live.empty());
        s.family=0;
        assert(run()==1&&s.resolves==3&&s.finds==3&&!session.busy&&a.live.empty());
        s.fail_player=true;out=&s;n=99;
        assert(run()==-1&&out==&s&&n==99&&s.allocations==3&&s.players==4&&!session.busy&&a.live.empty());
        s.fail_player=false;s.family=2;s.hostile=true;s.match=true;s.fire=true;
        assert(run()==1&&s.enemy_calls==2&&s.owner_calls==1&&s.identities==2&&s.fire_checks==1&&a.live.empty());
        // Same string text, different pooled identity: no replacement.
        s.match=false;s.fire=false;F::put(s.owner,0xc,5);
        assert(run()==1&&s.enemy_calls==4&&s.owner_calls==2&&s.identities==3&&s.fire_checks==2&&a.live.empty());
        s.hostile=false;s.fire=true;
        assert(run()==1&&s.enemy_calls==5&&s.owner_calls==2&&s.identities==3&&s.fire_checks==3&&a.live.empty());
    }
    if(removal){
        ResponseCriteriaRemovalBindings r{};r.tree=b;
        r.retire_child=[](void* p,void* root,void* child) noexcept -> int{
            auto& s=State::self(p);assert(root==&s.root&&child);auto* row=static_cast<State::Child*>(child);
            assert(row->name[0]);row->name[0]=0;return 1;};
        assert(api.construct(c,sizeof(c),&b)==1);
        const char* keys[]={"enemy","alpha","beta","gamma","delta","epsilon","zeta"};
        for(const auto* key:keys)assert(api.add(&b,c,key,"value",1));
        assert(api.add(&b,c,"ENEMY","new",1));
        assert(removal->remove(c,sizeof(c),&r,"enemy")==0&&F::word(c,0x18)==8);
        assert(removal->remove(c,sizeof(c),&r,"missing")==1&&F::word(c,0x18)==8);
        int idx=-1;std::uint32_t child=0;
        assert(api.find(c,sizeof(c),&b,"gamma",&idx,&child)==1&&idx>=0);
        const auto high=F::word(c,0x20);
        assert(removal->remove(c,sizeof(c),&r,"gamma")==1&&F::word(c,0x18)==7&&F::word(c,0x1c)==static_cast<unsigned>(idx));
        assert(api.find(c,sizeof(c),&b,"gamma",&idx,&child)==1&&idx==-1&&!child);
        assert(api.add(&b,c,"reused","new",1)&&F::word(c,0x20)==high&&F::word(c,0x18)==8);
        for(const auto* key:{"alpha","beta","delta","epsilon","zeta","reused"}){
            assert(removal->remove(c,sizeof(c),&r,key)==1);
            assert(api.find(c,sizeof(c),&b,key,&idx,&child)==1&&idx==-1&&!child);
        }
        assert(F::word(c,0x18)==2&&api.destroy(c,sizeof(c),&b)==1&&a.live.empty());
        assert(api.construct(c,sizeof(c),&b)==1&&api.add(&b,c,"enemy","old",1));
        assert(removal->remove(c,sizeof(c),&r,"enemy")==1&&F::word(c,0x14)==0xffffffffu&&F::word(c,0x18)==0);
        assert(api.add(&b,c,"enemy","replacement",1)&&F::word(c,0x20)==1);
        assert(api.find(c,sizeof(c),&b,"enemy",&idx,&child)==1&&idx==0);
        assert(!std::strcmp(reinterpret_cast<State::Child*>(static_cast<std::uintptr_t>(child))->value,"replacement"));
        assert(api.destroy(c,sizeof(c),&b)==1&&a.live.empty());
    }
    if(ranking){
        struct Rules {
            alignas(4) unsigned char sys[0xcc]{},criteria[3*0x14c]{},rules[4*0x40]{},ids[4][8]{};
            int draws=0;int pick=1;
            static Rules& self(void* p){return *static_cast<Rules*>(p);}
        } r;
        ResponseRuleRankingBindings h{};h.criteria=b;h.context=&r;
        h.storage=[](void* p,std::uint32_t word,void** out,unsigned* n) noexcept -> int{
            auto& r=Rules::self(p);
            if(word==F::ptr(r.criteria)){*out=r.criteria;*n=sizeof(r.criteria);return 1;}
            if(word==F::ptr(r.rules)){*out=r.rules;*n=sizeof(r.rules);return 1;}
            for(auto& ids:r.ids)if(word==F::ptr(ids)){*out=ids;*n=sizeof(ids);return 1;}
            return 0;};
        h.string=[](void*,std::uint32_t word,const char** out) noexcept -> int{
            switch(word){case 1:*out="concept";break;case 2:*out="enemy";break;case 3:*out="missing";break;default:return 0;}return 1;};
        h.enumeration=[](void*,const char*,float* v,unsigned* found) noexcept -> int{*v=0;*found=0;return 1;};
        h.get_string=[](void*,void* child,const char* field,const char* fallback,const char** out) noexcept -> int{
            assert(!std::strcmp(field,"value")&&!*fallback);*out=static_cast<State::Child*>(child)->value;return 1;};
        h.get_float=[](void*,void* child,const char* field,float fallback,float* out) noexcept -> int{
            assert(!std::strcmp(field,"weight")&&fallback==1.f);*out=static_cast<State::Child*>(child)->weight;return 1;};
        h.draw=[](void* p,int lo,int hi,int* out) noexcept -> int{auto& r=Rules::self(p);assert(lo==0&&hi==1);++r.draws;*out=r.pick;return 1;};
        F::put(r.sys,0x60,F::ptr(r.criteria));F::put(r.sys,0x64,3);
        F::put(r.sys,0xb8,F::ptr(r.rules));F::put(r.sys,0xbc,4);F::put(r.sys,0xc8,4);
        auto criterion=[&](unsigned i,unsigned key,const char* expr,float weight,bool required){
            auto* row=r.criteria+i*0x14c;F::put(row,0,0xffffffffu);row+=0x14;
            F::put(row,0,key);F::fl(row,8,weight);row[0xc]=required;
            assert(sin_port::response_rule::compile(expr,{},row+0x10,sin_port::response_rule::matcher_bytes));};
        criterion(0,1,"TLK_IDLE",2,true);criterion(1,2,"npc_sintekgrunt",1,true);criterion(2,3,"!absent",1,false);
        auto rule=[&](unsigned i,unsigned first,unsigned second,unsigned count){
            auto* row=r.rules+i*0x40;F::put(row,0,0xffffffffu);row+=0x14;
            F::put(r.ids[i],0,first);F::put(r.ids[i],4,second);
            F::put(row,0,F::ptr(r.ids[i]));F::put(row,0xc,count);row[0x29]=1;};
        rule(0,0,1,2);rule(1,0,0,1);rule(2,0,0,1);rule(3,2,0,1);
        assert(api.construct(c,sizeof(c),&b)==1&&api.add(&b,c,"concept","TLK_IDLE",5)&&api.add(&b,c,"enemy","none",1));
        int scratch[4]{},selected=99;
        auto best=[&](){return ranking->best(r.sys,sizeof(r.sys),c,sizeof(c),&h,scratch,4,&selected);};
        assert(best()==1&&selected==2&&r.draws==1);
        r.pick=0;assert(best()==1&&selected==1&&r.draws==2);
        assert(api.add(&b,c,"enemy","npc_sintekgrunt",1));
        assert(best()==1&&selected==0&&r.draws==2);
        F::put(r.sys,0xc8,0);
        assert(best()==1&&selected==-1);
        F::put(r.sys,0xc8,5);assert(best()==0);
        assert(api.destroy(c,sizeof(c),&b)==1&&a.live.empty());
    }
}

inline void check_vcd_clock_speech(VcdAuditArena& arena,const NativeVcdRuntimeApi& runtime,const NativeChoreographyClockApi& clock,const ServerFacialRecordApi* retail=nullptr,const SceneSpeechActorNoteApi* actor_note=nullptr,const SceneSpeechActorSinkApi* actor_sink=nullptr,const ResponseHistoryTreeApi* history_tree=nullptr,
    const ResponseHistoryManagedApi* managed_history=nullptr,TalkManagerFixture* manager=nullptr,const TalkDeadlineRuntimeApi* deadline_runtime=nullptr,const TalkNativePropagationApi* native_propagation=nullptr){
    using F=ScenePlaybackFixture;
    const char data[]=R"VCD(actor Jessica
{
 channel Voice
 {
  event speak greeting
  {
   time 0 2
   param Jessica.Line
   cctype cc_master
   cctoken "123456789"
   cc_noattenuate
  }
 }
}
)VCD";
    struct Fixture {
        VcdAuditArena& arena;
        alignas(4) unsigned char scene[0x5ec]{},actor[0x1458]{},exp[0x7c]{},hello[0x1c]{},neighbor[0x1458]{},neighbor_exp[0x7c]{};
        TalkManagerFixture* manager=nullptr;bool native_deadlines=false,native_propagate=false;float now=10.f;
        int player=0,filter=0;
        unsigned opened=0,closed=0,notified=0,emitted=0,registered=0,preferences=0,sent=0,begun=0,stopped=0,starts=0,stops=0;
        std::uint32_t root=0,actorword=0;
        void* event=nullptr;bool retail=false,native_note=false,native_sink=false,native_tree=false,hello_present=false;
        explicit Fixture(VcdAuditArena& a):arena(a){}
        static Fixture& self(void* p) noexcept{return *static_cast<Fixture*>(p);}
    } f(arena);f.retail=retail!=nullptr;f.native_note=actor_note!=nullptr;f.native_sink=actor_sink!=nullptr;f.native_tree=history_tree!=nullptr;f.manager=manager;f.native_deadlines=deadline_runtime!=nullptr;f.native_propagate=native_propagation!=nullptr;
    assert(!native_propagation||deadline_runtime);
    assert(!deadline_runtime||managed_history);
    assert(!managed_history||(history_tree&&actor_sink&&actor_note&&manager));
    auto raw=arena.runtime();
    F::put(f.scene,0x414,F::ptr(f.scene));F::fl(f.scene,0x564,.25f);
    assert(runtime.parse(f.scene+0x410,"speech.vcd",data,sizeof(data)-1,&raw,&f.root)==1&&f.root);
    F::put(f.scene,0x40c,f.root);
    void *root=nullptr,*rows=nullptr;unsigned extent=0,n=0;
    assert(arena.storage(&arena,f.root,&root,&extent)&&extent>=0x220&&F::word(root,0xc)==1);
    assert(arena.storage(&arena,F::word(root,0),&rows,&n)&&n>=4);
    assert(arena.storage(&arena,F::word(rows,0),&f.event,&n)&&n>=0x3c0);
    f.actorword=F::word(f.event,0x2b4);assert(f.actorword);
    SceneSpeechCallbackBindings speech{};speech.context=&f;
    speech.actor=[](void* p,void* scene,std::uint32_t word,void** out,unsigned* bytes) noexcept -> int{
        auto& f=Fixture::self(p);assert(scene==f.scene&&word==f.actorword);*out=f.actor;*bytes=sizeof(f.actor);return 1;
    };
    speech.trace=[](void* p,void* scene,void* event,float,SceneSpeechTrace trace) noexcept -> int{
        auto& f=Fixture::self(p);assert(scene==f.scene&&event==f.event);
        if(trace==SceneSpeechTrace::Start)++f.starts;
        else if(trace==SceneSpeechTrace::Stop)++f.stops;
        else assert(false);
        return 1;
    };
    auto& e=speech.speech;e.context=&f;
    e.open_filter=[](void* p,void* actor,float scale,void** out) noexcept -> int{
        auto& f=Fixture::self(p);assert(actor==f.actor&&scale==.8f);++f.opened;*out=&f.filter;return 1;
    };
    e.close_filter=[](void* p,void* filter) noexcept -> int{
        auto& f=Fixture::self(p);assert(filter==&f.filter);++f.closed;return 1;
    };
    e.clock=[](void* p,float* out) noexcept -> int{*out=Fixture::self(p).now;return 1;};
    e.recipient_count=[](void* p,void* filter,int* out) noexcept -> int{
        assert(filter==&Fixture::self(p).filter);*out=1;return 1;
    };
    e.recipient=[](void* p,void* filter,unsigned index,void** out) noexcept -> int{
        auto& f=Fixture::self(p);assert(filter==&f.filter&&!index);*out=&f.player;return 1;
    };
    e.language=[](void* p,void* player,unsigned* present,int* language,unsigned* forced) noexcept -> int{
        assert(player==&Fixture::self(p).player);*present=1;*language=1;*forced=0;return 1;
    };
    e.emit=[](void* p,void* actor,void* player,const char* name,int channel,float volume,int level,float start) noexcept -> int{
        auto& f=Fixture::self(p);assert(actor==f.actor&&player==&f.player&&channel==2&&volume==1.f&&level==80&&start==9.5f);
        assert(!std::strcmp(name,"Jessica.Line"));if(f.native_note)assert(f.notified==1&&VcdAuditArena::read_float(f.exp,0x68)==11.75f&&VcdAuditArena::read_float(f.exp,0x6c)==11.5f);
        if(f.native_tree){
            void* row=nullptr;unsigned bytes=0;
            assert(F::word(f.exp,0x24)==1&&f.arena.storage(&f.arena,F::word(f.exp,0x14),&row,&bytes)&&bytes>=28);
            assert(VcdAuditArena::read_float(row,0x14)==10.f&&F::word(row,0x18)==0);
        }else if(f.native_sink)assert(f.hello_present&&VcdAuditArena::read_float(f.hello,0x14)==10.f);
        if(f.manager){assert(f.manager->creations==1&&VcdAuditArena::read_float(f.manager->object,0x394)==17.f);
            if(f.native_deadlines)assert(f.manager->object[0x3b1]==48&&f.manager->inserts==0);}
        if(f.native_propagate){
            void* row=nullptr;unsigned n=0;
            assert(F::word(f.neighbor_exp,0x24)==1&&f.arena.storage(&f.arena,F::word(f.neighbor_exp,0x14),&row,&n)&&n>=28);
            assert(VcdAuditArena::read_float(row,0x14)==10.f&&!F::word(row,0x18));
        }
        ++f.emitted;return 1;
    };
    e.caption_register=[](void* p,std::uint32_t crc,const char* token) noexcept -> int{
        auto& f=Fixture::self(p);assert(crc==0xcbf43926u&&!std::strcmp(token,"123456789")&&f.emitted==1);++f.registered;return 1;
    };
    e.caption_preferences=[](void* p,void* filter) noexcept -> int{
        auto& f=Fixture::self(p);assert(filter==&f.filter);++f.preferences;return 1;
    };
    e.caption_far=[](void*,void*,unsigned*) noexcept -> int{assert(false);return 0;};
    e.remove_recipient=[](void*,void*,void*) noexcept -> int{assert(false);return 0;};
    e.caption_send=[](void* p,void* filter,std::uint32_t crc,unsigned ticks,unsigned flag) noexcept -> int{
        auto& f=Fixture::self(p);assert(filter==&f.filter&&crc==0xcbf43926u&&ticks==20&&flag==1);++f.sent;return 1;
    };
    auto& rec=e.records;rec.context=&f;
    rec.storage=[](void* p,std::uint32_t w,void** out,unsigned* bytes) noexcept -> int{
        auto& a=Fixture::self(p).arena;return a.storage(&a,w,out,bytes);
    };
    rec.grow=[](void* p,void* v,unsigned extra) noexcept -> int{
        auto& a=Fixture::self(p).arena;return a.vectors.grow_vector(&a.vector_bindings,v,extra,0x40);
    };
    rec.handle=[](void*,void*,std::uint32_t*) noexcept -> int{assert(false);return 0;};
    rec.diagnostic=[](void*,ActorEventDiagnostic,std::uint32_t) noexcept -> int{assert(false);return 0;};
    rec.begin=[](void* p,void* actor,void* record,std::uint32_t root,void* event,std::uint32_t choreo,void* target,unsigned* accepted) noexcept -> int{
        auto& f=Fixture::self(p);assert(!f.retail&&actor==f.actor&&root==f.root&&event==f.event&&choreo==f.actorword&&!target);
        assert(F::word(record,0)==F::ptr(event)&&F::word(record,4)==root&&F::word(record,0x24)==0xffffffffu);
        ++f.begun;*accepted=1;return 1;
    };
    rec.stop=[](void* p,void* actor,void* record,int argument,int zero,unsigned* accepted) noexcept -> int{
        auto& f=Fixture::self(p);assert(actor==f.actor&&F::word(record,0)==F::ptr(f.event)&&F::word(record,4)==f.root&&!argument);
        assert(zero==(f.retail?1:0));
        ++f.stopped;*accepted=f.retail?0u:1u;return 1;
    };
    ServerFacialRecordBindings cfg{};
    if(retail){
        cfg.host=rec;cfg.base_flex=true;
        cfg.extent=[](void* p,void* actor,unsigned* n) noexcept -> int{
            auto& f=Fixture::self(p);assert(actor==f.actor);*n=sizeof(f.actor);return 1;
        };
        cfg.now=[](void*,float* out) noexcept -> int{*out=10.f;return 1;};
        assert(retail->adapt(&cfg,&rec));
    }
    SpeechSemaphorePair sem{};
    SceneSpeechPreparationBindings prep{};prep.context=&f;prep.state=&sem;prep.instanced=1;
    prep.cast_base_actor=[](void* p,void* actor,void** out) noexcept -> int{
        assert(actor==Fixture::self(p).actor);*out=actor;return 1;
    };
    prep.actor_notify=[](void* p,void* actor,float remaining,float margin) noexcept -> int{
        auto& f=Fixture::self(p);assert(actor==f.actor&&remaining==1.5f&&margin==.25f);++f.notified;return 1;
    };
    prep.is_npc=[](void*,void*,unsigned*) noexcept -> int{assert(false);return 0;};
    prep.classify=[](void*,void*,int,unsigned*) noexcept -> int{assert(false);return 0;};
    prep.clock=e.clock;
    prep.handle=retail?cfg.host.handle:rec.handle;
    SceneSpeechActorNoteBindings note{};SceneSpeechPreparationBindings native_prep{};SceneSpeechActorSinkBindings sink{};ResponseHistoryTreeBindings history{};ResponseHistoryManagedBindings managed{};TalkDeadlineRuntimeBindings deadlines{};TalkNativePropagationCooldownBindings propagation{};
    if(actor_note){
        F::put(f.actor,0xffc,managed_history?F::ptr(f.exp):77u);
        note.preparation=prep;note.preparation.actor_notify=nullptr;note.context=&f;
        note.extent=[](void* p,void* a,unsigned* out) noexcept -> int{
            auto& f=Fixture::self(p);assert(a==f.actor);*out=sizeof(f.actor);return 1;};
        note.storage=[](void* p,std::uint32_t word,void** out,unsigned* n) noexcept -> int{
            auto& f=Fixture::self(p);assert(word==(f.manager?F::ptr(f.exp):77u));*out=f.exp;*n=sizeof(f.exp);return 1;};
        note.note.context=&f;
        note.note.owner_start=[](void* p,void* exp) noexcept -> int{
            auto& f=Fixture::self(p);assert(exp==f.exp&&!f.emitted);++f.notified;return 1;};
        note.note.clock=e.clock;
        note.note.diagnostic=[](void* p,void* exp,float total,float margin,float until) noexcept -> int{
            auto& f=Fixture::self(p);assert(exp==f.exp&&total==1.75f&&margin==.25f&&until==11.75f);return 1;};
        note.note.uses_semaphore=[](void*,void*,unsigned* out) noexcept -> int{*out=1;return 1;};
        note.note.classify=[](void*,void*,int arg,unsigned* out) noexcept -> int{assert(!arg);*out=1;return 1;};
        note.note.owner_handle=[](void*,void*,std::uint32_t* out) noexcept -> int{*out=0x1001;return 1;};
        if(actor_sink){
            F::put(f.exp,4,managed_history?F::ptr(f.actor):88u);sink.context=&f;sink.history.context=&f;
            sink.storage=[](void* p,std::uint32_t word,void** out,unsigned* n) noexcept -> int{
                auto& f=Fixture::self(p);
                if(word==(f.manager?F::ptr(f.exp):77u)){*out=f.exp;*n=sizeof(f.exp);return 1;}
                if(word==(f.manager?F::ptr(f.actor):88u)){*out=f.actor;*n=sizeof(f.actor);return 1;}
                return 0;};
            sink.kind=[](void* p,void* actor,SceneSpeechActorKind* out) noexcept -> int{
                assert(actor==Fixture::self(p).actor);*out=SceneSpeechActorKind::Jessica;return 1;};
            sink.history.lookup=[](void* p,void* exp,const char* name,int* out) noexcept -> int{
                auto& f=Fixture::self(p);assert(exp==f.exp&&!std::strcmp(name,"TLK_HELLO"));*out=f.hello_present?3:-1;return 1;};
            sink.history.insert=[](void* p,void* exp,const char* name,float now,int* out) noexcept -> int{
                auto& f=Fixture::self(p);assert(exp==f.exp&&!f.hello_present&&!std::strcmp(name,"TLK_HELLO")&&now==10.f);
                f.hello_present=true;*out=3;return 1;};
            sink.history.entry=[](void* p,void* exp,int idx,void** out,unsigned* n) noexcept -> int{
                auto& f=Fixture::self(p);assert(exp==f.exp&&idx==3&&f.hello_present);*out=f.hello;*n=sizeof(f.hello);return 1;};
            sink.history.clock=e.clock;
            sink.history.notify=[](void* p,void* exp,const char* name) noexcept -> int{
                auto& f=Fixture::self(p);assert(exp==f.exp&&!std::strcmp(name,"TLK_HELLO")&&!f.emitted&&VcdAuditArena::read_float(f.hello,0x14)==10.f);
                ++f.notified;return 1;};
            if(history_tree){
                F::put(f.exp,0x20,0xffffffffu);F::put(f.exp,0x28,0xffffffffu);
                assert(arena.symbol_api.construct_empty(f.exp+0x34,0x34,0,0x2228e9b0)==1);
                history.memory=arena.vector_bindings;history.symbols=arena.intern_bindings;history.context=&f;
                history.clock=e.clock;
                history.notify=[](void* p,void* exp,const char* name) noexcept -> int{
                    auto& f=Fixture::self(p);assert(exp==f.exp&&!std::strcmp(name,"TLK_HELLO")&&!f.emitted);
                    void* row=nullptr;unsigned bytes=0;
                    assert(F::word(exp,0x24)==1&&f.arena.storage(&f.arena,F::word(exp,0x14),&row,&bytes)&&bytes>=28);
                    assert(VcdAuditArena::read_float(row,0x14)==10.f);
                    ++f.notified;return 1;};
                if(managed_history){
                    F::put(f.exp,0xc,F::ptr(f.actor)+0xdfc);
                    managed.tree=history;managed.tree.notify=nullptr;
                    managed.context=&f;managed.storage=sink.storage;managed.kind=sink.kind;
                    managed.manager.session=&manager->session;managed.manager.manager=manager->b;
                    auto& cooldown=managed.manager.cooldown;cooldown.context=&f;cooldown.clock=e.clock;
                    cooldown.random=[](void*,float lo,float hi,float* out) noexcept -> int{
                        if(lo==5&&hi==10){*out=7;return 1;}
                        assert(lo==15&&hi==15);*out=15;return 1;};
                    cooldown.concept_deadline=[](void*,const char*,float**) noexcept -> int{assert(false);return 0;};
                    cooldown.cooldown_disabled=[](void*,unsigned* out) noexcept -> int{*out=0;return 1;};
                    cooldown.propagate=[](void* p,void* actor,const char* name) noexcept -> int{
                        auto& f=Fixture::self(p);assert(actor==f.actor&&!std::strcmp(name,"TLK_HELLO")&&!f.emitted);
                        void* row=nullptr;unsigned bytes=0;
                        assert(f.arena.storage(&f.arena,F::word(f.exp,0x14),&row,&bytes)&&bytes>=28);
                        assert(VcdAuditArena::read_float(row,0x14)==10.f);++f.notified;return 1;};
                    if(deadline_runtime){
                        deadlines.tree.memory=arena.vector_bindings;deadlines.tree.session=&manager->session;
                        deadlines.tree.context=const_cast<TalkCatalogApi*>(&manager->catalog);
                        deadlines.tree.intern=[](void* p,const char* key,const char** out) noexcept -> int{
                            const auto* d=static_cast<const TalkCatalogApi*>(p)->lookup(key);if(!d)return 0;*out=d->name;return 1;};
                        deadlines.release=&VcdAuditArena::release;deadlines.host=managed.manager;
                        deadlines.host.manager.insert_deadline=nullptr;deadlines.host.manager.free_tree=nullptr;
                        deadlines.host.cooldown.concept_deadline=nullptr;
                        assert(deadline_runtime->adapt(&deadlines,&managed.manager));
                        manager->b=managed.manager.manager;
                    }
                    assert(managed_history->adapt(&managed,&sink.history));
                    if(native_propagation){
                        F::fl(f.neighbor,0x2bc,100.f);
                        F::put(f.neighbor,0xffc,F::ptr(f.neighbor_exp));F::put(f.neighbor_exp,4,F::ptr(f.neighbor));
                        F::put(f.neighbor_exp,0x20,0xffffffffu);F::put(f.neighbor_exp,0x28,0xffffffffu);
                        assert(arena.symbol_api.construct_empty(f.neighbor_exp+0x34,0x34,0,0x2228e9b0)==1);
                        auto& b=propagation.propagation;b.context=&f;b.history=sink.history;
                        b.extent=[](void* p,void* npc,unsigned* n) noexcept -> int{
                            auto& f=Fixture::self(p);assert(npc==f.actor||npc==f.neighbor);*n=sizeof(f.actor);return 1;};
                        b.storage=[](void* p,std::uint32_t word,void** out,unsigned* n) noexcept -> int{
                            auto& f=Fixture::self(p);if(word!=F::ptr(f.neighbor_exp))return 0;
                            *out=f.neighbor_exp;*n=sizeof(f.neighbor_exp);return 1;};
                        auto& enumeration=b.enumeration;enumeration.context=&f;
                        enumeration.list=[](void* p,void** token) noexcept -> int{*token=p;return 1;};
                        enumeration.count=[](void*,int* count) noexcept -> int{*count=2;return 1;};
                        enumeration.candidate=[](void* p,void* token,unsigned i,void** out,unsigned* n) noexcept -> int{
                            auto& f=Fixture::self(p);assert(token==p&&i<2);*out=i?f.neighbor:f.actor;*n=sizeof(f.actor);return 1;};
                        enumeration.refresh=[](void*,void*) noexcept -> int{assert(false);return 0;};
                        enumeration.visible=[](void* p,void* source,void* target,unsigned mask,void* blocker,unsigned* out) noexcept -> int{
                            auto& f=Fixture::self(p);assert(source==f.actor&&target==f.neighbor&&mask==0x4081&&!blocker&&!f.emitted);
                            void* row=nullptr;unsigned n=0;
                            assert(f.arena.storage(&f.arena,F::word(f.exp,0x14),&row,&n)&&n>=28&&VcdAuditArena::read_float(row,0x14)==10.f);
                            ++f.notified;*out=1;return 1;};
                        propagation.host=managed.manager.cooldown;
                        // The adapted manager retains its native concept tree; propagation
                        // bypasses the synthetic greeting effect in the prior configurations.
                        assert(native_propagation->adapt(&propagation,&managed.manager.cooldown));
                    }
                }else assert(history_tree->adapt(&history,&sink.history));
            }
            sink.transport=note.note;sink.transport.owner_start=nullptr;sink.transport.uses_semaphore=nullptr;
            assert(actor_sink->adapt(&sink,&note.note));
        }
        assert(actor_note->adapt(&note,&native_prep));
    }
    NativeChoreographyClockBindings tick{};tick.vectors=arena.vector_bindings;tick.release_memory=&VcdAuditArena::release;
    tick.scene=f.scene;tick.scene_bytes=sizeof(f.scene);tick.speech=&speech;tick.preparation=actor_note?&native_prep:&prep;
    assert(clock.reset(root,extent,1,0,0,&tick)==1);
    F::fl(f.scene,0x3dc,.5f);
    assert(clock.simulate(root,extent,.5f,&tick)==1&&f.starts==1&&f.notified==1&&f.emitted==1&&f.begun==(retail?0u:1u));
    if(actor_note)assert(sem.classified.release==10.25f&&sem.classified.owner==0x1001);
    assert(f.opened==1&&f.closed==1&&f.registered==1&&f.preferences==1&&f.sent==1&&F::word(f.actor,0x5c4)==1&&F::word(f.actor,0x5bc)==1);
    assert(static_cast<unsigned char*>(f.event)[0x328]);
    if(retail){
        void* row=nullptr;unsigned bytes=0,accepted=0;
        assert(arena.storage(&arena,F::word(f.actor,0x5b8),&row,&bytes)&&bytes>=0x40);
        assert(F::word(row,0)==F::ptr(f.event)&&F::word(row,4)==f.root&&F::word(row,0x24)==0xffffffffu);
        assert(rec.check(rec.context,f.actor,row,1.f,f.root,f.event,&accepted)&&accepted==1);
        // High byte alone does not request stop; forced stop runs effect once
        // and returns true regardless of the host's accepted output.
        assert(rec.stop(rec.context,f.actor,row,0,256,&accepted)&&accepted==1&&!f.stopped);
        assert(rec.stop(rec.context,f.actor,row,0,1,&accepted)&&accepted==1&&f.stopped==1);
    }
    F::fl(f.scene,0x3dc,2.25f);
    assert(clock.simulate(root,extent,2.25f,&tick)==1&&f.emitted==1&&f.stopped==(retail?1u:0u));
    F::fl(f.scene,0x3dc,2.5f);
    assert(clock.simulate(root,extent,2.5f,&tick)==1&&f.stopped==1&&f.stops==1&&!F::word(f.actor,0x5c4));
    assert(!static_cast<unsigned char*>(f.event)[0x328]);
    if(manager)assert(manager->creations==1&&manager->inserts==(deadline_runtime?0u:48u)&&VcdAuditArena::read_float(manager->object,0x394)==17.f);
    if(deadline_runtime){
        // After the scene, a second concept shares the very same live manager.
        assert(manager->object[0x3b1]==48);
        assert(sink.history.notify(sink.history.context,f.exp,"TLK_PLPUSH"));
        float* push=nullptr;
        auto& cooldown=managed.manager.cooldown;
        assert(cooldown.concept_deadline(cooldown.context,"tlk_plpush",&push)&&push&&*push==25.f);
        unsigned ready=99;
        assert(manager->catalog.concept_ready(&cooldown,"TLK_PLPUSH",&ready)==1&&!ready);
        f.now=25;
        assert(manager->catalog.concept_ready(&cooldown,"TLK_PLPUSH",&ready)==1&&ready);
        assert(manager->api.destroy(&manager->session,&manager->control,&manager->tables,&manager->b)==1);
    }
    if(native_propagation){
        int index=-1;void* row=nullptr;unsigned bytes=0;
        assert(sink.history.lookup(sink.history.context,f.neighbor_exp,"tlk_hello",&index)&&index==0);
        assert(sink.history.entry(sink.history.context,f.neighbor_exp,index,&row,&bytes)&&bytes==28);
        assert(VcdAuditArena::read_float(row,0x14)==10.f&&!F::word(row,0x18));
        assert(arena.retire.symbol_destroy(&arena,f.neighbor_exp+0x34)&&arena.release(&arena,F::word(f.neighbor_exp,0x14)));
    }
    if(history_tree){
        int index=-1;
        assert(sink.history.lookup(sink.history.context,f.exp,"tlk_hello",&index)&&index==0);
        void* row=nullptr;unsigned bytes=0;
        assert(sink.history.entry(sink.history.context,f.exp,index,&row,&bytes)&&bytes==28);
        assert(VcdAuditArena::read_float(row,0x14)==10.f&&F::word(row,0x18)==0);
        assert(arena.retire.symbol_destroy(&arena,f.exp+0x34)&&arena.release(&arena,F::word(f.exp,0x14)));
    }
    assert(runtime.destroy(f.root,&raw)==1&&arena.live.size()==1);
    assert(arena.release(&arena,F::word(f.actor,0x5b8))&&arena.live.empty());
    std::puts(native_propagation?"PASS parsed Jessica speech -> native manager -> silent companion concept history":deadline_runtime?"PASS parsed Jessica speech and subsequent concept share native 48-row deadline manager":managed_history?"PASS parsed Jessica speech drives native history and shared ally cooldown":history_tree?"PASS parsed Jessica speech with native indexed concept history and pooled symbols":actor_sink?"PASS parsed Jessica native speech history seed, expeditor timing, semaphore, captions and record retirement":actor_note?"PASS parsed speech native actor expeditor note, shared semaphore, caption and record lifecycle":retail?"PASS parsed speech native record admission/check/stop and forced sound effect":"PASS parsed VCD clock -> native speech, caption and actor record lifecycle");
}
inline void check_native_choreography_dispatch(const NativeChoreographyDispatchApi& api){
    using F=ScenePlaybackFixture;
    struct Fixture {
        alignas(4) unsigned char scene[0x5ec]{},root[0x220]{},event[0x3c0]{},actor[0x604]{};
        int target=0;unsigned offset=0,output=0,traces=0,missing=0,debug=0,ignored=0;
        bool found=true,target_found=true;
        static Fixture& self(void* p) noexcept{return *static_cast<Fixture*>(p);}
    } f;
    F::put(f.scene,0x414,F::ptr(f.scene));F::put(f.event,0x2b4,1);
    std::strcpy(reinterpret_cast<char*>(f.event+4),"event");std::strcpy(reinterpret_cast<char*>(f.event+0x84),"target");
    NativeChoreographyDispatchBindings b{};b.context=&f;b.scene=f.scene;b.scene_bytes=sizeof(f.scene);
    b.actor=[](void* p,void* scene,std::uint32_t word,void** out,unsigned* bytes) noexcept -> int{
        auto& f=Fixture::self(p);assert(scene==f.scene&&word==1);*out=f.found?f.actor:nullptr;*bytes=f.found?sizeof(f.actor):0;return 1;
    };
    b.target=[](void* p,void* scene,const char* name,void* actor,unsigned flag,void** out) noexcept -> int{
        auto& f=Fixture::self(p);assert(scene==f.scene&&actor==f.actor&&!flag&&!std::strcmp(name,"target"));*out=f.target_found?&f.target:nullptr;return 1;
    };
    b.virtual_event=[](void* p,void* scene,unsigned offset,void* parsed,void* event,void* actor,void* target,const char*) noexcept -> int{
        auto& f=Fixture::self(p);assert(scene==f.scene&&parsed==f.root&&event==f.event&&actor==f.actor);
        assert(!target||target==&f.target);f.offset=offset;return 1;
    };
    b.trace=[](void* p,void* scene,void* event,float,ChoreographyDispatchTrace trace) noexcept -> int{
        auto& f=Fixture::self(p);assert(scene==f.scene&&event==f.event);++f.traces;
        if(trace==ChoreographyDispatchTrace::MissingTarget||trace==ChoreographyDispatchTrace::MissingActor)++f.missing;
        if(trace==ChoreographyDispatchTrace::Ignored)++f.ignored;
        return 1;
    };
    b.output=[](void* p,void* output,void* actor,void* scene,float delay) noexcept -> int{
        auto& f=Fixture::self(p);assert(scene==f.scene&&actor==f.actor&&!delay);
        f.output=static_cast<unsigned>(static_cast<unsigned char*>(output)-f.scene);return 1;
    };
    b.debug_text=[](void* p,const char* text) noexcept -> int{
        assert(!std::strcmp(text,"hello"));++Fixture::self(p).debug;return 1;
    };
    auto call=[&](unsigned type,unsigned slot){F::put(f.event,0,type);f.offset=0;unsigned result=0;
        assert(api.invoke(F::ptr(f.scene+0x410),slot,0.f,f.root,f.event,&b,&result)==1&&result);return f.offset;};
    const unsigned start[]={0,0x32c,0x2cc,0x2e4,0x2ec,0,0x2dc,0x304,0x2fc,0,0x2d4,0x30c,0x328,0x310,0x330,0x320,0x318};
    const unsigned stop[]={0,0,0x2d0,0x2e8,0x2f0,0,0x2e0,0x308,0x300,0,0x2d8,0,0,0x314,0,0x324,0x31c};
    F::fl(f.event,0x184,2.f);F::fl(f.event,0x188,-1.f);
    for(unsigned type=0;type<=16;++type)if(type!=5){assert(call(type,0)==start[type]);assert(call(type,4)==stop[type]);}
    assert(F::word(f.event,0x188)==0x40400000u);
    f.target_found=false;assert(!call(3,0)&&f.missing==1);
    f.found=false;assert(!call(2,0)&&f.missing==2);f.found=true;
    for(unsigned i=1;i<=8;++i){f.event[0x84]=static_cast<unsigned char>('0'+i);f.event[0x85]=0;
        assert(!call(9,0)&&f.output==0x464+24*(i-1));}
    f.scene[0x548]=1;f.output=0;assert(!call(9,0)&&!f.output);f.scene[0x548]=0;
    std::strcpy(reinterpret_cast<char*>(f.event+0x84),"debugtext hello");
    assert(!call(16,0)&&f.debug==1&&!call(16,4));
    std::strcpy(reinterpret_cast<char*>(f.event+4),"NULL");
    assert(!call(2,0)&&f.ignored==1&&!call(2,4));
    F::put(f.scene,0x414,0);assert(!call(2,0)&&f.ignored==1);
    std::puts("PASS native non-speech event dispatch table, target and trigger policy");
}
inline void check_native_scene_leaves(const NativeChoreographySceneLeafApi& api){
    using F=ScenePlaybackFixture;
    struct Fixture {
        alignas(4) unsigned char scene[0x5ec]{},root[0x220]{},event[0x3c0]{},actor[0x604]{},rows[0x100]{};
        int target=0;
        unsigned begins=0,stops=0,arg=0,outputs=0,admit=0;
        static Fixture& self(void* p) noexcept{return *static_cast<Fixture*>(p);}
    } f;
    ActorEventRecordBindings records{};records.context=&f;
    records.storage=[](void* p,std::uint32_t w,void** out,unsigned* n) noexcept -> int{
        auto& f=Fixture::self(p);if(w!=F::ptr(f.rows))return 0;*out=f.rows;*n=sizeof(f.rows);return 1;};
    records.grow=[](void*,void*,unsigned) noexcept -> int{return 0;};
    records.handle=[](void* p,void* target,std::uint32_t* w) noexcept -> int{assert(target==&Fixture::self(p).target);*w=123;return 1;};
    records.diagnostic=[](void*,ActorEventDiagnostic,std::uint32_t) noexcept -> int{return 1;};
    records.begin=[](void* p,void* a,void* row,std::uint32_t root,void* e,std::uint32_t ca,void* target,unsigned* out) noexcept -> int{
        auto& f=Fixture::self(p);assert(a==f.actor&&e==f.event&&root==F::ptr(f.root)&&ca==1);
        assert(F::word(row,0x24)==(target?123u:0xffffffffu));++f.begins;*out=1;return 1;};
    records.stop=[](void* p,void*,void*,int arg,int zero,unsigned* out) noexcept -> int{
        auto& f=Fixture::self(p);assert(!zero);++f.stops;f.arg=static_cast<unsigned>(arg);*out=1;return 1;};
    NativeChoreographySceneLeafBindings b{};b.context=&f;b.scene_bytes=sizeof(f.scene);b.records=&records;
    b.actor_extent=[](void* p,void* a,unsigned* n) noexcept -> int{auto& f=Fixture::self(p);assert(a==f.actor);*n=sizeof(f.actor);return 1;};
    b.admit_instanced=[](void* p,void*,unsigned* out) noexcept -> int{*out=Fixture::self(p).admit;return 1;};
    b.now=[](void*,float* n) noexcept -> int{*n=10.f;return 1;};
    b.output=[](void* p,void* o,void* a,void* c,float d) noexcept -> int{
        auto& f=Fixture::self(p);assert(o==f.scene+0x434&&a==f.scene&&c==f.scene&&!d);++f.outputs;return 1;};
    F::put(f.actor,0x5b8,F::ptr(f.rows));F::put(f.actor,0x5bc,4);F::put(f.event,0x2b4,1);
    std::strcpy(reinterpret_cast<char*>(f.event+4),"gesture");
    std::strcpy(reinterpret_cast<char*>(f.event+0x84),"2.5");
    auto call=[&](unsigned offset,void* target=nullptr){return api.invoke(&b,f.scene,offset,f.root,f.event,f.actor,target,reinterpret_cast<const char*>(f.event+0x84));};
    assert(call(0x2cc)&&f.begins==1&&F::word(f.actor,0x5c4)==1);
    f.scene[0x548]=3;assert(call(0x2d0)&&f.stops==1&&!f.arg&&!F::word(f.actor,0x5c4));
    assert(call(0x2dc)&&call(0x2e0)&&f.begins==2&&f.stops==2&&f.arg==3);
    assert(call(0x2e4,&f.target)&&F::word(f.rows,0x24)==123&&call(0x2e8));
    b.instanced=1;assert(call(0x304)&&call(0x308)&&f.begins==3&&f.stops==3);
    assert(call(0x2ec,&f.target)&&f.begins==3);f.admit=1;
    assert(call(0x2ec,&f.target)&&call(0x2f0)&&f.begins==4&&f.stops==4);
    assert(call(0x310)&&!F::word(f.scene,0x524));f.scene[0x548]=0;
    assert(call(0x310)&&call(0x310)&&F::word(f.scene,0x524)==2);
    assert(call(0x314)&&call(0x314)&&call(0x314)&&!F::word(f.scene,0x524));
    F::put(f.event,0x190,2);F::put(f.event,0x194,1);
    assert(call(0x328)&&F::word(f.event,0x194)==0&&F::word(f.scene,0x3dc)==0x40200000&&F::word(f.root,0x78)==0x40200000);
    F::put(f.scene,0x3dc,0);assert(call(0x328)&&!F::word(f.scene,0x3dc));
    F::put(f.event,0x190,0xffffffffu);assert(call(0x328)&&F::word(f.scene,0x3dc)==0x40200000);
    F::put(f.event,0x184,0x3f800000);F::put(f.event,0x188,0x40400000);
    assert(call(0x320)&&F::word(f.actor,0x5e8)==0x41400000&&call(0x324)&&!F::word(f.actor,0x5e8));
    assert(call(0x330)&&f.scene[0x530]&&f.outputs==1);
    assert(call(0x32c)); // instanced section is a no-op
    std::strcpy(reinterpret_cast<char*>(f.event+4),"NULL");
    assert(call(0x2dc)&&call(0x2e0)&&f.begins==4&&f.stops==4);
}
inline void check_native_section(VcdAuditArena& arena,const NativeVcdRuntimeApi& runtime,const NativeChoreographyClockApi& clock,const NativeChoreographySectionApi& api){
    using F=ScenePlaybackFixture;
    const char data[]=R"VCD(event section hold
{
 time 1 -1
 param "noaction"
}
actor Jessica
{
 channel Motion
 {
  event gesture motion
  {
   time 0 4
   resumecondition
  }
 }
}
)VCD";
    struct Fixture {
        alignas(4) unsigned char scene[0x5ec]{},actor[0x604]{},rows[0x40]{};
        void* event=nullptr;std::uint32_t root=0;
        unsigned checks=0,ready=0,timeouts=0,outputs=0,finishes=0;
        float now=0,last=0;
        static Fixture& self(void* p) noexcept{return *static_cast<Fixture*>(p);}
    } f;
    F::put(f.scene,0x414,F::ptr(f.scene));f.scene[0x3d8]=1;
    auto raw=arena.runtime();
    assert(runtime.parse(f.scene+0x410,"section.vcd",data,sizeof(data)-1,&raw,&f.root)==1&&f.root);
    F::put(f.scene,0x40c,f.root);
    void *root=nullptr,*events=nullptr,*section=nullptr;unsigned n=0,extent=0;
    assert(arena.storage(&arena,f.root,&root,&extent)&&arena.storage(&arena,F::word(root,0),&events,&n)&&n>=8);
    assert(arena.storage(&arena,F::word(events,0),&section,&n)&&arena.storage(&arena,F::word(events,4),&f.event,&n));
    assert(F::word(section,0)==1&&F::word(f.event,0)==6);
    NativeChoreographyClockBindings tick{};tick.vectors=arena.vector_bindings;tick.release_memory=&VcdAuditArena::release;
    tick.scene=f.scene;tick.scene_bytes=sizeof(f.scene);
    tick.other_event=[](void*,std::uint32_t,unsigned,float,void*,void*) noexcept -> int{return 1;};
    assert(clock.reset(root,extent,1,0,0,&tick)==1&&clock.simulate(root,extent,.5f,&tick)==1&&F::word(root,0x5c)==1);
    ActorEventRecordBindings records{};records.context=&f;
    records.storage=[](void* p,std::uint32_t word,void** out,unsigned* bytes) noexcept -> int{
        auto& f=Fixture::self(p);if(word!=F::ptr(f.rows))return 0;*out=f.rows;*bytes=sizeof(f.rows);return 1;};
    records.check=[](void* p,void* actor,void* row,float time,std::uint32_t root,void* event,unsigned* out) noexcept -> int{
        auto& f=Fixture::self(p);assert(actor==f.actor&&row==f.rows&&time==.5f&&root==f.root&&event==f.event);++f.checks;*out=f.ready;return 1;};
    F::put(f.actor,0x5b8,F::ptr(f.rows));F::put(f.actor,0x5bc,1);F::put(f.actor,0x5c4,1);
    F::put(f.rows,0,F::ptr(f.event));F::put(f.rows,4,f.root);
    NativeChoreographyDispatchBindings dispatch{};dispatch.context=&f;dispatch.scene=f.scene;dispatch.scene_bytes=sizeof(f.scene);dispatch.records=&records;
    dispatch.actor=[](void* p,void* scene,std::uint32_t word,void** out,unsigned* bytes) noexcept -> int{
        auto& f=Fixture::self(p);assert(scene==f.scene&&word==F::word(f.event,0x2b4));*out=f.actor;*bytes=sizeof(f.actor);return 1;};
    dispatch.trace=[](void*,void*,void*,float,ChoreographyDispatchTrace) noexcept -> int{return 1;};
    NativeChoreographySectionBindings b{};b.context=&f;b.scene_bytes=sizeof(f.scene);b.clock=&tick;b.dispatch=&dispatch;
    b.clock_now=[](void* p,float* out) noexcept -> int{*out=Fixture::self(p).now;return 1;};
    b.last_think=[](void* p,void* scene,int i,float* out) noexcept -> int{auto& f=Fixture::self(p);assert(scene==f.scene&&!i);*out=f.last;return 1;};
    b.timeout=[](void* p,void* scene,int mode) noexcept -> int{auto& f=Fixture::self(p);assert(scene==f.scene&&mode==1);++f.timeouts;return 1;};
    b.output=[](void* p,void* out,void* a,void* c,float d) noexcept -> int{auto& f=Fixture::self(p);assert(out==f.scene+0x44c&&a==f.scene&&c==f.scene&&!d);++f.outputs;return 1;};
    b.finish=[](void* p,void* scene,int completed,int argument) noexcept -> int{auto& f=Fixture::self(p);assert(scene==f.scene&&completed==1&&!argument);++f.finishes;return 1;};
    auto* param=static_cast<char*>(section)+0x84;
    assert(api.enter(&b,f.scene,root,section,param)&&f.scene[0x3d9]&&!f.scene[0x3e4]);
    assert(api.tick(&b,f.scene)&&f.scene[0x3d9]&&f.checks==1);
    f.scene[0x528]=1;assert(api.tick(&b,f.scene)&&f.checks==1);f.scene[0x528]=0;
    f.ready=1;assert(api.tick(&b,f.scene)&&!f.scene[0x3d9]&&f.checks==2&&!F::word(root,0x5c));
    // Rebuild tracked conditions, then expire an automate Cancel timer.
    assert(clock.reset(root,extent,1,0,0,&tick)==1&&clock.simulate(root,extent,.5f,&tick)==1);
    f.ready=0;std::strcpy(param,"AuToMaTe Cancel 1.5");
    assert(api.enter(&b,f.scene,root,section,param)&&f.scene[0x3d9]&&f.scene[0x3e4]&&F::word(f.scene,0x3e8)==1);
    f.now=1;f.last=.5f;assert(api.tick(&b,f.scene)&&f.scene[0x3d9]&&!f.timeouts&&F::word(f.scene,0x3f0)==0x3f000000);
    f.now=2;f.last=1;assert(api.tick(&b,f.scene)&&!f.scene[0x3d8]&&!f.scene[0x3d9]&&f.timeouts==1&&f.outputs==1&&f.finishes==1);
    assert(!F::word(f.scene,0x3e8)&&!F::word(f.scene,0x3ec)&&!F::word(f.scene,0x3f0));
    assert(runtime.destroy(f.root,&raw)==1&&arena.live.empty());
}
inline void check_native_movement_gate(const NativeSceneMovementGateApi& api){
    using F=ScenePlaybackFixture;
    struct Fixture {
        alignas(4) unsigned char actor[0x7a0]{},scene[0x5ec]{},space[0x18]{},range[0x14]{},schedule[0x2c]{};
        unsigned npc=1,suppressed=0,maps=0,storage=0;
        static Fixture& self(void* p) noexcept{return *static_cast<Fixture*>(p);}
    } f;
    NativeSceneMovementGateBindings b{};b.context=&f;b.scene=f.scene;
    b.is_npc=[](void* p,void* actor,unsigned* out) noexcept -> int{auto& f=Fixture::self(p);assert(actor==f.actor);*out=f.npc;return 1;};
    b.actor_extent=[](void* p,void* actor,unsigned* out) noexcept -> int{auto& f=Fixture::self(p);assert(actor==f.actor);*out=sizeof(f.actor);return 1;};
    b.schedule_space=[](void* p,void* actor,void** out,unsigned* n) noexcept -> int{auto& f=Fixture::self(p);assert(actor==f.actor);++f.maps;*out=f.space;*n=sizeof(f.space);return 1;};
    b.storage=[](void* p,std::uint32_t word,void** out,unsigned* n) noexcept -> int{
        auto& f=Fixture::self(p);++f.storage;
        if(word==F::ptr(f.range)){*out=f.range;*n=sizeof(f.range);return 1;}
        if(word==F::ptr(f.schedule)){*out=f.schedule;*n=sizeof(f.schedule);return 1;}
        return 0;};
    b.suppressed=[](void* p,void* scene,void* actor) noexcept -> int{auto& f=Fixture::self(p);assert(scene==f.scene&&actor==f.actor);++f.suppressed;return 1;};
    auto* first=f.space+4;
    F::put(first,0,1000);F::put(first,4,9999);F::put(first,8,9999);F::put(first,0x10,F::ptr(f.range));
    F::put(f.range,0,2000);F::put(f.range,4,50);F::put(f.range,8,70);
    int id=99;assert(api.map_schedule(first,0x14,60,&b,&id)==1&&id==2010);
    assert(api.map_schedule(first,0x14,50,&b,&id)==1&&id==2000);
    assert(api.map_schedule(first,0x14,70,&b,&id)==1&&id==2020);
    assert(api.map_schedule(first,0x14,71,&b,&id)==1&&id==-1);
    assert(api.map_schedule(nullptr,0,-1,&b,&id)==1&&id==-1);
    F::put(f.range,0x10,F::ptr(f.range));assert(api.map_schedule(first,0x14,71,&b,&id)==-1);F::put(f.range,0x10,0);
    unsigned allowed=99;
    assert(api.admit(&b,nullptr,&allowed)&&!allowed&&!f.suppressed);
    f.npc=0;assert(api.admit(&b,f.actor,&allowed)&&!allowed&&!f.suppressed&&!f.maps);
    f.npc=1;assert(api.admit(&b,f.actor,&allowed)&&!allowed&&f.suppressed==1&&!f.maps);
    F::put(f.actor,0x798,F::ptr(f.schedule));F::put(f.actor,0x79c,2010);
    assert(api.admit(&b,f.actor,&allowed)&&allowed&&f.maps==1&&f.suppressed==1);
    F::put(f.actor,0x79c,2011);F::put(f.schedule,0xc,4);
    assert(api.admit(&b,f.actor,&allowed)&&allowed&&f.maps==2);
    F::put(f.schedule,0xc,2);assert(api.admit(&b,f.actor,&allowed)&&!allowed&&f.suppressed==2);
    F::put(f.actor,0x798,0x1234);assert(!api.admit(&b,f.actor,&allowed));
}
inline void check_composed_scene_execution(VcdAuditArena& arena,const NativeVcdRuntimeApi& runtime,const NativeSceneExecutionApi& api){
    using F=ScenePlaybackFixture;
    const char data[]=R"VCD(actor Jessica
{
 channel Motion
 {
  event expression smile
  {
   time 0 1
   param happy
  }
  event firetrigger trigger
  {
   time 0.5 -1
   param 2
  }
 }
}
event section pause
{
 time 1.5 -1
 param noaction
}
)VCD";
    struct Fixture {
        VcdAuditArena& arena;
        alignas(4) unsigned char scene[0x5ec]{},actor[0x604]{};
        unsigned starts=0,stops=0,outputs=0,traces=0,pre=0;
        std::uint32_t root=0;
        explicit Fixture(VcdAuditArena& a):arena(a){}
        static Fixture& self(void* p) noexcept{return *static_cast<Fixture*>(p);}
    } f(arena);
    F::put(f.scene,0x414,F::ptr(f.scene));f.scene[0x3d8]=1;
    auto raw=arena.runtime();
    assert(runtime.parse(f.scene+0x410,"composed.vcd",data,sizeof(data)-1,&raw,&f.root)==1&&f.root);
    F::put(f.scene,0x40c,f.root);
    ActorEventRecordBindings records{};records.context=&f;
    records.storage=[](void* p,std::uint32_t w,void** out,unsigned* n) noexcept -> int{
        auto& a=Fixture::self(p).arena;return a.storage(&a,w,out,n);};
    records.grow=[](void* p,void* v,unsigned extra) noexcept -> int{
        auto& a=Fixture::self(p).arena;return a.vectors.grow_vector(&a.vector_bindings,v,extra,0x40);};
    records.handle=[](void*,void*,std::uint32_t*) noexcept -> int{assert(false);return 0;};
    records.diagnostic=[](void*,ActorEventDiagnostic,std::uint32_t) noexcept -> int{assert(false);return 0;};
    records.begin=[](void* p,void* actor,void*,std::uint32_t root,void* event,std::uint32_t,void* target,unsigned* accepted) noexcept -> int{
        auto& f=Fixture::self(p);assert(actor==f.actor&&root==f.root&&F::word(event,0)==2&&!target);++f.starts;*accepted=1;return 1;};
    records.stop=[](void* p,void* actor,void* row,int arg,int zero,unsigned* accepted) noexcept -> int{
        auto& f=Fixture::self(p);assert(actor==f.actor&&F::word(row,4)==f.root&&!arg&&!zero);++f.stops;*accepted=1;return 1;};
    NativeSceneExecutionBindings b{};b.clock.vectors=arena.vector_bindings;b.clock.release_memory=&VcdAuditArena::release;
    b.dispatch.context=&f;b.dispatch.records=&records;
    b.dispatch.actor=[](void* p,void* scene,std::uint32_t word,void** out,unsigned* n) noexcept -> int{
        auto& f=Fixture::self(p);assert(scene==f.scene&&word);*out=f.actor;*n=sizeof(f.actor);return 1;};
    b.dispatch.trace=[](void* p,void* scene,void*,float,ChoreographyDispatchTrace) noexcept -> int{
        auto& f=Fixture::self(p);assert(scene==f.scene);++f.traces;return 1;};
    b.dispatch.output=[](void* p,void* output,void* actor,void* caller,float delay) noexcept -> int{
        auto& f=Fixture::self(p);assert(output==f.scene+0x47c&&actor==f.actor&&caller==f.scene&&!delay);++f.outputs;return 1;};
    b.leaves.context=&f;b.leaves.actor_extent=[](void* p,void* actor,unsigned* n) noexcept -> int{
        auto& f=Fixture::self(p);assert(actor==f.actor);*n=sizeof(f.actor);return 1;};
    // Distinct host contexts: arena for raw storage, fixture for engine
    // events, and independent section context not needed on noaction.
    b.advance.context=&f;
    b.advance.pre_tick=[](void* p,void* scene) noexcept -> int{auto& f=Fixture::self(p);assert(scene==f.scene);++f.pre;return 1;};
    b.advance.start=[](void*,void*) noexcept -> int{assert(false);return 0;};
    b.advance.parsed=[](void* p,std::uint32_t w,void** out,unsigned* n) noexcept -> int{
        auto& a=Fixture::self(p).arena;return a.storage(&a,w,out,n);};
    b.advance.latency=[](void*,std::uint32_t,float*) noexcept -> int{assert(false);return 0;};
    b.advance.finish=[](void*,void*,int,int) noexcept -> int{assert(false);return 0;};
    b.advance.release_actors=[](void*,void*,std::uint32_t) noexcept -> int{assert(false);return 0;};
    assert(api.reset(f.scene,sizeof(f.scene),1,0,0,&b)==1);
    F::fl(f.scene,0x3dc,.5f);
    assert(api.simulate(f.scene,sizeof(f.scene),.5f,&b)==1&&f.starts==1&&f.outputs==1&&F::word(f.actor,0x5c4)==1);
    F::fl(f.scene,0x3dc,1.5f);
    assert(api.simulate(f.scene,sizeof(f.scene),1.5f,&b)==1&&f.scene[0x3d9]&&!f.stops);
    assert(api.advance(f.scene,sizeof(f.scene),.25f,&b)==1&&f.pre==1&&!f.scene[0x3d9]&&!f.stops);
    assert(api.advance(f.scene,sizeof(f.scene),.25f,&b)==1&&f.pre==2&&f.stops==1&&!F::word(f.actor,0x5c4));
    assert(f.outputs==1&&F::word(f.scene,0x3dc)==0x3fe00000u);
    assert(runtime.destroy(f.root,&raw)==1&&arena.live.size()==1);
    assert(arena.release(&arena,F::word(f.actor,0x5b8))&&arena.live.empty());
}
// Parsed VCD -> scene clock -> scene virtual leaf -> native facial
// record admission -> native actor think -> removal and graph retirement.
// Real replacement SendProp/RecvProp scalar functions; no packet codec.
struct ParsedFacialClientPair {
    const FlexSendApi& send;const FlexRecvApi& recv;const ClientComposedFacialApi& frame;
    s::SendProp entity_props[24]{},overlay_prop{};
    s::SendTable entity,overlay;
    RecvProp client_props[24]{},client_overlay_prop{};
    RecvTable client_entity{client_props,24,nullptr,"DT_BaseEntity"},client_overlay{&client_overlay_prop,1,nullptr,"DT_OverlayVars"};
    const AnimSendApi& anim_send;const AnimRecvApi& anim_recv;
    sin_independent::EntityHandleEntry entries[1]{};
    AnimSendView av{};AnimRecvView arv{};FlexSendView sv{};FlexRecvView rv{};
    alignas(4) unsigned char face[client_facial_object_bytes]{};
    float previous=0;
    ParsedFacialClientPair(const FlexSendApi& s_,const FlexRecvApi& r_,const AnimSendApi& as,const AnimRecvApi& ar,const ClientComposedFacialApi& f)
        :send(s_),recv(r_),frame(f),anim_send(as),anim_recv(ar){
        assert(entity.Construct(entity_props,24,"DT_BaseEntity")&&overlay.Construct(&overlay_prop,1,"DT_OverlayVars"));
        AnimSendBindings ab{&entity,entries,1};AnimRecvBindings arb{&client_entity};
        assert(anim_send.create(&ab,&av)&&anim_recv.create(&arb,&arv));
        FlexSendBindings sb{av.base_animating,&overlay};FlexRecvBindings rb{arv.base_animating,&client_overlay};
        assert(send.create(&sb,&sv)&&recv.create(&rb,&rv));
        const int unknown=-1;std::memcpy(face+0xacc,&unknown,4);
    }
    ~ParsedFacialClientPair(){
        assert(send.destroy(&sv)&&recv.destroy(&rv));
        assert(anim_send.destroy(&av)&&anim_recv.destroy(&arv));
    }
    void check(unsigned char* actor,float expected,float expected_smooth,float now){
        auto& x=sv.weights->props[0];auto& y=rv.weights->props[0];
        s::DVariant value{};x.scalarProxy(&x,actor,actor+sv.flex->props[1].offset+x.offset,&value,0,1);
        RecvData d{&y,value,0,1};y.proxy(&d,face,face+rv.flex->props[1].offset+y.offset);
        ClientFacialController controller{3,0,1};
        ClientFacialFrameBindings blink{};
        blink.lookup=[](void*,const char*,int* out) noexcept -> int {*out=127;return 1;};
        ClientVfePhonemeBindings speech{};
        speech.resolve=[](void*,std::uint32_t,ClientPhonemeSentence*) noexcept -> int{return 0;};
        speech.map=[](void*,const char*) noexcept -> int{return 3;};
        ClientFlexOp ops[]={{2,0}};ClientFlexRule rule{0,1,ops};
        ClientComposedFacialInput in{};
        in.studio_present=in.rules_enabled=in.smoothing_enabled=true;
        in.now=now;in.blink_duration=.5f;in.window=.1f;in.dt=.033f;
        in.controllers=&controller;in.controller_count=1;in.blink=&blink;in.speech=&speech;
        in.rules=&rule;in.rule_count=in.descriptor_count=1;in.previous=&previous;
        ClientComposedFacialOutput out{};
        assert(frame.run(face,sizeof(face),&in,&out)==1&&out.descriptors_valid&&out.smoothed_valid);
        assert(std::fabs(out.global[3]-expected)<.00001f&&std::fabs(out.current[0]-expected)<.00001f);
        assert(std::fabs(out.smoothed[0]-expected_smooth)<.0001f);
        previous=out.smoothed[0];
    }
};
inline void check_parsed_facial_lifecycle(VcdAuditArena& arena,const NativeVcdRuntimeApi& runtime,
    const NativeSceneExecutionApi& execution,const ServerFacialRecordApi& admission,const ServerNamedFacialThinkApi& think,ParsedFacialClientPair& paired){
    using F=ScenePlaybackFixture;
    const char data[]=R"VCD(actor Jessica
{
 channel Face
 {
  event flexanimation smile
  {
   time 0 2
   flexanimations samples_use_time
   {
    "jaw" { 0.5 0.8 1.5 0.4 }
   }
  }
 }
}
)VCD";
    struct Fixture {
        VcdAuditArena& arena;
        alignas(4) unsigned char scene[0x5ec]{},actor[server_npc_random_expression_extent]{};
        unsigned maps{},starts{},stops{};float now=10;
        std::uint32_t root=0;
        explicit Fixture(VcdAuditArena& a):arena(a){}
        static Fixture& self(void* p) noexcept{return *static_cast<Fixture*>(p);}
        static int __cdecl storage(void* p,std::uint32_t w,void** out,unsigned* n) noexcept{
            auto& a=self(p).arena;return a.storage(&a,w,out,n);
        }
    } f(arena);
    F::put(f.scene,0x414,F::ptr(f.scene));f.scene[0x3d8]=1;f.actor[0x1f0]=1;
    auto raw=arena.runtime();
    assert(runtime.parse(f.scene+0x410,"face.vcd",data,sizeof(data)-1,&raw,&f.root)==1&&f.root);
    F::put(f.scene,0x40c,f.root);
    ServerFacialRecordBindings cfg{};cfg.base_flex=true;cfg.host.context=&f;
    cfg.host.storage=&Fixture::storage;
    cfg.host.grow=[](void* p,void* v,unsigned extra) noexcept -> int{
        auto& a=Fixture::self(p).arena;return a.vectors.grow_vector(&a.vector_bindings,v,extra,0x40);
    };
    cfg.host.handle=[](void*,void*,std::uint32_t*) noexcept -> int{assert(false);return 0;};
    cfg.host.diagnostic=[](void*,ActorEventDiagnostic,std::uint32_t) noexcept -> int{assert(false);return 0;};
    cfg.extent=[](void* p,void* actor,unsigned* n) noexcept -> int{
        auto& f=Fixture::self(p);assert(actor==f.actor);*n=sizeof(f.actor);return 1;
    };
    cfg.now=[](void* p,float* out) noexcept -> int{*out=Fixture::self(p).now;return 1;};
    ActorEventRecordBindings records{};assert(admission.adapt(&cfg,&records));
    NativeSceneExecutionBindings b{};b.clock.vectors=arena.vector_bindings;b.clock.release_memory=&VcdAuditArena::release;
    b.dispatch.context=&f;b.dispatch.records=&records;
    b.dispatch.actor=[](void* p,void* scene,std::uint32_t word,void** out,unsigned* n) noexcept -> int{
        auto& f=Fixture::self(p);assert(scene==f.scene&&word);*out=f.actor;*n=sizeof(f.actor);return 1;
    };
    b.dispatch.trace=[](void* p,void* scene,void* event,float,ChoreographyDispatchTrace kind) noexcept -> int{
        auto& f=Fixture::self(p);assert(scene==f.scene&&F::word(event,0)==10);
        if(kind==ChoreographyDispatchTrace::Start)++f.starts;
        else if(kind==ChoreographyDispatchTrace::Stop)++f.stops;
        return 1;
    };
    b.leaves.context=&f;b.leaves.actor_extent=cfg.extent;
    ServerFlexRange range{0,1};
    ServerFlexAnimationBindings flex{};flex.context=&f;flex.ranges=&range;flex.count=1;flex.storage=&Fixture::storage;
    // Controller lookup is now native; the host callback is intentionally null.
    ServerComposedFacialThinkBindings facial{};facial.native_intensity=true;
    facial.base.context=&f;facial.base.ranges=&range;facial.base.count=1;
    facial.base.storage=&Fixture::storage;facial.base.flex=&flex;facial.base.base_flex=1;
    const char* names[]={"JAW"};ServerVfeRemapEntry entries[1]{};ServerVfeRemapCache cache{entries,0,1};
    ServerNamedFacialThinkBindings named{facial,names,1,&cache};
    assert(execution.reset(f.scene,sizeof(f.scene),1,0,0,&b)==1);
    auto simulate=[&](float time){
        F::fl(f.scene,0x3dc,time);return execution.simulate(f.scene,sizeof(f.scene),time,&b);
    };
    assert(simulate(.5f)==1&&f.starts==1&&F::word(f.actor,0x5c4)==1);
    void* rows=nullptr;unsigned n=0;
    assert(Fixture::storage(&f,F::word(f.actor,0x5b8),&rows,&n)&&n>=0x40);
    assert(VcdAuditArena::read_float(rows,0x20)==1);
    assert(think.run(f.actor,sizeof(f.actor),10,&named)&&!f.maps);
    assert(std::fabs(VcdAuditArena::read_float(f.actor,0x4a8)-.8f)<.00001f&&f.actor[0x200]);
    paired.check(f.actor,.8f,.16f,10);
    assert(simulate(1.5f)==1&&F::word(f.actor,0x5c4)==1);
    assert(think.run(f.actor,sizeof(f.actor),11,&named)&&!f.maps);
    assert(std::fabs(VcdAuditArena::read_float(f.actor,0x4a8)-.4f)<.00001f);
    paired.check(f.actor,.4f,.208f,11);
    // Original classifier uses the lower bound (previous time when forward),
    // not the new target. Crossing the end is still an update; the following
    // step retires the record.
    assert(simulate(2.5f)==1&&!f.stops&&F::word(f.actor,0x5c4)==1);
    assert(simulate(2.75f)==1&&f.stops==1&&!F::word(f.actor,0x5c4));
    assert(think.run(f.actor,sizeof(f.actor),12,&named));
    assert(std::fabs(VcdAuditArena::read_float(f.actor,0x4a8)-.38f)<.00001f);
    paired.check(f.actor,.38f,.2424f,12);
    assert(runtime.destroy(f.root,&raw)==1&&arena.live.size()==1);
    assert(arena.release(&arena,F::word(f.actor,0x5b8))&&arena.live.empty());
    std::puts("PASS parsed VCD facial lifecycle, native controller mapping and paired client descriptor smoothing");
}
inline void check_parsed_cached_expression(VcdAuditArena& arena,const NativeVcdRuntimeApi& runtime,
    const NativeSceneExecutionApi& execution,const ServerFacialRecordApi& admission,const ServerCachedFacialThinkApi& think,const ServerVfeCacheApi& bank_api,const ServerVfeRemapApi& remap_api,ParsedFacialClientPair& paired,bool retail=false){
    using F=ScenePlaybackFixture;
    const char data[]=R"VCD(actor Jessica
{
 channel Face
 {
  event expression smile
  {
   time 0 2
   param phonemes_test
   param2 smile
  }
 }
}
)VCD";
    struct Fixture {
        VcdAuditArena& arena;
        alignas(4) unsigned char scene[0x5ec]{},actor[server_npc_random_expression_extent]{};
        alignas(4) unsigned char source[256]{},copy[256]{};
        unsigned reads{},closes{},allocs{},frees{},starts{},stops{},rejections{};float now=10;
        bool retail=false;
        std::uint32_t root=0;
        explicit Fixture(VcdAuditArena& a):arena(a){}
        static Fixture& self(void* p) noexcept{return *static_cast<Fixture*>(p);}
        static int __cdecl storage(void* p,std::uint32_t w,void** out,unsigned* n) noexcept{
            auto& a=self(p).arena;return a.storage(&a,w,out,n);
        }
    } f(arena);f.retail=retail;
    F::put(f.scene,0x414,F::ptr(f.scene));f.scene[0x3d8]=1;f.actor[0x1f0]=1;
    auto raw=arena.runtime();
    assert(runtime.parse(f.scene+0x410,"face.vcd",data,sizeof(data)-1,&raw,&f.root)==1&&f.root);
    F::put(f.scene,0x40c,f.root);
    ServerFacialRecordBindings cfg{};cfg.base_flex=true;cfg.host.context=&f;
    cfg.host.storage=&Fixture::storage;
    cfg.host.grow=[](void* p,void* v,unsigned extra) noexcept -> int{
        auto& a=Fixture::self(p).arena;return a.vectors.grow_vector(&a.vector_bindings,v,extra,0x40);
    };
    cfg.host.handle=[](void*,void*,std::uint32_t*) noexcept -> int{assert(false);return 0;};
    cfg.host.diagnostic=[](void* p,ActorEventDiagnostic why,std::uint32_t root) noexcept -> int{
        auto& f=Fixture::self(p);assert(f.retail&&why==ActorEventDiagnostic::StartRejected&&root==f.root);++f.rejections;return 1;
    };
    cfg.extent=[](void* p,void* actor,unsigned* n) noexcept -> int{
        auto& f=Fixture::self(p);assert(actor==f.actor);*n=sizeof(f.actor);return 1;
    };
    cfg.now=[](void* p,float* out) noexcept -> int{*out=Fixture::self(p).now;return 1;};
    // Expression virtual admission remains hosted, unlike flexanimation.
    cfg.host.begin=[](void* p,void* actor,void*,std::uint32_t root,void* event,std::uint32_t,void* target,unsigned* accepted) noexcept -> int{
        auto& f=Fixture::self(p);assert(!f.retail&&actor==f.actor&&root==f.root&&!target&&F::word(event,0)==2);
        assert(!std::strcmp(static_cast<const char*>(event)+0x84,"phonemes_test"));
        assert(!std::strcmp(static_cast<const char*>(event)+0x104,"smile"));
        *accepted=1;return 1;
    };
    cfg.host.stop=[](void* p,void* actor,void* row,int arg,int zero,unsigned* accepted) noexcept -> int{
        auto& f=Fixture::self(p);assert(!f.retail&&actor==f.actor&&F::word(row,4)==f.root&&!arg&&!zero);*accepted=1;return 1;
    };
    cfg.host.check=[](void*,void*,void*,float,std::uint32_t,void* event,unsigned* accepted) noexcept -> int{
        assert(F::word(event,0)==2);*accepted=1;return 1;
    };
    ActorEventRecordBindings records{};assert(admission.adapt(&cfg,&records));
    NativeSceneExecutionBindings b{};b.clock.vectors=arena.vector_bindings;b.clock.release_memory=&VcdAuditArena::release;
    b.dispatch.context=&f;b.dispatch.records=&records;
    b.dispatch.actor=[](void* p,void* scene,std::uint32_t word,void** out,unsigned* n) noexcept -> int{
        auto& f=Fixture::self(p);assert(scene==f.scene&&word);*out=f.actor;*n=sizeof(f.actor);return 1;
    };
    b.dispatch.trace=[](void* p,void* scene,void* event,float,ChoreographyDispatchTrace kind) noexcept -> int{
        auto& f=Fixture::self(p);assert(scene==f.scene&&F::word(event,0)==2);
        if(kind==ChoreographyDispatchTrace::Start)++f.starts;
        else if(kind==ChoreographyDispatchTrace::Stop)++f.stops;
        return 1;
    };
    b.leaves.context=&f;b.leaves.actor_extent=cfg.extent;
    // Synthetic EFV with one setting and two additive contributions to jaw.
    std::memcpy(f.source,"EFV",4);F::put(f.source,0x48,256);F::put(f.source,0x4c,1);F::put(f.source,0x50,108);
    F::put(f.source,0x60,1);F::put(f.source,0x64,156);F::put(f.source,0x68,160);
    F::put(f.source,108,64);F::put(f.source,116,2);F::put(f.source,128,24);
    F::put(f.source,132,0);F::fl(f.source,136,.2f);F::put(f.source,144,0);F::fl(f.source,148,.2f);
    F::put(f.source,156,192);F::put(f.source,160,0xffffffffu);
    std::strcpy(reinterpret_cast<char*>(f.source+172),"smile");
    std::strcpy(reinterpret_cast<char*>(f.source+192),"jaw");
    ServerVfeCacheEntry banks[1]{};ServerVfeCache bank{banks,0,1};
    ServerVfeRemapEntry entries[1]{};ServerVfeRemapCache remap{entries,0,1};
    ServerVfeCacheBindings files{};files.context=&f;
    files.read=[](void* p,const char* path,const unsigned char** out,unsigned* n) noexcept -> int{
        auto& f=Fixture::self(p);assert(!std::strcmp(path,"expressions/phonemes_test.vfe"));++f.reads;
        *out=f.source;*n=sizeof(f.source);return 1;
    };
    files.close=[](void* p,const unsigned char* q) noexcept -> int{
        auto& f=Fixture::self(p);assert(q==f.source);++f.closes;return 1;
    };
    files.allocate=[](void* p,unsigned n,void** out) noexcept -> int{
        auto& f=Fixture::self(p);assert(n==sizeof(f.copy)&&!f.allocs);++f.allocs;*out=f.copy;return 1;
    };
    files.release=[](void* p,void* q) noexcept -> int{
        auto& f=Fixture::self(p);assert(q==f.copy);++f.frees;return 1;
    };
    ServerFlexRange range{0,1};
    ServerExpressionBindings expression{};expression.ranges=&range;expression.count=1;
    const char* names[]={"JAW"};
    ServerCachedFacialThinkBindings cached{};cached.banks=&bank;cached.files=&files;
    cached.named.controller_names=names;cached.named.controller_count=1;cached.named.remap=&remap;
    cached.named.composed.native_intensity=true;
    auto& facial=cached.named.composed.base;
    facial.context=&f;facial.ranges=&range;facial.count=1;
    facial.storage=&Fixture::storage;facial.expression=&expression;facial.base_expression=1;
    assert(execution.reset(f.scene,sizeof(f.scene),1,0,0,&b)==1);
    auto simulate=[&](float time){
        F::fl(f.scene,0x3dc,time);return execution.simulate(f.scene,sizeof(f.scene),time,&b);
    };
    if(retail){
        assert(simulate(.5f)==1&&f.starts==1&&f.rejections==1&&!F::word(f.actor,0x5c4)&&!F::word(f.actor,0x5b8));
        assert(think.run(f.actor,sizeof(f.actor),10,&cached));
        paired.check(f.actor,0,0,10);
        assert(simulate(1.5f)==1&&f.rejections==1&&!F::word(f.actor,0x5c4));
        assert(think.run(f.actor,sizeof(f.actor),11,&cached));
        paired.check(f.actor,0,0,11);
        assert(simulate(2.5f)==1&&!f.stops);
        assert(simulate(2.75f)==1&&f.stops==1&&!F::word(f.actor,0x5c4));
        assert(think.run(f.actor,sizeof(f.actor),12,&cached));
        paired.check(f.actor,0,0,12);
        assert(!bank.count&&!remap.count&&!f.reads&&!f.allocs&&!f.closes&&!f.frees);
        assert(runtime.destroy(f.root,&raw)==1&&arena.live.empty());
        std::puts("PASS retail expression admission rejection preserves scene clock without facial record or VFE read");
        return;
    }
    assert(simulate(.5f)==1&&f.starts==1&&F::word(f.actor,0x5c4)==1);
    void* rows=nullptr;unsigned n=0;
    assert(Fixture::storage(&f,F::word(f.actor,0x5b8),&rows,&n)&&n>=0x40);
    assert(think.run(f.actor,sizeof(f.actor),10,&cached)&&static_cast<unsigned char*>(rows)[0xc]);
    assert(bank.count==1&&remap.count==1&&entries[0].bank==banks[0].bank.data);
    assert(f.reads==1&&f.closes==1&&f.allocs==1);
    paired.check(f.actor,.4f,.08f,10);
    assert(simulate(1.5f)==1&&F::word(f.actor,0x5c4)==1);
    assert(think.run(f.actor,sizeof(f.actor),11,&cached)&&f.reads==1);
    paired.check(f.actor,.78f,.22f,11);
    assert(simulate(2.5f)==1&&!f.stops&&F::word(f.actor,0x5c4)==1);
    assert(simulate(2.75f)==1&&f.stops==1&&!F::word(f.actor,0x5c4));
    assert(think.run(f.actor,sizeof(f.actor),12,&cached)&&f.reads==1);
    paired.check(f.actor,.741f,.3242f,12);
    assert(remap_api.clear(&remap)&&bank_api.clear(&bank,&files)&&!bank.count&&!remap.count&&f.frees==1);
    assert(runtime.destroy(f.root,&raw)==1&&arena.live.size()==1);
    assert(arena.release(&arena,F::word(f.actor,0x5b8))&&arena.live.empty());
    std::puts("PASS parsed VCD cached expression, paired client descriptor smoothing and cache retirement");
}
inline void check_parsed_npc_weapon_scene(VcdAuditArena& arena,const NativeVcdRuntimeApi& runtime,
    const NativeSceneExecutionApi& execution,const ServerNpcSceneRecordApi& admission,const ActorEventRecordApi& record_api,
    const ServerNpcWeaponSceneApi& weapons,const ServerNamedFacialThinkApi& think){
    using F=ScenePlaybackFixture;
    const char data[]=R"VCD(actor Jessica
{
 channel Actions
 {
  event generic lower
  {
   time 0 2
   param AI_HOLSTER
  }
  event generic raise
  {
   time 3 5
   param AI_UNHOLSTER
  }
 }
}
)VCD";
    struct Fixture {
        VcdAuditArena& arena;const ServerNpcWeaponSceneApi& weapons;
        alignas(4) unsigned char scene[0x5ec]{},actor[server_npc_random_expression_extent]{},layers[0x40]{};
        unsigned starts=0,stops=0,gestures=0,slots=0,selects=0,updates=0;
        std::uint32_t root=0;
        explicit Fixture(VcdAuditArena& a,const ServerNpcWeaponSceneApi& w):arena(a),weapons(w){}
        static Fixture& self(void* p) noexcept{return *static_cast<Fixture*>(p);}
        static int __cdecl storage(void* p,std::uint32_t w,void** out,unsigned* n) noexcept{
            auto& a=self(p).arena;return a.storage(&a,w,out,n);
        }
    } f(arena,weapons);
    F::put(f.scene,0x414,F::ptr(f.scene));f.scene[0x3d8]=1;f.actor[0x1f0]=1;
    F::put(f.actor,0x494,F::ptr(f.layers));F::put(f.actor,0x4a0,1);
    f.layers[0]=1;F::put(f.layers,8,7);F::fl(f.layers,0xc,.25f);
    auto raw=arena.runtime();
    assert(runtime.parse(f.scene+0x410,"weapon_scene.vcd",data,sizeof(data)-1,&raw,&f.root)==1&&f.root);
    F::put(f.scene,0x40c,f.root);
    ServerNpcSceneRecordBindings cfg{};auto& facial=cfg.facial;
    facial.base_flex=facial.npc_random=true;facial.host.context=&f;
    facial.host.storage=&Fixture::storage;
    facial.host.grow=[](void* p,void* v,unsigned extra) noexcept -> int{
        auto& a=Fixture::self(p).arena;return a.vectors.grow_vector(&a.vector_bindings,v,extra,0x40);
    };
    facial.host.handle=[](void*,void*,std::uint32_t*) noexcept -> int{assert(false);return 0;};
    facial.host.diagnostic=[](void*,ActorEventDiagnostic,std::uint32_t) noexcept -> int{assert(false);return 0;};
    facial.extent=[](void* p,void* actor,unsigned* n) noexcept -> int{
        auto& f=Fixture::self(p);assert(actor==f.actor);*n=sizeof(f.actor);return 1;
    };
    facial.now=[](void*,float* out) noexcept -> int{*out=10;return 1;};
    auto& w=cfg.weapons;w.context=&f;
    w.active=[](void*,void*,std::uint32_t* out) noexcept -> int{*out=11;return 1;};
    w.weapon_at=[](void* p,void*,unsigned i,std::uint32_t* out) noexcept -> int{
        auto& f=Fixture::self(p);++f.slots;*out=i==2?22u:0u;return 1;
    };
    w.gesture=[](void* p,void*,int activity,int autokill,int* layer) noexcept -> int{
        auto& f=Fixture::self(p);assert(autokill==1&&activity==(f.gestures?0x44:0x45));++f.gestures;*layer=0;return 1;
    };
    w.select=[](void* p,void*,std::uint32_t weapon) noexcept -> int{
        auto& f=Fixture::self(p);assert(weapon==22);++f.selects;return 1;
    };
    w.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{
        auto& f=Fixture::self(p);assert(token==F::ptr(f.layers));*out=f.layers;*n=sizeof(f.layers);return 1;
    };
    w.duration=[](void*,void*,int seq,double* out) noexcept -> int{assert(seq==7);*out=2.;return 1;};
    ActorEventRecordBindings records{};assert(admission.adapt(&cfg,&records));
    NativeSceneExecutionBindings b{};b.clock.vectors=arena.vector_bindings;b.clock.release_memory=&VcdAuditArena::release;
    b.dispatch.context=&f;b.dispatch.records=&records;
    b.dispatch.actor=[](void* p,void* scene,std::uint32_t word,void** out,unsigned* n) noexcept -> int{
        auto& f=Fixture::self(p);assert(scene==f.scene&&word);*out=f.actor;*n=sizeof(f.actor);return 1;
    };
    b.dispatch.trace=[](void* p,void* scene,void* event,float,ChoreographyDispatchTrace kind) noexcept -> int{
        auto& f=Fixture::self(p);assert(scene==f.scene&&F::word(event,0)==16);
        if(kind==ChoreographyDispatchTrace::Start)++f.starts;
        else if(kind==ChoreographyDispatchTrace::Stop)++f.stops;
        return 1;
    };
    b.leaves.context=&f;b.leaves.actor_extent=facial.extent;
    ServerFlexRange range{0,1};const char* names[]={"jaw"};
    ServerVfeRemapEntry entry[1]{};ServerVfeRemapCache remap{entry,0,1};
    ServerNamedFacialThinkBindings think_cfg{};think_cfg.controller_names=names;think_cfg.controller_count=1;think_cfg.remap=&remap;
    auto& base=think_cfg.composed.base;base.context=&f;base.ranges=&range;base.count=1;base.storage=&Fixture::storage;
    // Existing actor think's explicit other leaf calls the recovered no-op.
    base.other=[](void* p,void*,void* row,void*,void* event,unsigned* accepted) noexcept -> int{
        auto& f=Fixture::self(p);++f.updates;return f.weapons.noop(row,0x40,event,0x3c0,accepted)==1;
    };
    assert(execution.reset(f.scene,sizeof(f.scene),1,0,0,&b)==1);
    auto simulate=[&](float time){F::fl(f.scene,0x3dc,time);return execution.simulate(f.scene,sizeof(f.scene),time,&b);};
    auto verify=[&](unsigned subtype,float early,float late){
        void *rows=nullptr,*event=nullptr;unsigned n=0;
        assert(F::word(f.actor,0x5c4)==1&&Fixture::storage(&f,F::word(f.actor,0x5b8),&rows,&n)&&n>=0x40);
        assert(F::word(rows,0x38)==subtype&&!F::word(rows,0x10));
        assert(Fixture::storage(&f,F::word(rows,0),&event,&n)&&n>=0x3c0);
        unsigned accepted=0;
        assert(record_api.check(f.actor,sizeof(f.actor),early,f.root,event,&records,&accepted)==1&&accepted==1);
        assert(record_api.check(f.actor,sizeof(f.actor),late,f.root,event,&records,&accepted)==1&&accepted==0);
        assert(think.run(f.actor,sizeof(f.actor),10,&think_cfg)&&static_cast<unsigned char*>(rows)[0xc]);
    };
    assert(simulate(.5f)==1&&f.starts==1&&f.gestures==1);verify(2,.5f,.75f);
    assert(simulate(2.5f)==1&&simulate(2.75f)==1&&f.stops==1&&!F::word(f.actor,0x5c4));
    // Duration-bearing events are classified against the previous time, not
    // the new target. Crossing start first leaves the event inactive.
    assert(simulate(3.25f)==1&&f.starts==1&&f.gestures==1&&!f.slots&&!f.selects&&!F::word(f.actor,0x5c4));
    assert(simulate(3.5f)==1&&f.starts==2&&f.gestures==2&&f.slots==4&&f.selects==1);verify(3,3.5f,3.75f);
    assert(simulate(5.5f)==1&&simulate(5.75f)==1&&f.stops==2&&!F::word(f.actor,0x5c4)&&f.updates==2);
    assert(runtime.destroy(f.root,&raw)==1&&arena.live.size()==1);
    assert(arena.release(&arena,F::word(f.actor,0x5b8))&&arena.live.empty());
    std::puts("PASS parsed generic NPC weapon scene lifecycle with native admission and completion");
}
inline void check_parsed_npc_animation_scene(VcdAuditArena& arena,const NativeVcdRuntimeApi& runtime,
    const NativeSceneExecutionApi& execution,const ServerNpcAnimationRecordApi& admission,const ActorEventRecordApi& record_api,
    const ServerAnimationSceneFrameApi& frame,const ServerGestureMetadataApi& metadata,const ServerNpcAnimationThinkApi* think=nullptr,const ServerNpcAnimationGateApi* gate=nullptr){
    using F=ScenePlaybackFixture;
    const char data[]=R"VCD(actor Jessica
{
 channel Gestures
 {
  event gesture wave
  {
   time 0 2
   param gesture_anim
   absolutetags playback_time
   {
    "loop" 0.25
    "end" 0.75
   }
   absolutetags shifted_time
   {
    "loop" 0.125
    "end" 0.5
   }
   sequenceduration 2
  }
 }
 channel Body
 {
  event sequence stand
  {
   time 3 5
   param sequence_anim
  }
 }
}
)VCD";
    struct Fixture {
        VcdAuditArena& arena;
        alignas(4) unsigned char scene[0x5ec]{},actor[server_npc_random_expression_extent]{},layer[0x40]{};
        unsigned starts{},stops{},lookups{},blends{},allocs{},keepalives{},warnings{};
        float last_blend{};
        unsigned condition=1;int activity=3;
        const ServerNpcAnimationGateApi* gate=nullptr;
        ServerNpcAnimationGateBindings gate_bindings{};
        unsigned gate_queries=0,gate_capability=0,gate_sets=0;
        std::uint32_t root=0;
        explicit Fixture(VcdAuditArena& a):arena(a){}
        static Fixture& self(void* p) noexcept{return *static_cast<Fixture*>(p);}
        static int __cdecl storage(void* p,std::uint32_t w,void** out,unsigned* n) noexcept{
            auto& f=self(p);
            if(w==F::ptr(f.layer)){*out=f.layer;*n=sizeof(f.layer);return 1;}
            auto& a=f.arena;return a.storage(&a,w,out,n);
        }
    } f(arena);f.gate=gate;
    assert(!gate||think);
    F::put(f.scene,0x414,F::ptr(f.scene));f.scene[0x3d8]=1;
    F::put(f.actor,0x494,F::ptr(f.layer));F::put(f.actor,0x4a0,1);f.layer[0]=1;
    auto raw=arena.runtime();
    assert(runtime.parse(f.scene+0x410,"animation_scene.vcd",data,sizeof(data)-1,&raw,&f.root)==1&&f.root);
    F::put(f.scene,0x40c,f.root);
    ServerNpcAnimationRecordBindings cfg{};auto& facial=cfg.npc.facial;
    facial.base_flex=facial.npc_random=true;facial.host.context=&f;
    facial.host.storage=&Fixture::storage;
    facial.host.grow=[](void* p,void* v,unsigned extra) noexcept -> int{
        auto& a=Fixture::self(p).arena;return a.vectors.grow_vector(&a.vector_bindings,v,extra,0x40);
    };
    facial.host.handle=[](void*,void*,std::uint32_t*) noexcept -> int{assert(false);return 0;};
    facial.host.diagnostic=[](void*,ActorEventDiagnostic,std::uint32_t) noexcept -> int{assert(false);return 0;};
    facial.extent=[](void* p,void* actor,unsigned* n) noexcept -> int{
        auto& f=Fixture::self(p);assert(actor==f.actor);*n=sizeof(f.actor);return 1;
    };
    facial.now=[](void*,float* out) noexcept -> int{*out=10.f;return 1;};
    cfg.animation.context=&f;
    cfg.animation.lookup=[](void* p,void* actor,const char* name,int* out) noexcept -> int{
        auto& f=Fixture::self(p);assert(actor==f.actor);
        if(!std::strcmp(name,"gesture_anim"))*out=7;
        else if(!std::strcmp(name,"sequence_anim"))*out=9;
        else {assert(false);return 0;}
        ++f.lookups;return 1;
    };
    cfg.animation.blend_out=[](void* p,void* actor,int layer,float duration,float delay) noexcept -> int{
        auto& f=Fixture::self(p);assert(actor==f.actor&&layer==0&&delay==0);++f.blends;f.last_blend=duration;return 1;
    };
    ActorEventRecordBindings records{};assert(admission.adapt(&cfg,&records));
    NativeSceneExecutionBindings b{};b.clock.vectors=arena.vector_bindings;b.clock.release_memory=&VcdAuditArena::release;
    b.dispatch.context=&f;b.dispatch.records=&records;
    b.dispatch.actor=[](void* p,void* scene,std::uint32_t word,void** out,unsigned* n) noexcept -> int{
        auto& f=Fixture::self(p);assert(scene==f.scene&&word);*out=f.actor;*n=sizeof(f.actor);return 1;
    };
    b.dispatch.trace=[](void* p,void* scene,void* event,float,ChoreographyDispatchTrace kind) noexcept -> int{
        auto& f=Fixture::self(p);assert(scene==f.scene&&(F::word(event,0)==6||F::word(event,0)==7));
        if(kind==ChoreographyDispatchTrace::Start)++f.starts;
        else if(kind==ChoreographyDispatchTrace::Stop)++f.stops;
        return 1;
    };
    b.leaves.context=&f;b.leaves.actor_extent=facial.extent;
    ServerAnimationSceneFrameBindings fb{};fb.context=&f;fb.storage=&Fixture::storage;
    ServerVfeRemapCache remap{};
    ServerNpcAnimationThinkBindings tb{};tb.context=&f;
    tb.facial.named.remap=&remap;tb.facial.named.composed.base.context=&f;
    tb.facial.named.composed.base.storage=&Fixture::storage;
    tb.facial.named.composed.native_intensity=true;
    tb.animation.admission=cfg.animation;tb.animation.frame=fb;tb.animation.context=&f;
    tb.animation.metadata=[](void*,void*,int seq,ServerGestureMetadata* out) noexcept -> int{
        assert(seq==7);out->available=1;return 1;
    };
    tb.animation.sequence_gate=[](void* p,void* actor,unsigned* out) noexcept -> int{
        auto& f=Fixture::self(p);
        if(f.gate)return f.gate->admit(&f.gate_bindings,actor,out);
        *out=1;return 1;
    };
    if(gate){
        f.condition=0;
        F::put(f.actor,0x970,0xffffffffu);
        f.gate_bindings.context=&f;
        f.gate_bindings.npc=[](void* p,void* actor,void** npc,unsigned* n) noexcept -> int{
            auto& f=Fixture::self(p);assert(actor==f.actor);*npc=f.actor;*n=sizeof(f.actor);return 1;
        };
        f.gate_bindings.condition=[](void* p,void* npc,int id,int flag,unsigned* out) noexcept -> int{
            auto& f=Fixture::self(p);assert(npc==f.actor&&id==0x3c&&flag==1);
            ++f.gate_queries;*out=f.condition;return 1;
        };
        f.gate_bindings.can_enter=[](void* p,void* npc,unsigned* out) noexcept -> int{
            auto& f=Fixture::self(p);assert(npc==f.actor);++f.gate_capability;*out=1;return 1;
        };
        f.gate_bindings.set_condition=[](void* p,void* npc,int id) noexcept -> int{
            auto& f=Fixture::self(p);assert(npc==f.actor&&id==0x3c);++f.gate_sets;f.condition=1;return 1;
        };
    }
    tb.animation.suppressed=[](void*,void*,unsigned* out) noexcept -> int{*out=0;return 1;};
    tb.animation.allocate_layer=[](void* p,void* actor,int seq,int priority,int* out) noexcept -> int{
        auto& f=Fixture::self(p);assert(actor==f.actor&&((seq==7&&priority==0)||(seq==9&&priority==1)));
        ++f.allocs;std::memset(f.layer,0,sizeof(f.layer));f.layer[0]=1;*out=0;return 1;
    };
    tb.animation.sequence_info=[](void*,void*,int seq,unsigned* flags,double* duration) noexcept -> int{
        assert(seq==7||seq==9);*flags=0;*duration=4.;return 1;
    };
    tb.npc_state=[](void* p,void* actor,void** npc,unsigned* condition,int* activity) noexcept -> int{
        auto& f=Fixture::self(p);assert(actor==f.actor);*npc=actor;*condition=f.condition;*activity=f.activity;return 1;
    };
    tb.keepalive=[](void* p,void* npc,float duration) noexcept -> int{
        auto& f=Fixture::self(p);assert(npc==f.actor&&duration==.2f);++f.keepalives;return 1;
    };
    if(gate)tb.keepalive=nullptr;
    tb.interrupted=[](void* p,void* actor) noexcept -> int{
        auto& f=Fixture::self(p);assert(actor==f.actor);++f.warnings;return 1;
    };
    assert(execution.reset(f.scene,sizeof(f.scene),1,0,0,&b)==1);
    auto simulate=[&](float time){F::fl(f.scene,0x3dc,time);return execution.simulate(f.scene,sizeof(f.scene),time,&b);};
    auto verify=[&](unsigned type,unsigned sequence,float time){
        void *row=nullptr,*event=nullptr;unsigned n=0,accepted=0;
        assert(F::word(f.actor,0x5c4)==1&&Fixture::storage(&f,F::word(f.actor,0x5b8),&row,&n)&&n>=0x40);
        assert(F::word(row,0x18)==sequence&&F::word(row,0x10)==0xfffffffeu);
        assert(Fixture::storage(&f,F::word(row,0),&event,&n)&&n>=0x3c0&&F::word(event,0)==type);
        assert(F::word(row,8)==F::word(event,0x2b4));
        assert(record_api.check(f.actor,sizeof(f.actor),time,f.root,event,&records,&accepted)==1&&accepted);
        if(think){
            f.actor[0x600]=1;
            assert(think->run(f.actor,sizeof(f.actor),10+time,&tb));
            assert(!f.actor[0x600]&&static_cast<unsigned char*>(row)[0xc]&&F::word(row,0x10)==0);
            assert(F::word(row,0x14)==(type==6?0u:1u)&&F::word(f.layer,0x30)==F::word(row,0x14));
            assert(F::word(f.layer,0xc)==(type==6?0x3ea00000u:0x3e800000u));
            assert(F::word(f.layer,0x18)==(type==6?0x3f800000u:0u));
            return;
        }
        // External engine layer allocation remains a fixture effect.
        F::put(row,0x10,0);
        ServerAnimationSceneFrameInput in{};in.scene_time=time;in.now=10+time;in.intensity=1;
        if(type==6){
            assert(F::word(event,0x2f0)==2&&F::word(event,0x304)==2);
            ServerGestureMetadata md{};md.available=1;
            assert(metadata.apply(row,0x40,event,n,&md,0,&fb));
        }else{F::fl(row,0x20,1.f);in.sequence_duration=4.;in.first_sequence_frame=true;}
        assert(frame.update(f.actor,sizeof(f.actor),row,0x40,event,n,&in,&fb,&accepted)==1&&accepted);
        assert(F::word(f.layer,0xc)==(type==6?0x3ea00000u:0x3e800000u));
        assert(F::word(f.layer,0x18)==(type==6?0x3f800000u:0u));
    };
    assert(simulate(.5f)==1&&f.starts==1&&f.lookups==1);
    assert(simulate(1.f)==1);verify(6,7,1.f);
    assert(simulate(2.5f)==1&&simulate(2.75f)==1&&f.stops==1&&f.blends==1&&f.last_blend==.1f&&!F::word(f.actor,0x5c4));
    assert(simulate(3.25f)==1&&f.starts==1&&!F::word(f.actor,0x5c4));
    assert(simulate(3.5f)==1&&f.starts==2&&f.lookups==(think?3u:2u));
    assert(simulate(4.f)==1);verify(7,9,4.f);
    if(think){
        assert(f.lookups==4&&f.allocs==2&&f.keepalives==(gate?0u:1u));
        if(gate){
            assert(f.gate_queries==1&&f.gate_capability==1&&f.gate_sets==1&&f.condition==1);
            assert(VcdAuditArena::read_float(f.actor,0x97c)==14.2f);
        }
        assert(simulate(4.5f)==1);
        f.condition=0;
        assert(think->run(f.actor,sizeof(f.actor),14.5f,&tb)&&f.keepalives==(gate?0u:2u)&&f.warnings==1);
        if(gate)assert(VcdAuditArena::read_float(f.actor,0x97c)==14.7f);
        assert(F::word(f.layer,0xc)==0x3ec00000u);
        assert(std::fabs(VcdAuditArena::read_float(f.layer,0x18)-.896f)<.00001f);
        f.activity=0x48;
        assert(think->run(f.actor,sizeof(f.actor),14.5f,&tb)&&f.keepalives==(gate?0u:3u)&&f.warnings==1);
        if(gate){
            assert(VcdAuditArena::read_float(f.actor,0x97c)==14.7f);
            // A lower global timestamp cannot shorten an existing deadline.
            assert(think->run(f.actor,sizeof(f.actor),14.f,&tb));
            assert(VcdAuditArena::read_float(f.actor,0x97c)==14.7f&&!f.keepalives&&f.gate_sets==1);
        }
        assert(F::word(f.layer,0x18)==0x3f800000u);
    }
    assert(simulate(5.5f)==1&&simulate(5.75f)==1&&f.stops==2&&f.blends==2&&f.last_blend==.3f&&!F::word(f.actor,0x5c4));
    assert(runtime.destroy(f.root,&raw)==1&&arena.live.size()==1);
    assert(arena.release(&arena,F::word(f.actor,0x5b8))&&arena.live.empty());
    std::puts(gate?"PASS parsed NPC animation think with native admission gate and native keepalive deadline":
        think?"PASS automatic NPC animation think from parsed scene through deferred initialization and native frame":
        "PASS parsed gesture/sequence native actor record, tag warp, layer frame and retirement");
}
inline void check_parsed_npc_target_scene(VcdAuditArena& arena,const NativeVcdRuntimeApi& runtime,
    const NativeSceneExecutionApi& execution,const ServerNpcAnimationRecordApi& admission,const ServerNpcTargetThinkApi& think){
    using F=ScenePlaybackFixture;
    const char data[]=R"VCD(actor Jessica
{
 channel Eyes
 {
  event lookat eyes
  {
   time 0 2
   param "!player"
  }
 }
 channel Body
 {
  event face body
  {
   time 0 2
   param "!player"
  }
 }
}
)VCD";
    struct Fixture {
        VcdAuditArena& arena;alignas(4) unsigned char scene[0x5ec]{},actor[0x1000]{},target[4]{};
        unsigned starts{},stops{},handles{},targets{},looks{},faces{},gates{},motors{},conditions{},condition=1,suppressed=0;
        float yaw=0;std::uint32_t root=0;
        explicit Fixture(VcdAuditArena& a):arena(a){}
        static Fixture& self(void* p) noexcept{return *static_cast<Fixture*>(p);}
        static int __cdecl storage(void* p,std::uint32_t w,void** out,unsigned* n) noexcept{
            auto& a=self(p).arena;return a.storage(&a,w,out,n);
        }
    } f(arena);
    F::put(f.scene,0x414,F::ptr(f.scene));f.scene[0x3d8]=1;
    auto raw=arena.runtime();
    assert(runtime.parse(f.scene+0x410,"target_scene.vcd",data,sizeof(data)-1,&raw,&f.root)==1&&f.root);
    F::put(f.scene,0x40c,f.root);
    ServerNpcAnimationRecordBindings cfg{};auto& facial=cfg.npc.facial;
    facial.host.context=&f;facial.host.storage=&Fixture::storage;
    facial.host.grow=[](void* p,void* v,unsigned extra) noexcept -> int{
        auto& a=Fixture::self(p).arena;return a.vectors.grow_vector(&a.vector_bindings,v,extra,0x40);
    };
    facial.host.handle=[](void* p,void* target,std::uint32_t* out) noexcept -> int{
        auto& f=Fixture::self(p);assert(target==f.target);++f.handles;*out=17;return 1;
    };
    facial.host.diagnostic=[](void*,ActorEventDiagnostic,std::uint32_t) noexcept -> int{assert(false);return 0;};
    facial.extent=[](void* p,void* actor,unsigned* n) noexcept -> int{
        auto& f=Fixture::self(p);assert(actor==f.actor);*n=sizeof(f.actor);return 1;
    };
    facial.now=[](void*,float* out) noexcept -> int{*out=10;return 1;};
    ActorEventRecordBindings records{};assert(admission.adapt(&cfg,&records));
    NativeSceneExecutionBindings b{};b.clock.vectors=arena.vector_bindings;b.clock.release_memory=&VcdAuditArena::release;
    b.dispatch.context=&f;b.dispatch.records=&records;
    b.dispatch.actor=[](void* p,void* scene,std::uint32_t word,void** out,unsigned* n) noexcept -> int{
        auto& f=Fixture::self(p);assert(scene==f.scene&&word);*out=f.actor;*n=sizeof(f.actor);return 1;
    };
    b.dispatch.target=[](void* p,void* scene,const char* name,void* actor,unsigned flag,void** out) noexcept -> int{
        auto& f=Fixture::self(p);assert(scene==f.scene&&actor==f.actor&&!flag&&!std::strcmp(name,"!player"));
        ++f.targets;*out=f.target;return 1;
    };
    b.dispatch.trace=[](void* p,void* scene,void* event,float,ChoreographyDispatchTrace kind) noexcept -> int{
        auto& f=Fixture::self(p);assert(scene==f.scene&&(F::word(event,0)==3||F::word(event,0)==8));
        if(kind==ChoreographyDispatchTrace::Start)++f.starts;
        else if(kind==ChoreographyDispatchTrace::Stop)++f.stops;
        return 1;
    };
    b.leaves.context=&f;b.leaves.actor_extent=facial.extent;
    ServerVfeRemapCache remap{};ServerNpcTargetThinkBindings tb{};
    auto& facial_think=tb.animation.facial.named;
    facial_think.remap=&remap;facial_think.composed.base.context=&f;facial_think.composed.base.storage=&Fixture::storage;
    facial_think.composed.native_intensity=true;
    tb.look.context=&f;tb.look.storage=&Fixture::storage;
    tb.look.npc=[](void* p,void* actor,void** npc) noexcept -> int{auto& f=Fixture::self(p);assert(actor==f.actor);*npc=actor;return 1;};
    tb.look.resolve=[](void* p,std::uint32_t token,void** out) noexcept -> int{
        auto& f=Fixture::self(p);assert(token==17);*out=f.target;return 1;
    };
    tb.look.look=[](void* p,void* npc,void* target,float weight,float duration,int flags) noexcept -> int{
        auto& f=Fixture::self(p);assert(npc==f.actor&&target==f.target&&weight==1&&duration==.2f&&!flags);++f.looks;return 1;
    };
    tb.face.target=tb.look;tb.face.context=&f;
    tb.face.target.look=[](void* p,void* npc,void* target,float weight,float duration,int flags) noexcept -> int{
        auto& f=Fixture::self(p);assert(npc==f.actor&&target==f.target&&weight==1&&duration==.2f&&!flags);++f.faces;return 1;
    };
    tb.face.admit=[](void* p,void*,unsigned* out) noexcept -> int{auto& f=Fixture::self(p);++f.gates;*out=1;return 1;};
    tb.face.suppressed=[](void* p,void*,unsigned* out) noexcept -> int{*out=Fixture::self(p).suppressed;return 1;};
    tb.face.target_yaw=[](void*,void*,void*,float* out) noexcept -> int{*out=90;return 1;};
    assert(execution.reset(f.scene,sizeof(f.scene),1,0,0,&b)==1);
    auto simulate=[&](float time){F::fl(f.scene,0x3dc,time);return execution.simulate(f.scene,sizeof(f.scene),time,&b);};
    assert(simulate(.5f)==1&&f.starts==2&&f.handles==3&&f.targets==2&&F::word(f.actor,0x5c4)==2);
    assert(think.run(f.actor,sizeof(f.actor),10,&tb)&&f.looks==1&&f.faces==1&&f.gates==1);
    void* rows=nullptr;unsigned n=0;
    assert(Fixture::storage(&f,F::word(f.actor,0x5b8),&rows,&n)&&n>=0x80);
    assert(static_cast<unsigned char*>(rows)[0xc]&&static_cast<unsigned char*>(rows)[0x4c]);
    assert(simulate(1.f)==1&&think.run(f.actor,sizeof(f.actor),11,&tb)&&f.looks==2&&f.faces==2&&f.gates==2);
    assert(VcdAuditArena::read_float(f.actor,0xe00)==90&&VcdAuditArena::read_float(f.actor,0xe04)==2);
    assert(VcdAuditArena::read_float(f.actor,0xe40)==30&&VcdAuditArena::read_float(f.actor,0xe44)==60);
    ServerBodyTurnBindings consume{};consume.context=&f;
    consume.condition=[](void* p,void* actor,int id,int flag,unsigned* out) noexcept -> int{
        auto& f=Fixture::self(p);assert(actor==f.actor&&id==0x3c&&flag==1&&!F::word(actor,0xe00)&&!F::word(actor,0xe04));
        ++f.conditions;*out=f.condition;return 1;
    };
    consume.suppressed=tb.face.suppressed;
    consume.motor_yaw=[](void* p,void* actor,float yaw,float speed) noexcept -> int{
        auto& f=Fixture::self(p);assert(actor==f.actor&&speed==-1);++f.motors;f.yaw=yaw;return 1;
    };
    unsigned accepted=99;
    auto body=[&](){return think.consume_body(f.actor,sizeof(f.actor),&consume,&accepted);};
    assert(body()&&accepted==1&&f.motors==1&&f.yaw==45&&f.conditions==1);
    assert(body()&&!accepted&&f.conditions==1);
    F::fl(f.actor,0xe00,30);F::fl(f.actor,0xe04,1);f.condition=0;
    assert(body()&&!accepted&&f.conditions==2&&f.motors==1&&!F::word(f.actor,0xe04));
    F::fl(f.actor,0xe00,30);F::fl(f.actor,0xe04,1);f.condition=1;f.suppressed=1;
    assert(body()&&!accepted&&f.conditions==3&&f.motors==1&&!F::word(f.actor,0xe04));
    assert(simulate(2.5f)==1&&simulate(2.75f)==1&&f.stops==2&&!F::word(f.actor,0x5c4));
    assert(runtime.destroy(f.root,&raw)==1&&arena.live.size()==1);
    assert(arena.release(&arena,F::word(f.actor,0x5b8))&&arena.live.empty());
    std::puts("PASS parsed lookat and face native actor think and separate weighted body-turn consumption");
}

inline void check_parsed_npc_movement_scene(VcdAuditArena& arena,const NativeVcdRuntimeApi& runtime,
    const NativeSceneExecutionApi& execution,const ServerNpcMovementSceneApi& api,const NativeChoreographySectionApi& section){
    using F=ScenePlaybackFixture;
    const char data[]=R"VCD(actor Jessica
{
 channel Body
 {
  event moveto move
  {
   time 0 4
   param "marker"
   param2 "Walk"
   resumecondition
   distancetotarget 5
  }
 }
}
)VCD";
    struct Fixture {
        VcdAuditArena& arena;alignas(4) unsigned char scene[0x5ec]{},actor[0x1000]{},target[4]{};
        unsigned starts{},stops{},handles{},targets{},gates{},submits{},configs{},cancels{},moving{},goal{};
        float distance=30;double travel=1;std::uint32_t root=0;
        explicit Fixture(VcdAuditArena& a):arena(a){}
        static Fixture& self(void* p) noexcept{return *static_cast<Fixture*>(p);}
        static int __cdecl storage(void* p,std::uint32_t w,void** out,unsigned* n) noexcept{
            auto& a=self(p).arena;return a.storage(&a,w,out,n);
        }
    } f(arena);
    F::put(f.scene,0x414,F::ptr(f.scene));f.scene[0x3d8]=1;
    auto raw=arena.runtime();
    assert(runtime.parse(f.scene+0x410,"move_scene.vcd",data,sizeof(data)-1,&raw,&f.root)==1&&f.root);
    F::put(f.scene,0x40c,f.root);
    ServerNpcMovementRecordBindings cfg{};auto& facial=cfg.animation.npc.facial;
    facial.host.context=&f;facial.host.storage=&Fixture::storage;
    facial.host.grow=[](void* p,void* v,unsigned extra) noexcept -> int{
        auto& a=Fixture::self(p).arena;return a.vectors.grow_vector(&a.vector_bindings,v,extra,0x40);};
    facial.host.handle=[](void* p,void* target,std::uint32_t* out) noexcept -> int{
        auto& f=Fixture::self(p);assert(target==f.target);++f.handles;*out=17;return 1;};
    facial.host.diagnostic=[](void*,ActorEventDiagnostic,std::uint32_t) noexcept -> int{assert(false);return 0;};
    facial.extent=[](void* p,void* actor,unsigned* n) noexcept -> int{
        auto& f=Fixture::self(p);assert(actor==f.actor);*n=sizeof(f.actor);return 1;};
    facial.now=[](void*,float* out) noexcept -> int{*out=10;return 1;};
    auto& m=cfg.move;m.context=&f;
    m.npc=[](void* p,void* actor,void** out,unsigned* n) noexcept -> int{
        auto& f=Fixture::self(p);assert(actor==f.actor);*out=f.actor;*n=sizeof(f.actor);return 1;};
    m.admit=[](void* p,void*,unsigned* out) noexcept -> int{++Fixture::self(p).gates;*out=1;return 1;};
    m.moving=[](void* p,void*,unsigned* out) noexcept -> int{*out=Fixture::self(p).moving;return 1;};
    m.cancel_navigation=[](void* p,void*,int clear) noexcept -> int{
        auto& f=Fixture::self(p);assert(clear==1);++f.cancels;f.moving=f.goal=0;return 1;};
    m.has_goal=[](void* p,void*,unsigned* out) noexcept -> int{*out=Fixture::self(p).goal;return 1;};
    m.travel_time=[](void* p,void*,double* out) noexcept -> int{*out=Fixture::self(p).travel;return 1;};
    ActorEventRecordBindings records{};assert(api.adapt(&cfg,&records));
    NativeSceneExecutionBindings b{};b.clock.vectors=arena.vector_bindings;b.clock.release_memory=&VcdAuditArena::release;
    b.dispatch.context=&f;b.dispatch.records=&records;
    b.dispatch.actor=[](void* p,void* scene,std::uint32_t word,void** out,unsigned* n) noexcept -> int{
        auto& f=Fixture::self(p);assert(scene==f.scene&&word);*out=f.actor;*n=sizeof(f.actor);return 1;};
    b.dispatch.target=[](void* p,void* scene,const char* name,void* actor,unsigned flag,void** out) noexcept -> int{
        auto& f=Fixture::self(p);assert(scene==f.scene&&actor==f.actor&&!flag&&!std::strcmp(name,"marker"));
        ++f.targets;*out=f.target;return 1;};
    b.dispatch.trace=[](void* p,void* scene,void* event,float,ChoreographyDispatchTrace kind) noexcept -> int{
        auto& f=Fixture::self(p);assert(scene==f.scene&&F::word(event,0)==4);
        if(kind==ChoreographyDispatchTrace::Start)++f.starts;
        else if(kind==ChoreographyDispatchTrace::Stop)++f.stops;return 1;};
    b.leaves.context=&f;b.leaves.actor_extent=facial.extent;
    ServerVfeRemapCache remap{};ServerNpcMovementThinkBindings tb{};
    auto& ft=tb.target.animation.facial.named;ft.remap=&remap;
    ft.composed.base.context=&f;ft.composed.base.storage=&Fixture::storage;ft.composed.native_intensity=true;
    auto& move=tb.move;move.move=m;move.context=&f;move.handle=facial.host.handle;
    move.resolve=[](void* p,std::uint32_t token,void** out) noexcept -> int{
        auto& f=Fixture::self(p);assert(token==17);*out=f.target;return 1;};
    move.task=[](void*,void*,int* out) noexcept -> int{*out=0x71;return 1;};
    move.hull_width=[](void*,void*,float* out) noexcept -> int{*out=20;return 1;};
    move.horizontal_delta=[](void* p,void* actor,void* target,float* x,float* y) noexcept -> int{
        auto& f=Fixture::self(p);assert(actor==f.actor&&target==f.target);*x=f.distance;*y=0;return 1;};
    move.submit_goal=[](void* p,void* npc,void* target,int activity,float half,unsigned* out) noexcept -> int{
        auto& f=Fixture::self(p);assert(npc==f.actor&&target==f.target&&activity==6&&half==10);
        ++f.submits;f.goal=f.moving=1;*out=1;return 1;};
    move.goal_activity=[](void* p,void*,int activity) noexcept -> int{assert(activity==6);++Fixture::self(p).configs;return 1;};
    move.goal_tolerance=[](void* p,void*,float tolerance) noexcept -> int{assert(tolerance==5);++Fixture::self(p).configs;return 1;};
    assert(execution.reset(f.scene,sizeof(f.scene),1,0,0,&b)==1);
    auto simulate=[&](float time){F::fl(f.scene,0x3dc,time);return execution.simulate(f.scene,sizeof(f.scene),time,&b);};
    assert(simulate(.5f)==1&&f.starts==1&&f.targets==1&&f.handles==1&&F::word(f.actor,0x5c4)==1&&f.gates==1);
    void* rows=nullptr;unsigned n=0;
    assert(Fixture::storage(&f,F::word(f.actor,0x5b8),&rows,&n)&&n>=0x40);
    auto* row=static_cast<unsigned char*>(rows);
    void* event=nullptr;assert(Fixture::storage(&f,F::word(row,0),&event,&n)&&n>=0x3c0);
    void* root=nullptr;unsigned extent=0;
    assert(arena.storage(&arena,f.root,&root,&extent)&&extent>=0x220&&F::word(root,0x5c)==1);
    // Exercise original callback slot 12 through section readiness, not a hosted ready flag.
    NativeChoreographyClockBindings tick=b.clock;tick.scene=f.scene;tick.scene_bytes=sizeof(f.scene);
    tick.other_event=[](void*,std::uint32_t,unsigned,float,void*,void*) noexcept -> int{return 1;};
    NativeChoreographyDispatchBindings dispatch=b.dispatch;
    dispatch.scene=f.scene;dispatch.scene_bytes=sizeof(f.scene);dispatch.clock=&tick;dispatch.storage=&Fixture::storage;
    NativeChoreographySectionBindings sb{};sb.context=&f;sb.scene_bytes=sizeof(f.scene);sb.clock=&tick;sb.dispatch=&dispatch;
    unsigned ready=99;auto section_ready=[&](unsigned expected){
        assert(section.ready(root,extent,&sb,&ready)==1&&ready==expected);
    };
    section_ready(0);
    unsigned accepted=99;assert(records.check(records.context,f.actor,row,.5f,f.root,event,&accepted)&&!accepted);
    assert(api.run(f.actor,sizeof(f.actor),10,&tb)&&f.submits==1&&f.configs==2&&row[0xc]&&row[0x28]&&!row[0x29]);
    assert(F::word(f.actor,0x8a8)==17&&VcdAuditArena::read_float(f.actor,0x97c)==12);
    assert(records.check(records.context,f.actor,row,2.f,f.root,event,&accepted)&&accepted);
    f.travel=5;section_ready(0);
    f.travel=3.5;section_ready(1); // estimate equals remaining scene duration
    f.travel=0;section_ready(0);
    f.travel=1;section_ready(1);
    f.distance=5;
    assert(api.run(f.actor,sizeof(f.actor),11,&tb)&&f.cancels==1&&row[0x29]&&VcdAuditArena::read_float(f.actor,0x97c)==13);
    assert(records.check(records.context,f.actor,row,3.f,f.root,event,&accepted)&&accepted);
    section_ready(1); // navigator no longer has a goal; completed record is sufficient
    assert(simulate(4.5f)==1&&simulate(4.75f)==1&&f.stops==1&&!F::word(f.actor,0x5c4));
    assert(runtime.destroy(f.root,&raw)==1&&arena.live.size()==1);
    assert(arena.release(&arena,F::word(f.actor,0x5b8))&&arena.live.empty());
    std::puts("PASS parsed movement event native record admission, think, completion and retirement");
}

// Retail-inspired edge cases: strict binary32 duration trim, duplicate
// timestamps, reverse ranges and disabled tracks with/without samples.
inline void check_retail_flex_edge_cases(VcdAuditArena& arena,const NativeVcdRuntimeApi& runtime,const ServerFlexSplineApi& spline){
    using F=ScenePlaybackFixture;
    const char data[]=R"VCD(actor Jessica
{
 channel Face
 {
  event flexanimation edge
  {
   time 0 2.973984
   flexanimations samples_use_time
   {
    "jaw" combo range 15 -15
    {
     0.5 0.2
     0.5 0.9
     2.974 0.7
     2.974 0.8
    }
    {
     0.5 0.75
     2.974 0.6
    }
    "empty_disabled" disabled
    {
    }
    "retained_disabled" disabled range 0 1
    {
     0.5 0.4
    }
   }
  }
 }
}
)VCD";
    auto raw=arena.runtime();std::uint32_t rootword=0;
    assert(runtime.parse(nullptr,"facial_edge.vcd",data,sizeof(data)-1,&raw,&rootword)==1&&rootword);
    void* root=nullptr;unsigned n=0;
    assert(arena.storage(&arena,rootword,&root,&n)&&n>=0x220);
    VcdAuditCounts stats;arena.spline=&spline;
    assert(arena.inspect(root,stats)&&stats.flex_events==1&&stats.tracks==2&&stats.samples==4&&stats.spline_evals==9);
    void *events=nullptr,*event=nullptr,*tracks=nullptr,*track=nullptr,*disabled=nullptr;
    unsigned count=0;
    assert(arena.rows(root,0,4,events,count)&&count==1&&arena.child(events,0,0x3c0,event));
    assert(arena.rows(event,0x30c,4,tracks,count)&&count==2);
    assert(arena.child(tracks,0,0x50,track)&&arena.child(tracks,1,0x50,disabled));
    assert(static_cast<unsigned char*>(track)[8]&&static_cast<unsigned char*>(track)[9]&&!static_cast<unsigned char*>(disabled)[8]);
    const float low=VcdAuditArena::read_float(track,0xc),high=VcdAuditArena::read_float(track,0x10);
    const float duration=VcdAuditArena::read_float(event,0x188)-VcdAuditArena::read_float(event,0x184);
    assert(low==15&&high==-15&&duration<2.974f);
    void* samples=nullptr;
    assert(arena.rows(track,0x14,12,samples,count)&&count==2);
    auto* primary=static_cast<const ServerFlexSample*>(samples);
    assert(primary[0].time==.5f&&primary[1].time==.5f&&primary[0].value==.2f&&primary[1].value==.9f);
    float value=0;
    assert(spline.sample(primary,count,low,high,duration,.5f,0,&value)&&std::fabs(value-.2f)<.00001f);
    assert(spline.sample(primary,count,low,high,duration,duration,0,&value)&&std::fabs(value-.5f)<.00001f);
    assert(arena.rows(track,0x28,12,samples,count)&&count==1);
    assert(spline.sample(static_cast<const ServerFlexSample*>(samples),count,low,high,duration,.5f,1,&value)&&value==.75f);
    assert(runtime.destroy(rootword,&raw)==1&&arena.live.empty());
    std::puts("PASS retail-inspired flex duplicate timestamps, strict duration trim and reversed ranges");
}
inline void check_vcd_audit_arena(const PortableVcdFinalizedGrammarApi& grammar,const ChoreographyConstructApi& c,
    const ChoreographySymbolApi& s,const ChoreographyFlexOwnershipApi& t,const ChoreographyDestructionApi& d,const ChoreographyScaleTreeApi& st,const ChoreographySymbolInternApi& si,const ChoreographyVectorApi& va,const NativeVcdRuntimeApi& runtime,const ChoreographyStartupSoundApi& startup_sound,const NativeChoreographyClockApi& clock,const NativeChoreographyDispatchApi& dispatch,const NativeChoreographySceneLeafApi& leaves,const NativeChoreographySectionApi& section,const NativeSceneMovementGateApi& gate,const NativeSceneExecutionApi& execution,const ServerEventIntensityApi& intensity,const ServerFlexSplineApi& spline,const ServerFacialRecordApi& admission,const ServerNamedFacialThinkApi& think,ParsedFacialClientPair& paired){
    VcdAuditArena edge_arena(c,s,t,d,st,si,va);check_retail_flex_edge_cases(edge_arena,runtime,spline);
    VcdAuditArena facial_arena(c,s,t,d,st,si,va);check_parsed_facial_lifecycle(facial_arena,runtime,execution,admission,think,paired);
    VcdAuditArena execution_arena(c,s,t,d,st,si,va);check_composed_scene_execution(execution_arena,runtime,execution);
    check_native_movement_gate(gate);
    check_native_choreography_dispatch(dispatch);check_native_scene_leaves(leaves);
    VcdAuditArena section_arena(c,s,t,d,st,si,va);check_native_section(section_arena,runtime,clock,section);
    // Native CUtlMemory capacities and indexed word insertion, including
    // null words, borrowed storage and failure-atomic raw allocator.
    {
        using F=ScenePlaybackFixture;VcdAuditArena arena(c,s,t,d,st,si,va);
        auto* b=&arena.vector_bindings;
        unsigned v[5]{};
        for(unsigned i=0;i<8;++i)assert(va.insert_word(b,v,i,i+1));
        assert(v[1]==8&&v[3]==8&&v[4]==v[0]);
        const auto old=v[0];arena.fail_resize=true;
        assert(!va.insert_word(b,v,2,99)&&v[0]==old&&v[1]==8&&v[3]==8);
        arena.fail_resize=false;
        assert(va.insert_word(b,v,2,0)&&v[1]==16&&v[3]==9&&v[4]==v[0]);
        void* rows=nullptr;unsigned extent=0;
        assert(arena.storage(&arena,v[0],&rows,&extent)&&F::word(rows,8)==0&&F::word(rows,12)==3&&F::word(rows,32)==8);
        v[2]=0xffffffffu;
        assert(!va.grow_vector(b,v,1,4)&&v[1]==16);
        assert(!va.insert_word(b,v,10,99));
        assert(arena.release(&arena,v[0]));
        for(unsigned stride:{4u,12u,24u,0x40u,0x88u,0x8cu}){
            unsigned mem[3]{};
            assert(va.grow_memory(b,mem,1,stride)&&mem[1]==(31+stride)/stride);
            mem[2]=3;const unsigned cap=mem[1];
            assert(va.grow_memory(b,mem,4,stride)&&mem[1]==cap+6);
            assert(arena.release(&arena,mem[0]));
        }
        assert(arena.live.empty());
    }
    // Real pooled symbol storage, case-insensitive duplicate, first-fit
    // pool reuse after a second allocation, native destruction.
    {
        using F=ScenePlaybackFixture;VcdAuditArena arena(c,s,t,d,st,si,va);
        void* table=nullptr;unsigned extent=0;
        assert(arena.allocate(&arena,0x34,&table,&extent)&&s.construct_empty(table,extent,0,0x2228e9b0)==1);
        std::uint16_t id=99;auto add=[&](const char* name){return si.intern(&arena.intern_bindings,table,name,&id);};
        const std::string a(1500,'a'),b(700,'b'),dword(500,'d');
        assert(add(nullptr)&&id==0xffff);
        assert(add(a.c_str())&&id==0);
        assert(add(b.c_str())&&id==1&&F::word(table,0x28)==2);
        assert(add(dword.c_str())&&id==2&&F::word(table,0x28)==2);
        assert(add("ABC")&&id==3);assert(add("abc")&&id==3);
        void* rows=nullptr;unsigned n=0;
        assert(arena.storage(&arena,F::word(table,4),&rows,&n)&&n>=48);
        assert(F::word(rows,8)==0&&F::word(rows,20)==1&&F::word(rows,32)==(1501u<<16));
        assert(F::word(rows,44)==(2002u<<16));
        assert((F::word(table,0x10)>>16)==4&&F::word(table,0x28)==2);
        assert(arena.retire.symbol_destroy(&arena,table)&&arena.release(&arena,F::ptr(table))&&arena.live.empty());
    }
    const char input[]=R"VCD(event loop repeat
{
 time 0 8
 param 0
}
actor Jessica
{
 channel Voice
 {
  event speak first
  {
   time 0 2
   param Jessica.One
   tags { mark 0.5 }
   cctype cc_master
  }
  event speak second
  {
   time 3 5
   param Jessica.Two
   cctype cc_master
  }
 }
 channel Face
 {
  event flexanimation face
  {
   time 0 2
   flexanimations samples_use_time
   {
    "jaw" combo { 0.1 0.2 } { 0.3 0.4 }
    "brow" { 0.2 0.3 }
   }
  }
 }
}
actor Blade
{
 channel Body
 {
  event gesture greet
  {
   time 1 2
   param gesture
   relativetag Jessica.One mark
   absolutetags playback_time { "in" 0.1 "out" 0.8 }
  }
 }
}
scalesettings { RampTool 80 GestureTool 70 RampTool 60 }
)VCD";
    VcdAuditArena a(c,s,t,d,st,si,va);VcdAuditCounts out;
    assert(a.parse(grammar,"multi.vcd",input,sizeof(input)-1,out));
    assert(out.actors==2&&out.channels==3&&out.events==5&&out.tracks==2&&out.samples==3&&out.scales==2&&!out.missing);
    VcdAuditArena integrated(c,s,t,d,st,si,va);VcdAuditCounts actual;
    integrated.intensity=&intensity;integrated.spline=&spline;
    assert(integrated.parse_runtime(runtime,"multi.vcd",input,sizeof(input)-1,actual));
    assert(actual.actors==out.actors&&actual.channels==out.channels&&actual.events==out.events&&
        actual.tracks==out.tracks&&actual.samples==out.samples&&actual.scales==out.scales&&actual.missing==out.missing);
    assert(actual.flex_events==1&&actual.expressions==0&&actual.intensity_evals==3&&actual.spline_evals==9);
    // Malformed text must retire partially constructed actors/children.
    auto raw=integrated.runtime();std::uint32_t rootword=99;
    const char bad[]="actor Jessica { channel Voice { event speak dialogue { time 0 1";
    assert(runtime.parse(nullptr,"bad.vcd",bad,sizeof(bad)-1,&raw,&rootword)==-1&&!rootword&&integrated.live.empty());
    // Native cache load: independent allocator/cache contexts, unlocked
    // before parsing, filename normalization and callback/diagnostic words.
    struct Cache {const char* data;unsigned n;bool locked=false,unlocked=false,missing=false;} cache{input,sizeof(input)-1};
    NamedSceneLoadBindings load{};load.context=&cache;load.cache.context=&cache;
    load.set_extension=[](void*,char* path,const char* ext,unsigned cap) noexcept -> int{
        const auto n=std::strlen(path),m=std::strlen(ext);if(n+m>=cap)return 0;std::memcpy(path+n,ext,m+1);return 1;
    };
    load.missing=[](void* p,const char*) noexcept -> int{static_cast<Cache*>(p)->missing=true;return 1;};
    load.cache.ensure=[](void*,const char* key) noexcept -> int{return !std::strcmp(key,"scenes\\multi.vcd");};
    load.cache.find=[](void*,const char*,std::uint32_t* out) noexcept -> int{*out=7;return 1;};
    load.cache.lock=[](void* p,std::uint32_t i,const void** out,unsigned* n) noexcept -> int{
        auto& c=*static_cast<Cache*>(p);if(i!=7||c.locked)return 0;c.locked=true;*out=c.data;*n=c.n;return 1;
    };
    load.cache.reload=[](void*,const char*,std::uint32_t,std::uint32_t*) noexcept -> int{return 0;};
    load.cache.unlock=[](void* p,std::uint32_t i) noexcept -> int{
        auto& c=*static_cast<Cache*>(p);if(i!=7||!c.locked)return 0;c.locked=false;c.unlocked=true;return 1;
    };
    VcdAuditArena loader(c,s,t,d,st,si,va);auto lb=loader.runtime();alignas(4) unsigned char scene[0x414]{};
    assert(runtime.load(scene,sizeof(scene),"Scenes/Multi",&load,&lb,&rootword)==1&&rootword&&cache.unlocked&&!cache.locked);
    void* root=nullptr;unsigned extent=0;VcdAuditCounts loaded;
    assert(loader.storage(&loader,rootword,&root,&extent)&&loader.inspect(root,loaded)&&loaded.events==out.events);
    assert(ScenePlaybackFixture::word(root,0x9c)==ScenePlaybackFixture::ptr(scene+0x410)&&ScenePlaybackFixture::word(root,0x98)==lb.diagnostic_word);
    assert(runtime.destroy(rootword,&lb)==1&&loader.live.empty()&&!runtime.destroy(rootword,&lb));
    cache.n=0;cache.unlocked=false;rootword=99;
    assert(runtime.load(scene,sizeof(scene),"Scenes/Multi",&load,&lb,&rootword)==1&&!rootword&&cache.missing&&cache.unlocked&&loader.live.empty());
    std::puts("PASS multi-object VCD grammar, native track lifecycle, finalization, owned runtime and cache load retirement");
    VcdAuditArena playback_arena(c,s,t,d,st,si,va);
    VcdOwnedPlaybackFixture playback(playback_arena,startup_sound);playback.run(runtime);
    VcdAuditArena clock_arena(c,s,t,d,st,si,va);check_native_choreography_clock_graph(clock_arena,runtime,clock);
    VcdAuditArena speech_arena(c,s,t,d,st,si,va);check_vcd_clock_speech(speech_arena,runtime,clock);
    std::puts("PASS owned VCD named NPC startup, nested precache, actor association and retirement");
}
inline bool audit_retail_vcd_directory(const char* directory,const PortableVcdFinalizedGrammarApi& grammar,
    const ChoreographyConstructApi& c,const ChoreographySymbolApi& s,const ChoreographyFlexOwnershipApi& t,const ChoreographyDestructionApi& d,const ChoreographyScaleTreeApi& st,const ChoreographySymbolInternApi& si,const ChoreographyVectorApi& va,const NativeVcdRuntimeApi& runtime,const ServerEventIntensityApi& intensity,const ServerFlexSplineApi& spline){
    namespace fs=std::filesystem;std::error_code ec;
    const fs::path root(directory);
    unsigned files=0,passed=0;VcdAuditCounts sum;
    fs::recursive_directory_iterator it(root,ec),end;
    if(ec){std::fprintf(stderr,"VCD audit cannot open directory: %s\n",directory);return false;}
    for(;it!=end;it.increment(ec)){
        if(ec)break;
        if(!it->is_regular_file(ec)){if(ec)break;continue;}
        auto ext=it->path().extension().string();for(char& ch:ext)if(ch>='A'&&ch<='Z')ch=static_cast<char>(ch+32);
        if(ext!=".vcd")continue;
        ++files;const auto name=it->path().lexically_relative(root).generic_string();
        std::ifstream stream(it->path(),std::ios::binary|std::ios::ate);
        const auto bytes=stream.tellg();
        if(!stream||bytes<=0||bytes>1024*1024){std::fprintf(stderr,"VCD unreadable/oversized %s\n",name.c_str());continue;}
        std::string data(static_cast<std::size_t>(bytes),'\0');stream.seekg(0);
        if(!stream.read(data.data(),static_cast<std::streamsize>(data.size())))continue;
        VcdAuditArena arena(c,s,t,d,st,si,va);VcdAuditCounts stats;
        arena.intensity=&intensity;arena.spline=&spline;
        if(arena.parse_runtime(runtime,name.c_str(),data.data(),static_cast<unsigned>(data.size()),stats)){++passed;sum.add(stats);}
    }
    std::printf("VCD AUDIT files=%u passed=%u actors=%u channels=%u events=%u tracks=%u samples=%u scales=%u missing=%u\n",
        files,passed,sum.actors,sum.channels,sum.events,sum.tracks,sum.samples,sum.scales,sum.missing);
    std::printf("FACIAL AUDIT expression=%u flex=%u ramps=%u intensity_evals=%u spline_evals=%u\n",
        sum.expressions,sum.flex_events,sum.facial_ramps,sum.intensity_evals,sum.spline_evals);
    return !ec&&files>0&&passed==files;
}
}
