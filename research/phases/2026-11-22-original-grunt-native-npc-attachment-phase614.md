# Gameplay Phase614: existing native Grunt NPC session attachment

## Scope and exact 2006 factory classnames

Phase613 delivered a loadable Windows x86 Grunt gameplay C ABI. Phase614 adds safe session lifetime tracking for **already constructed, engine-owned NPCs**, not another synthetic entity factory. The three source-authenticated 2006 Druglab Grunt classnames (Phase531) are:

- npc_druglab_grunt_pistol
- npc_druglab_grunt_scattergun
- npc_druglab_grunt_assault_rifle

These names alone do not authorize creating a gameplay entity. The independent GameplayEntityFactory still intentionally supports weapon_assault_rifle and grenade_ar, not Grunt spawn classes. No original 2006 C++ vtables are copied into portable host objects.

## Implemented owner and lifetime contract

Files:

- include/sin_compat/original_grunt_npc_session_registry_phase614.hpp
- src/original_grunt_npc_session_registry_phase614.cpp
- tests/test_original_grunt_npc_session_registry_phase614.cpp

The engine-owned NativeHost must first confirm that a **real, existing native NPC** matches the original classname, actor (index,generation), and nonzero opaque owner token. A separate Phase613 callback must report actor_live = 1. Only then is an original-free per-NPC behavior session created.

The registry rejects unsupported classes, invalid zero actor identifiers, unconfirmed or dead engine entities, reentrant callbacks, and duplicate entity indices. It stores the full generation for each slot. Stale generations cannot step, snapshot or detach the current session. A confirmed owner can detach the exact current generation **after NPC death**, freeing only portable session memory, then reuse the same index for a subsequent generation. The destructor releases owned sessions, never game engine entities.

The host must not destroy the registry or its userdata during a callback; this is a single-threaded registration and lifetime contract. No original Source entity allocation, factory registration, or gameplay vtable is synthesized.

## Verification

Local original-free GCC C++17 strict-warning build and regression passed, linking the genuine Phase613 shared library. Cases include all three authentic classnames, rejecting non-Grunt aliases, denied host confirmation and dead actors, attach -> step -> snapshot -> detach, exact generation mismatch refusal, index reuse only after detach, three concurrent supported variants and callback-triggered reentry rejection.

Root CMake builds STATIC sin_grunt_npc_session_registry and registers its standalone CTest. The Windows x86 PE32 proxy subproject builds the registry against the tested Phase613 Win32 DLL and registers sin_pc_grunt_native_NPC_session_registry_phase614, without changing original server.dll CreateInterface.

Local build:
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
    cmake --build build --target sin_grunt_npc_session_registry_phase614_tests
    ctest --test-dir build -R sin_grunt_npc_session_registry_phase614_tests --output-on-failure

Windows x86:
    cmake -S pc/windows_x86_proxy -B build-pc-win32 -G "Visual Studio 17 2022" -A Win32
    cmake --build build-pc-win32 --config Release
    ctest --test-dir build-pc-win32 -C Release -R sin_pc_grunt_native_NPC_session_registry_phase614 --output-on-failure

## Next genuine runtime boundary

A real engine adapter must construct a proper native Grunt NPC, register its actual factory, preserve its generation-safe identity and original network/AI lifecycle, and map real Source engine condition/schedule/entity callbacks to the independent behavior session. Only after a native instance exists should this registry attach. **No real SiN NPC spawned or original retail engine execution is claimed**.

This phase adds no new original DLL function bodies. Cumulative Phases601–614 remain **40 exact source-authenticated 2006 native method addresses / 11,224 original machine-code bytes**. No HL2-equivalent physics, surfaces, models, prop/rendering work or proprietary original assets are included.
