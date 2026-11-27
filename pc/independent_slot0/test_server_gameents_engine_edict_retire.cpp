// Synthetic original-engine edict stride, release order, serial invalidation.
// The tested engine code is not executed and no original binary is packaged.
#include "server_gameents_engine_edict_retire_core.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
using namespace sin_independent;
struct EngineEdict {
    std::uint32_t flags=0x67,serial=43,networkable=0xabc,
                  unknown=0xdef,next=0x12345678;
};
static_assert(sizeof(EngineEdict)==0x14,"original engine edict stride");
static_assert(offsetof(EngineEdict,unknown)==0x0c,"unknown offset");
static_assert(offsetof(EngineEdict,next)==0x10,"free-link offset");
struct Fixture {
    EngineEdict elements[3]{};
    unsigned authorized=0,freed=0;
    bool refuse=false,valid=true;
};
static bool Authorized(void* context,const void* edict,std::size_t count) noexcept {
    auto& f=*static_cast<Fixture*>(context);
    ++f.authorized;
    if(count!=0x14)return false;
    for(const auto& item:f.elements)if(edict==&item)return true;
    return false;
}
static bool AuthorizedArray(void* context,const void* array,
                            std::size_t bytes) noexcept {
    auto& f=*static_cast<Fixture*>(context);
    return array==f.elements&&bytes<=sizeof(f.elements)&&
           bytes>=sizeof(EngineEdict);
}
static bool FreeEntity(void* context,void* edict) noexcept {
    auto& f=*static_cast<Fixture*>(context);
    ++f.freed;
    auto* raw=static_cast<EngineEdict*>(edict);
    // Source owner must be invoked after clearing bit 0x02, but BEFORE
    // engine clears the trailing pointer and poisons serial+0x04.
    if((raw->flags&2u)!=0||raw->serial!=43||
       raw->next!=0x12345678)f.valid=false;
    if(f.refuse)return false;
    // Independently model server.dll's separate clearing of flags+unknown.
    raw->flags=0;
    raw->unknown=0;
    return true;
}
static int Fail(int n,const char* why) {
    std::fprintf(stderr,"FAIL engine edict retirement %d: %s\n",n,why);return n;
}
int main() {
    Fixture f;
    const EngineEdictRetireHooks h{&f,&Authorized,&AuthorizedArray,&FreeEntity};
    auto& target=f.elements[1];
    if(RetireEngineEdictAt(f.elements,sizeof(f.elements),1,h)!=
       EngineEdictRetireResult::Retired||!f.valid||f.freed!=1||
       target.flags!=0||target.unknown!=0||target.next!=0||
       target.serial!=0xffffffffu)
        return Fail(1,"original indexed release ordering/fields changed");
    const auto earlier_freed=f.freed;
    if(RetireEngineEdictAt(f.elements,sizeof(f.elements),1,h)!=
       EngineEdictRetireResult::Inactive||f.freed!=earlier_freed)
        return Fail(2,"retired indexed edict double-freed");
    if(RetireEngineEdictAt(f.elements,sizeof(f.elements),-1,h)!=
       EngineEdictRetireResult::InvalidInput||
       RetireEngineEdictAt(f.elements,sizeof(f.elements),3,h)!=
       EngineEdictRetireResult::InvalidInput||f.freed!=earlier_freed)
        return Fail(3,"bad edict indices accepted");
    EngineEdict rogue{};
    const auto old_flags=rogue.flags;
    if(RetireEngineEdictAt(&rogue,sizeof(rogue),0,h)!=
       EngineEdictRetireResult::Unauthorized||
       rogue.flags!=old_flags||f.freed!=earlier_freed)
        return Fail(8,"unowned engine array was accepted before index lookup");
    EngineEdictRetireHooks incomplete_hooks=h;
    incomplete_hooks.authorized_array=nullptr;
    if(RetireEngineEdictAt(f.elements,sizeof(f.elements),0,incomplete_hooks)!=
       EngineEdictRetireResult::InvalidInput||
       f.freed!=earlier_freed)
        return Fail(9,"unbound array ownership accepted");
    if(RetireEngineEdict(&rogue,sizeof(rogue),h)!=
       EngineEdictRetireResult::Unauthorized||
       rogue.flags!=old_flags||f.freed!=earlier_freed)
        return Fail(4,"foreign edict was touched");
    if(RetireEngineEdict(&f.elements[0],0x13,h)!=
       EngineEdictRetireResult::InvalidInput||f.freed!=earlier_freed)
        return Fail(5,"truncated engine edict prefix accepted");
    f.refuse=true;
    auto& incomplete=f.elements[2];
    if(RetireEngineEdictAt(f.elements,sizeof(f.elements),2,h)!=
       EngineEdictRetireResult::CallbackRefused||
       incomplete.flags!=(0x67u&~2u)||
       incomplete.serial!=43||incomplete.next!=0x12345678||
       f.freed!=earlier_freed+1)
        return Fail(6,"callback refusal incorrectly finalized edict");
    f.refuse=false;
    if(RetireEngineEdict(&f.elements[0],sizeof(EngineEdict),h)!=
       EngineEdictRetireResult::Retired||
       f.elements[0].serial!=0xffffffffu||
       f.elements[0].next!=0||!f.valid)
        return Fail(7,"direct engine release not supported");
    std::puts("PASS engine original 0x14 edict stride, index gate, before/after GameEnts release writes and refusal boundary");
    return 0;
}
