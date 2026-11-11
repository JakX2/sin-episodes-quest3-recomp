#include "sin_compat/original_client_postinit.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace abi=sin_compat::original_client_postinit;
struct AppHost {
    std::vector<std::uintptr_t> planned;
    std::vector<std::uintptr_t> released;
    std::size_t index=0;
    std::uintptr_t create(abi::AppNode&) {
        assert(index<planned.size()); return planned[index++];
    }
    void cleanup(abi::AppNode&,std::uintptr_t value) { released.push_back(value); }
};
struct PhysicsHost {
    std::vector<std::uintptr_t> planned;
    std::size_t pos=0;
    std::vector<std::string> names;
    std::vector<std::pair<std::uint32_t,std::uintptr_t>> writes;
    std::vector<std::pair<std::uintptr_t,std::uintptr_t>> coupling;
    std::uintptr_t query(const char* name){
        names.emplace_back(name);
        assert(pos<planned.size());return planned[pos++];
    }
    void publish(std::uint32_t va,std::uintptr_t value){writes.emplace_back(va,value);}
    void after_all(std::uintptr_t s,std::uintptr_t existing){coupling.emplace_back(s,existing);}
};
void app_system_tests(){
    assert(sizeof(abi::NativeAppSystemNode32)==16);
    assert(abi::kAppSystemHeadVA==0x24369374u);
    std::uintptr_t a=0,b=0,c=0;
    abi::AppNode third{&c,nullptr};
    abi::AppNode second{&b,&third};
    abi::AppNode first{&a,&second};
    AppHost good; good.planned={10,20,30};
    assert(abi::original_initialize_app_systems(&first,good));
    assert(a==10&&b==20&&c==30&&good.index==3);
    abi::original_cleanup_app_systems(&first,good);
    assert(a==0&&b==0&&c==0);
    assert((good.released==std::vector<std::uintptr_t>{10,20,30}));
    abi::original_cleanup_app_systems(&first,good);
    assert(good.released.size()==3);
    for(std::size_t failure=0;failure<3;++failure){
        a=b=c=0; AppHost host; host.planned={10,20,30};host.planned[failure]=0;
        assert(!abi::original_initialize_app_systems(&first,host));
        assert(host.index==failure+1);
        assert(a==0&&b==0&&c==0);
        assert(host.released.size()==failure);
        for(std::size_t i=0;i<failure;++i)assert(host.released[i]==10*(i+1));
    }
    // Native callback failure with null output is optional, not fatal.
    a=b=0;abi::AppNode optional{nullptr,&second};first.next=&optional;
    AppHost skipped; skipped.planned={1,0,2,3};
    assert(abi::original_initialize_app_systems(&first,skipped));
    assert(a==1&&b==2&&c==3&&skipped.index==4);
    abi::original_cleanup_app_systems(&first,skipped);
    assert((skipped.released==std::vector<std::uintptr_t>{1,2,3}));
    // A successful native callback with null output would dereference null;
    // host-neutral test rejects it explicitly, rather than pretending safe.
    AppHost invalid; invalid.planned={9};
    bool rejected=false;
    try { abi::original_initialize_app_systems(&optional,invalid); }
    catch(const std::invalid_argument&){ rejected=true; }
    assert(rejected);
    first.next=&second;
    AppHost no_nodes;
    assert(abi::original_initialize_app_systems(nullptr,no_nodes));
    abi::original_cleanup_app_systems(nullptr,no_nodes);
}
void physics_tests(){
    assert(abi::kPhysicsVA==0x24374398u);
    assert(abi::kPhysicsSurfacePropsVA==0x243743A8u);
    assert(abi::kPhysicsCollisionVA==0x243743A0u);
    const std::vector<std::string> expected{"VPhysics030","VPhysicsSurfaceProps001","VPhysicsCollision007"};
    PhysicsHost ok; ok.planned={111,222,333};
    assert(abi::original_initialize_physics(ok,0xdeadbeefu));
    assert(ok.names==expected && ok.writes.size()==3);
    assert(ok.writes[0]==std::make_pair(abi::kPhysicsVA,std::uintptr_t{111}));
    assert(ok.writes[1]==std::make_pair(abi::kPhysicsSurfacePropsVA,std::uintptr_t{222}));
    assert(ok.writes[2]==std::make_pair(abi::kPhysicsCollisionVA,std::uintptr_t{333}));
    assert(ok.coupling.size()==1&&ok.coupling[0]==std::make_pair(std::uintptr_t{222},std::uintptr_t{0xdeadbeef}));
    for(std::size_t fail=0;fail<3;++fail){
        PhysicsHost host; host.planned={111,222,333}; host.planned[fail]=0;
        assert(!abi::original_initialize_physics(host,0x10));
        assert(host.pos==fail+1 && host.writes.size()==fail+1);
        assert(host.writes[fail].second==0 && host.coupling.empty());
        for(std::size_t i=0;i<=fail;++i) assert(host.names[i]==expected[i]);
    }
}
int main(){app_system_tests();physics_tests();}