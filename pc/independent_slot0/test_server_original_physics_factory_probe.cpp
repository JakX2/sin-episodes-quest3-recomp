// Invented native MSVC x86 physics factory, no retail game process.
#include "server_original_physics_factory_probe.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
namespace s=sin_independent::server_original_physics_factory;
static std::vector<std::string> calls;
static unsigned failAt=999;
static int fakeInterface=123;
static bool badStatus=false;
static void* __cdecl Factory(const char* name,int* status){
    if(status)badStatus=true;
    calls.emplace_back(name?name:"");
    if(calls.size()==failAt)return nullptr;
    return &fakeInterface;
}
static int Fail(int n){std::fprintf(stderr,"server physics ABI=%d\n",n);return n;}
int main(){
    static_assert(sizeof(void*)==4,"Source Windows 2006 x86");
    const char* order[]={"VPhysics030","VPhysicsCollision007","VPhysicsSurfaceProps001"};
    auto r=s::ReadOnlyThree(nullptr);
    if(!s::Consistent(r)||r.outcome!=s::Outcome::MissingFactory||!calls.empty())
        return Fail(1);
    for(unsigned stop=1;stop<=4;++stop){
        failAt=stop;calls.clear();badStatus=false;
        r=s::ReadOnlyThree(&Factory);
        const unsigned expected=stop==4?3:stop;
        if(!s::Consistent(r)||r.attempted!=expected||
           r.found!=(stop==4?3:stop-1)||
           r.successful()!=(stop==4)||badStatus||
           calls.size()!=expected)return Fail(2+stop);
        for(unsigned i=0;i<expected;++i)
            if(calls[i]!=order[i])return Fail(10+stop);
        if(stop==2&&r.outcome!=s::Outcome::MissingCollision)
            return Fail(20);
        if(stop==3&&r.outcome!=s::Outcome::MissingSurfaceProps)
            return Fail(21);
    }
    // Negative evidence must never let fabricated success through.
    r.attempted=2;
    if(s::Consistent(r)||r.successful())return Fail(22);
    std::puts("PASS original 2006 server physics factory: 3 source-order x86 cdecl probes, all early exits, null status, no retained pointers or gameplay");
    return 0;
}
