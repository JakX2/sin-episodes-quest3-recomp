#pragma once
#include <cstdint>

namespace sin_compat::original_client_world_network_prelude {
// Logical host-owned state, NOT the original entity or global memory layout.
// World wrapper 0x240FCB10 clears these three source fields in this order.
struct State final {
    std::uint32_t worldField480 = 0;
    std::uint32_t clientGlobal2432B254 = 0;
    std::uint32_t clientGlobal2436115C = 0;
};

// The caller supplies the shared initializer; this does not bind or call
// original PE32 addresses. The actual engine initializer remains unresolved.
// Use explicit 32-bit argument bits to avoid guessing semantic types.
template <class SharedInitializer>
bool Initialize(State& state, std::uint32_t first, std::uint32_t second,
                SharedInitializer&& shared) {
    state.worldField480 = 0;
    state.clientGlobal2432B254 = 0;
    state.clientGlobal2436115C = 0;
    return shared(state, first, second);
}
} // namespace sin_compat::original_client_world_network_prelude
