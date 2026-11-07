#include "sin_compat/sourcevr_game_startup_abi.hpp"
#include "sin_compat/sourcevr_game_factory_preflight.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstddef>

using namespace sin_compat::sourcevr_startup_abi;
int main() {
    using sin_compat::native_game_vtables::verified_layouts;
    static_assert(calls.size()==4);
    static_assert(client_init.vtable_method_index==0);
    static_assert(client_init.pointer_register_count==3);
    static_assert(client_init.observed_result==ResultUsage::BranchOnNonzeroW0);
    static_assert(client_cleanup.vtable_method_index==2);
    static_assert(client_cleanup.observed_result==ResultUsage::Ignored);
    static_assert(server_description.vtable_method_index==12);
    static_assert(server_description.observed_result==
                  ResultUsage::PointerAsLogString);
    static_assert(server_startup_candidate.vtable_method_index==0);
    static_assert(server_startup_candidate.pointer_register_count==4);
    static_assert(!server_startup_candidate.engine_dispatch_verified);
    static_assert(client_init.vtable_method_index <
                  verified_layouts[1].method_slots);
    static_assert(server_description.vtable_method_index <
                  verified_layouts[0].method_slots);
    for (const auto& item : calls) {
        assert(item.callee_prologue_verified);
        assert(!safe_to_dispatch(item));
        assert(!item.complete_cpp_signature_verified);
    }
    // Proving only a guessed method name or a matching vtable slot
    // must never unlock a virtual call into a real SourceVR game object.
    constexpr NativeStartupCallsite guessed{
        "dangerous guessed alias",client_init.role,0,3,
        ResultUsage::BranchOnNonzeroW0,false,false,false
    };
    static_assert(!safe_to_dispatch(guessed));
}
