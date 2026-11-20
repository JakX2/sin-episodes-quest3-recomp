// Phase 496: native Win32 original soundscape virtual slot3
// ICvar::FindVar read-only cache adapter with a synthetic engine object.
// No owner retail 2006 DLL, global CVar pointers or engine audio used.
#include "client_soundscape_original_convar_cache.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace s=sin_independent::soundscape_original_cvars;
static int Fail(int n){
    std::fprintf(stderr,"original-soundscape-cvar-cache-test=%d\n",n);
    return n;
}
struct FakeCvar {
    int mixer_value=11;
    int dsp_value=22;
    bool expose_mixer=true;
    bool expose_dsp=true;
    bool throw_on_find=false;
    std::vector<std::string> requested;
    // Explicit first four vtable slots, matching original 2006
    // VEngineCvar002 FindVar location +0x0C. Only slot3 is called.
    virtual void Slot0(){throw std::logic_error("wrong slot 0");}
    virtual void Slot1(){throw std::logic_error("wrong slot 1");}
    virtual void Slot2(){throw std::logic_error("wrong slot 2");}
    virtual const void* FindVar(const char* name){
        if(throw_on_find)throw std::runtime_error("fabricated engine refusal");
        requested.emplace_back(name?name:"");
        if(name &&std::strcmp(name,s::kMixerName)==0)
            return expose_mixer?&mixer_value:nullptr;
        if(name &&std::strcmp(name,s::kDspName)==0)
            return expose_dsp?&dsp_value:nullptr;
        return nullptr;
    }
};
int main(){
    static_assert(sizeof(void*)==4,"native original 2006 x86 ABI");
    static_assert(s::kOriginalCvarVirtualSlot==3,"slot3");
    static_assert(s::kOriginalSystemVtableSlot==3,"system slot3");
    s::OwnedCache cache{};
    const auto noEngine=cache.Resolve(nullptr);
    if(noEngine.result!=s::Result::InvalidCvarInterface ||
       noEngine.requested!=0 ||cache.mixer()||cache.dsp() ||
       cache.associated_engine())
        return Fail(1);

    FakeCvar engine{};
    auto first=cache.Resolve(&engine);
    if(first.result!=s::Result::BothCached ||
       first.requested!=2 ||first.found_this_call!=2 ||
       !first.mixer_cached||!first.dsp_cached ||
       first.different_engine_instance ||
       cache.mixer()!=&engine.mixer_value ||
       cache.dsp()!=&engine.dsp_value ||
       cache.associated_engine()!=&engine ||
       engine.requested.size()!=2 ||
       engine.requested[0]!="snd_soundmixer" ||
       engine.requested[1]!="dsp_volume")
        return Fail(2);
    // Original code tests both GLOBAL pointers against zero and
    // skips FindVar completely when they were cached previously.
    engine.requested.clear();
    const auto again=cache.Resolve(&engine);
    if(again.result!=s::Result::BothCached ||again.requested!=0 ||
       again.found_this_call!=0 ||!engine.requested.empty())
        return Fail(3);

    cache.Reset();
    engine.requested.clear();
    engine.expose_dsp=false;
    auto one=cache.Resolve(&engine);
    if(one.result!=s::Result::OneCached ||
       one.requested!=2 ||one.found_this_call!=1 ||
       cache.mixer()!=&engine.mixer_value ||cache.dsp()!=nullptr)
        return Fail(4);
    engine.requested.clear();
    engine.expose_dsp=true;
    const auto retry=cache.Resolve(&engine);
    if(retry.result!=s::Result::BothCached ||
       retry.requested!=1 ||retry.found_this_call!=1 ||
       engine.requested.size()!=1 ||
       engine.requested[0]!="dsp_volume" ||
       cache.dsp()!=&engine.dsp_value)return Fail(5);

    // A SECOND engine provider must not be handed references that
    // belong to the first fake module's destroyed lifetime.
    FakeCvar engine2{};
    engine2.expose_mixer=false;
    engine2.expose_dsp=false;
    const auto changed=cache.Resolve(&engine2);
    if(changed.result!=s::Result::NeitherCached ||
       !changed.different_engine_instance ||
       changed.requested!=2 ||
       cache.mixer()!=nullptr ||cache.dsp()!=nullptr ||
       cache.associated_engine()!=&engine2 ||
       engine2.requested.size()!=2)return Fail(6);
    engine2.requested.clear();
    engine2.expose_mixer=true;
    const auto missingRetry=cache.Resolve(&engine2);
    if(missingRetry.result!=s::Result::OneCached ||
       missingRetry.requested!=2 ||
       engine2.requested.size()!=2 ||
       cache.mixer()!=&engine2.mixer_value ||
       cache.dsp()!=nullptr)return Fail(7);

    // Invalid interfaces are refused before any engine call and
    // do not destroy an already accepted owner cache.
    const auto invalid=cache.Resolve(nullptr);
    if(invalid.result!=s::Result::InvalidCvarInterface ||
       invalid.requested!=0 ||
       cache.mixer()!=&engine2.mixer_value ||
       cache.associated_engine()!=&engine2)return Fail(8);

    // Engine callback exceptions also refuse atomically. This
    // C++ exception fixture does not emulate Win32 SEH.
    engine2.throw_on_find=true;
    engine2.requested.clear();
    const auto refused=cache.Resolve(&engine2);
    if(refused.result!=s::Result::CallbackRefused ||
       cache.mixer()!=&engine2.mixer_value ||
       cache.dsp()!=nullptr ||
       cache.associated_engine()!=&engine2 ||
       !engine2.requested.empty())return Fail(9);
    engine2.throw_on_find=false;
    engine2.expose_dsp=true;
    const auto recovered=cache.Resolve(&engine2);
    if(recovered.result!=s::Result::BothCached ||
       recovered.requested!=1 ||recovered.found_this_call!=1 ||
       cache.dsp()!=&engine2.dsp_value)return Fail(10);

    // A different engine throwing on its first lookup must not leave
    // stale pointers or an association to the previous engine.
    FakeCvar throwingEngine{};
    throwingEngine.throw_on_find=true;
    const auto switchRefused=cache.Resolve(&throwingEngine);
    if(switchRefused.result!=s::Result::CallbackRefused ||
       !switchRefused.different_engine_instance ||
       switchRefused.requested!=1 ||
       switchRefused.mixer_cached || switchRefused.dsp_cached ||
       cache.associated_engine() || cache.mixer() || cache.dsp())
        return Fail(11);
    throwingEngine.throw_on_find=false;
    const auto afterSwitch=cache.Resolve(&throwingEngine);
    if(afterSwitch.result!=s::Result::BothCached ||
       afterSwitch.requested!=2 ||
       cache.associated_engine()!=&throwingEngine ||
       cache.mixer()!=&throwingEngine.mixer_value ||
       cache.dsp()!=&throwingEngine.dsp_value)return Fail(12);

    cache.Reset();
    if(cache.associated_engine()||cache.mixer()||cache.dsp())
        return Fail(11);
    std::puts("PASS original soundscape slot3 61B CVar cache: native VEngineCvar002 FindVar slot3, snd_soundmixer+dsp_volume source order, cached skips, missing retry, engine identity isolation and safe rejection");
    return 0;
}
