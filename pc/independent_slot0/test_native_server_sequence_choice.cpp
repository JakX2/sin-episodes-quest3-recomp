#include "native_server_sequence_choice.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <limits>
#include <vector>
#include <cstdio>
using namespace sin_port::native_weapon_tables;
struct Draw {int lo,hi,index,value;};
struct F {std::vector<Draw> draws;unsigned used=0;};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto get=reinterpret_cast<GetServerSequenceChoiceApi>(GetProcAddress(server,server_sequence_choice_export));
    assert(get&&!GetProcAddress(client,server_sequence_choice_export)&&!get(0,sizeof(ServerSequenceChoiceApi))&&!get(1,0));
    auto* api=get(1,sizeof(ServerSequenceChoiceApi));assert(api);
    ServerSequenceChoiceEntry e[]={{4,0},{5,100},{4,2},{4,-5},{4,5},{4,0}};
    int out=99;F f;
    ServerSequenceChoiceBindings b{&f,[](void* p,int lo,int hi,int i,int* out) noexcept -> int{
        auto& f=*static_cast<F*>(p);assert(f.used<f.draws.size());const auto d=f.draws[f.used++];
        assert(lo==d.lo&&hi==d.hi&&i==d.index);*out=d.value;return 1;}};
    assert(api->choose(nullptr,0,4,-1,true,nullptr,&out)&&out==0);
    assert(api->choose(e,0,4,-1,false,nullptr,&out)&&out==-1);
    assert(api->choose(e,6,4,-1,false,nullptr,&out)&&out==3);
    assert(api->choose(e,6,77,-1,true,nullptr,&out)&&out==-1);
    f.draws={{0,-1,0,123},{0,1,2,1},{0,6,3,4},{0,11,4,7},{0,11,5,0}};
    assert(api->choose(e,6,4,-1,true,&b,&out)&&out==3&&f.used==5);
    f.used=0;f.draws={{0,-1,0,123},{0,1,2,0}};
    assert(api->choose(e,6,4,3,true,&b,&out)&&out==3&&f.used==2);
    f.used=0;f.draws={{0,-1,0,123},{0,1,2,1},{0,6,3,6},{0,11,4,0},{0,11,5,0}};
    assert(api->choose(e,6,4,4,true,&b,&out)&&out==4&&f.used==5);
    ServerSequenceChoiceEntry zeros[]={{1,0},{1,0}};
    f.used=0;f.draws={{0,-1,0,0},{0,-1,1,-1}};
    assert(api->choose(zeros,2,1,-1,true,&b,&out)&&out==1&&f.used==2);
    assert(api->choose(zeros,2,1,-1,false,nullptr,&out)&&out==-1);
    ServerSequenceChoiceEntry bad[]={{1,std::numeric_limits<int>::min()}};
    assert(!api->choose(bad,1,1,-1,false,nullptr,&out)&&out==-1);
    assert(!api->choose(e,6,4,-1,true,nullptr,&out)&&out==-1);
    FreeLibrary(server);FreeLibrary(client);
    std::puts("PASS original server sequence selection weights, current negative priority and RNG call order");
}
