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
inline void check_vcd_audit_arena(const PortableVcdFinalizedGrammarApi& grammar,const ChoreographyConstructApi& c,
    const ChoreographySymbolApi& s,const ChoreographyFlexOwnershipApi& t,const ChoreographyDestructionApi& d,const ChoreographyScaleTreeApi& st,const ChoreographySymbolInternApi& si,const ChoreographyVectorApi& va,const NativeVcdRuntimeApi& runtime){
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
