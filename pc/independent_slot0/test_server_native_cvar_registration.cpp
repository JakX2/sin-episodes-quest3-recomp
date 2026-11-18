#include "server_native_cvar_registration.h"
#include <cstdio>
#include <cstring>
using namespace sin_independent::console_abi;
static int fail(int n,const char* s) {
    std::fprintf(stderr,"FAIL native console registration %d: %s\n",n,s);return n;
}
static void __cdecl Execute(){}
class EngineRoot:public Variable {
public:
    void __thiscall InternalString(const char* value) override {
        ++stringCalls;
        Variable::InternalString(value);
    }
    unsigned stringCalls=0;
};
class EngineFixture final:public EngineCvar {
public:
    EngineRoot replicated,overrideRoot;
    Variable developer,cheats;
    CommandBase* head=nullptr;
    Variable* linked=nullptr;
    Variable* overridden=nullptr;
    CommandBase* expected[3]{};
    NativeConsoleRegistration* manager=nullptr;
    unsigned registers=0,values=0;
    mutable unsigned constFinds=0;
    bool correct=true;
    explicit EngineFixture(bool hasCheats) {
        replicated.CreateVariable("shared","99",0x2000,"engine root",false,0,false,0,nullptr);
        overrideRoot.CreateVariable("override","9",0x2000,"engine root",false,0,false,0,nullptr);
        developer.CreateVariable("developer","0",0,"",false,0,false,0,nullptr);
        cheats.CreateVariable("sv_cheats","0",0,"",false,0,false,0,nullptr);
        replicated.registered=overrideRoot.registered=developer.registered=cheats.registered=true;
        head=&developer;
        developer.next=hasCheats?&cheats:nullptr;
    }
    void __thiscall RegisterConCommandBase(CommandBase* p) noexcept override {
        if(!p || registers>=3) {correct=false;return;}
        if(p!=expected[registers] || p->next || p->registered ||
           !(p->flags&4) || values!=registers)correct=false;
        if(manager && manager->Initialize(nullptr,16,this)!=RegisterResult::Busy)
            correct=false;
        ++registers;
        // Real shared variables can have a root object in the engine.
        // The synthetic root uses a different vtable and records every
        // string setter call, so accidental static dispatch is detected.
        if(p==linked)linked->parent=&replicated;
        if(p==overridden)overridden->parent=&overrideRoot;
        p->next=head;head=p;
    }
    const char* __thiscall GetCommandLineValue(const char* name) noexcept override {
        if(values>=3 || registers!=values+1 || !name ||
           std::strcmp(name,expected[values]->GetName()))correct=false;
        ++values;
        if(SameName(name,"override"))return "17";
        if(SameName(name,"test_command"))return "must not set a command";
        return nullptr;
    }
    Variable* __thiscall FindMutableVar(const char*) noexcept override {
        correct=false;return nullptr;
    }
    const Variable* __thiscall FindConstVar(const char* name) const noexcept override {
        ++constFinds;
        return name && !std::strcmp(name,"developer") && registers==3 && values==3?
            &developer:nullptr;
    }
    CommandBase* __thiscall GetCommands() noexcept override {correct=false;return head;}
};
struct LocalFixture {
    Variable shared,overridden,already;
    Command command;
    explicit LocalFixture(EngineFixture& engine) {
        shared.CreateVariable("shared","5",0x2000,"",false,0,false,0,nullptr);
        overridden.CreateVariable("override","7",0x2000,"",false,0,false,0,nullptr);
        already.CreateVariable("already","1",0,"",false,0,false,0,nullptr);
        already.registered=true;
        command.CreateCommand("test_command",&Execute,"",0,nullptr);
        shared.next=&already;already.next=&command;command.next=&overridden;
        engine.linked=&shared;engine.overridden=&overridden;
        engine.expected[0]=&shared;engine.expected[1]=&command;engine.expected[2]=&overridden;
    }
};
int main() {
    EngineFixture engine(true);
    LocalFixture local(engine);
    NativeConsoleRegistration manager;
    engine.manager=&manager;
    if(manager.Initialize(&local.shared,16,nullptr)!=RegisterResult::InvalidEngine ||
       manager.Initialize(&local.shared,3,&engine)!=RegisterResult::InvalidList ||
       engine.registers || manager.Attempted() || local.shared.next!=&local.already)
        return fail(1,"preflight");
    local.overridden.next=&local.shared;
    if(manager.Initialize(&local.shared,16,&engine)!=RegisterResult::InvalidList ||
       engine.registers)return fail(2,"cycle");
    local.overridden.next=nullptr;
    if(manager.Initialize(&local.shared,16,&engine)!=RegisterResult::Registered ||
       !engine.correct || engine.registers!=3 || engine.values!=3 ||
       engine.constFinds!=1 || !manager.Ready() ||
       manager.Developer()!=&engine.developer || manager.SvCheats()!=&engine.cheats)
        return fail(3,"native accessor and original local-head lookup sequence");
    if(local.shared.parent!=&engine.replicated ||
       local.overridden.parent!=&engine.overrideRoot ||
       !local.shared.registered || !local.overridden.registered ||
       !local.command.registered || local.already.flags ||
       engine.replicated.stringCalls!=1 || engine.overrideRoot.stringCalls!=1 ||
       std::strcmp(engine.replicated.String(),"5") ||
       std::strcmp(engine.overrideRoot.String(),"17") ||
       std::strcmp(local.shared.string,"5") || std::strcmp(local.overridden.string,"7"))
        return fail(4,"pre-link default, override, root virtual dispatch, stable alias storage");
    if(engine.head!=&local.overridden || local.overridden.next!=&local.command ||
       local.command.next!=&local.shared || local.shared.next!=&engine.developer ||
       engine.developer.next!=&engine.cheats)
        return fail(5,"engine-owned relinking");
    if(manager.Initialize(&local.shared,16,&engine)!=RegisterResult::AlreadyAttempted ||
       engine.registers!=3)return fail(6,"no duplicate effects");
    EngineFixture absent(false);
    LocalFixture second(absent);
    NativeConsoleRegistration missing;
    absent.manager=&missing;
    if(missing.Initialize(&second.shared,16,&absent)!=RegisterResult::MissingSvCheats ||
       missing.Ready() || !missing.Attempted() || absent.registers!=3 ||
       missing.Initialize(&second.shared,16,&absent)!=RegisterResult::AlreadyAttempted)
        return fail(7,"missing required sv_cheats after irreversible effects");
    std::puts("PASS native console registration: VEngineCvar002 virtual dispatch, root relinking, pre-link defaults, overrides and startup references");
    return 0;
}
