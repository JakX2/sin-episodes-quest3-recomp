#include "native_server_activity_translate.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <vector>
#include <cstdio>
using namespace sin_port::native_weapon_tables;
struct F {
    alignas(4) unsigned char actor[0x1028]{},attachment[0xc24]{},desc[4][0x40]{};
    ServerActivityDescriptor rows[4]{};
    std::uint32_t generation=7;
    ServerActivityGenerationNode group{&generation,-1};
    ServerActivityCompositeView view{rows,4,&group,1};
    std::vector<int> calls;
    int mode=0;
    bool available=true;
    template<class T> void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    void seed(int a,int b,int c,int d){
        int ids[]{a,b,c,d};
        for(unsigned i=0;i<4;++i){std::memset(desc[i],0,0x40);rows[i]={desc[i],0x40,0};desc[i][0xd]=0x10;put(desc[i],0x10,ids[i]);put(desc[i],0x14,1);}
    }
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto g=reinterpret_cast<GetServerNpcActivityTranslateApi>(GetProcAddress(server,server_npc_activity_translate_export));
    assert(g&&!GetProcAddress(client,server_npc_activity_translate_export)&&!g(0,sizeof(ServerNpcActivityTranslateApi))&&!g(1,0));
    auto* api=g(1,sizeof(ServerNpcActivityTranslateApi));assert(api);
    F f;ServerEventDictionaryEntry ar[8]{},er[8]{};
    ServerActivityDictionary a{{ar,8,0,7,0},-1};ServerEventDictionary e{er,8,0,1,0};
    ServerNpcActivityTranslateBindings b{};b.context=&f;b.activities=&a;b.events=&e;b.random.context=&f;
    b.metadata=[](void* p,void* actor,const ServerActivityCompositeView** out) noexcept -> int {
        auto& f=F::self(p);assert(actor==f.actor);f.calls.push_back(900);*out=f.available?&f.view:nullptr;return f.available;};
    b.random.random=[](void* p,int lo,int hi,int idx,int* out) noexcept -> int {
        auto& f=F::self(p);assert(lo==0&&hi==0);f.calls.push_back(1000+idx);*out=0;return 1;};
    b.weapon=[](void* p,void* actor,int input,bool* required,int* out) noexcept -> int {
        auto& f=F::self(p);assert(actor==f.actor);f.calls.push_back(100+input);*required=f.mode==1;
        if(f.mode==1)*out=input==40?41:input==50?51:input;
        else if(f.mode==2)*out=input+1;
        else *out=input;return 1;};
    b.other=[](void* p,void* actor,int input,int* out) noexcept -> int {
        auto& f=F::self(p);assert(actor==f.actor);f.calls.push_back(200+input);
        *out=f.mode==1?50:input;return 1;};
    b.missing_required=[](void* p,void*,int input) noexcept {F::self(p).calls.push_back(300+input);};
    b.missing_sequence=[](void* p,void*,int input) noexcept {F::self(p).calls.push_back(400+input);};
    b.scripted_sequence=[](void* p,void*,int* out) noexcept -> int {F::self(p).calls.push_back(500);*out=3;return 1;};
    ServerNpcActivityTranslateResult tr{};ServerNpcActivitySelectionResult sr{};
    f.seed(41,99,98,97);f.mode=1;
    assert(api->translate(f.actor,sizeof(f.actor),40,&b,&tr)&&tr.activity==40&&tr.initial_weapon==41&&tr.last_npc==50&&tr.final_weapon==51&&tr.iterations==2&&tr.probes==3&&tr.required_missing);
    assert((f.calls==std::vector<int>{140,240,150,251,150,900,351,900,900,1000}));
    f.seed(50,99,98,97);f.calls.clear();
    assert(api->translate(f.actor,sizeof(f.actor),40,&b,&tr)&&tr.activity==50&&tr.probes==2);
    f.mode=2;f.calls.clear();f.seed(99,98,97,96);
    assert(api->translate(f.actor,sizeof(f.actor),40,&b,&tr)&&tr.iterations==5&&tr.final_weapon==45&&tr.activity==40);
    f.mode=0;f.calls.clear();f.seed(6,99,98,97);
    assert(api->select(f.actor,sizeof(f.actor),10,&b,&sr)&&sr.activity==6&&sr.sequence==0&&sr.selections==1);
    assert((f.calls==std::vector<int>{110,210,110,900,900,1000}));
    f.calls.clear();
    assert(api->select(f.actor,sizeof(f.actor),15,&b,&sr)&&sr.activity==15&&sr.sequence==3&&!sr.selections);
    assert((f.calls==std::vector<int>{115,215,115,500}));
    // Actual Grunt +338, no hosted NPC translator.
    b.actor_class=NpcActivityGrunt;b.other=nullptr;b.grunt.context=&f;
    b.grunt.component_identity=[](void*,void*,std::uint32_t,bool* out) noexcept -> int {*out=false;return 1;};
    b.grunt.parent_translate=[](void*,void*,int i,int* out) noexcept -> int {*out=i;return 1;};
    f.actor[0xca0]=1;f.seed(199,99,98,97);f.calls.clear();
    assert(api->select(f.actor,sizeof(f.actor),16,&b,&sr)&&sr.activity==199&&sr.sequence==0&&sr.translation.iterations==2);
    // Actual Jetpack override via attachment, then native Grunt fallback.
    b.actor_class=NpcActivityJetpack;b.jetpack.grunt=b.grunt;b.jetpack.context=&f;
    b.jetpack.attachment=[](void* p,std::uint32_t h,void** out,unsigned* size) noexcept -> int {
        auto& f=F::self(p);assert(h==0x1234);*out=f.attachment;*size=sizeof(f.attachment);return 1;};
    f.put(f.actor,0x1024,0x1234u);f.put(f.attachment,0xc18,51.f);f.seed(213,99,98,97);
    assert(api->select(f.actor,sizeof(f.actor),1,&b,&sr)&&sr.activity==213&&sr.sequence==0&&sr.translation.iterations==2);
    f.available=false;f.calls.clear();
    assert(api->select(f.actor,sizeof(f.actor),1,&b,&sr)&&sr.activity==1&&sr.sequence==0);
    FreeLibrary(server);FreeLibrary(client);std::puts("PASS native NPC activity cascade and original weighted availability fallback");
}
