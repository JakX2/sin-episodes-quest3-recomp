#include "server_extended_cvar_catalog.h"
#include <cstdio>
#include <cstring>
using namespace sin_independent::console_abi;
struct TraceState {
    std::size_t entered=0,returned=0,verified=0;
    bool correct=true,preflight=false;
};
static TraceState traceState;
static void Trace(const char* phase,const char* name) noexcept {
    auto& t=traceState;
    if(!std::strcmp(phase,"cvar-batch-preflight-complete")) {
        if(t.preflight || std::strcmp(name,"181"))t.correct=false;
        t.preflight=true;return;
    }
    if(!t.preflight || t.verified>=kExtendedCount ||
       std::strcmp(name,kExtendedSpecs[t.verified].name))t.correct=false;
    if(!std::strcmp(phase,"cvar-batch-register-entered")) {
        if(t.entered!=t.verified || t.returned!=t.verified)t.correct=false;
        ++t.entered;
    } else if(!std::strcmp(phase,"cvar-batch-register-returned")) {
        if(t.entered!=t.verified+1 || t.returned!=t.verified)t.correct=false;
        ++t.returned;
    } else if(!std::strcmp(phase,"cvar-batch-registration-verified")) {
        if(t.returned!=t.verified+1)t.correct=false;
        ++t.verified;
    } else t.correct=false;
}
class Engine final:public EngineCvar {
public:
    enum Mode {Normal,LastConflict,LastOverride,Relink,FinalNumericMutation};
    Mode mode;
    Variable existing;
    CommandBase* head=nullptr;
    Variable* objects[kExtendedCount]{};
    std::size_t registered=0,overrides=0,lists=0;
    mutable std::size_t finds=0;
    bool correct=true;
    explicit Engine(Mode m):mode(m) {
        existing.CreateVariable(kExtendedSpecs[kExtendedCount-1].name,"99",0,nullptr,false,0,false,0,nullptr);
    }
    std::size_t Index(const char* name) const noexcept {
        for(std::size_t i=0;i<kExtendedCount;++i)
            if(SameName(name,kExtendedSpecs[i].name))return i;
        return kExtendedCount;
    }
    void __thiscall RegisterConCommandBase(CommandBase* p) noexcept override {
        if(!p || registered>=kExtendedCount || p->IsCommand() ||
           p->registered || p->next || !p->IsBitSet(4) ||
           overrides!=kExtendedCount+registered || lists!=1) {
            correct=false;return;
        }
        auto* v=static_cast<Variable*>(p);
        const auto& spec=kExtendedSpecs[registered];
        if(std::strcmp(v->GetName(),spec.name) || std::strcmp(v->String(),spec.value) ||
           v->parent!=v || v->callback || v->hasMin || v->hasMax)correct=false;
        objects[registered++]=v;
        v->next=head;head=v;
        if(mode==Relink && registered==100)v->parent=&existing;
        if(mode==FinalNumericMutation && registered==kExtendedCount)
            objects[0]->integer+=1;
    }
    const char* __thiscall GetCommandLineValue(const char* name) noexcept override {
        ++overrides;const auto i=Index(name);
        if(i==kExtendedCount)correct=false;
        return mode==LastOverride && i==kExtendedCount-1?"1":nullptr;
    }
    Variable* __thiscall FindMutableVar(const char*) noexcept override {
        correct=false;return nullptr;
    }
    const Variable* __thiscall FindConstVar(const char* name) const noexcept override {
        ++finds;const auto i=Index(name);
        if(i>=kExtendedCount)return nullptr;
        if(mode==LastConflict && i==kExtendedCount-1)return &existing;
        return objects[i];
    }
    CommandBase* __thiscall GetCommands() noexcept override {
        if(++lists!=1 || registered || overrides!=kExtendedCount)correct=false;
        return head;
    }
};
int main() {
    for(auto mode:{Engine::Normal,Engine::LastConflict,Engine::LastOverride,
                   Engine::Relink,Engine::FinalNumericMutation}) {
        Engine engine(mode);
        BatchVariablesFor<kExtendedCount> owned;
        traceState={};
        const auto outcome=owned.Register(kExtendedSpecs,&engine,&Trace);
        const std::size_t expected=mode==Engine::Normal || mode==Engine::FinalNumericMutation?
            kExtendedCount:mode==Engine::Relink?99u:0u;
        const auto result=mode==Engine::Normal?SingleCvarResult::Verified:
            mode==Engine::LastConflict?SingleCvarResult::ExistingName:
            mode==Engine::LastOverride?SingleCvarResult::OverridePresent:
            mode==Engine::Relink?SingleCvarResult::Relinked:SingleCvarResult::LookupMismatch;
        if(outcome.result!=result || outcome.verified!=expected ||
           traceState.verified!=expected || !traceState.correct || !engine.correct)
            return 1+mode;
        if(mode==Engine::Normal || mode==Engine::FinalNumericMutation) {
            if(engine.registered!=kExtendedCount || traceState.entered!=kExtendedCount ||
               traceState.returned!=kExtendedCount)return 10;
        } else if(mode==Engine::Relink) {
            if(engine.registered!=100 || traceState.entered!=100 ||
               traceState.returned!=100)return 11;
        } else if(engine.registered || traceState.preflight || traceState.entered)return 12;
        const auto before=engine.registered;
        if(owned.Register(kExtendedSpecs,&engine,&Trace).result!=SingleCvarResult::InvalidInterface ||
           engine.registered!=before)return 13;
    }
    // Duplicate names are rejected before any engine calls.
    static constexpr BatchSpec duplicate[]={{"Same","0",1,2},{"same","1",3,4}};
    BatchVariablesFor<2> invalid;
    Engine engine(Engine::Normal);
    if(invalid.Register(duplicate,&engine,&Trace).result!=SingleCvarResult::BadObject ||
       engine.finds || engine.overrides || engine.registered)return 14;
    std::puts("PASS 181 original candidate records: native registration, all-name preflight, partial failure, final numeric recheck");
    return 0;
}
