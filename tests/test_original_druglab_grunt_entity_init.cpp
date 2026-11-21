#include "sin_compat/original_druglab_grunt_entity_init.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace sin_compat::original_druglab_grunt::entity_init;
struct NativeHost final:IHost {
    mutable std::vector<std::string> calls;
    bool edict_live=false,create_edt=true;
    bool all_good=true,attached_from_existing=false;
    Stage refuse=Stage::None,throw_at=Stage::None;
    int clear_calls=0,backlink_calls=0;
    EntityInitializer* reenter=nullptr;
    Code nested_code=Code::Applied;
    bool accept(Stage stage,std::string_view msg) {
        calls.emplace_back(msg);
        if(throw_at==stage)throw std::runtime_error("fixture host failure");
        return stage!=refuse;
    }
    bool intern_native_classname(std::string_view name) override {
        assert(name=="npc_druglab_grunt_pistol"||name.empty());
        return accept(Stage::ClassnameSymbol,"intern");
    }
    bool special_bit9_native_registration() override {
        return accept(Stage::SpecialBit9,"special-bit9");
    }
    bool attach_native_networkable(bool context_present) override {
        if(!accept(Stage::AttachNetworkable,"attach"))return false;
        attached_from_existing=context_present;
        edict_live=context_present||create_edt;
        if(reenter)nested_code=reenter->initialize({
            true,"npc_druglab_grunt_pistol",0,true}).code;
        return edict_live;
    }
    bool clear_original_factory_edict_context() override {
        ++clear_calls;
        return accept(Stage::ClearEdictContext,"clear-context");
    }
    bool has_live_native_edict() const override {
        calls.emplace_back("read-native-edict");
        return edict_live;
    }
    bool link_edict_networkable_backpointer() override {
        assert(edict_live);
        ++backlink_calls;
        return accept(Stage::BacklinkEdict,"edict-backlink");
    }
    bool base_initialize_2214BF30_zero() override {
        return accept(Stage::BaseInitBF30,"base-bf30");
    }
    bool base_initialize_2214B280() override {
        return accept(Stage::BaseInitB280,"base-b280");
    }
    bool invoke_real_grunt_postinit_slot3F8() override {
        return accept(Stage::GruntPostInit3F8,"grunt-postinit");
    }
};
void test_existing_factory_edict_reused_before_postinit() {
    NativeHost host;
    EntityInitializer runtime(host);
    auto result=runtime.initialize({
        true,"npc_druglab_grunt_pistol",0, true
    });
    assert(result.ok()&&result.attachment_invoked);
    assert(!result.special_bit9&&!result.skipped_edict_attachment);
    assert(result.edict_backlink_written);
    assert(host.attached_from_existing);
    assert(host.clear_calls==1&&host.backlink_calls==1);
    assert((host.calls==std::vector<std::string>{
        "intern","attach","clear-context","read-native-edict",
        "edict-backlink","base-bf30","base-b280","grunt-postinit"
    }));
}
void test_null_context_allocates_new_native_edict() {
    NativeHost host;
    EntityInitializer runtime(host);
    auto result=runtime.initialize({
        true,"npc_druglab_grunt_pistol",0,false
    });
    assert(result.ok());
    assert(host.edict_live&&!host.attached_from_existing);
    assert(host.clear_calls==1&&host.backlink_calls==1);
}
void test_flag_bit9_branch_precedes_bit10_and_edict_attach() {
    for(std::uint32_t flags:{0x200u,0x600u}) {
        NativeHost host;
        EntityInitializer runtime(host);
        const auto result=runtime.initialize({
            true,"npc_druglab_grunt_pistol",flags,true
        });
        assert(result.ok()&&result.special_bit9 &&
               result.skipped_edict_attachment&&!result.attachment_invoked);
        assert(host.clear_calls==0&&host.backlink_calls==0);
        assert((host.calls==std::vector<std::string>{
            "intern","special-bit9","base-bf30","base-b280","grunt-postinit"
        }));
    }
}
void test_bit10_skips_attach_but_can_link_existing_edict() {
    for(bool edict:{false,true}) {
        NativeHost host;
        host.edict_live=edict;
        EntityInitializer runtime(host);
        const auto result=runtime.initialize({
            true,"npc_druglab_grunt_pistol",0x400u,true
        });
        assert(result.ok()&&result.skipped_edict_attachment);
        assert(!result.attachment_invoked);
        assert(host.clear_calls==0);
        assert(host.backlink_calls==(edict?1:0));
        assert((host.calls==(
            edict?std::vector<std::string>{
                "intern","read-native-edict","edict-backlink",
                "base-bf30","base-b280","grunt-postinit"
            }:std::vector<std::string>{
                "intern","read-native-edict","base-bf30",
                "base-b280","grunt-postinit"
            }
        )));
    }
}
void test_null_classname_does_not_intern() {
    NativeHost host;
    EntityInitializer runtime(host);
    const auto result=runtime.initialize({
        false,"not-read",0,false
    });
    assert(result.ok());
    assert(host.calls[0]=="attach");
}
void test_every_failure_stage_refused_without_fake_success() {
    constexpr Stage stages[]{
        Stage::ClassnameSymbol,Stage::SpecialBit9,Stage::AttachNetworkable,
        Stage::ClearEdictContext,Stage::BacklinkEdict,
        Stage::BaseInitBF30,Stage::BaseInitB280,Stage::GruntPostInit3F8
    };
    for(const auto fail_stage:stages) {
        NativeHost host;
        host.refuse=fail_stage;
        EntityInitializer runtime(host);
        const auto mask=fail_stage==Stage::SpecialBit9?0x200u:0u;
        const auto r=runtime.initialize({
            true,"npc_druglab_grunt_pistol",mask,true
        });
        assert(r.code==Code::HostRefused&&r.failed_stage==fail_stage);
        // If an actual native attach failed, the independent port
        // still clears the temporary global edict context.
        if(fail_stage==Stage::AttachNetworkable) {
            assert(host.clear_calls==1);
            assert(host.calls.back()=="clear-context");
        }
    }
}
void test_reentrant_edict_binding_callback_refused() {
    NativeHost host;
    EntityInitializer runtime(host);
    host.reenter=&runtime;
    const auto r=runtime.initialize({
        true,"npc_druglab_grunt_pistol",0,true
    });
    assert(r.ok()&&host.nested_code==Code::Reentrant);
    assert(host.clear_calls==1&&host.backlink_calls==1);
}
void test_throw_refuses_and_clears_edict_context() {
    NativeHost host;
    host.throw_at=Stage::AttachNetworkable;
    EntityInitializer runtime(host);
    auto r=runtime.initialize({
        true,"npc_druglab_grunt_pistol",0,true
    });
    assert(r.code==Code::HostRefused&&r.failed_stage==Stage::AttachNetworkable);
    assert(host.clear_calls==1&&host.calls.back()=="clear-context");
}
int main(){
    test_existing_factory_edict_reused_before_postinit();
    test_null_context_allocates_new_native_edict();
    test_flag_bit9_branch_precedes_bit10_and_edict_attach();
    test_bit10_skips_attach_but_can_link_existing_edict();
    test_null_classname_does_not_intern();
    test_every_failure_stage_refused_without_fake_success();
    test_reentrant_edict_binding_callback_refused();
    test_throw_refuses_and_clears_edict_context();
    std::puts("PASS original CDruglabGrunt +0x70 entity initiation: classname, source bit9/bit10, edict bind+backpointer, postinitialize +0x3f8 (synthetic host)");
}
