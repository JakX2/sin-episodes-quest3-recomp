#include "native_server_inherited_activity.h"
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
    alignas(4) unsigned char actor[0xa1c]{},target[0x3a6]{},weapon[4]{},desc[4][0x40]{};
    sin_independent::EntityHandleEntry slots[3]{};
    ServerActivityDescriptor rows[4]{};
    std::uint32_t gen=3;
    ServerActivityGenerationNode root{&gen,-1};
    ServerActivityCompositeView view{rows,4,&root,1};
    std::vector<int> calls;
    bool model=true,context=true;float fraction=.75f;
    template<class T> void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto get=reinterpret_cast<GetServerInheritedActivityApi>(GetProcAddress(server,server_inherited_activity_export));
    assert(get&&!GetProcAddress(client,server_inherited_activity_export)&&!get(0,sizeof(ServerInheritedActivityApi))&&!get(1,0));
    auto* api=get(1,sizeof(ServerInheritedActivityApi));assert(api);
    F f;f.slots[1].serial=4;f.slots[1].entity=reinterpret_cast<CBaseEntity*>(f.target);
    f.slots[2].serial=5;f.slots[2].entity=reinterpret_cast<CBaseEntity*>(f.weapon);
    f.put(f.actor,0x78c,0x5002u);f.put(f.actor,0xa18,0x4001u);
    const int activities[]{3,4,5,67};
    for(unsigned i=0;i<4;++i){
        f.rows[i]={f.desc[i],0x40,0};f.desc[i][0xd]=0x10;f.put(f.desc[i],0x10,activities[i]);f.put(f.desc[i],0x14,1);
    }
    ServerEventDictionaryEntry ar[2]{},er[2]{};
    ServerActivityDictionary ad{{ar,2,0,3,0},-1};ServerEventDictionary ed{er,2,0,0,0};
    ServerNpcActivityTranslateBindings seq{};seq.context=&f;seq.activities=&ad;seq.events=&ed;seq.random.context=&f;
    seq.metadata=[](void* p,void*,const ServerActivityCompositeView** out) noexcept -> int {
        auto& f=F::self(p);f.calls.push_back(4);*out=f.model?&f.view:nullptr;return f.model;};
    seq.random.random=[](void* p,int lo,int hi,int idx,int* out) noexcept -> int {
        assert(lo==0&&hi==0);F::self(p).calls.push_back(10+idx);*out=0;return 1;};
    ServerInheritedActivityBindings b{};b.context=&f;b.sequences=&seq;b.weapons={f.slots,3,&f,
        [](void* p,void* w,unsigned* out) noexcept -> int {auto& f=F::self(p);assert(w==f.weapon);f.calls.push_back(1);*out=WeaponActivityAssault;return 1;},nullptr};
    b.target_extent=[](void* p,void* t,unsigned* out) noexcept -> int {
        auto& f=F::self(p);assert(t==f.target);f.calls.push_back(3);*out=sizeof(f.target);return 1;};
    b.reload_context=[](void* p,void*) noexcept -> int {auto& f=F::self(p);f.calls.push_back(2);return f.context;};
    b.reload_trace=[](void* p,void* a,void* t,int activity,float* out) noexcept -> int {
        auto& f=F::self(p);assert(a==f.actor&&t==f.target&&activity==67);f.calls.push_back(5);*out=f.fraction;return 1;};
    ServerInheritedActivityResult out{};
    auto run=[&](int activity){f.calls.clear();return api->translate(f.actor,sizeof(f.actor),activity,&b,&out);};
    assert(run(66)&&out.activity==66&&out.capabilities==0x2000u&&(f.calls==std::vector<int>{1}));
    f.put(f.actor,0x8ec,0x08000000u);f.put(f.target,0x3a4,std::int16_t{100});
    assert(run(3)&&out.activity==4&&out.sequence==1&&out.target_live&&(f.calls==std::vector<int>{1,3,4,11}));
    f.put(f.target,0x3a4,std::int16_t{101});assert(run(3)&&out.activity==5&&out.sequence==2);
    assert(run(1)&&out.activity==1&&!out.probes);f.actor[0x988]=2;
    assert(run(1)&&out.activity==5&&out.sequence==2);
    f.put(f.desc[2],0x10,99);assert(run(1)&&out.activity==1&&out.sequence==-1);
    f.put(f.actor,0xa18,0x6001u);
    assert(run(3)&&out.activity==3&&!out.target_live&&(f.calls==std::vector<int>{1,4,10}));
    assert(run(66)&&out.activity==66&&!out.trace_called&&(f.calls==std::vector<int>{1}));
    f.put(f.actor,0xa18,0x4001u);f.context=false;
    assert(run(66)&&out.activity==66&&(f.calls==std::vector<int>{1,2}));
    f.context=true;f.put(f.target,0x3a4,std::int16_t{99});
    assert(run(66)&&out.activity==66&&(f.calls==std::vector<int>{1,2,3}));
    f.put(f.target,0x3a4,std::int16_t{100});
    assert(run(66)&&out.activity==67&&out.trace_called&&out.sequence==3&&(f.calls==std::vector<int>{1,2,3,4,13,5}));
    f.fraction=1.f;assert(run(66)&&out.activity==66&&out.trace_called);
    f.put(f.desc[3],0x10,99);assert(run(66)&&out.activity==66&&!out.trace_called&&out.sequence==-1);
    // Original weighted selector null-model result is zero, not -1.
    f.model=false;f.fraction=.5f;assert(run(66)&&out.activity==67&&out.sequence==0&&out.trace_called);
    FreeLibrary(server);FreeLibrary(client);std::puts("PASS inherited NPC capability gate, target posture, sequence choice and reload trace boundary");
}
