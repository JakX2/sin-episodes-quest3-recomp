#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>
#include "sin_compat/original_server_post_emitter.hpp"
namespace native = sin_compat::original_server_post_emitter;
struct MockHost {
    std::vector<std::string> operations;
    std::uintptr_t fail_system = 0;
    bool repo_a = true, repo_b = true;
    void duplicate_system(native::SystemNode& node) {
        operations.push_back("duplicate:"+std::to_string(node.identity));
    }
    bool init_system(native::SystemNode& node) {
        operations.push_back("init:"+std::to_string(node.identity));
        return node.identity != fail_system;
    }
    bool init_repository_a(std::uint32_t object,std::uint32_t impl) {
        assert(object==native::kRepositoryAObjectVA && impl==native::kRepositoryAImplVA);
        operations.push_back("repoA");return repo_a;
    }
    bool init_repository_b(std::uint32_t object,std::uint32_t impl) {
        assert(object==native::kRepositoryBObjectVA && impl==native::kRepositoryBImplVA);
        operations.push_back("repoB");return repo_b;
    }
    void register_cvar_descriptor(std::uint32_t va) {
        assert(va==native::kDeveloperCvarDescriptorVA);operations.push_back("cvar-register");
    }
    std::uintptr_t query_cvar(const char* n) {
        assert(std::string(n)=="developer");operations.push_back("cvar-query");return 0;
    }
    void publish_cvar(std::uint32_t va,std::uintptr_t result) {
        assert(va==native::kDeveloperCvarResultVA && result==0);
        operations.push_back("cvar-publish-null");
    }
    void forward_registry_count(std::uint32_t array,std::uint32_t count,
                                std::uint32_t method,std::uint32_t raw) {
        assert(array==native::kSystemArrayVA && count==native::kSystemCountVA);
        assert(method==0x221F54F0u && raw==0xC0FFEEu);
        operations.push_back("registry-forward");
    }
};
int main() {
    static_assert(native::kGameSystemsInitVA==0x221F5700u);
    assert(std::string(native::kGameEventsPath)=="resource/gameevents.res");
    assert(std::string(native::kModEventsPath)=="resource/ModEvents.res");
    // Empty pending vector succeeds and still raises original init flag.
    native::Registry empty;
    MockHost h;
    assert(native::run_three_required_gates(empty,h));
    assert(empty.pending == nullptr && empty.init_flag);
    assert((h.operations==std::vector<std::string>{"repoA","repoB"}));

    // Pointer identity dedupe, pending head consumed before all callbacks.
    native::SystemNode a{100u},b{200u},dup{100u};a.next=&b;b.next=&dup;
    native::Registry registry;registry.pending=&a;
    MockHost normal;
    assert(native::run_three_required_gates(registry,normal));
    assert(registry.init_flag && registry.pending==nullptr);
    assert(registry.installed.size()==2);
    assert((normal.operations==std::vector<std::string>{"duplicate:100","init:100","init:200","repoA","repoB"}));
    // Native flag stays set even if a virtual Init call fails.
    native::Registry fail;fail.installed={&a,&b};
    MockHost early;early.fail_system=100;
    assert(!native::run_three_required_gates(fail,early));
    assert(fail.init_flag && fail.pending==nullptr);
    assert((early.operations==std::vector<std::string>{"init:100"}));
    native::Registry second;second.installed={&a,&b};
    MockHost failLastSystem;failLastSystem.fail_system=200;
    assert(!native::run_three_required_gates(second,failLastSystem));
    assert((failLastSystem.operations==std::vector<std::string>{"init:100","init:200"}));
    native::Registry failureA;failureA.installed={&a};
    MockHost noA;noA.repo_a=false;
    assert(!native::run_three_required_gates(failureA,noA));
    assert((noA.operations==std::vector<std::string>{"init:100","repoA"}));
    native::Registry failureB;failureB.installed={&a};
    MockHost noB;noB.repo_b=false;
    assert(!native::run_three_required_gates(failureB,noB));
    assert((noB.operations==std::vector<std::string>{"init:100","repoA","repoB"}));
    // The original developer cvar helper does NOT reject null answers.
    MockHost cvar;
    assert(native::register_and_query_developer(cvar)==0u);
    assert((cvar.operations==std::vector<std::string>{"cvar-register","cvar-query","cvar-publish-null"}));
    native::forward_registry_count(cvar,0xC0FFEEu);
    assert(cvar.operations.back()=="registry-forward");

    // Portable safety guard, intentionally not in the original x86 binary.
    native::SystemNode self{123u};self.next=&self;
    native::Registry cyc;cyc.pending=&self;
    MockHost guarded;
    bool threw=false;
    try { native::initialize_game_systems(cyc,guarded,2); }
    catch (const std::length_error&) { threw=true; }
    assert(threw);
    // No implicit native rollback is asserted for the synthetic exception.
    return 0;
}