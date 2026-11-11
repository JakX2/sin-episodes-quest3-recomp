#include "sin_compat/original_dll_engine_interface_objects.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>
namespace a = sin_compat::original_dll_engine_interface_objects;

int main() {
    using a::Module;
    static_assert(sizeof(a::NativeObject32)==4);
    static_assert(sizeof(a::NativeMSVCCompleteObjectLocator32)==20);
    static_assert(a::kOriginalInterfaceFactories.size()==9);
    assert(a::find_factory(Module::Client,"ServerGameDLL003")==nullptr);
    assert(a::find_factory(Module::Server,"VClient011")==nullptr);
    assert(a::find_factory(Module::Server,"servergamedll003")==nullptr);
    assert(a::find_factory(Module::Server,nullptr)==nullptr);

    std::size_t server=0,client=0,static_count=0,dynamic_count=0,slots=0;
    for (const auto& pin:a::kOriginalInterfaceFactories) {
        if(pin.module==Module::Server)++server;else ++client;
        assert(a::find_factory(pin.module,pin.name)==&pin);
        assert(a::original_factory_return_va(pin)==pin.original_object_va);
        if(pin.pe_initialized_vtable_va==0) {
            ++dynamic_count;
            assert(pin.verified_executable_prefix_slots==0);
            bool threw=false;
            try { (void)a::original_observed_method_va(pin,{0x12345678},0); }
            catch(const std::out_of_range&) { threw=true; }
            assert(threw);
        }else {
            ++static_count;
            slots+=pin.verified_executable_prefix_slots;
            std::vector<std::uint32_t> method_vas(pin.verified_executable_prefix_slots);
            for(std::size_t i=0;i<method_vas.size();++i)method_vas[i]=0x22000000u+static_cast<std::uint32_t>(i*16);
            assert(a::original_observed_method_va(pin,method_vas,0)==0x22000000u);
            assert(a::original_observed_method_va(pin,method_vas,method_vas.size()-1)==method_vas.back());
            for(std::size_t bad:{method_vas.size(),method_vas.size()+1u}) {
                bool threw=false;
                try { (void)a::original_observed_method_va(pin,method_vas,bad); }
                catch(const std::out_of_range&) { threw=true; }
                assert(threw);
            }
            bool threw=false;
            try { (void)a::original_observed_method_va(pin,{},0); }
            catch(const std::out_of_range&) { threw=true; }
            assert(threw);
        }
    }
    assert(server==4 && client==5 && static_count==5 && dynamic_count==4);
    assert(slots==105);
    const auto* game=a::find_factory(Module::Server,"ServerGameDLL003");
    assert(game && game->original_object_va==0x22510650u && game->verified_executable_prefix_slots==31);
    const auto* ents=a::find_factory(Module::Server,"ServerGameEnts001");
    const auto* clients=a::find_factory(Module::Server,"ServerGameClients003");
    assert(ents && clients && ents->original_object_va+4u==clients->original_object_va);
    const auto* vclient=a::find_factory(Module::Client,"VClient011");
    assert(vclient && vclient->original_object_va==0x242FA4B8u && vclient->verified_executable_prefix_slots==48);
    const auto* gm1=a::find_factory(Module::Server,"GameMovement001");
    const auto* gm2=a::find_factory(Module::Client,"GameMovement001");
    assert(gm1 && gm2 && gm1->original_object_va!=gm2->original_object_va);
}