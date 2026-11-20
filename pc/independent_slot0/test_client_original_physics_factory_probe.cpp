// Native x86 isolated CClientDLL 2006 three-physics-factory probe.
// The SOURCE helper 0x24182C60 is 97B; it queries the *second* Init
// factory (physics) three times and does not call the later coupling
// helper until all three are found. No original engine objects used.
#include "client_original_physics_factory_probe.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
namespace p=sin_independent::physics_factory_probe;
struct Recorded {
    int unavailable=-1;
    int callbacks=0;
    bool bad_null_status=false;
    bool bad_name_order=false;
    int simulated[3]={1,2,3};
    std::vector<std::string> names{};
};
static Recorded* active=nullptr;
static void* __cdecl Query(const char* name,int* status) {
    if(!active)return nullptr;
    const int index=active->callbacks++;
    active->names.emplace_back(name?name:"");
    if(status)active->bad_null_status=true;
    if(index>2 ||!name ||std::strcmp(name,p::kOriginalNames[index])!=0)
        active->bad_name_order=true;
    if(index==active->unavailable||index>2)return nullptr;
    return &active->simulated[index];
}
static int Fail(int n){
    std::fprintf(stderr,"original physics factory triple fixture %d\n",n);
    return n;
}
static bool Validate(const Recorded& r,int expected) {
    if(r.callbacks!=expected||r.bad_null_status||r.bad_name_order||
       int(r.names.size())!=expected)return false;
    for(int i=0;i<expected;++i)
        if(r.names[i]!=p::kOriginalNames[i])return false;
    return true;
}
int main(){
    static_assert(sizeof(void*)==4,"original factory is x86");
    static_assert(p::kOriginalPhysicsHelperVA==0x24182C60u);
    static_assert(p::kOriginalPhysicsGlobalVA==0x24374398u);
    static_assert(p::kOriginalSurfaceGlobalVA==0x243743A8u);
    static_assert(p::kOriginalCollisionGlobalVA==0x243743A0u);
    Recorded fake{};
    active=&fake;
    const auto absent=p::ReadOnlyQueryThree(nullptr);
    if(absent.outcome!=p::Outcome::MissingFactory ||
       !p::Consistent(absent)||fake.callbacks!=0)return Fail(1);
    const auto full=p::ReadOnlyQueryThree(&Query);
    if(full.outcome!=p::Outcome::AllPresent ||
       !full.would_pass_original_three_query_gate ||
       full.attempted!=3 ||full.found!=3 ||
       !p::Consistent(full)||!Validate(fake,3))return Fail(2);
    // Original aborts immediately at first missing pointer,
    // before invoking any later factory callback or the unknown
    // physics-coupling callback 0x241927A0.
    for(int missing=0;missing<3;++missing){
        fake=Recorded{};
        fake.unavailable=missing;
        const auto result=p::ReadOnlyQueryThree(&Query);
        const auto status=missing==0?p::Outcome::FirstUnavailable:
                          missing==1?p::Outcome::SecondUnavailable:
                                     p::Outcome::ThirdUnavailable;
        if(result.outcome!=status ||result.attempted!=unsigned(missing+1) ||
           result.found!=unsigned(missing) ||
           result.would_pass_original_three_query_gate ||
           !p::Consistent(result)||!Validate(fake,missing+1))
            return Fail(3+missing);
    }
    fake=Recorded{};
    fake.unavailable=-1;
    auto again=p::ReadOnlyQueryThree(&Query);
    if(again.outcome!=p::Outcome::AllPresent ||
       !Validate(fake,3) ||!p::Consistent(again))
        return Fail(6);
    // Reject inconsistent encoded proof and source-unknown enum,
    // so owner transcript parsing can never promote fabricated
    // "all three" with two actual provider calls.
    again.found=2;
    if(p::Consistent(again))return Fail(7);
    again.found=3;
    again.outcome=static_cast<p::Outcome>(77);
    if(p::Consistent(again))return Fail(8);
    active=nullptr;
    std::puts("PASS original 2006 97B client physics factory: exact MSVC x86 cdecl 3 queries, null error pointers, early refusals, no physics coupling invocation");
    return 0;
}
