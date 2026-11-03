#pragma once
// Original SiN Episodes 2006 CreateInterface list behavior, tested on host.
// This is NOT a Source game module, CreateInterface export or valid ARM64 vtable.
#include <cstring>
namespace sin_compat::original_factory {
using CreateFn = void* (*)();
struct Registration {
    CreateFn create = nullptr;  // original x86 offset +0
    const char* name = nullptr; // original x86 offset +4
    Registration* next = nullptr; // original x86 offset +8
};
struct Registry {
    Registration* first = nullptr;
    void add(Registration& entry) noexcept {
        entry.next = first;
        first = &entry;
    }
    void* query(const char* name, int* return_code) const noexcept {
        // Original null request is not modeled; reject safely.
        if (name) {
            for (auto* p=first;p;p=p->next) {
                if (p->name && std::strcmp(name,p->name)==0) {
                    if (return_code) *return_code=0;
                    return p->create ? p->create() : nullptr;
                }
            }
        }
        if (return_code) *return_code=1;
        return nullptr;
    }
};
inline constexpr const char* client_interface="VClient011";
inline constexpr const char* server_interface="ServerGameDLL003";
inline constexpr const char* server_ents_interface="ServerGameEnts001";
inline constexpr const char* server_clients_interface="ServerGameClients003";
} // namespace sin_compat::original_factory
