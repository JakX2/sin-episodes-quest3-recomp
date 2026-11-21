// Original-free integrated original-2006 NPC factory then Spawn sequence.
// No game binaries, native CBaseNPC allocations or actual Source engine.
#include "sin_compat/original_druglab_grunt_factory.hpp"
#include "sin_compat/original_druglab_grunt_spawn.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace grunt=sin_compat::original_druglab_grunt;
namespace spawn=sin_compat::original_druglab_grunt::spawn;

struct EngineStub final:grunt::IHost,spawn::IHost {
    std::vector<std::string> observed;
    bool base_ctor_ok=true;
    std::uintptr_t object=0x400000;
    std::string_view selected_weapon{};
    std::int32_t selected_mode=-1,field98=-1,field9c=-1;
    bool allocate_called=false;
    bool native_virtual70_called=false;
    std::uintptr_t allocate_native_object(std::size_t n) override {
        assert(n==0xFD8);observed.emplace_back("alloc");
        allocate_called=true;return object;
    }
    bool invoke_verified_base_constructor(std::uintptr_t p) override {
        assert(p==object);observed.emplace_back("ctor");
        return base_ctor_ok;
    }
    bool establish_native_vtable(std::uintptr_t p,
                                  std::uint32_t o,
                                  grunt::VptrRole role) override {
        assert(p==object);
        if(o==0)assert(role==grunt::VptrRole::Primary);
        else if(o==0x790)assert(role==grunt::VptrRole::Secondary790);
        else if(o==0xC14)assert(role==grunt::VptrRole::TertiaryC14);
        else assert(false);
        observed.emplace_back("vtable");
        return true;
    }
    bool invoke_native_virtual_70(std::uintptr_t p,
                                   std::uintptr_t arg) override {
        assert(p==object&&arg==0x42);
        observed.emplace_back("factory-virtual70");
        native_virtual70_called=true;
        return true;
    }
    bool reclaim_unpublished_object(std::uintptr_t p) override {
        assert(p==object);observed.emplace_back("factory-rollback");
        return true;
    }
    bool invoke_delegate_virtual_18(std::uintptr_t) override {
        observed.emplace_back("factory-delegate");return true;
    }
    bool assign_original_weapon_symbol(std::string_view n) override {
        assert(native_virtual70_called);
        selected_weapon=n;observed.emplace_back("spawn-weapon");
        return true;
    }
    bool commit_original_field_3B0(std::int32_t n,spawn::DirtyPath d) override {
        assert(native_virtual70_called);
        assert(d==spawn::DirtyPath::EdictStateBit0);
        selected_mode=n;observed.emplace_back("spawn-mode");
        return true;
    }
    bool native_virtual_68() override {
        observed.emplace_back("spawn-v68");return true;
    }
    bool native_virtual_1C_then_6C() override {
        observed.emplace_back("spawn-v1c-v6c");return true;
    }
    bool inherited_npc_spawn_2235F2A0() override {
        observed.emplace_back("spawn-inherited");return true;
    }
    bool virtual_1B0_then_write_field_98(std::int32_t n) override {
        field98=n;observed.emplace_back("spawn-field98");return true;
    }
    bool virtual_1B4_then_write_field_9C(std::int32_t n) override {
        field9c=n;observed.emplace_back("spawn-field9c");return true;
    }
};
int main(){
    for(const auto& test:std::vector<std::pair<std::string_view,std::string_view>>{
        {"npc_druglab_grunt_pistol","weapon_npc_pistol"},
        {"npc_druglab_grunt_scattergun","weapon_scattergun"},
        {"npc_druglab_grunt_assault_rifle","weapon_assault_rifle"}
    }){
        EngineStub h;
        grunt::GruntFactoryReference factory(h);
        spawn::SpawnReference spawn_runtime(h);
        const auto ctor=factory.create(test.first,0x42);
        assert(ctor.ok()&&ctor.factory_return_view==h.object+0x160);
        const auto result=spawn_runtime.apply({
            test.first,"wrong_weapon",99,-1,-1,false,true,4.9f});
        assert(result.ok()&&h.selected_weapon==test.second);
        assert(h.selected_mode==result.plan.desired_field_3B0);
        assert(h.field98==4&&h.field9c==4);
        assert((h.observed==std::vector<std::string>{
            "alloc","ctor","vtable","vtable","vtable","factory-virtual70",
            "spawn-weapon","spawn-mode","spawn-v68","spawn-v1c-v6c",
            "spawn-inherited","spawn-field98","spawn-field9c"
        }));
    }
    {
        EngineStub h;
        h.base_ctor_ok=false;
        grunt::GruntFactoryReference factory(h);
        spawn::SpawnReference spawn_runtime(h);
        assert(factory.create("npc_druglab_grunt_pistol",0x42).code==
               grunt::Code::HostRejected);
        assert(h.observed.back()=="factory-rollback");
        assert(!h.native_virtual70_called);
        // Native game adapter MUST NOT invoke Spawn on a rejected factory.
        assert(h.selected_weapon.empty()&&h.selected_mode==-1);
    }
    std::puts("PASS independent original Grunt 3 class factories initialize native object first, then class-based weapon Spawn and per-global fields (synthetic engine)");
}
