#include "sin_compat/original_client_world_network_prelude.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <cstdio>

namespace prelude = sin_compat::original_client_world_network_prelude;
int main() {
    prelude::State state{0x12345678u, 0xFFFFFFFFu, 0x80000000u};
    unsigned calls = 0;
    auto shared = [&](prelude::State& observed, std::uint32_t first,
                      std::uint32_t second) {
        assert(&observed == &state);
        assert(observed.worldField480 == 0);
        assert(observed.clientGlobal2432B254 == 0);
        assert(observed.clientGlobal2436115C == 0);
        assert(first == 0xFFFFFFFFu && second == 0x80000000u);
        ++calls;
        observed.worldField480 = 17u;
        return true;
    };
    assert(prelude::Initialize(state, 0xFFFFFFFFu, 0x80000000u, shared));
    assert(calls == 1 && state.worldField480 == 17u);
    // Repeat after state changes: clears occur before every dispatch.
    state.clientGlobal2432B254 = 23u;
    state.clientGlobal2436115C = 42u;
    assert(prelude::Initialize(state, 0xFFFFFFFFu, 0x80000000u, shared));
    assert(calls == 2 && state.worldField480 == 17u);
    // False is a host-injected witness of wrapper result forwarding only;
    // the authenticated original shared initializer returns AL=1.
    assert(!prelude::Initialize(state, 0u, 0u,
        [&](prelude::State& observed, std::uint32_t a, std::uint32_t b) {
            assert(observed.worldField480 == 0 && a == 0 && b == 0);
            ++calls;
            return false;
        }));
    assert(calls == 3);
    std::puts("PASS world network prelude: three resets, exact argument/result forwarding, repeated initialization");
}
