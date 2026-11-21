#include "sin_compat/original_grunt_full_ai_postinit.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <vector>

namespace ns=sin_compat::original_druglab_grunt;
namespace ai=ns::ai_init;
namespace be=ns::behavior_postinit;
namespace full=ns::full_ai_postinit;

struct Host final : ai::IHost, be::IHost {
    int ai_calls=0;
    int behavior_calls=0;
    bool fail_core=false;
    bool fail_behavior=false;
    full::Initializer* reenter=nullptr;
    full::Code reentrant_status=full::Code::Success;
    bool construct_and_store(ai::ComponentSpec) override {
        ++ai_calls;
        return !(fail_core && ai_calls==3);
    }
    bool wire_embedded_owner_relay() override {++ai_calls;return true;}
    bool wire_motor_to_local_navigator() override {++ai_calls;return true;}
    bool wire_local_navigator_to_navigator() override {++ai_calls;return true;}
    bool wire_navigator_shared_context() override {++ai_calls;return true;}
    bool wire_pathfinder_shared_context() override {++ai_calls;return true;}
    bool wire_tactical_services_shared_context() override {++ai_calls;return true;}
    bool append_to_collection(be::Spec,std::uint32_t,std::uint32_t) override {
        ++behavior_calls;
        if(reenter && behavior_calls==4)
            reentrant_status=reenter->initialize().code;
        return !(fail_behavior && behavior_calls==7);
    }
    bool bind_to_npc(be::Spec) override {++behavior_calls;return true;}
    bool set_owner_subobject_reference(be::Spec,std::uint32_t,std::uint32_t) override {
        ++behavior_calls;return true;
    }
    bool initialize_sentence_parameters(std::uint32_t,std::string_view) override {
        ++behavior_calls;return true;
    }
};

int main() {
    {
        Host h;
        ai::Initializer core(h);be::Registrar beh(h);full::Initializer pipeline(core,beh);
        const auto r=pipeline.initialize();
        assert(r.ok() && r.core.created_count==7);
        assert(r.behavior.registered_count==8);
        assert(h.ai_calls==13 && h.behavior_calls==25);
    }
    {
        Host h;h.fail_core=true;
        ai::Initializer core(h);be::Registrar beh(h);full::Initializer pipeline(core,beh);
        const auto r=pipeline.initialize();
        assert(!r.ok() && r.failed_stage==full::Stage::CoreSubsystems);
        assert(r.core.created_count==2 && h.ai_calls==3 && h.behavior_calls==0);
    }
    {
        Host h;h.fail_behavior=true;
        ai::Initializer core(h);be::Registrar beh(h);full::Initializer pipeline(core,beh);
        const auto r=pipeline.initialize();
        assert(!r.ok() && r.failed_stage==full::Stage::EmbeddedBehaviorsAndSentences);
        assert(r.core.created_count==7 && r.behavior.registered_count==2);
        assert(h.ai_calls==13 && h.behavior_calls==7);
    }
    {
        Host h;
        ai::Initializer core(h);be::Registrar beh(h);full::Initializer pipeline(core,beh);
        h.reenter=&pipeline;
        const auto r=pipeline.initialize();
        assert(r.ok() && h.reentrant_status==full::Code::Reentrant);
    }
    std::puts("PASS original Grunt post-init: 7 core dependencies before 8 behaviors and sentence key; failure gates");
}