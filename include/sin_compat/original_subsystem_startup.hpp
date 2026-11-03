#pragma once
// SiN Episodes (2006) original x86 post-Init behavioral reference.
// Host-only C++17; NOT a SourceVR/Android ABI, DLL binding or executable game.
#include <cstddef>
#include <string_view>
#include <vector>

namespace sin_compat::startup {
struct ClientSetupNode {
    void** slot;  // null slot means callback failure can be skipped
    void* (*setup)();
    void (*teardown)(void*);
};
// Original client 0x241640F0 iterates ALL registered nodes on rollback,
// calling teardown even if the object's stored value is nullptr.
inline void cleanup_client_nodes(const std::vector<ClientSetupNode>& nodes) {
    for (const auto& node : nodes) {
        if (node.slot) {
            node.teardown(*node.slot);
            *node.slot = nullptr;
        }
    }
}
// Original 0x24164120: setup callback at node+4, result stored through
// node+0 if nonnull; null from required node triggers full rollback.
// The extra null-slot guard here protects a malformed host fixture.
inline bool start_client_nodes(const std::vector<ClientSetupNode>& nodes) {
    for (const auto& node : nodes) {
        void* value = node.setup();
        if (value) {
            if (!node.slot) return false;
            *node.slot = value;
        } else if (node.slot) {
            cleanup_client_nodes(nodes);
            return false;
        }
    }
    return true;
}

struct ServerSystem {
    void* identity;
    bool (*initialize)(void*);
};
struct ServerSystemStartup {
    std::vector<ServerSystem> registered;
    std::vector<ServerSystem> pending; // original pending-linked-list order
    bool initialized = false;
    std::size_t duplicate_count = 0;
};
// Original server 0x221F5700 collects objects, deduplicates by pointer,
// clears pending and marks initialized BEFORE per-system vtable+0 init calls.
inline bool start_server_systems(ServerSystemStartup& state) {
    for (const auto& system : state.pending) {
        bool found = false;
        for (const auto& old : state.registered) {
            if (old.identity == system.identity) { found = true; break; }
        }
        if (found) ++state.duplicate_count;
        else state.registered.push_back(system);
    }
    state.pending.clear();
    state.initialized = true;
    for (const auto& system : state.registered)
        if (!system.initialize(system.identity)) return false;
    return true;
}

inline constexpr std::string_view client_vgui_module = "CLIENT";
inline constexpr std::string_view client_vgui_input = "VGUI_InputInternal001";
inline constexpr std::string_view client_vgui_system = "VGUI_System010";

struct VGuiStartupState {
    void* input = nullptr;
    void* system = nullptr;
    bool one_time_registered = false;
};
// Original client 0x241EDFB0: module check, two exact VGUI factories,
// one-time registration, two later helpers. Late error leaves globals
// assigned. This standalone adapter does not implement GUI objects.
template<typename LoadModule,typename Factory,typename Register,typename First,typename Second>
bool start_client_vgui(VGuiStartupState& state, LoadModule&& load_module,
                       Factory&& factory, Register&& register_once,
                       First&& first, Second&& second) {
    if (!load_module(client_vgui_module)) return false;
    state.input = factory(client_vgui_input);
    if (!state.input) return false;
    state.system = factory(client_vgui_system);
    if (!state.system) return false;
    if (!state.one_time_registered) {
        state.one_time_registered = true;
        register_once();
    }
    first();
    return second();
}
inline constexpr std::size_t original_particle_bucket_allocation = 0x30;
inline constexpr std::size_t original_particle_bucket_constructor_arg = 0x60;
inline constexpr std::string_view original_particle_bucket_label =
    "CParticleMgr::m_pParticleBucket";
} // namespace sin_compat::startup
