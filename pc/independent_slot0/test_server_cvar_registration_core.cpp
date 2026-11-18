#include "server_cvar_registration_core.h"
#include "server_post_connect_core.h"
#include <cstdio>
#include <cstring>
using namespace sin_independent;
struct Fixture {
    ServerConsoleRegistration registration;
    ConsoleRecord globalCheats{nullptr,"SV_CHEATS","0",0,false,true};
    ConsoleRecord developer{&globalCheats,"developer","0",0,false,true};
    ConsoleRecord tail{nullptr,"tail","3",0,false};
    ConsoleRecord command{&tail,"test_command",nullptr,kCvarReplicated,true};
    ConsoleRecord replicated{&command,"replicated","5",kCvarReplicated,false};
    ConsoleRecord overridden{&replicated,"overridden","7",kCvarReplicated,false};
    ConsoleRecord already{&overridden,"already","1",0,false,true};
    ConsoleRecord head{&already,"head","1",0,false};
    ConsoleRecord* expected[5]={&head,&overridden,&replicated,&command,&tail};
    ConsoleRecord* engineHead=&developer;
    unsigned registrations=0,lookups=0,sets=0,finds=0;
    const char* chosen[5]{};
    bool valid=true;
};
static void Register(ConsoleRecord& r,void* ctx) noexcept {
    auto& f=*static_cast<Fixture*>(ctx);
    if(f.registrations>=5 || &r!=f.expected[f.registrations] ||
       r.next || !(r.flags&kCvarGameDll) || r.registered) f.valid=false;
    ++f.registrations;
    // Simulate the engine relinking after the original accessor detached.
    // All new next pointers now address the engine list, not the pending
    // DLL-local list.
    r.next=f.engineHead; f.engineHead=&r;
    if(&r==&f.replicated || &r==&f.overridden)r.defaultValue="engine-linked-default";
    if(f.registration.Initialize(nullptr,1,{})!=ConsoleInitResult::Busy)f.valid=false;
}
static const char* CommandLine(const char* name,void* ctx) noexcept {
    auto& f=*static_cast<Fixture*>(ctx);
    if(f.lookups>=5 || f.registrations!=f.lookups+1 ||
       std::strcmp(name,f.expected[f.lookups]->name))f.valid=false;
    ++f.lookups;
    if(EqualConsoleNames(name,"overridden"))return "99";
    if(EqualConsoleNames(name,"test_command"))return "must-not-set-command";
    if(EqualConsoleNames(name,"tail"))return ""; // present empty override
    return nullptr;
}
static void Set(ConsoleRecord& r,const char* value,void* ctx) noexcept {
    auto& f=*static_cast<Fixture*>(ctx);
    if(f.registrations!=f.lookups || f.registrations==0 || f.sets>=5 ||
       &r!=f.expected[f.registrations-1] || r.command || !value)f.valid=false;
    if(f.registrations>0 && f.registrations<=5)f.chosen[f.registrations-1]=value;
    ++f.sets;
}
static ConsoleRecord* Find(const char* name,void* ctx) noexcept {
    auto& f=*static_cast<Fixture*>(ctx);
    if(f.registrations!=5 || std::strcmp(name,"developer"))f.valid=false;
    ++f.finds;return &f.developer;
}
static ConsoleHooks Hooks(Fixture& f) {return {&Register,&CommandLine,&Set,&Find,&f};}
static int fail(int code,const char* why) {
    std::fprintf(stderr,"FAIL server cvar startup %d: %s\n",code,why);return code;
}
// First two actual original startup stages are exercised in the same
// owned fixture, rather than just calling the new registration manager.
static void* __cdecl Factory(const char*,int*) {return nullptr;}
class EventManager {
public:
    virtual void __thiscall Guard() noexcept {valid=false;}
    virtual int __thiscall Load(const char* path) noexcept {
        ++calls;valid=valid && path && !std::strcmp(path,"resource/gameevents.res");
        return 42;
    }
    bool valid=true;unsigned calls=0;
};
int main() {
    Fixture f;
    ServerPostConnectState startup;
    EventManager events;
    int globals=0,count=-1;
    if(!startup.Initialize(&globals,&Factory,&Factory,&Factory) ||
       !startup.LoadInitialEvents(&events,count) || count!=42 ||
       !events.valid || events.calls!=1)return fail(1,"prior startup stages");
    auto hooks=Hooks(f);
    if(f.registration.Initialize(&f.head,3,hooks)!=ConsoleInitResult::InvalidList ||
       f.registrations || f.head.next!=&f.already ||
       f.registration.RegistrationAttempted())return fail(2,"bounded preflight");
    f.tail.next=&f.head;
    if(f.registration.Initialize(&f.head,16,hooks)!=ConsoleInitResult::InvalidList ||
       f.registrations)return fail(3,"cycle rejection");
    f.tail.next=nullptr;
    if(f.registration.Initialize(&f.head,16,{})!=ConsoleInitResult::InvalidInput ||
       f.registrations)return fail(4,"missing hooks");
    if(f.registration.Initialize(&f.head,16,hooks)!=ConsoleInitResult::Initialized ||
       !f.valid || !f.registration.Ready() || !f.registration.RegistrationAttempted() ||
       f.registrations!=5 || f.lookups!=5 || f.sets!=3 || f.finds!=1 ||
       f.registration.Developer()!=&f.developer || f.registration.SvCheats()!=&f.globalCheats)
        return fail(5,"original registration and required lookup sequence");
    if(f.chosen[0] || !f.chosen[1] || std::strcmp(f.chosen[1],"99") ||
       !f.chosen[2] || std::strcmp(f.chosen[2],"5") || f.chosen[3] ||
       !f.chosen[4] || std::strcmp(f.chosen[4],""))
        return fail(6,"override, pre-link replicated default, command, empty override");
    for(auto* p:f.expected)if(!p->registered || !(p->flags&kCvarGameDll))
        return fail(7,"registration marker");
    if(f.already.flags || f.registration.Initialize(&f.head,16,hooks)!=ConsoleInitResult::AlreadyInitialized ||
       f.registrations!=5)return fail(8,"skip and no duplicate side effects");
    Fixture missing;
    missing.developer.next=nullptr;
    if(missing.registration.Initialize(&missing.head,16,Hooks(missing))!=ConsoleInitResult::MissingSvCheats ||
       missing.registration.Ready() || !missing.registration.RegistrationAttempted() ||
       missing.registrations!=5 ||
       missing.registration.Initialize(&missing.head,16,Hooks(missing))!=ConsoleInitResult::AlreadyInitialized)
        return fail(9,"missing sv_cheats after irreversible registration");
    std::puts("PASS post-Connect scalar/event fixture and cvar registration relinking, defaults, overrides, required sv_cheats");
    return 0;
}
