#include "client_original_gamesystem_sequence.h"
#include <cstdio>
#include <cstring>
#include <set>
using namespace sin_independent::gamesystem_sequence;
int main(){
    static_assert(MatchesVerifiedCalls(),"Original source order");
    static_assert(ReconstructedCount()==8,"Eight finished");
    static_assert(PendingCount()==4,"Four missing");
    std::set<std::uint32_t> objects,methods;
    for(const auto& row:kOriginalOrder){
        if(!row.class_name || !row.class_name[0] ||
           !objects.insert(row.registered_subobject_va).second ||
           !methods.insert(row.original_init_va).second)return 1;
        if(row.original_vtable_va<0x24276000u ||
           row.original_object_va<0x242F3000u)return 2;
    }
    const auto& leaf=kOriginalOrder[2];
    if(std::strcmp(leaf.class_name,"CClientLeafSystem") ||
       leaf.registered_subobject_va-leaf.original_object_va!=4)return 3;
    const auto& lazy=kOriginalOrder[5];
    if(lazy.original_object_va!=0x2433FD9Cu ||
       sin_independent::auto_gamesystem::kOriginalDirectRegistrations[5].source!=
           sin_independent::auto_gamesystem::DirectSource::LazySingletonReturn)
        return 4;
    if(kOriginalOrder[6].init_body_reconstructed ||
       kOriginalOrder[7].init_body_reconstructed ||
       kOriginalOrder[10].init_body_reconstructed ||
       kOriginalOrder[11].init_body_reconstructed)return 5;
    std::puts("PASS all 12 original RTTI systems, registration order, secondary subobject and 7/5 Init status");
    return 0;
}
