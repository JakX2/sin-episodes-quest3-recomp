// Source-free native MSVC Win32 x86 fixture of original automatic
// game-system linked registration and first-virtual Init ordering.
#include "client_auto_gamesystem_init_core.h"
#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>
namespace reg=sin_independent::auto_gamesystem;
struct Subject:reg::System {
    std::vector<int>* calls=nullptr;
    reg::Registry* registry=nullptr;
    Subject* append=nullptr;
    int id=0;
    bool result=true;
    bool appended=false;
    bool Init() noexcept override {
        calls->push_back(id);
        if(append && registry && !appended) {
            appended=true;
            registry->Construct(append);
        }
        return result;
    }
};
static bool Equal(const std::vector<int>& a,const std::vector<int>& b){
    return a==b;
}
int main(){
    static_assert(sizeof(void*)==4,"original ABI is 32-bit");
    // Genuine constructor writes one pointer at +0 and a next link at +4.
    std::vector<int> calls;
    reg::Registry startup;
    Subject a,b,c;
    a.calls=b.calls=c.calls=&calls;a.id=1;b.id=2;c.id=3;
    if(reinterpret_cast<std::uintptr_t>(&a.next)-
       reinterpret_cast<std::uintptr_t>(&a)!=4)return 1;
    startup.Construct(&a);
    startup.Construct(&b);
    startup.DirectRegister(&c);
    if(!startup.has_pending() || startup.initialization_started())return 2;
    auto result=startup.Initialize();
    if(!result.succeeded() || result.initialized_count!=3 ||
       !Equal(calls,{3,2,1}) || !startup.initialization_started() ||
       startup.has_pending())return 3;
    // Constructing after start appends to live vector, not the old list.
    Subject d;d.calls=&calls;d.id=4;
    startup.Construct(&d);
    if(startup.has_pending() || startup.registered_size()!=4)return 4;
    // Native direct registration does NOT perform a duplicate check.
    calls.clear();
    reg::Registry direct;
    direct.DirectRegister(&a);direct.DirectRegister(&a);
    result=direct.Initialize();
    if(!result.succeeded() || !Equal(calls,{1,1}) ||
       result.duplicate_skipped)return 5;
    // Only the pending-list merger skips pointers already in vector.
    calls.clear();
    reg::Registry duplicate;
    duplicate.DirectRegister(&a);duplicate.Construct(&a);
    duplicate.Construct(&b);
    result=duplicate.Initialize();
    if(!result.succeeded() || result.duplicate_skipped!=1 ||
       duplicate.registered_size()!=2 || !Equal(calls,{1,2}))return 6;
    // Failure stops at that position, does not roll back the bit/list.
    calls.clear();
    reg::Registry failure;
    b.result=false;
    failure.DirectRegister(&a);failure.DirectRegister(&b);
    failure.DirectRegister(&c);
    result=failure.Initialize();
    if(result.outcome!=reg::Outcome::InitRefused ||
       result.initialized_count!=1 || !Equal(calls,{1,2}) ||
       !failure.initialization_started() || failure.has_pending())return 7;
    b.result=true;
    calls.clear();
    result=failure.Initialize();
    if(!result.succeeded() || !Equal(calls,{1,2,3}))return 8;
    // A running original Init may append a new object, and the loop
    // compares the live count, so that new object runs in this pass.
    calls.clear();
    reg::Registry growing;
    Subject first,late;
    first.calls=late.calls=&calls;
    first.id=11;late.id=12;
    first.registry=&growing;first.append=&late;
    growing.DirectRegister(&first);
    result=growing.Initialize();
    if(!result.succeeded() || !Equal(calls,{11,12}) ||
       result.initialized_count!=2)return 9;
    reg::Registry invalid;
    invalid.DirectRegister(nullptr);
    if(invalid.Initialize().outcome!=reg::Outcome::InvalidEntry ||
       !invalid.initialization_started())return 10;
    reg::Registry empty;
    if(!empty.Initialize().succeeded() || !empty.initialization_started())
        return 11;
    // Original would spin on malformed cyclic pending lists; host refuses.
    reg::Registry cycle;
    Subject loop;loop.calls=&calls;loop.id=13;
    cycle.Construct(&loop);loop.next=&loop;
    if(cycle.Initialize().outcome!=reg::Outcome::CyclicPending ||
       cycle.initialization_started())return 12;
    std::puts("PASS original auto-game-system pending LIFO, direct registrations, duplicates, Init failure and live growth");
    return 0;
}
