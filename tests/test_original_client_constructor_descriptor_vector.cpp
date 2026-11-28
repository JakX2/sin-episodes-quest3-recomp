#include "sin_compat/original_client_constructor_descriptor_vector.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>

namespace dv=sin_compat::original_client_constructor_descriptor_vector;
int main() {
    const dv::Descriptor a{{2u,0x10000220u,0x1000022cu}};
    const dv::Descriptor b{{2u,0x1000025cu,0x10000268u}};
    const dv::Descriptor c{{9u,0x1000066cu,0x1000067cu}};
    dv::State state;
    auto r=dv::Insert(state,0,a);
    assert(r && r->index==0 && r->grew && !r->shifted);
    assert(state.capacity==3 && state.cacheRefreshed);
    r=dv::Insert(state,1,b);
    assert(r && !r->grew && !r->shifted);
    r=dv::Insert(state,1,c);
    assert(r && !r->grew && r->shifted);
    assert(state.records[0]==a && state.records[1]==c && state.records[2]==b);
    r=dv::Insert(state,0,state.records[2]);
    assert(r && r->grew && r->shifted && state.capacity==6);
    assert(state.records[0]==b && state.records[1]==a && state.records[2]==c && state.records[3]==b);
    assert(dv::PlanCapacity(0,1,7)==3);
    assert(dv::PlanCapacity(0,4,7)==10);
    assert(dv::PlanCapacity(3,4,7)==10);
    assert(dv::PlanCapacity(3,7,0)==12);
    assert(dv::PlanCapacity(3,3,-1)==3);
    assert(!dv::PlanCapacity(3,4,-1));
    assert(!dv::PlanCapacity(-1,4,0));
    assert(!dv::PlanCapacity(3,4,-2));
    assert(!dv::PlanCapacity(3,dv::kMaxRecords+1,0));
    assert(!dv::PlanCapacity(dv::kMaxRecords-1,dv::kMaxRecords,0));
    assert(!dv::PlanCapacity(3,4,0x7fffffff));
    dv::State external{2,-1,{a},false};
    r=dv::Insert(external,1,b);
    assert(r && !r->grew && external.cacheRefreshed);
    const auto before=external.records;
    external.cacheRefreshed=false;
    assert(!dv::Insert(external,2,c));
    assert(!dv::Insert(external,-1,c));
    assert(!dv::Insert(external,3,c));
    assert(external.records==before && external.capacity==2 && !external.cacheRefreshed);
    dv::State malformed{0,0,{a},false};
    assert(!dv::Insert(malformed,0,b));
    assert(malformed.records.size()==1 && malformed.records[0]==a);
    std::puts("PASS Phase1727 12-byte constructor descriptor insertion and growth");
}
