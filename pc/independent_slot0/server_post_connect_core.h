#pragma once
// Recovered first post-Connect server operations. The original stores
// globals, initializes math, caches three factories, then invokes
// GAMEEVENTSMANAGER002 slot 1 with resource/gameevents.res.
// No original addresses or global state are used.
#include "server_math_tables_core.h"

namespace sin_independent {
using StartupFactory=void* (__cdecl *)(const char*,int*);
using LoadEventsFromFile=int (__thiscall *)(void*,const char*);
struct ServerPostConnectState {
    void* globals=nullptr;
    ServerMathTables math{};
    StartupFactory engine=nullptr,physics=nullptr,filesystem=nullptr;

    bool Initialize(void* globalData,StartupFactory engineFactory,
                    StartupFactory physicsFactory,StartupFactory filesystemFactory) noexcept {
        if(!globalData || !engineFactory || !physicsFactory || !filesystemFactory)
            return false;
        globals=globalData;
        if(!math.InitializeRetailDefaults())return false;
        engine=engineFactory;
        physics=physicsFactory;
        filesystem=filesystemFactory;
        return true;
    }
    bool LoadInitialEvents(void* manager,int& count) const noexcept {
        if(!globals || !math.initialized || !engine || !physics || !filesystem || !manager)
            return false;
        auto* table=*reinterpret_cast<void***>(manager);
        if(!table || !table[1])return false;
        auto load=reinterpret_cast<LoadEventsFromFile>(table[1]);
        // Preserve the signed result: original DLLInit does not test it
        // here. Returning zero/negative does not prove event loading.
        count=load(manager,"resource/gameevents.res");
        return true;
    }
};
} // namespace sin_independent
