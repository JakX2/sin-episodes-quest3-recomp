// Phase 649: local Windows x86 CreateInterface inspection, without invoking unknown vtables.
// Build: cl /nologo /W4 /WX /EHsc /std:c++17 research\tools\sin_win32_factory_probe_phase649.cpp
// Run: sin_win32_factory_probe_phase649.exe C:\path\to\client.dll C:\path\to\server.dll
// Does not execute engine startup or prove gameplay boot. Use only trusted local DLLs.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstdint>
#include <cstring>

#if !defined(_M_IX86) && !defined(__i386__)
#error This probe requires a 32-bit x86 compiler and process.
#endif

using CreateInterfaceFn = void* (__cdecl *)(const char*, int*);
struct Query { const char* module; const char* name; };
static constexpr Query queries[] = {
    {"client", "VClient011"},
    {"client", "VClientPrediction001"},
    {"client", "VClientEntityList003"},
    {"server", "ServerGameDLL003"},
    {"server", "ServerGameClients003"},
    {"server", "ServerGameEnts001"}
};
static void emit(const char* module, const char* name, const char* result, int rc) {
    std::printf("{\"module\":\"%s\",\"interface\":\"%s\",\"result\":\"%s\",\"return_code\":%d}\n",
                module, name, result, rc);
}
int main(int argc, char** argv) {
    if (argc != 3) {
        std::fprintf(stderr, "usage: %s <trusted-client.dll> <trusted-server.dll>\n", argv[0]);
        return 2;
    }
    const char* paths[] = {argv[1], argv[2]};
    const char* names[] = {"client", "server"};
    int failures = 0;
    for (int i = 0; i < 2; ++i) {
        // LoadLibrary executes DllMain. Never use untrusted binaries.
        HMODULE module = LoadLibraryA(paths[i]);
        if (!module) {
            emit(names[i], "CreateInterface", "load_failed", static_cast<int>(GetLastError()));
            ++failures;
            continue;
        }
        auto factory = reinterpret_cast<CreateInterfaceFn>(
            GetProcAddress(module, "CreateInterface"));
        if (!factory) {
            emit(names[i], "CreateInterface", "export_missing", static_cast<int>(GetLastError()));
            ++failures;
            FreeLibrary(module);
            continue;
        }
        for (const auto& query : queries) {
            if (std::strcmp(query.module, names[i]) != 0) continue;
            int rc = -9999;
            void* object = factory(query.name, &rc);
            if (!object) {
                emit(names[i], query.name, "not_found", rc);
                ++failures;
                continue;
            }
            // Do not dereference the object or infer vtable slots from a factory hit.
            emit(names[i], query.name, "factory_returned_nonnull", rc);
        }
        FreeLibrary(module);
    }
    return failures ? 1 : 0;
}
