#include "sin_compat/sourcevr_game_vtable_layout.hpp"
#include "sin_compat/sourcevr_game_factory_preflight.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

int main() {
    using namespace sin_compat::native_game_vtables;
    using sin_compat::sourcevr_game_factory::sourcevr_055_native_game_modules;
    constexpr auto server = verified_layouts[0];
    constexpr auto client = verified_layouts[1];
    static_assert(server.role == NativeGameObjectRole::Server);
    static_assert(client.role == NativeGameObjectRole::Client);
    static_assert(server.method_slots == 47 && client.method_slots == 78);
    static_assert(server.native_factory == "ServerGameDLL010");
    static_assert(client.native_factory == "VClient017");
    static_assert(server.base_type_name == "IServerGameDLL");
    static_assert(client.base_type_name == "IBaseClientDLL");
    static_assert(!native_game_object_dispatch_allowed(server, 47));
    static_assert(!native_game_object_dispatch_allowed(client, 78));
    assert(!native_game_object_dispatch_allowed(server, 0));
    assert(!native_game_object_dispatch_allowed(client, 0));
    assert(!native_game_object_dispatch_allowed(server, 46));
    assert(!native_game_object_dispatch_allowed(client, 79));
    // Even if the server has precisely all 47 native slots, the old x86
    // ServerGameDLL003 interface is not equivalent to the ARM64 010 ABI.
    static_assert(sourcevr_055_native_game_modules[0].original_required ==
                  "ServerGameDLL003");
    static_assert(sourcevr_055_native_game_modules[1].original_required ==
                  "VClient011");
    assert(server.method_signatures == MethodAuditLevel::Unverified);
    assert(client.method_signatures == MethodAuditLevel::Unverified);
}
