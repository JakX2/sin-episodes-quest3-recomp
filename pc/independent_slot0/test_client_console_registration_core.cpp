#include "client_console_registration_core.h"
#include <cstdio>
#include <cstring>
using namespace sin_independent::console_abi;
static void __cdecl Execute(){}
class Engine final:public EngineCvar {
public:
    Variable shared;
    CommandBase* head=nullptr;
    CommandBase* expected[3]{};
    Variable* duplicate=nullptr;
    ClientConsoleRegistration<4>* manager=nullptr;
    unsigned calls=0;
    bool good=true;
    Engine() {
        shared.CreateVariable("shared","99",0x2000,"",false,0,false,0,nullptr);
        shared.registered=true;head=&shared;
    }
    void __thiscall RegisterConCommandBase(CommandBase* p) noexcept override {
        if(calls>=3 || expected[calls]!=p || p->next || p->registered ||
           !(p->flags&8) || (p->flags&4) ||
           manager->Initialize(this)!=ClientRegisterResult::AlreadyAttempted)
            good=false;
        ++calls;
        if(p==duplicate)duplicate->parent=&shared;
        p->next=head;head=p;
    }
    const char* __thiscall GetCommandLineValue(const char*) noexcept override {
        good=false;return nullptr;
    }
    Variable* __thiscall FindMutableVar(const char*) noexcept override {
        good=false;return nullptr;
    }
    const Variable* __thiscall FindConstVar(const char*) const noexcept override {
        return nullptr;
    }
    CommandBase* __thiscall GetCommands() noexcept override {good=false;return head;}
};
int main() {
    Engine e;
    ClientConsoleRegistration<4> manager;
    e.manager=&manager;
    Variable local,duplicate,excluded,already;
    Command input;
    local.CreateVariable("shared","7",0x2000,"",false,0,false,0,nullptr);
    duplicate.CreateVariable("shared","5",0x2000,"",false,0,false,0,nullptr);
    excluded.CreateVariable("excluded","1",1,"",false,0,false,0,nullptr);
    already.CreateVariable("already","0",0,"",false,0,false,0,nullptr);
    input.CreateCommand("+attack",&Execute,"",0,nullptr);
    if(!manager.Add(local) || !manager.Add(already) ||
       !manager.Add(excluded) || !manager.Add(input) ||
       !manager.Add(duplicate) || manager.Count()!=4 ||
       manager.Add(local) || manager.Head()!=&duplicate ||
       duplicate.next!=&input || input.next!=&already ||
       already.next!=&local || excluded.next)return 1;
    already.registered=true;
    e.expected[0]=&duplicate;e.expected[1]=&input;e.expected[2]=&local;
    e.duplicate=&duplicate;
    local.next=&duplicate;
    if(manager.Initialize(&e)!=ClientRegisterResult::InvalidList || e.calls ||
       manager.Attempted())return 2;
    local.next=nullptr;
    if(manager.Initialize(nullptr)!=ClientRegisterResult::InvalidInput ||
       manager.Initialize(&e)!=ClientRegisterResult::Completed || !e.good ||
       e.calls!=3 || !manager.Attempted() ||
       e.head!=&local || local.next!=&input || input.next!=&duplicate ||
       duplicate.next!=&e.shared || e.shared.next)return 3;
    if(!local.registered || !duplicate.registered || !input.registered ||
       duplicate.parent!=&e.shared || std::strcmp(e.shared.String(),"99") ||
       std::strcmp(duplicate.string,"5") || std::strcmp(local.String(),"7") ||
       excluded.registered || excluded.flags!=1 || already.flags)return 4;
    if(manager.Initialize(&e)!=ClientRegisterResult::AlreadyAttempted ||
       manager.Add(excluded) || e.calls!=3)return 5;
    std::puts("PASS native client console registration: prepend order, duplicate names, FCVAR_CLIENTDLL, no server override and parent relinking");
    return 0;
}
