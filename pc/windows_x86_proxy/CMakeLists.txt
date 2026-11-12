cmake_minimum_required(VERSION 3.20)
project(sin_pc_win32_factory_proxy LANGUAGES C CXX)

# Synthetic original server's decorated MSVC virtual exports must be generated
# by the C++ compiler itself; DEF aliases for C targets truncate at '@'.
set_source_files_properties(tests/fake_original.c PROPERTIES LANGUAGE CXX)

if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "This smoke target requires a Windows x86 (Win32/PE32) compiler, not x64 or Android.")
endif()

enable_testing()
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

function(sin_pc_configure target output)
  set_target_properties(${target} PROPERTIES
    PREFIX ""
    OUTPUT_NAME "${output}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /WX)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Werror)
  endif()
endfunction()

# These proxies preserve the original game interfaces by forwarding to
# owner-supplied, locally renamed original DLLs. They do NOT replace gameplay.
add_library(sin_pc_server SHARED src/factory_proxy.c src/server.def)
target_compile_definitions(sin_pc_server PRIVATE SIN_PC_PROXY_SERVER=1)
sin_pc_configure(sin_pc_server server)

add_library(sin_pc_client SHARED src/factory_proxy.c src/client.def)
target_compile_definitions(sin_pc_client PRIVATE SIN_PC_PROXY_CLIENT=1)
sin_pc_configure(sin_pc_client client)

# Test-only synthetic originals: NEVER bundle commercial SiN DLL bytes in CI.
add_library(sin_pc_fake_server SHARED tests/fake_original.c tests/fake_server.def)
target_compile_definitions(sin_pc_fake_server PRIVATE SIN_PC_FAKE_SERVER=1)
sin_pc_configure(sin_pc_fake_server sin_original_server)

add_library(sin_pc_fake_client SHARED tests/fake_original.c tests/fake_client.def)
target_compile_definitions(sin_pc_fake_client PRIVATE SIN_PC_FAKE_CLIENT=1)
sin_pc_configure(sin_pc_fake_client sin_original_client)

add_executable(sin_pc_proxy_tests tests/proxy_tests.c)
sin_pc_configure(sin_pc_proxy_tests sin_pc_proxy_tests)
add_dependencies(sin_pc_proxy_tests
  sin_pc_server sin_pc_client sin_pc_fake_server sin_pc_fake_client)
add_test(NAME sin_pc_win32_factory_forwarding COMMAND $<TARGET_FILE:sin_pc_proxy_tests>)
set_tests_properties(sin_pc_win32_factory_forwarding PROPERTIES
  WORKING_DIRECTORY "$<TARGET_FILE_DIR:sin_pc_proxy_tests>")


# Phase177: test the original 2006 DLL CreateInterface registry semantics
# using independently reconstructed host-neutral code.
add_executable(sin_pc_abi_registry_tests
  ../../tests/test_original_dll_interface_abi.cpp)
target_include_directories(sin_pc_abi_registry_tests PRIVATE ../../include)
# MSVC defaults to an older dialect here; the recovered ABI header is C++17.
set_target_properties(sin_pc_abi_registry_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_abi_registry_tests sin_pc_abi_registry_tests)
add_test(NAME sin_original_dll_interface_abi_win32
  COMMAND $<TARGET_FILE:sin_pc_abi_registry_tests>)

# Real Windows x86 LoadLibrary/GetProcAddress probe of a SYNTHETIC DLL.
# It checks only the undecorated CreateInterface cdecl export and the
# matched/missing return-status convention; never claims to load the game.
add_library(sin_pc_abi_probe SHARED
  ../../tests/original_win32_loader_probe.cpp
  ../../tests/original_win32_loader_probe.def)
target_include_directories(sin_pc_abi_probe PRIVATE ../../include)
set_target_properties(sin_pc_abi_probe PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_abi_probe sin_original_abi_probe)

add_executable(sin_pc_abi_loader_tests
  ../../tests/test_original_win32_loader.cpp)
sin_pc_configure(sin_pc_abi_loader_tests sin_pc_abi_loader_tests)
add_dependencies(sin_pc_abi_loader_tests sin_pc_abi_probe)
add_test(NAME sin_original_win32_CreateInterface_loader
  COMMAND $<TARGET_FILE:sin_pc_abi_loader_tests>
    $<TARGET_FILE:sin_pc_abi_probe>)


# Phase179: test original server/client Init early interface negotiation
# using the real Windows x86 MSVC compiler, against synthetic callbacks.
# This does NOT load the original game or implement full engine startup.
add_executable(sin_pc_original_init_contract_tests
  ../../tests/test_original_dll_initialization.cpp)
target_include_directories(sin_pc_original_init_contract_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_init_contract_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_init_contract_tests sin_pc_original_init_contract_tests)
add_test(NAME sin_original_win32_initialization_contract
  COMMAND $<TARGET_FILE:sin_pc_original_init_contract_tests>)


# Phase180: verify original app-system creation rollback and physics three-
# interface failure ordering on native MSVC Win32 x86. Synthetic callbacks.
add_executable(sin_pc_original_client_postinit_contract_tests
  ../../tests/test_original_client_postinit.cpp)
target_include_directories(sin_pc_original_client_postinit_contract_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_client_postinit_contract_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_client_postinit_contract_tests
  sin_pc_original_client_postinit_contract_tests)
add_test(NAME sin_original_win32_client_postinit_contract
  COMMAND $<TARGET_FILE:sin_pc_original_client_postinit_contract_tests>)


# Phase181: run native MSVC x86 synthetic manifest startup contract.
# KeyValues parser and surface-file loader are callbacks, not game binaries.
add_executable(sin_pc_original_surface_manifest_tests
  ../../tests/test_original_client_surface_manifest.cpp)
target_include_directories(sin_pc_original_surface_manifest_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_surface_manifest_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_surface_manifest_tests
  sin_pc_original_surface_manifest_tests)
add_test(NAME sin_original_win32_client_surface_manifest_contract
  COMMAND $<TARGET_FILE:sin_pc_original_surface_manifest_tests>)


# Phase182: real MSVC Win32 x86 tests of reconstructed original client file
# read/close/parse order, KeyValues type-switch, and surface dispatch guards.
# All engine and filesystem interfaces here are synthetic callbacks.
add_executable(sin_pc_original_surface_file_tests
  ../../tests/test_original_client_surface_file.cpp)
target_include_directories(sin_pc_original_surface_file_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_surface_file_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_surface_file_tests
  sin_pc_original_surface_file_tests)
add_test(NAME sin_original_win32_client_surface_file_contract
  COMMAND $<TARGET_FILE:sin_pc_original_surface_file_tests>)


# Phase183: native Win32 MSVC x86 synthetic KeyValues branch and typed setter
# tests; no actual SiN game engine object or commercial DLL bytes involved.
add_executable(sin_pc_original_keyvalues_setters_tests
  ../../tests/test_original_client_keyvalues_setters.cpp)
target_include_directories(sin_pc_original_keyvalues_setters_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_keyvalues_setters_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_keyvalues_setters_tests
  sin_pc_original_keyvalues_setters_tests)
add_test(NAME sin_original_win32_client_keyvalues_setters_contract
  COMMAND $<TARGET_FILE:sin_pc_original_keyvalues_setters_tests>)


# Phase184: exercise original KeyValues lifetime branch order and native
# 40-byte x86 field snapshot under MSVC Win32. Synthetic callbacks only.
add_executable(sin_pc_original_keyvalues_lifetime_tests
  ../../tests/test_original_client_keyvalues_lifetime.cpp)
target_include_directories(sin_pc_original_keyvalues_lifetime_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_keyvalues_lifetime_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_keyvalues_lifetime_tests
  sin_pc_original_keyvalues_lifetime_tests)
add_test(NAME sin_original_win32_client_keyvalues_lifetime_contract
  COMMAND $<TARGET_FILE:sin_pc_original_keyvalues_lifetime_tests>)


# Phase185: verify original x86 tier0 g_pMemAlloc virtual slot and shared
# cached-context branches under real MSVC Win32 CI. Synthetic tier0 only.
add_executable(sin_pc_original_memalloc_bridge_tests
  ../../tests/test_original_client_memalloc_bridge.cpp)
target_include_directories(sin_pc_original_memalloc_bridge_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_memalloc_bridge_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_memalloc_bridge_tests
  sin_pc_original_memalloc_bridge_tests)
add_test(NAME sin_original_win32_client_memalloc_bridge_contract
  COMMAND $<TARGET_FILE:sin_pc_original_memalloc_bridge_tests>)


# Phase186: verify native 2006 x86 cache dispatch under MSVC, and separately
# exercise actual kernel32 GetProcessHeap/HeapAlloc/VirtualQuery/
# GetModuleFileNameA on real 32-bit Windows (synthetic executable anchor).
add_executable(sin_pc_original_debug_owner_tests
  ../../tests/test_original_client_debug_owner.cpp)
target_include_directories(sin_pc_original_debug_owner_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_debug_owner_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_debug_owner_tests
  sin_pc_original_debug_owner_tests)
add_test(NAME sin_original_win32_debug_owner_model
  COMMAND $<TARGET_FILE:sin_pc_original_debug_owner_tests>)

add_executable(sin_pc_original_debug_owner_kernel32_tests
  ../../tests/test_original_client_debug_owner_win32.cpp)
target_include_directories(sin_pc_original_debug_owner_kernel32_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_debug_owner_kernel32_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_debug_owner_kernel32_tests
  sin_pc_original_debug_owner_kernel32_tests)
add_test(NAME sin_original_win32_debug_owner_real_kernel32
  COMMAND $<TARGET_FILE:sin_pc_original_debug_owner_kernel32_tests>)


# Phase187: run original server game-system list and three required Init gates
# under native 32-bit MSVC CI. Repository callback targets are synthetic and
# this is NOT original Windows engine gameplay.
add_executable(sin_pc_original_server_post_emitter_tests
  ../../tests/test_original_server_post_emitter.cpp)
target_include_directories(sin_pc_original_server_post_emitter_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_server_post_emitter_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_server_post_emitter_tests
  sin_pc_original_server_post_emitter_tests)
add_test(NAME sin_original_win32_server_post_emitter_contract
  COMMAND $<TARGET_FILE:sin_pc_original_server_post_emitter_tests>)


# Phase188: x86 MSVC original server scene/model cache Init header validation
# and reload branch model. This is synthetic; not game engine DLL loading.
add_executable(sin_pc_original_server_cache_repositories_tests
  ../../tests/test_original_server_cache_repositories.cpp)
target_include_directories(sin_pc_original_server_cache_repositories_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_server_cache_repositories_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_server_cache_repositories_tests
  sin_pc_original_server_cache_repositories_tests)
add_test(NAME sin_original_win32_server_cache_repositories_contract
  COMMAND $<TARGET_FILE:sin_pc_original_server_cache_repositories_tests>)


# Phase189: native x86 MSVC test of the synthetic three-gate composition.
add_executable(sin_pc_original_server_startup_composition_tests
  ../../tests/test_original_server_startup_composition.cpp)
target_include_directories(sin_pc_original_server_startup_composition_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_server_startup_composition_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_server_startup_composition_tests
  sin_pc_original_server_startup_composition_tests)
add_test(NAME sin_original_win32_server_startup_composition_contract
  COMMAND $<TARGET_FILE:sin_pc_original_server_startup_composition_tests>)


# Phase190: native 32-bit MSVC contract for host-modeled original cache records.
add_executable(sin_pc_original_cache_record_identity_tests
  ../../tests/test_original_server_cache_record_identity.cpp)
target_include_directories(sin_pc_original_cache_record_identity_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_cache_record_identity_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_cache_record_identity_tests
  sin_pc_original_cache_record_identity_tests)
add_test(NAME sin_original_win32_server_cache_record_identity_contract
  COMMAND $<TARGET_FILE:sin_pc_original_cache_record_identity_tests>)


# Phase191: original server tree lookup contract, compiled with native x86 MSVC.
# Only independent host-simulated callbacks, not a loadable game DLL.
add_executable(sin_pc_original_cache_tree_lookup_tests
  ../../tests/test_original_server_cache_tree_lookup.cpp)
target_include_directories(sin_pc_original_cache_tree_lookup_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_cache_tree_lookup_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_cache_tree_lookup_tests
  sin_pc_original_cache_tree_lookup_tests)
add_test(NAME sin_original_win32_server_cache_tree_lookup_contract
  COMMAND $<TARGET_FILE:sin_pc_original_cache_tree_lookup_tests>)


# Phase192: compile and test recovered unsigned comparator through the
# Phase191 reference tree on real native 32-bit MSVC. Synthetic, not game DLL.
add_executable(sin_pc_original_cache_key_comparator_tests
  ../../tests/test_original_server_cache_key_comparators.cpp)
target_include_directories(sin_pc_original_cache_key_comparator_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_cache_key_comparator_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_cache_key_comparator_tests
  sin_pc_original_cache_key_comparator_tests)
add_test(NAME sin_original_win32_server_cache_key_comparator_contract
  COMMAND $<TARGET_FILE:sin_pc_original_cache_key_comparator_tests>)


# Phase193: native Win32 x86 MSVC contract for insertion wrappers and
# linked Phase191/192 lookup/comparator models; not commercial DLL loading.
add_executable(sin_pc_original_cache_tree_insert_tests
  ../../tests/test_original_server_cache_tree_insert.cpp)
target_include_directories(sin_pc_original_cache_tree_insert_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_cache_tree_insert_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_cache_tree_insert_tests
  sin_pc_original_cache_tree_insert_tests)
add_test(NAME sin_original_win32_server_cache_tree_insert_contract
  COMMAND $<TARGET_FILE:sin_pc_original_cache_tree_insert_tests>)


# Phase194 native Win32 x86 CTest for host-neutral original free-list allocators.
add_executable(sin_pc_original_cache_slot_allocation_tests
  ../../tests/test_original_server_cache_slot_allocation.cpp)
target_include_directories(sin_pc_original_cache_slot_allocation_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_cache_slot_allocation_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_cache_slot_allocation_tests
  sin_pc_original_cache_slot_allocation_tests)
add_test(NAME sin_original_win32_server_cache_slot_allocation_contract
  COMMAND $<TARGET_FILE:sin_pc_original_cache_slot_allocation_tests>)


# Phase195: native Windows x86 C++17 contract for original node/sentinel
# resolution and Phase194 free-list integration; synthetic engine state.
add_executable(sin_pc_original_cache_node_resolver_tests
  ../../tests/test_original_server_cache_node_resolver.cpp)
target_include_directories(sin_pc_original_cache_node_resolver_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_cache_node_resolver_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_cache_node_resolver_tests
  sin_pc_original_cache_node_resolver_tests)
add_test(NAME sin_original_win32_server_cache_node_resolver_contract
  COMMAND $<TARGET_FILE:sin_pc_original_cache_node_resolver_tests>)
