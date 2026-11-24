#pragma once
#include "test_native_npc_scene_playback.h"
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
    void add(const VcdAuditCounts& x) noexcept {
        actors+=x.actors;channels+=x.channels;events+=x.events;tracks+=x.tracks;
        samples+=x.samples;scales+=x.scales;missing+=x.missing;
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
            void* e=nullptr;if(!child(events,i,0x3c0,e)||F::word(e,0x330)!=F::ptr(root))return false;
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
    // At previous time -0.1, no duration starts; next call at 0.5
    // starts tracked gesture and crosses point pause at 1.
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
inline void check_vcd_audit_arena(const PortableVcdFinalizedGrammarApi& grammar,const ChoreographyConstructApi& c,
    const ChoreographySymbolApi& s,const ChoreographyFlexOwnershipApi& t,const ChoreographyDestructionApi& d,const ChoreographyScaleTreeApi& st,const ChoreographySymbolInternApi& si,const ChoreographyVectorApi& va,const NativeVcdRuntimeApi& runtime,const ChoreographyStartupSoundApi& startup_sound,const NativeChoreographyClockApi& clock){
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
        for(unsigned stride:{4u,12u,24u,0x88u,0x8cu}){
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
    assert(integrated.parse_runtime(runtime,"multi.vcd",input,sizeof(input)-1,actual));
    assert(actual.actors==out.actors&&actual.channels==out.channels&&actual.events==out.events&&
        actual.tracks==out.tracks&&actual.samples==out.samples&&actual.scales==out.scales&&actual.missing==out.missing);
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
    std::puts("PASS owned VCD named NPC startup, nested precache, actor association and retirement");
}
inline bool audit_retail_vcd_directory(const char* directory,const PortableVcdFinalizedGrammarApi& grammar,
    const ChoreographyConstructApi& c,const ChoreographySymbolApi& s,const ChoreographyFlexOwnershipApi& t,const ChoreographyDestructionApi& d,const ChoreographyScaleTreeApi& st,const ChoreographySymbolInternApi& si,const ChoreographyVectorApi& va,const NativeVcdRuntimeApi& runtime){
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
        if(arena.parse_runtime(runtime,name.c_str(),data.data(),static_cast<unsigned>(data.size()),stats)){++passed;sum.add(stats);}
    }
    std::printf("VCD AUDIT files=%u passed=%u actors=%u channels=%u events=%u tracks=%u samples=%u scales=%u missing=%u\n",
        files,passed,sum.actors,sum.channels,sum.events,sum.tracks,sum.samples,sum.scales,sum.missing);
    return !ec&&files>0&&passed==files;
}
}
