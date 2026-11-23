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

# Opt-in real Win32 file-backed CreateInterface trace. Both client and server
# are the independently built proxy plus fake original; never retail binaries.
add_executable(sin_pc_factory_trace_tests tests/test_factory_trace_file.c)
sin_pc_configure(sin_pc_factory_trace_tests sin_pc_factory_trace_tests)
add_dependencies(sin_pc_factory_trace_tests
  sin_pc_server sin_pc_client sin_pc_fake_server sin_pc_fake_client)
add_test(NAME sin_pc_win32_original_factory_startup_trace
  COMMAND $<TARGET_FILE:sin_pc_factory_trace_tests>)
set_tests_properties(sin_pc_win32_original_factory_startup_trace PROPERTIES
  WORKING_DIRECTORY "$<TARGET_FILE_DIR:sin_pc_factory_trace_tests>")

# Parse the owner-only launcher under the real Windows PowerShell interpreter.
# It never launches a real game in public CI.
add_test(NAME sin_pc_owner_startup_trace_launcher_syntax
  COMMAND powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass
    -File "${CMAKE_CURRENT_SOURCE_DIR}/tests/Test-OwnerStartupTraceLauncherSyntax.ps1"
    -Launcher "${CMAKE_CURRENT_SOURCE_DIR}/Invoke-OwnerSiNStartupTrace.ps1")


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


# Phase196: MSVC x86 host-neutral parent/child/root tree-link contract,
# including native-index-width handling and synthetic balancing callbacks.
add_executable(sin_pc_original_cache_tree_link_tests
  ../../tests/test_original_server_cache_tree_link.cpp)
target_include_directories(sin_pc_original_cache_tree_link_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_cache_tree_link_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_cache_tree_link_tests
  sin_pc_original_cache_tree_link_tests)
add_test(NAME sin_original_win32_server_cache_tree_link_contract
  COMMAND $<TARGET_FILE:sin_pc_original_cache_tree_link_tests>)


# Phase197: native PE32/MSVC C++17 tests of original cache rebalance fast
# paths. Red-parent branches fail closed, not a full native rebalancer.
add_executable(sin_pc_original_cache_balance_preflight_tests
  ../../tests/test_original_server_cache_balance_preflight.cpp)
target_include_directories(sin_pc_original_cache_balance_preflight_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_cache_balance_preflight_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_cache_balance_preflight_tests
  sin_pc_original_cache_balance_preflight_tests)
add_test(NAME sin_original_win32_server_cache_balance_preflight_contract
  COMMAND $<TARGET_FILE:sin_pc_original_cache_balance_preflight_tests>)


# Phase198: native 32-bit MSVC contract for original cache tree rotations.
# Synthetic node storage; not a game DLL, real engine or original heap.
add_executable(sin_pc_original_cache_tree_rotations_tests
  ../../tests/test_original_server_cache_tree_rotations.cpp)
target_include_directories(sin_pc_original_cache_tree_rotations_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_cache_tree_rotations_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_cache_tree_rotations_tests
  sin_pc_original_cache_tree_rotations_tests)
add_test(NAME sin_original_win32_server_cache_tree_rotations_contract
  COMMAND $<TARGET_FILE:sin_pc_original_cache_tree_rotations_tests>)


# Phase199: MSVC x86 tests of the host-side compositional red-black fixup.
# The actual 1387-byte native balancing ABI is not fully reconstructed.
add_executable(sin_pc_original_cache_balance_composition_tests
  ../../tests/test_original_server_cache_balance_composition.cpp)
target_include_directories(sin_pc_original_cache_balance_composition_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_cache_balance_composition_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_cache_balance_composition_tests
  sin_pc_original_cache_balance_composition_tests)
add_test(NAME sin_original_win32_server_cache_balance_composition_contract
  COMMAND $<TARGET_FILE:sin_pc_original_cache_balance_composition_tests>)


# Phase200: independent native Win32 x86 MSVC randomized cache tree stress.
# Uses synthetic host nodes; not an original PE32 playable engine module.
add_executable(sin_pc_original_cache_balance_stress_tests
  ../../tests/test_original_server_cache_balance_stress.cpp)
target_include_directories(sin_pc_original_cache_balance_stress_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_cache_balance_stress_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_cache_balance_stress_tests
  sin_pc_original_cache_balance_stress_tests)
add_test(NAME sin_original_win32_server_cache_balance_stress_contract
  COMMAND $<TARGET_FILE:sin_pc_original_cache_balance_stress_tests>)


# Phase201: native MSVC x86 contract against the 32-bit original array
# capacity algorithm; only synthetic host pointer tokens and memory.
add_executable(sin_pc_original_cache_array_growth_tests
  ../../tests/test_original_server_cache_array_growth.cpp)
target_include_directories(sin_pc_original_cache_array_growth_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_cache_array_growth_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_cache_array_growth_tests
  sin_pc_original_cache_array_growth_tests)
add_test(NAME sin_original_win32_server_cache_array_growth_contract
  COMMAND $<TARGET_FILE:sin_pc_original_cache_array_growth_tests>)


# Phase202: native MSVC Win32 x86 CTest for the original server allocator
# wrapper path and Phase194/201 cache vector ownership integration.
add_executable(sin_pc_original_server_allocator_dispatch_tests
  ../../tests/test_original_server_allocator_dispatch.cpp)
target_include_directories(sin_pc_original_server_allocator_dispatch_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_server_allocator_dispatch_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_server_allocator_dispatch_tests
  sin_pc_original_server_allocator_dispatch_tests)
add_test(NAME sin_original_win32_server_allocator_dispatch_contract
  COMMAND $<TARGET_FILE:sin_pc_original_server_allocator_dispatch_tests>)


# Phase203: actual Windows x86 LoadLibrary/GetProcAddress preflight for the
# independently built proxies and (in owner-only use) exact real SiN DLLs.
# CI uses ONLY synthesized fake originals. The default probe NEVER invokes
# a game factory; the optional flag does so explicitly, with no engine host.
add_executable(sin_pc_owner_module_probe tests/owner_module_probe.c)
sin_pc_configure(sin_pc_owner_module_probe sin_pc_owner_module_probe)
add_dependencies(sin_pc_owner_module_probe
  sin_pc_server sin_pc_client sin_pc_fake_server sin_pc_fake_client)

add_test(NAME sin_pc_original_module_exports_only
  COMMAND $<TARGET_FILE:sin_pc_owner_module_probe>)
add_test(NAME sin_pc_original_module_factory_handshake
  COMMAND $<TARGET_FILE:sin_pc_owner_module_probe> --query-factories)
add_test(NAME sin_pc_original_module_factory_traced
  COMMAND ${CMAKE_COMMAND} -E env SIN_PC_PROXY_TRACE=1
    $<TARGET_FILE:sin_pc_owner_module_probe> --query-factories)
set_tests_properties(
  sin_pc_original_module_exports_only
  sin_pc_original_module_factory_handshake
  sin_pc_original_module_factory_traced
  PROPERTIES WORKING_DIRECTORY "$<TARGET_FILE_DIR:sin_pc_owner_module_probe>")


# Phase203: fail-closed PowerShell staging safety test, using ONLY synthetic
# CI binaries. Script parses but must reject fake original SHA256s and any
# mutation without explicit user permission; no commercial DLLs in CI.
add_test(NAME sin_pc_original_owner_staging_fail_closed
  COMMAND powershell.exe -NoProfile -ExecutionPolicy Bypass
    -File "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_staging_script.ps1"
    -ScriptPath "${CMAKE_CURRENT_SOURCE_DIR}/Prepare-SiNWin32ProxyTest.ps1"
    -GameBin "$<TARGET_FILE_DIR:sin_pc_owner_module_probe>")
set_tests_properties(sin_pc_original_owner_staging_fail_closed
  PROPERTIES WORKING_DIRECTORY "$<TARGET_FILE_DIR:sin_pc_owner_module_probe>")


# Phase205: genuine Win32 x86/MSVC validation of the paired original
# SiN server/client allocator wrapper model (synthetic tier0 callbacks).
add_executable(sin_pc_original_cross_dll_tier0_allocator_tests
  ../../tests/test_original_cross_dll_tier0_allocator.cpp)
target_include_directories(sin_pc_original_cross_dll_tier0_allocator_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_cross_dll_tier0_allocator_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_cross_dll_tier0_allocator_tests
  sin_pc_original_cross_dll_tier0_allocator_tests)
add_test(NAME sin_original_win32_cross_dll_tier0_allocator_contract
  COMMAND $<TARGET_FILE:sin_pc_original_cross_dll_tier0_allocator_tests>)


# Phase206: true Windows i386/MSVC compile and execution of synthetic
# original server/client zeroed allocation and opaque tier0 vcall paths.
add_executable(sin_pc_original_tier0_zero_allocation_tests
  ../../tests/test_original_tier0_zero_allocation.cpp)
target_include_directories(sin_pc_original_tier0_zero_allocation_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_tier0_zero_allocation_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_tier0_zero_allocation_tests
  sin_pc_original_tier0_zero_allocation_tests)
add_test(NAME sin_original_win32_tier0_zero_allocation_contract
  COMMAND $<TARGET_FILE:sin_pc_original_tier0_zero_allocation_tests>)


# Phase207: native Win32 MSVC i386 CTest for original duplicate allocator
# wrappers and opaque two-argument tier0 virtual dispatch contract.
add_executable(sin_pc_original_tier0_allocator_aliases_tests
  ../../tests/test_original_tier0_allocator_aliases.cpp)
target_include_directories(sin_pc_original_tier0_allocator_aliases_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_tier0_allocator_aliases_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_tier0_allocator_aliases_tests
  sin_pc_original_tier0_allocator_aliases_tests)
add_test(NAME sin_original_win32_tier0_allocator_aliases_contract
  COMMAND $<TARGET_FILE:sin_pc_original_tier0_allocator_aliases_tests>)


# Phase208: real Win32 x86/MSVC test target, synthetic original tier0
# virtual-method stubs and native layout constants; no real game DLLs in CI.
add_executable(sin_pc_original_tier0_allocator_bridges_tests
  ../../tests/test_original_tier0_allocator_bridges.cpp)
target_include_directories(sin_pc_original_tier0_allocator_bridges_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_tier0_allocator_bridges_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_tier0_allocator_bridges_tests
  sin_pc_original_tier0_allocator_bridges_tests)
add_test(NAME sin_original_win32_tier0_allocator_bridges_contract
  COMMAND $<TARGET_FILE:sin_pc_original_tier0_allocator_bridges_tests>)


# Phase209: native MSVC Win32 i386 validation of original server/client
# allocator callers and their host-owned synthetic game objects/buffers.
add_executable(sin_pc_original_allocator_game_callers_tests
  ../../tests/test_original_allocator_game_callers.cpp)
target_include_directories(sin_pc_original_allocator_game_callers_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_allocator_game_callers_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_allocator_game_callers_tests
  sin_pc_original_allocator_game_callers_tests)
add_test(NAME sin_original_win32_allocator_game_callers_contract
  COMMAND $<TARGET_FILE:sin_pc_original_allocator_game_callers_tests>)


# Phase210: real Win32 x86/MSVC regression for original client prediction
# setup order and SaveData buffer slots, with independent fake host callbacks.
add_executable(sin_pc_original_client_prediction_savedata_tests
  ../../tests/test_original_client_prediction_savedata.cpp)
target_include_directories(sin_pc_original_client_prediction_savedata_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_client_prediction_savedata_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_client_prediction_savedata_tests
  sin_pc_original_client_prediction_savedata_tests)
add_test(NAME sin_original_win32_client_prediction_savedata_contract
  COMMAND $<TARGET_FILE:sin_pc_original_client_prediction_savedata_tests>)


# Phase211: genuine Win32 i386/MSVC synthetic-host original prediction
# snapshot argument-block, counter and linked record dispatch validation.
add_executable(sin_pc_original_client_prediction_snapshot_dispatch_tests
  ../../tests/test_original_client_prediction_snapshot_dispatch.cpp)
target_include_directories(sin_pc_original_client_prediction_snapshot_dispatch_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_client_prediction_snapshot_dispatch_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_client_prediction_snapshot_dispatch_tests
  sin_pc_original_client_prediction_snapshot_dispatch_tests)
add_test(NAME sin_original_win32_client_prediction_snapshot_dispatch_contract
  COMMAND $<TARGET_FILE:sin_pc_original_client_prediction_snapshot_dispatch_tests>)


# Phase212: original native record type routing under actual Win32 x86
# MSVC. Synthetic helper callbacks only; no original retail gameplay.
add_executable(sin_pc_original_client_snapshot_record_router_tests
  ../../tests/test_original_client_snapshot_record_router.cpp)
target_include_directories(sin_pc_original_client_snapshot_record_router_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_client_snapshot_record_router_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_client_snapshot_record_router_tests
  sin_pc_original_client_snapshot_record_router_tests)
add_test(NAME sin_original_win32_client_snapshot_record_router_contract
  COMMAND $<TARGET_FILE:sin_pc_original_client_snapshot_record_router_tests>)


# Phase213: genuine Windows i386/MSVC synthetic-host validation of 4
# original snapshot field-copy functions (not a retail engine launch).
add_executable(sin_pc_original_client_snapshot_field_copy_tests
  ../../tests/test_original_client_snapshot_field_copy.cpp)
target_include_directories(sin_pc_original_client_snapshot_field_copy_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_client_snapshot_field_copy_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_client_snapshot_field_copy_tests
  sin_pc_original_client_snapshot_field_copy_tests)
add_test(NAME sin_original_win32_client_snapshot_field_copy_contract
  COMMAND $<TARGET_FILE:sin_pc_original_client_snapshot_field_copy_tests>)

# Phase215: run original 2006 type-2 client snapshot specialist host reference
# with real native Win32 x86 MSVC. No retail engine module is loaded.
add_executable(sin_pc_original_snapshot_type2_tests
  ../../tests/test_original_client_snapshot_type2.cpp)
target_include_directories(sin_pc_original_snapshot_type2_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_snapshot_type2_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_snapshot_type2_tests
  sin_pc_original_snapshot_type2_tests)
add_test(NAME sin_original_win32_snapshot_type2_specialists
  COMMAND $<TARGET_FILE:sin_pc_original_snapshot_type2_tests>)

# Phase216: MSVC Win32 x86 synthetic host reference of original
# type-1 float comparator/trace methods, NOT retail engine startup.
add_executable(sin_pc_original_snapshot_type1_tests
  ../../tests/test_original_client_snapshot_type1.cpp)
target_include_directories(sin_pc_original_snapshot_type1_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_snapshot_type1_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_snapshot_type1_tests
  sin_pc_original_snapshot_type1_tests)
add_test(NAME sin_original_win32_snapshot_type1_specialists
  COMMAND $<TARGET_FILE:sin_pc_original_snapshot_type1_tests>)

# Phase217: genuine MSVC Win32 x86 compilation/test of original-fidelity
# snapshot binary/int host reference; NO actual SiN engine loaded.
add_executable(sin_pc_original_snapshot_binary_integer_tests
  ../../tests/test_original_client_snapshot_binary_integer.cpp)
target_include_directories(sin_pc_original_snapshot_binary_integer_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_snapshot_binary_integer_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_snapshot_binary_integer_tests
  sin_pc_original_snapshot_binary_integer_tests)
add_test(NAME sin_original_win32_snapshot_binary_integer_specialists
  COMMAND $<TARGET_FILE:sin_pc_original_snapshot_binary_integer_tests>)

# Phase218: native MSVC Win32 x86 CTest of the original 2006 client type7
# signed-short field helper host reconstruction. No actual SiN engine loaded.
add_executable(sin_pc_original_snapshot_short_tests
  ../../tests/test_original_client_snapshot_short.cpp)
target_include_directories(sin_pc_original_snapshot_short_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_snapshot_short_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_snapshot_short_tests
  sin_pc_original_snapshot_short_tests)
add_test(NAME sin_original_win32_snapshot_short_specialists
  COMMAND $<TARGET_FILE:sin_pc_original_snapshot_short_tests>)

# Phase219 native MSVC Win32 x86 compilation of independent original Boolean
# snapshot reference; not actual retail game engine gameplay DLLs.
add_executable(sin_pc_original_snapshot_boolean_tests
  ../../tests/test_original_client_snapshot_boolean.cpp)
target_include_directories(sin_pc_original_snapshot_boolean_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_snapshot_boolean_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_snapshot_boolean_tests
  sin_pc_original_snapshot_boolean_tests)
add_test(NAME sin_original_win32_snapshot_boolean_specialists
  COMMAND $<TARGET_FILE:sin_pc_original_snapshot_boolean_tests>)

# Phase220 native MSVC Win32 x86 test of clean-room original DWORD helpers.
add_executable(sin_pc_original_snapshot_dword_tests
  ../../tests/test_original_client_snapshot_dword.cpp)
target_include_directories(sin_pc_original_snapshot_dword_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_snapshot_dword_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_snapshot_dword_tests
  sin_pc_original_snapshot_dword_tests)
add_test(NAME sin_original_win32_snapshot_dword_specialists
  COMMAND $<TARGET_FILE:sin_pc_original_snapshot_dword_tests>)

# Phase221: actual MSVC native Win32 x86 compilation of the independent
# original type3/4 fixed-stride copy reference, not the retail game engine.
add_executable(sin_pc_original_vector_copy_tests
  ../../tests/test_original_client_snapshot_vector_copy.cpp)
target_include_directories(sin_pc_original_vector_copy_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_vector_copy_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_vector_copy_tests
  sin_pc_original_vector_copy_tests)
add_test(NAME sin_original_win32_vector_copy_specialists
  COMMAND $<TARGET_FILE:sin_pc_original_vector_copy_tests>)

# Phase222: genuine MSVC native Win32 x86 synthetic vec3 specialist host
# tests, not a reconstructed game DLL running in original engine.
add_executable(sin_pc_original_snapshot_vec3_tests
  ../../tests/test_original_client_snapshot_vec3.cpp)
target_include_directories(sin_pc_original_snapshot_vec3_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_snapshot_vec3_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_snapshot_vec3_tests
  sin_pc_original_snapshot_vec3_tests)
add_test(NAME sin_original_win32_snapshot_vec3_specialists
  COMMAND $<TARGET_FILE:sin_pc_original_snapshot_vec3_tests>)

# Phase223: true MSVC Win32 x86 host regression of original quaternion
# serializer helpers. This does NOT load retail gameplay DLLs.
add_executable(sin_pc_original_snapshot_quaternion_tests
  ../../tests/test_original_client_snapshot_quaternion.cpp)
target_include_directories(sin_pc_original_snapshot_quaternion_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_snapshot_quaternion_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_snapshot_quaternion_tests
  sin_pc_original_snapshot_quaternion_tests)
add_test(NAME sin_original_win32_snapshot_quaternion_specialists
  COMMAND $<TARGET_FILE:sin_pc_original_snapshot_quaternion_tests>)

# Phase224: native MSVC Win32 x86 original EHandle host regression,
# not a real original Source engine client.dll/server.dll runtime.
add_executable(sin_pc_original_snapshot_ehandle_tests
  ../../tests/test_original_client_snapshot_ehandle.cpp)
target_include_directories(sin_pc_original_snapshot_ehandle_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_snapshot_ehandle_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_snapshot_ehandle_tests
  sin_pc_original_snapshot_ehandle_tests)
add_test(NAME sin_original_win32_snapshot_ehandle_specialists
  COMMAND $<TARGET_FILE:sin_pc_original_snapshot_ehandle_tests>)

# Phase225: genuine Win32 x86 MSVC compilation/test of independent
# common original trace models. No rebuilt game DLL is engine-loaded.
add_executable(sin_pc_original_client_shared_trace_tests
  ../../tests/test_original_client_shared_trace.cpp)
target_include_directories(sin_pc_original_client_shared_trace_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_client_shared_trace_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_client_shared_trace_tests
  sin_pc_original_client_shared_trace_tests)
add_test(NAME sin_original_win32_shared_trace_formatters
  COMMAND $<TARGET_FILE:sin_pc_original_client_shared_trace_tests>)

# Phase226: genuine MSVC Win32 x86 host test of original singleton factory
# return token and HIGHLOW relocation calculations; not engine startup.
add_executable(sin_pc_original_core_factory_thunks_tests
  ../../tests/test_original_core_factory_thunks.cpp)
target_include_directories(sin_pc_original_core_factory_thunks_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_core_factory_thunks_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_core_factory_thunks_tests
  sin_pc_original_core_factory_thunks_tests)
add_test(NAME sin_original_win32_core_factory_thunks
  COMMAND $<TARGET_FILE:sin_pc_original_core_factory_thunks_tests>)

# Phase227: native Win32 MSVC x86 synthetic model of original PE .data
# initializer callbacks, vptr overwrites and singleton destruction.
add_executable(sin_pc_original_singleton_init_tests
  ../../tests/test_original_runtime_singleton_initializers.cpp)
target_include_directories(sin_pc_original_singleton_init_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_singleton_init_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_singleton_init_tests
  sin_pc_original_singleton_init_tests)
add_test(NAME sin_original_win32_runtime_singleton_initializers
  COMMAND $<TARGET_FILE:sin_pc_original_singleton_init_tests>)

# Phase228: genuine native MSVC Win32 x86 tests of original embedded
# secondary interface return addresses and constructor/atexit events.
add_executable(sin_pc_original_embedded_interface_tests
  ../../tests/test_original_embedded_interface_initializers.cpp)
target_include_directories(sin_pc_original_embedded_interface_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_embedded_interface_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_embedded_interface_tests
  sin_pc_original_embedded_interface_tests)
add_test(NAME sin_original_win32_embedded_interface_initializers
  COMMAND $<TARGET_FILE:sin_pc_original_embedded_interface_tests>)

# Phase229: real MSVC Win32 x86 host regression of original embedded
# interface vtable slot semantics, not actual gameplay module startup.
add_executable(sin_pc_original_embedded_virtual_tests
  ../../tests/test_original_embedded_virtual_accessors.cpp)
target_include_directories(sin_pc_original_embedded_virtual_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_embedded_virtual_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_embedded_virtual_tests
  sin_pc_original_embedded_virtual_tests)
add_test(NAME sin_original_win32_embedded_virtual_accessors
  COMMAND $<TARGET_FILE:sin_pc_original_embedded_virtual_tests>)

# Phase230: real Win32 MSVC x86 build/run of independent original
# VClientEntityList003 EHandle and index-dispatch host reference.
add_executable(sin_pc_original_client_entitylist_dispatch_tests
  ../../tests/test_original_client_entitylist_handle_dispatch.cpp)
target_include_directories(sin_pc_original_client_entitylist_dispatch_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_client_entitylist_dispatch_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_client_entitylist_dispatch_tests
  sin_pc_original_client_entitylist_dispatch_tests)
add_test(NAME sin_original_win32_client_entitylist_handle_dispatch
  COMMAND $<TARGET_FILE:sin_pc_original_client_entitylist_dispatch_tests>)

# Phase231: native MSVC Win32 x86 regression for original parent
# entity-list registration/removal/deleting destructor event reference.
add_executable(sin_pc_original_client_entitylist_parent_tests
  ../../tests/test_original_client_entitylist_parent_lifecycle.cpp)
target_include_directories(sin_pc_original_client_entitylist_parent_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_client_entitylist_parent_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_client_entitylist_parent_tests
  sin_pc_original_client_entitylist_parent_tests)
add_test(NAME sin_original_win32_client_entitylist_parent_lifecycle
  COMMAND $<TARGET_FILE:sin_pc_original_client_entitylist_parent_tests>)

# Phase232: native Win32 MSVC x86 tests of original-free PVS notifier
# callback sequence and 8-byte auxiliary list link/unlink semantics.
add_executable(sin_pc_original_client_pvs_aux_helpers_tests
  ../../tests/test_original_client_pvs_aux_helpers.cpp)
target_include_directories(sin_pc_original_client_pvs_aux_helpers_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_client_pvs_aux_helpers_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_client_pvs_aux_helpers_tests
  sin_pc_original_client_pvs_aux_helpers_tests)
add_test(NAME sin_original_win32_client_pvs_aux_helpers
  COMMAND $<TARGET_FILE:sin_pc_original_client_pvs_aux_helpers_tests>)

# Phase233: actual native MSVC Win32 x86 CTest for independent PVS
# sixteen-byte linked-list index append/unlink model.
add_executable(sin_pc_original_client_pvs_sixteen_tests
  ../../tests/test_original_client_pvs_sixteen_list.cpp)
target_include_directories(sin_pc_original_client_pvs_sixteen_tests PRIVATE ../../include)
set_target_properties(sin_pc_original_client_pvs_sixteen_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_client_pvs_sixteen_tests
  sin_pc_original_client_pvs_sixteen_tests)
add_test(NAME sin_original_win32_client_pvs_sixteen_list
  COMMAND $<TARGET_FILE:sin_pc_original_client_pvs_sixteen_tests>)


# Phase234: native MSVC Win32 x86 host-reference regression for the two
# original client PVS index allocators. Synthetic storage, NOT gameplay DLLs.
add_executable(sin_pc_original_client_pvs_index_allocators_tests
  ../../tests/test_original_client_pvs_index_allocators.cpp)
target_include_directories(sin_pc_original_client_pvs_index_allocators_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_client_pvs_index_allocators_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_client_pvs_index_allocators_tests
  sin_pc_original_client_pvs_index_allocators_tests)
add_test(NAME sin_original_win32_client_pvs_index_allocators_contract
  COMMAND $<TARGET_FILE:sin_pc_original_client_pvs_index_allocators_tests>)


# Phase235: actual MSVC Win32 x86 compilation of the independent client
# PVS 8/16-byte growth model against synthetic storage/backend callbacks.
# This does NOT run the original game or replace its gameplay DLLs.
add_executable(sin_pc_original_client_pvs_storage_growth_tests
  ../../tests/test_original_client_pvs_storage_growth.cpp)
target_include_directories(sin_pc_original_client_pvs_storage_growth_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_client_pvs_storage_growth_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_client_pvs_storage_growth_tests
  sin_pc_original_client_pvs_storage_growth_tests)
add_test(NAME sin_original_win32_client_pvs_storage_growth_contract
  COMMAND $<TARGET_FILE:sin_pc_original_client_pvs_storage_growth_tests>)


# Phase236: independent original x86 client PVS map frontend host contract.
# Native MSVC Win32 x86 tests with synthetic callbacks, not game execution.
add_executable(sin_pc_original_client_pvs_map_frontends_tests
  ../../tests/test_original_client_pvs_map_frontends.cpp)
target_include_directories(sin_pc_original_client_pvs_map_frontends_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_client_pvs_map_frontends_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_client_pvs_map_frontends_tests
  sin_pc_original_client_pvs_map_frontends_tests)
add_test(NAME sin_original_win32_client_pvs_map_frontends_contract
  COMMAND $<TARGET_FILE:sin_pc_original_client_pvs_map_frontends_tests>)


# Phase237: compile original PVS map placement/index host contracts under real
# MSVC Win32 x86, with synthetic callbacks (not original game startup).
add_executable(sin_pc_original_client_pvs_map_search_index_tests
  ../../tests/test_original_client_pvs_map_search_index.cpp)
target_include_directories(sin_pc_original_client_pvs_map_search_index_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_client_pvs_map_search_index_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_client_pvs_map_search_index_tests
  sin_pc_original_client_pvs_map_search_index_tests)
add_test(NAME sin_original_win32_client_pvs_map_search_index_contract
  COMMAND $<TARGET_FILE:sin_pc_original_client_pvs_map_search_index_tests>)


# Phase238: native MSVC Win32 x86 test of the standalone 2006 client PVS
# map node resolver / 16-byte growth host model, not original game loading.
add_executable(sin_pc_original_client_pvs_map_node_growth_tests
  ../../tests/test_original_client_pvs_map_node_growth.cpp)
target_include_directories(sin_pc_original_client_pvs_map_node_growth_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_client_pvs_map_node_growth_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_client_pvs_map_node_growth_tests
  sin_pc_original_client_pvs_map_node_growth_tests)
add_test(NAME sin_original_win32_client_pvs_map_node_growth_contract
  COMMAND $<TARGET_FILE:sin_pc_original_client_pvs_map_node_growth_tests>)


# Phase239: native MSVC Win32 x86 test for original-client PVS tree attach
# host model. Does not execute or replace original game-engine modules.
add_executable(sin_pc_original_client_pvs_tree_attach_tests
  ../../tests/test_original_client_pvs_tree_attach.cpp)
target_include_directories(sin_pc_original_client_pvs_tree_attach_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_client_pvs_tree_attach_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_client_pvs_tree_attach_tests
  sin_pc_original_client_pvs_tree_attach_tests)
add_test(NAME sin_original_win32_client_pvs_tree_attach_contract
  COMMAND $<TARGET_FILE:sin_pc_original_client_pvs_tree_attach_tests>)


# Phase240 original-client PVS red-black fixup + rotations host contracts.
# Genuine MSVC Win32 x86 tests; no reconstructed game-engine gameplay boot.
add_executable(sin_pc_original_client_pvs_rb_fixup_tests
  ../../tests/test_original_client_pvs_rb_fixup.cpp)
target_include_directories(sin_pc_original_client_pvs_rb_fixup_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_client_pvs_rb_fixup_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_client_pvs_rb_fixup_tests
  sin_pc_original_client_pvs_rb_fixup_tests)
add_test(NAME sin_original_win32_client_pvs_rb_fixup_contract
  COMMAND $<TARGET_FILE:sin_pc_original_client_pvs_rb_fixup_tests>)


# Phase241: real MSVC Win32 x86 compilation of independent original client
# PVS deletion frontend; no reconstructed game-engine module executed.
add_executable(sin_pc_original_client_pvs_tree_erase_tests
  ../../tests/test_original_client_pvs_tree_erase.cpp)
target_include_directories(sin_pc_original_client_pvs_tree_erase_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_client_pvs_tree_erase_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_client_pvs_tree_erase_tests
  sin_pc_original_client_pvs_tree_erase_tests)
add_test(NAME sin_original_win32_client_pvs_tree_erase_contract
  COMMAND $<TARGET_FILE:sin_pc_original_client_pvs_tree_erase_tests>)


# Phase242: genuine MSVC Win32 x86 build/test of the independent original
# PVS red-black delete fixup host reference, not retail gameplay startup.
add_executable(sin_pc_original_client_pvs_delete_fixup_tests
  ../../tests/test_original_client_pvs_delete_fixup.cpp)
target_include_directories(sin_pc_original_client_pvs_delete_fixup_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_client_pvs_delete_fixup_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_client_pvs_delete_fixup_tests
  sin_pc_original_client_pvs_delete_fixup_tests)
add_test(NAME sin_original_win32_client_pvs_delete_fixup_contract
  COMMAND $<TARGET_FILE:sin_pc_original_client_pvs_delete_fixup_tests>)


# Phase243: native MSVC Win32 x86 host test for original PVS comparator
# and global dispatch adapter. Not original gameplay DLL boot.
add_executable(sin_pc_original_client_pvs_comparator_dispatch_tests
  ../../tests/test_original_client_pvs_comparator_dispatch.cpp)
target_include_directories(sin_pc_original_client_pvs_comparator_dispatch_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_client_pvs_comparator_dispatch_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_client_pvs_comparator_dispatch_tests
  sin_pc_original_client_pvs_comparator_dispatch_tests)
add_test(NAME sin_original_win32_client_pvs_comparator_dispatch_contract
  COMMAND $<TARGET_FILE:sin_pc_original_client_pvs_comparator_dispatch_tests>)


# Phase244: genuine MSVC Win32 x86 tests for independent original
# client PVS indexed-node field and eight-byte free-chain host models.
add_executable(sin_pc_original_client_pvs_node_fields_list_reset_tests
  ../../tests/test_original_client_pvs_node_fields_list_reset.cpp)
target_include_directories(sin_pc_original_client_pvs_node_fields_list_reset_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_client_pvs_node_fields_list_reset_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_client_pvs_node_fields_list_reset_tests
  sin_pc_original_client_pvs_node_fields_list_reset_tests)
add_test(NAME sin_original_win32_client_pvs_node_fields_list_reset_contract
  COMMAND $<TARGET_FILE:sin_pc_original_client_pvs_node_fields_list_reset_tests>)


# Phase245: original 2006 client entity-list drain/destructor and PVS
# index wrappers as genuine MSVC PE32 x86 host-only contract tests.
add_executable(sin_pc_original_client_entitylist_pvs_lifecycle_wrappers_tests
  ../../tests/test_original_client_entitylist_pvs_lifecycle_wrappers.cpp)
target_include_directories(sin_pc_original_client_entitylist_pvs_lifecycle_wrappers_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_client_entitylist_pvs_lifecycle_wrappers_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_client_entitylist_pvs_lifecycle_wrappers_tests
  sin_pc_original_client_entitylist_pvs_lifecycle_wrappers_tests)
add_test(NAME sin_original_win32_client_entitylist_pvs_lifecycle_wrappers_contract
  COMMAND $<TARGET_FILE:sin_pc_original_client_entitylist_pvs_lifecycle_wrappers_tests>)


# Phase247: genuine MSVC Windows x86 host-only contract for retail
# client base entity-list destructor and masked handle removal.
add_executable(sin_pc_original_client_entitylist_base_removal_tests
  ../../tests/test_original_client_entitylist_base_removal.cpp)
target_include_directories(sin_pc_original_client_entitylist_base_removal_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_client_entitylist_base_removal_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_client_entitylist_base_removal_tests
  sin_pc_original_client_entitylist_base_removal_tests)
add_test(NAME sin_original_win32_client_entitylist_base_removal_contract
  COMMAND $<TARGET_FILE:sin_pc_original_client_entitylist_base_removal_tests>)


# Phase248 genuine MSVC PE32 x86 CTest of original client entity-list
# ctor/registration host reference. Not an independent gameplay DLL boot.
add_executable(sin_pc_original_client_entitylist_base_register_tests
  ../../tests/test_original_client_entitylist_base_register.cpp)
target_include_directories(sin_pc_original_client_entitylist_base_register_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_client_entitylist_base_register_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_client_entitylist_base_register_tests
  sin_pc_original_client_entitylist_base_register_tests)
add_test(NAME sin_original_win32_client_entitylist_base_register_contract
  COMMAND $<TARGET_FILE:sin_pc_original_client_entitylist_base_register_tests>)


# Phase249: original 2006 client intrusive-list/vtable native MSVC Win32 x86
# host-only CTest, not original gameplay client.dll replacement startup.
add_executable(sin_pc_original_client_entitylist_intrusive_list_vtable_tests
  ../../tests/test_original_client_entitylist_intrusive_list_vtable.cpp)
target_include_directories(sin_pc_original_client_entitylist_intrusive_list_vtable_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_client_entitylist_intrusive_list_vtable_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_client_entitylist_intrusive_list_vtable_tests
  sin_pc_original_client_entitylist_intrusive_list_vtable_tests)
add_test(NAME sin_original_win32_client_entitylist_intrusive_list_vtable_contract
  COMMAND $<TARGET_FILE:sin_pc_original_client_entitylist_intrusive_list_vtable_tests>)


# Phase250 genuine MSVC Win32 x86 test of original client
# DT_EntityParticleTrailInfo RecvTable and CRT initialization/lifetime.
add_executable(sin_pc_original_client_particle_trail_recvtable_crt_tests
  ../../tests/test_original_client_particle_trail_recvtable_crt.cpp)
target_include_directories(sin_pc_original_client_particle_trail_recvtable_crt_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_client_particle_trail_recvtable_crt_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_client_particle_trail_recvtable_crt_tests
  sin_pc_original_client_particle_trail_recvtable_crt_tests)
add_test(NAME sin_original_win32_client_particle_trail_recvtable_crt_contract
  COMMAND $<TARGET_FILE:sin_pc_original_client_particle_trail_recvtable_crt_tests>)


# Phase251: genuine MSVC x86 native host-contract checks for retail
# client ReceiveProp/ReceiveTable constructors and receive proxies.
add_executable(sin_pc_original_client_recvprop_proxies_factories_tests
  ../../tests/test_original_client_recvprop_proxies_factories.cpp)
target_include_directories(sin_pc_original_client_recvprop_proxies_factories_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_client_recvprop_proxies_factories_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_client_recvprop_proxies_factories_tests
  sin_pc_original_client_recvprop_proxies_factories_tests)
add_test(NAME sin_original_win32_client_recvprop_proxies_factories_contract
  COMMAND $<TARGET_FILE:sin_pc_original_client_recvprop_proxies_factories_tests>)


# Phase279: a real native Win32 x86 tool for owner-original DLL vptr prefix
# *observation only*. Public CI uses ONLY synthetic fake original DLLs.
# Actual original use is explicit, SHA-gated, and never invokes a native
# virtual method whose signature has not been independently recovered.
add_executable(sin_pc_owner_vptr_probe tests/owner_vptr_probe.cpp)
sin_pc_configure(sin_pc_owner_vptr_probe sin_pc_owner_vptr_probe)
set_target_properties(sin_pc_owner_vptr_probe PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
target_link_libraries(sin_pc_owner_vptr_probe PRIVATE bcrypt)
add_dependencies(sin_pc_owner_vptr_probe sin_pc_fake_server sin_pc_fake_client)
add_test(NAME sin_pc_synthetic_server_vptr_readonly_probe
  COMMAND $<TARGET_FILE:sin_pc_owner_vptr_probe>
    --synthetic server $<TARGET_FILE:sin_pc_fake_server>)
add_test(NAME sin_pc_synthetic_client_vptr_readonly_probe
  COMMAND $<TARGET_FILE:sin_pc_owner_vptr_probe>
    --synthetic client $<TARGET_FILE:sin_pc_fake_client>)
set_tests_properties(
  sin_pc_synthetic_server_vptr_readonly_probe
  sin_pc_synthetic_client_vptr_readonly_probe
  PROPERTIES WORKING_DIRECTORY "$<TARGET_FILE_DIR:sin_pc_owner_vptr_probe>")
add_test(NAME sin_pc_owner_vptr_rejects_fake_original_by_sha256
  COMMAND powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass
    -File "${CMAKE_CURRENT_SOURCE_DIR}/tests/Test-OwnerVptrHashGate.ps1"
    -Probe "$<TARGET_FILE:sin_pc_owner_vptr_probe>"
    -FakeDll "$<TARGET_FILE:sin_pc_fake_server>" -Role server)


# Phase613: compile the ORIGINAL-FREE Grunt gameplay bridge as real Win32 PE32
# and verify its exported undecorated C ABI through LoadLibrary/GetProcAddress.
# Not the original game's CreateInterface server.dll and no retail code.
add_library(sin_pc_grunt_behavior_c_abi SHARED
  ../../src/grunt_behavior_c_api_v1.cpp)
target_include_directories(sin_pc_grunt_behavior_c_abi PRIVATE ../../include)
set_target_properties(sin_pc_grunt_behavior_c_abi PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
if(MSVC)
  target_sources(sin_pc_grunt_behavior_c_abi PRIVATE
    ../../src/grunt_behavior_win32.def)
endif()
sin_pc_configure(sin_pc_grunt_behavior_c_abi sin_grunt_behavior_c_abi)

add_executable(sin_pc_grunt_behavior_dynload_phase613
  ../../tests/test_grunt_behavior_dynload_phase613.cpp)
target_include_directories(sin_pc_grunt_behavior_dynload_phase613 PRIVATE
  ../../include)
set_target_properties(sin_pc_grunt_behavior_dynload_phase613 PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_grunt_behavior_dynload_phase613
  sin_pc_grunt_behavior_dynload_phase613)
if(MSVC)
  target_compile_options(sin_pc_grunt_behavior_dynload_phase613 PRIVATE
    /UNDEBUG)
else()
  target_compile_options(sin_pc_grunt_behavior_dynload_phase613 PRIVATE
    -UNDEBUG)
endif()
add_dependencies(sin_pc_grunt_behavior_dynload_phase613
  sin_pc_grunt_behavior_c_abi)
add_test(NAME sin_pc_grunt_behavior_win32_dynload_phase613
  COMMAND $<TARGET_FILE:sin_pc_grunt_behavior_dynload_phase613>
    $<TARGET_FILE:sin_pc_grunt_behavior_c_abi>)

add_executable(sin_pc_grunt_behavior_c_abi_phase613_tests
  ../../tests/test_grunt_behavior_c_api_v1.cpp)
target_include_directories(sin_pc_grunt_behavior_c_abi_phase613_tests
  PRIVATE ../../include)
target_link_libraries(sin_pc_grunt_behavior_c_abi_phase613_tests
  PRIVATE sin_pc_grunt_behavior_c_abi)
set_target_properties(sin_pc_grunt_behavior_c_abi_phase613_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_grunt_behavior_c_abi_phase613_tests
  sin_pc_grunt_behavior_c_abi_phase613_tests)
if(MSVC)
  target_compile_options(sin_pc_grunt_behavior_c_abi_phase613_tests PRIVATE
    /UNDEBUG)
else()
  target_compile_options(sin_pc_grunt_behavior_c_abi_phase613_tests PRIVATE
    -UNDEBUG)
endif()
add_test(NAME sin_pc_grunt_behavior_win32_host_phase613
  COMMAND $<TARGET_FILE:sin_pc_grunt_behavior_c_abi_phase613_tests>)

# Phase614: attach only to a verified existing native Druglab Grunt NPC.
# Independent per-NPC behavior sessions; no fake original Source factories.
add_library(sin_pc_grunt_npc_session_registry_phase614 STATIC
  ../../src/original_grunt_npc_session_registry_phase614.cpp)
target_include_directories(sin_pc_grunt_npc_session_registry_phase614
  PUBLIC ../../include)
target_link_libraries(sin_pc_grunt_npc_session_registry_phase614
  PUBLIC sin_pc_grunt_behavior_c_abi)
set_target_properties(sin_pc_grunt_npc_session_registry_phase614 PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_grunt_npc_session_registry_phase614
  sin_pc_grunt_npc_session_registry_phase614)

add_executable(sin_pc_grunt_npc_session_registry_phase614_tests
  ../../tests/test_original_grunt_npc_session_registry_phase614.cpp)
target_include_directories(sin_pc_grunt_npc_session_registry_phase614_tests
  PRIVATE ../../include)
target_link_libraries(sin_pc_grunt_npc_session_registry_phase614_tests
  PRIVATE sin_pc_grunt_npc_session_registry_phase614)
set_target_properties(sin_pc_grunt_npc_session_registry_phase614_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_grunt_npc_session_registry_phase614_tests
  sin_pc_grunt_npc_session_registry_phase614_tests)
if(MSVC)
  target_compile_options(sin_pc_grunt_npc_session_registry_phase614_tests
    PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_grunt_npc_session_registry_phase614_tests
    PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_grunt_native_NPC_session_registry_phase614
  COMMAND $<TARGET_FILE:sin_pc_grunt_npc_session_registry_phase614_tests>)

# Phase615: genuine Win32 server proxy optionally loads the independent Grunt
# gameplay C ABI from its FIXED sibling path after original factory startup.
# In all cases it continues to return the original SiN CreateInterface object.
add_library(sin_pc_bad_grunt_abi_phase615 SHARED
  tests/fake_grunt_bad_version_phase615.c
  tests/fake_grunt_bad_version_phase615.def)
target_include_directories(sin_pc_bad_grunt_abi_phase615 PRIVATE ../../include)
sin_pc_configure(sin_pc_bad_grunt_abi_phase615 sin_grunt_bad_version_phase615)

add_executable(sin_pc_grunt_proxy_bridge_phase615_tests
  tests/test_grunt_proxy_bridge_phase615.c)
sin_pc_configure(sin_pc_grunt_proxy_bridge_phase615_tests
  sin_pc_grunt_proxy_bridge_phase615_tests)
add_dependencies(sin_pc_grunt_proxy_bridge_phase615_tests
  sin_pc_server sin_pc_fake_server sin_pc_grunt_behavior_c_abi
  sin_pc_bad_grunt_abi_phase615)
foreach(sin_grunt_probe_mode IN ITEMS disabled valid missing wrong-version)
  add_test(NAME sin_pc_grunt_proxy_bridge_phase615_${sin_grunt_probe_mode}
    COMMAND $<TARGET_FILE:sin_pc_grunt_proxy_bridge_phase615_tests>
      ${sin_grunt_probe_mode})
  set_tests_properties(sin_pc_grunt_proxy_bridge_phase615_${sin_grunt_probe_mode}
    PROPERTIES
      WORKING_DIRECTORY "$<TARGET_FILE_DIR:sin_pc_grunt_proxy_bridge_phase615_tests>")
endforeach()

# Phase617: original native Assault enter +0x1C/+0x28 RET, +0x24 bridge.
add_executable(sin_pc_original_assault_activation_phase617_tests
  ../../tests/test_original_grunt_assault_activation_phase617.cpp)
target_include_directories(sin_pc_original_assault_activation_phase617_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_original_assault_activation_phase617_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_assault_activation_phase617_tests
  sin_pc_original_assault_activation_phase617_tests)
if(MSVC)
  target_compile_options(sin_pc_original_assault_activation_phase617_tests
    PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_assault_activation_phase617_tests
    PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_assault_activation_phase617
  COMMAND $<TARGET_FILE:sin_pc_original_assault_activation_phase617_tests>)

# Phase619: callable C host adapter (seven undecorated exports) for an
# ALREADY engine-created native Grunt. Phase613's five-export behavior DLL
# stays binary-identical; original server.dll still owns CreateInterface.
add_library(sin_pc_grunt_native_host_c_abi_phase619 SHARED
  ../../src/grunt_native_host_c_api_v1.cpp)
target_include_directories(sin_pc_grunt_native_host_c_abi_phase619
  PRIVATE ../../include)
target_link_libraries(sin_pc_grunt_native_host_c_abi_phase619
  PRIVATE sin_pc_grunt_npc_session_registry_phase614)
set_target_properties(sin_pc_grunt_native_host_c_abi_phase619 PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
if(MSVC)
  target_sources(sin_pc_grunt_native_host_c_abi_phase619 PRIVATE
    ../../src/grunt_native_host_win32.def)
endif()
sin_pc_configure(sin_pc_grunt_native_host_c_abi_phase619
  sin_grunt_native_host_c_abi)

add_executable(sin_pc_grunt_native_host_c_api_phase619_tests
  ../../tests/test_grunt_native_host_c_api_phase619.c)
target_include_directories(sin_pc_grunt_native_host_c_api_phase619_tests
  PRIVATE ../../include)
target_link_libraries(sin_pc_grunt_native_host_c_api_phase619_tests
  PRIVATE sin_pc_grunt_native_host_c_abi_phase619)
sin_pc_configure(sin_pc_grunt_native_host_c_api_phase619_tests
  sin_pc_grunt_native_host_c_api_phase619_tests)
if(MSVC)
  target_compile_options(sin_pc_grunt_native_host_c_api_phase619_tests
    PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_grunt_native_host_c_api_phase619_tests
    PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_grunt_native_host_c_api_phase619
  COMMAND $<TARGET_FILE:sin_pc_grunt_native_host_c_api_phase619_tests>)

add_executable(sin_pc_grunt_native_host_dynload_phase619_tests
  ../../tests/test_grunt_native_host_dynload_phase619.cpp)
target_include_directories(sin_pc_grunt_native_host_dynload_phase619_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_grunt_native_host_dynload_phase619_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_grunt_native_host_dynload_phase619_tests
  sin_pc_grunt_native_host_dynload_phase619_tests)
if(MSVC)
  target_compile_options(sin_pc_grunt_native_host_dynload_phase619_tests
    PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_grunt_native_host_dynload_phase619_tests
    PRIVATE -UNDEBUG)
endif()
add_dependencies(sin_pc_grunt_native_host_dynload_phase619_tests
  sin_pc_grunt_native_host_c_abi_phase619)
add_test(NAME sin_pc_grunt_native_host_dynload_phase619
  COMMAND $<TARGET_FILE:sin_pc_grunt_native_host_dynload_phase619_tests>
    $<TARGET_FILE:sin_pc_grunt_native_host_c_abi_phase619>)

# Phase620: original server factory forwards identically while an OPTIONAL
# seven-export native-host DLL probe loads or safely rejects a sibling module.
# NO NPC constructor, factory hook, session attach, or Source vtable calls.
add_library(sin_pc_grunt_native_bad_version_phase620 SHARED
  tests/fake_grunt_native_host_phase620.c
  tests/fake_grunt_native_bad_version_phase620.def)
sin_pc_configure(sin_pc_grunt_native_bad_version_phase620
  sin_grunt_native_bad_version_phase620)
add_library(sin_pc_grunt_native_missing_export_phase620 SHARED
  tests/fake_grunt_native_host_phase620.c
  tests/fake_grunt_native_missing_export_phase620.def)
target_compile_definitions(sin_pc_grunt_native_missing_export_phase620
  PRIVATE SIN_NATIVE_FAKE_MISSING_EXPORT=1)
sin_pc_configure(sin_pc_grunt_native_missing_export_phase620
  sin_grunt_native_missing_export_phase620)

add_executable(sin_pc_grunt_native_host_proxy_phase620_tests
  tests/test_grunt_native_host_proxy_phase620.c)
sin_pc_configure(sin_pc_grunt_native_host_proxy_phase620_tests
  sin_pc_grunt_native_host_proxy_phase620_tests)
add_dependencies(sin_pc_grunt_native_host_proxy_phase620_tests
  sin_pc_server sin_pc_fake_server sin_pc_grunt_behavior_c_abi
  sin_pc_grunt_native_host_c_abi_phase619
  sin_pc_grunt_native_bad_version_phase620
  sin_pc_grunt_native_missing_export_phase620)
foreach(sin_native_host_probe_mode IN ITEMS
    disabled valid missing wrong-version missing-export)
  add_test(NAME sin_pc_grunt_native_host_proxy_phase620_${sin_native_host_probe_mode}
    COMMAND $<TARGET_FILE:sin_pc_grunt_native_host_proxy_phase620_tests>
      ${sin_native_host_probe_mode})
  set_tests_properties(
    sin_pc_grunt_native_host_proxy_phase620_${sin_native_host_probe_mode}
    PROPERTIES WORKING_DIRECTORY
      "$<TARGET_FILE_DIR:sin_pc_grunt_native_host_proxy_phase620_tests>")
endforeach()

# Phase623: actually compile and execute the source-authenticated 2006
# EliteGrunt::Spawn clean-room reference on MSVC x86, not only Linux.
# It never allocates a Source NPC or calls the original retail DLL.
add_library(sin_pc_original_elite_grunt_spawn_phase623 STATIC
  ../../src/original_elite_grunt_spawn_phase623.cpp)
target_include_directories(sin_pc_original_elite_grunt_spawn_phase623
  PUBLIC ../../include)
set_target_properties(sin_pc_original_elite_grunt_spawn_phase623 PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_elite_grunt_spawn_phase623
  sin_pc_original_elite_grunt_spawn_phase623)

add_executable(sin_pc_original_elite_grunt_spawn_phase623_tests
  ../../tests/test_original_elite_grunt_spawn_phase623.cpp)
target_link_libraries(sin_pc_original_elite_grunt_spawn_phase623_tests
  PRIVATE sin_pc_original_elite_grunt_spawn_phase623)
set_target_properties(sin_pc_original_elite_grunt_spawn_phase623_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_elite_grunt_spawn_phase623_tests
  sin_pc_original_elite_grunt_spawn_phase623_tests)
if(MSVC)
  target_compile_options(sin_pc_original_elite_grunt_spawn_phase623_tests
    PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_elite_grunt_spawn_phase623_tests
    PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_elite_grunt_spawn_phase623_native_x86
  COMMAND $<TARGET_FILE:sin_pc_original_elite_grunt_spawn_phase623_tests>)

# Phase626: true MSVC Win32 x86 shared-library implementation of the
# separate eight-export Elite native host C ABI. It binds original-source
# Elite Spawn to event 0x43 for already engine-owned NPCs only.
# No Source factory CreateInterface replacement or vtable guesses.
add_library(sin_pc_elite_native_host_c_abi_phase626 SHARED
  ../../src/elite_native_host_c_api_v1.cpp
  ../../src/original_elite_grunt_spawn_phase623.cpp
  ../../src/elite_native_host_win32.def)
target_include_directories(sin_pc_elite_native_host_c_abi_phase626
  PUBLIC ../../include)
set_target_properties(sin_pc_elite_native_host_c_abi_phase626 PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_elite_native_host_c_abi_phase626
  sin_elite_native_host_c_abi)

add_executable(sin_pc_elite_native_host_c_api_phase626_tests
  ../../tests/test_elite_native_host_c_api_phase626.c)
target_include_directories(sin_pc_elite_native_host_c_api_phase626_tests
  PRIVATE ../../include)
target_link_libraries(sin_pc_elite_native_host_c_api_phase626_tests
  PRIVATE sin_pc_elite_native_host_c_abi_phase626)
sin_pc_configure(sin_pc_elite_native_host_c_api_phase626_tests
  sin_pc_elite_native_host_c_api_phase626_tests)
if(MSVC)
  target_compile_options(sin_pc_elite_native_host_c_api_phase626_tests
    PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_elite_native_host_c_api_phase626_tests
    PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_elite_native_host_c_api_phase626
  COMMAND $<TARGET_FILE:sin_pc_elite_native_host_c_api_phase626_tests>)

add_executable(sin_pc_elite_native_host_dynload_phase626_tests
  ../../tests/test_elite_native_host_dynload_phase626.cpp)
target_include_directories(sin_pc_elite_native_host_dynload_phase626_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_elite_native_host_dynload_phase626_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_elite_native_host_dynload_phase626_tests
  sin_pc_elite_native_host_dynload_phase626_tests)
if(MSVC)
  target_compile_options(sin_pc_elite_native_host_dynload_phase626_tests
    PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_elite_native_host_dynload_phase626_tests
    PRIVATE -UNDEBUG)
endif()
add_dependencies(sin_pc_elite_native_host_dynload_phase626_tests
  sin_pc_elite_native_host_c_abi_phase626)
add_test(NAME sin_pc_elite_native_host_dynload_phase626
  COMMAND $<TARGET_FILE:sin_pc_elite_native_host_dynload_phase626_tests>
          $<TARGET_FILE:sin_pc_elite_native_host_c_abi_phase626>)

# Phase627: owner-only debugger for AUTHENTIC retail Elite NPC callbacks,
# using per-thread x86 hardware execute breakpoints (DR0..DR3).
# It never writes original .text, vtables, game state, or CreateInterface.
# CI executes --self-test only; no commercial game DLLs are shipped.
add_executable(sin_pc_owner_elite_hwtrace_phase627
  tools/owner_elite_hwtrace_phase627.cpp)
set_target_properties(sin_pc_owner_elite_hwtrace_phase627 PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
target_link_libraries(sin_pc_owner_elite_hwtrace_phase627 PRIVATE bcrypt)
sin_pc_configure(sin_pc_owner_elite_hwtrace_phase627
  sin_pc_owner_elite_hwtrace_phase627)
add_test(NAME sin_pc_owner_elite_hwtrace_phase627_selftest
  COMMAND $<TARGET_FILE:sin_pc_owner_elite_hwtrace_phase627> --self-test)

add_test(NAME sin_pc_owner_elite_hwtrace_phase627_script_syntax
  COMMAND powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass
    -File "${CMAKE_CURRENT_SOURCE_DIR}/tests/Test-OwnerEliteHardwareTracePhase627.ps1"
    -Launcher "${CMAKE_CURRENT_SOURCE_DIR}/Invoke-OwnerEliteHardwareTrace.ps1")

# Phase629: run independent original-source Elite event43 weapon-gate
# state machine under ACTUAL MSVC x86. No guessed native weapon vtable
# or original executable is used in CI.
add_library(sin_pc_original_elite_event43_chaingun_gate_phase629 STATIC
  ../../src/original_elite_event43_chaingun_gate_phase629.cpp)
target_include_directories(sin_pc_original_elite_event43_chaingun_gate_phase629
  PUBLIC ../../include)
set_target_properties(sin_pc_original_elite_event43_chaingun_gate_phase629
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_elite_event43_chaingun_gate_phase629
  sin_pc_original_elite_event43_chaingun_gate_phase629)

add_executable(sin_pc_original_elite_event43_chaingun_gate_phase629_tests
  ../../tests/test_original_elite_event43_chaingun_gate_phase629.cpp)
target_link_libraries(sin_pc_original_elite_event43_chaingun_gate_phase629_tests
  PRIVATE sin_pc_original_elite_event43_chaingun_gate_phase629)
set_target_properties(sin_pc_original_elite_event43_chaingun_gate_phase629_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_elite_event43_chaingun_gate_phase629_tests
  sin_pc_original_elite_event43_chaingun_gate_phase629_tests)
if(MSVC)
  target_compile_options(sin_pc_original_elite_event43_chaingun_gate_phase629_tests
    PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_elite_event43_chaingun_gate_phase629_tests
    PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_elite_event43_chaingun_gate_phase629
  COMMAND $<TARGET_FILE:sin_pc_original_elite_event43_chaingun_gate_phase629_tests>)

# Phase630: independent native x86 DLL links to the existing original
# Phase626 Elite actor host (8 exports unchanged), while exporting exactly
# three source-backed gate/tick functions. No retail binaries or Source
# engine game objects exist in these CI tests.
add_library(sin_pc_elite_source_gate_phase630 SHARED
  ../../src/elite_native_source_gate_phase630.cpp
  ../../src/original_elite_event43_chaingun_gate_phase629.cpp
  ../../src/elite_native_source_gate_phase630.win32.def)
target_include_directories(sin_pc_elite_source_gate_phase630 PUBLIC ../../include)
target_link_libraries(sin_pc_elite_source_gate_phase630 PRIVATE
  sin_pc_elite_native_host_c_abi_phase626)
set_target_properties(sin_pc_elite_source_gate_phase630 PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_elite_source_gate_phase630
  sin_elite_source_gate_phase630)

add_executable(sin_pc_elite_source_gate_phase630_tests
  ../../tests/test_elite_native_source_gate_phase630.cpp)
target_include_directories(sin_pc_elite_source_gate_phase630_tests
  PRIVATE ../../include)
target_link_libraries(sin_pc_elite_source_gate_phase630_tests PRIVATE
  sin_pc_elite_source_gate_phase630 sin_pc_elite_native_host_c_abi_phase626)
set_target_properties(sin_pc_elite_source_gate_phase630_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_elite_source_gate_phase630_tests
  sin_pc_elite_source_gate_phase630_tests)
if(MSVC)
  target_compile_options(sin_pc_elite_source_gate_phase630_tests PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_elite_source_gate_phase630_tests PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_elite_source_gate_phase630
  COMMAND $<TARGET_FILE:sin_pc_elite_source_gate_phase630_tests>)

add_executable(sin_pc_elite_source_gate_phase630_dynload_tests
  ../../tests/test_elite_native_source_gate_dynload_phase630.cpp)
target_include_directories(sin_pc_elite_source_gate_phase630_dynload_tests
  PRIVATE ../../include)
set_target_properties(sin_pc_elite_source_gate_phase630_dynload_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_elite_source_gate_phase630_dynload_tests
  sin_pc_elite_source_gate_phase630_dynload_tests)
if(MSVC)
  target_compile_options(sin_pc_elite_source_gate_phase630_dynload_tests PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_elite_source_gate_phase630_dynload_tests PRIVATE -UNDEBUG)
endif()
add_dependencies(sin_pc_elite_source_gate_phase630_dynload_tests
  sin_pc_elite_source_gate_phase630)
add_test(NAME sin_pc_elite_source_gate_phase630_dynload
  COMMAND $<TARGET_FILE:sin_pc_elite_source_gate_phase630_dynload_tests>
          $<TARGET_FILE:sin_pc_elite_source_gate_phase630>)

# Phase631: compile real Win32 x86 C++17 original Elite tick reference
# and its second-handle/null-hazard regressions under MSVC.
add_library(sin_pc_original_elite_weapon_tick_phase631 STATIC
  ../../src/original_elite_weapon_tick_phase631.cpp)
target_include_directories(sin_pc_original_elite_weapon_tick_phase631 PUBLIC
  ../../include)
set_target_properties(sin_pc_original_elite_weapon_tick_phase631 PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_elite_weapon_tick_phase631
  sin_pc_original_elite_weapon_tick_phase631)
add_executable(sin_pc_original_elite_weapon_tick_phase631_tests
  ../../tests/test_original_elite_weapon_tick_phase631.cpp)
target_link_libraries(sin_pc_original_elite_weapon_tick_phase631_tests PRIVATE
  sin_pc_original_elite_weapon_tick_phase631)
set_target_properties(sin_pc_original_elite_weapon_tick_phase631_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_elite_weapon_tick_phase631_tests
  sin_pc_original_elite_weapon_tick_phase631_tests)
if(MSVC)
  target_compile_options(sin_pc_original_elite_weapon_tick_phase631_tests
    PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_elite_weapon_tick_phase631_tests
    PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_elite_weapon_tick_phase631
  COMMAND $<TARGET_FILE:sin_pc_original_elite_weapon_tick_phase631_tests>)

# Phase632 authentic 2006 Elite +0x44C event43/47 model under native
# MSVC Win32 x86. Source host callbacks use no original engine classes.
add_library(sin_pc_original_elite_secondary_event_phase632 STATIC
  ../../src/original_elite_secondary_event_phase632.cpp)
target_include_directories(sin_pc_original_elite_secondary_event_phase632
  PUBLIC ../../include)
set_target_properties(sin_pc_original_elite_secondary_event_phase632 PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_elite_secondary_event_phase632
  sin_pc_original_elite_secondary_event_phase632)
add_executable(sin_pc_original_elite_secondary_event_phase632_tests
  ../../tests/test_original_elite_secondary_event_phase632.cpp)
target_link_libraries(sin_pc_original_elite_secondary_event_phase632_tests
  PRIVATE sin_pc_original_elite_secondary_event_phase632)
set_target_properties(sin_pc_original_elite_secondary_event_phase632_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_elite_secondary_event_phase632_tests
  sin_pc_original_elite_secondary_event_phase632_tests)
if(MSVC)
  target_compile_options(sin_pc_original_elite_secondary_event_phase632_tests
    PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_elite_secondary_event_phase632_tests
    PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_elite_secondary_event_phase632
  COMMAND $<TARGET_FILE:sin_pc_original_elite_secondary_event_phase632_tests>)

# Phase633: original Elite event47 2006 helper functions plus a native MSVC
# Win32 x86 integration test through the recovered Phase632 event dispatcher.
add_library(sin_pc_original_elite_event47_helpers_phase633 STATIC
  ../../src/original_elite_event47_helpers_phase633.cpp)
target_include_directories(sin_pc_original_elite_event47_helpers_phase633
  PUBLIC ../../include)
set_target_properties(sin_pc_original_elite_event47_helpers_phase633 PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_elite_event47_helpers_phase633
  sin_pc_original_elite_event47_helpers_phase633)
add_executable(sin_pc_original_elite_event47_helpers_phase633_tests
  ../../tests/test_original_elite_event47_helpers_phase633.cpp)
target_link_libraries(sin_pc_original_elite_event47_helpers_phase633_tests
  PRIVATE sin_pc_original_elite_event47_helpers_phase633
  sin_pc_original_elite_secondary_event_phase632)
set_target_properties(sin_pc_original_elite_event47_helpers_phase633_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_elite_event47_helpers_phase633_tests
  sin_pc_original_elite_event47_helpers_phase633_tests)
if(MSVC)
  target_compile_options(sin_pc_original_elite_event47_helpers_phase633_tests
    PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_elite_event47_helpers_phase633_tests
    PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_elite_event47_helpers_phase633
  COMMAND $<TARGET_FILE:sin_pc_original_elite_event47_helpers_phase633_tests>)

# Phase634 native MSVC Win32 x86 authentic 2006 Elite event47 flag clear,
# state-zero condition and fully composed 632+633+634 handler tests.
add_library(sin_pc_original_elite_event47_terminal_phase634 STATIC
  ../../src/original_elite_event47_terminal_phase634.cpp)
target_include_directories(sin_pc_original_elite_event47_terminal_phase634
  PUBLIC ../../include)
set_target_properties(sin_pc_original_elite_event47_terminal_phase634 PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_elite_event47_terminal_phase634
  sin_pc_original_elite_event47_terminal_phase634)
add_executable(sin_pc_original_elite_event47_terminal_phase634_tests
  ../../tests/test_original_elite_event47_terminal_phase634.cpp)
target_link_libraries(sin_pc_original_elite_event47_terminal_phase634_tests PRIVATE
  sin_pc_original_elite_event47_terminal_phase634
  sin_pc_original_elite_event47_helpers_phase633
  sin_pc_original_elite_secondary_event_phase632)
set_target_properties(sin_pc_original_elite_event47_terminal_phase634_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_elite_event47_terminal_phase634_tests
  sin_pc_original_elite_event47_terminal_phase634_tests)
if(MSVC)
  target_compile_options(sin_pc_original_elite_event47_terminal_phase634_tests
    PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_elite_event47_terminal_phase634_tests
    PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_elite_event47_terminal_phase634
  COMMAND $<TARGET_FILE:sin_pc_original_elite_event47_terminal_phase634_tests>)

# Phase635 native MSVC Win32 x86 2006 Elite 58-byte ID-range converter.
# In-process tests link complete source reference chain Phase632-635.
add_library(sin_pc_original_elite_id_mapping_phase635 STATIC
  ../../src/original_elite_id_mapping_phase635.cpp)
target_include_directories(sin_pc_original_elite_id_mapping_phase635
  PUBLIC ../../include)
set_target_properties(sin_pc_original_elite_id_mapping_phase635 PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_elite_id_mapping_phase635
  sin_pc_original_elite_id_mapping_phase635)
add_executable(sin_pc_original_elite_id_mapping_phase635_tests
  ../../tests/test_original_elite_id_mapping_phase635.cpp)
target_link_libraries(sin_pc_original_elite_id_mapping_phase635_tests PRIVATE
  sin_pc_original_elite_id_mapping_phase635
  sin_pc_original_elite_event47_terminal_phase634
  sin_pc_original_elite_event47_helpers_phase633
  sin_pc_original_elite_secondary_event_phase632)
set_target_properties(sin_pc_original_elite_id_mapping_phase635_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_elite_id_mapping_phase635_tests
  sin_pc_original_elite_id_mapping_phase635_tests)
if(MSVC)
  target_compile_options(sin_pc_original_elite_id_mapping_phase635_tests
    PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_elite_id_mapping_phase635_tests
    PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_elite_id_mapping_phase635
  COMMAND $<TARGET_FILE:sin_pc_original_elite_id_mapping_phase635_tests>)

# Phase636 exact original Elite mapping node/owner source registration
# semantics, independently built and tested in native MSVC Win32 x86.
add_library(sin_pc_original_elite_mapping_init_phase636 STATIC
  ../../src/original_elite_mapping_init_phase636.cpp)
target_include_directories(sin_pc_original_elite_mapping_init_phase636
  PUBLIC ../../include)
set_target_properties(sin_pc_original_elite_mapping_init_phase636 PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_elite_mapping_init_phase636
  sin_pc_original_elite_mapping_init_phase636)
add_executable(sin_pc_original_elite_mapping_init_phase636_tests
  ../../tests/test_original_elite_mapping_init_phase636.cpp)
target_link_libraries(sin_pc_original_elite_mapping_init_phase636_tests
  PRIVATE sin_pc_original_elite_mapping_init_phase636
  sin_pc_original_elite_id_mapping_phase635)
set_target_properties(sin_pc_original_elite_mapping_init_phase636_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_elite_mapping_init_phase636_tests
  sin_pc_original_elite_mapping_init_phase636_tests)
if(MSVC)
  target_compile_options(sin_pc_original_elite_mapping_init_phase636_tests
    PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_elite_mapping_init_phase636_tests
    PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_elite_mapping_init_phase636
  COMMAND $<TARGET_FILE:sin_pc_original_elite_mapping_init_phase636_tests>)

# Phase637 native Win32 x86 MSVC original range LOCALID registration and
# task-wrapper source references; not an original game DLL load.
add_library(sin_pc_original_elite_range_registration_phase637 STATIC
  ../../src/original_elite_range_registration_phase637.cpp)
target_include_directories(sin_pc_original_elite_range_registration_phase637
  PUBLIC ../../include)
set_target_properties(sin_pc_original_elite_range_registration_phase637
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_elite_range_registration_phase637
  sin_pc_original_elite_range_registration_phase637)
add_executable(sin_pc_original_elite_range_registration_phase637_tests
  ../../tests/test_original_elite_range_registration_phase637.cpp)
target_link_libraries(sin_pc_original_elite_range_registration_phase637_tests
  PRIVATE sin_pc_original_elite_range_registration_phase637
  sin_pc_original_elite_mapping_init_phase636
  sin_pc_original_elite_id_mapping_phase635)
set_target_properties(sin_pc_original_elite_range_registration_phase637_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_elite_range_registration_phase637_tests
  sin_pc_original_elite_range_registration_phase637_tests)
if(MSVC)
  target_compile_options(sin_pc_original_elite_range_registration_phase637_tests
    PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_elite_range_registration_phase637_tests
    PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_elite_range_registration_phase637
  COMMAND $<TARGET_FILE:sin_pc_original_elite_range_registration_phase637_tests>)

# Phase638 original 2006 SiN condition mapper and forwarding helper
# source reference, compiled natively under MSVC Win32 x86.
add_library(sin_pc_original_symbol_forward_phase638 STATIC
  ../../src/original_symbol_forward_phase638.cpp)
target_include_directories(sin_pc_original_symbol_forward_phase638
  PUBLIC ../../include)
set_target_properties(sin_pc_original_symbol_forward_phase638 PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_symbol_forward_phase638
  sin_pc_original_symbol_forward_phase638)
add_executable(sin_pc_original_symbol_forward_phase638_tests
  ../../tests/test_original_symbol_forward_phase638.cpp)
target_link_libraries(sin_pc_original_symbol_forward_phase638_tests PRIVATE
  sin_pc_original_symbol_forward_phase638
  sin_pc_original_elite_range_registration_phase637)
set_target_properties(sin_pc_original_symbol_forward_phase638_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_symbol_forward_phase638_tests
  sin_pc_original_symbol_forward_phase638_tests)
if(MSVC)
  target_compile_options(sin_pc_original_symbol_forward_phase638_tests
    PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_symbol_forward_phase638_tests
    PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_symbol_forward_phase638
  COMMAND $<TARGET_FILE:sin_pc_original_symbol_forward_phase638_tests>)

# Phase639 original native Win32 x86 slot registration + Phase638 composed test.
add_library(sin_pc_original_symbol_slot_allocation_phase639 STATIC
  ../../src/original_symbol_slot_allocation_phase639.cpp)
target_include_directories(sin_pc_original_symbol_slot_allocation_phase639
  PUBLIC ../../include)
set_target_properties(sin_pc_original_symbol_slot_allocation_phase639 PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_symbol_slot_allocation_phase639
  sin_pc_original_symbol_slot_allocation_phase639)
add_executable(sin_pc_original_symbol_slot_allocation_phase639_tests
  ../../tests/test_original_symbol_slot_allocation_phase639.cpp)
target_link_libraries(sin_pc_original_symbol_slot_allocation_phase639_tests PRIVATE
  sin_pc_original_symbol_slot_allocation_phase639
  sin_pc_original_symbol_forward_phase638
  sin_pc_original_elite_range_registration_phase637)
set_target_properties(sin_pc_original_symbol_slot_allocation_phase639_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_symbol_slot_allocation_phase639_tests
  sin_pc_original_symbol_slot_allocation_phase639_tests)
if(MSVC)
  target_compile_options(sin_pc_original_symbol_slot_allocation_phase639_tests
    PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_symbol_slot_allocation_phase639_tests
    PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_symbol_slot_allocation_phase639
  COMMAND $<TARGET_FILE:sin_pc_original_symbol_slot_allocation_phase639_tests>)

# Phase640 original SiN 2006 106B slot linker and 14B direct accessor.
add_library(sin_pc_original_symbol_tree_link_phase640 STATIC
  ../../src/original_symbol_tree_link_phase640.cpp)
target_include_directories(sin_pc_original_symbol_tree_link_phase640 PUBLIC
  ../../include)
set_target_properties(sin_pc_original_symbol_tree_link_phase640 PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_symbol_tree_link_phase640
  sin_pc_original_symbol_tree_link_phase640)
add_executable(sin_pc_original_symbol_tree_link_phase640_tests
  ../../tests/test_original_symbol_tree_link_phase640.cpp)
target_link_libraries(sin_pc_original_symbol_tree_link_phase640_tests PRIVATE
  sin_pc_original_symbol_tree_link_phase640
  sin_pc_original_symbol_slot_allocation_phase639
  sin_pc_original_symbol_forward_phase638
  sin_pc_original_elite_range_registration_phase637)
set_target_properties(sin_pc_original_symbol_tree_link_phase640_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_symbol_tree_link_phase640_tests
  sin_pc_original_symbol_tree_link_phase640_tests)
if(MSVC)
  target_compile_options(sin_pc_original_symbol_tree_link_phase640_tests
    PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_symbol_tree_link_phase640_tests
    PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_symbol_tree_link_phase640
  COMMAND $<TARGET_FILE:sin_pc_original_symbol_tree_link_phase640_tests>)

# Phase641 authenticated retail SiN left/right rotations + sentinel accessor.
add_library(sin_pc_original_symbol_rotations_phase641 STATIC
  ../../src/original_symbol_rotations_phase641.cpp)
target_include_directories(sin_pc_original_symbol_rotations_phase641 PUBLIC ../../include)
set_target_properties(sin_pc_original_symbol_rotations_phase641 PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_symbol_rotations_phase641
  sin_pc_original_symbol_rotations_phase641)
add_executable(sin_pc_original_symbol_rotations_phase641_tests
  ../../tests/test_original_symbol_rotations_phase641.cpp)
target_link_libraries(sin_pc_original_symbol_rotations_phase641_tests PRIVATE
  sin_pc_original_symbol_rotations_phase641)
set_target_properties(sin_pc_original_symbol_rotations_phase641_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_symbol_rotations_phase641_tests
  sin_pc_original_symbol_rotations_phase641_tests)
if(MSVC)
  target_compile_options(sin_pc_original_symbol_rotations_phase641_tests
    PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_symbol_rotations_phase641_tests
    PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_symbol_rotations_phase641
  COMMAND $<TARGET_FILE:sin_pc_original_symbol_rotations_phase641_tests>)

# Phase642 native Win32 x86 original 2006 SiN 1482B tree rebalancer.
add_library(sin_pc_original_symbol_rebalance_phase642 STATIC
  ../../src/original_symbol_rebalance_phase642.cpp)
target_include_directories(sin_pc_original_symbol_rebalance_phase642 PUBLIC ../../include)
target_link_libraries(sin_pc_original_symbol_rebalance_phase642 PUBLIC
  sin_pc_original_symbol_rotations_phase641)
set_target_properties(sin_pc_original_symbol_rebalance_phase642 PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_symbol_rebalance_phase642
  sin_pc_original_symbol_rebalance_phase642)
add_executable(sin_pc_original_symbol_rebalance_phase642_tests
  ../../tests/test_original_symbol_rebalance_phase642.cpp)
target_link_libraries(sin_pc_original_symbol_rebalance_phase642_tests PRIVATE
  sin_pc_original_symbol_rebalance_phase642
  sin_pc_original_symbol_rotations_phase641
  sin_pc_original_symbol_tree_link_phase640)
set_target_properties(sin_pc_original_symbol_rebalance_phase642_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_symbol_rebalance_phase642_tests
  sin_pc_original_symbol_rebalance_phase642_tests)
if(MSVC)
  target_compile_options(sin_pc_original_symbol_rebalance_phase642_tests PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_symbol_rebalance_phase642_tests PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_symbol_rebalance_phase642
  COMMAND $<TARGET_FILE:sin_pc_original_symbol_rebalance_phase642_tests>)

# Phase643 native MSVC Win32 x86 original SiN symbol enumeration and
# full Phase638..643 synthetic registration tree integration.
add_library(sin_pc_original_symbol_enumeration_phase643 STATIC
  ../../src/original_symbol_enumeration_phase643.cpp)
target_include_directories(sin_pc_original_symbol_enumeration_phase643
  PUBLIC ../../include)
set_target_properties(sin_pc_original_symbol_enumeration_phase643 PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_symbol_enumeration_phase643
  sin_pc_original_symbol_enumeration_phase643)
add_executable(sin_pc_original_symbol_enumeration_phase643_tests
  ../../tests/test_original_symbol_enumeration_phase643.cpp)
target_link_libraries(sin_pc_original_symbol_enumeration_phase643_tests PRIVATE
  sin_pc_original_symbol_enumeration_phase643
  sin_pc_original_symbol_slot_allocation_phase639
  sin_pc_original_symbol_tree_link_phase640
  sin_pc_original_symbol_rebalance_phase642
  sin_pc_original_symbol_rotations_phase641
  sin_pc_original_symbol_forward_phase638
  sin_pc_original_elite_range_registration_phase637)
set_target_properties(sin_pc_original_symbol_enumeration_phase643_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_symbol_enumeration_phase643_tests
  sin_pc_original_symbol_enumeration_phase643_tests)
if(MSVC)
  target_compile_options(sin_pc_original_symbol_enumeration_phase643_tests
    PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_symbol_enumeration_phase643_tests
    PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_symbol_enumeration_phase643
  COMMAND $<TARGET_FILE:sin_pc_original_symbol_enumeration_phase643_tests>)

# Phase644 native Win32 x86 original SiN 110-byte slot growth +39-byte release.
add_library(sin_pc_original_symbol_growth_phase644 STATIC
  ../../src/original_symbol_growth_phase644.cpp)
target_include_directories(sin_pc_original_symbol_growth_phase644 PUBLIC ../../include)
set_target_properties(sin_pc_original_symbol_growth_phase644 PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_symbol_growth_phase644
  sin_pc_original_symbol_growth_phase644)
add_executable(sin_pc_original_symbol_growth_phase644_tests
  ../../tests/test_original_symbol_growth_phase644.cpp)
target_link_libraries(sin_pc_original_symbol_growth_phase644_tests PRIVATE
  sin_pc_original_symbol_growth_phase644
  sin_pc_original_symbol_enumeration_phase643
  sin_pc_original_symbol_slot_allocation_phase639
  sin_pc_original_symbol_tree_link_phase640
  sin_pc_original_symbol_rebalance_phase642
  sin_pc_original_symbol_rotations_phase641
  sin_pc_original_symbol_forward_phase638
  sin_pc_original_elite_range_registration_phase637)
set_target_properties(sin_pc_original_symbol_growth_phase644_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_symbol_growth_phase644_tests
  sin_pc_original_symbol_growth_phase644_tests)
if(MSVC)
  target_compile_options(sin_pc_original_symbol_growth_phase644_tests PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_symbol_growth_phase644_tests PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_symbol_growth_phase644
  COMMAND $<TARGET_FILE:sin_pc_original_symbol_growth_phase644_tests>)

# Phase645 native Win32 original allocation + reallocation + free wrappers.
add_library(sin_pc_original_memory_wrappers_phase645 STATIC
  ../../src/original_memory_wrappers_phase645.cpp)
target_include_directories(sin_pc_original_memory_wrappers_phase645 PUBLIC ../../include)
set_target_properties(sin_pc_original_memory_wrappers_phase645 PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_memory_wrappers_phase645
  sin_pc_original_memory_wrappers_phase645)
add_executable(sin_pc_original_memory_wrappers_phase645_tests
  ../../tests/test_original_memory_wrappers_phase645.cpp)
target_link_libraries(sin_pc_original_memory_wrappers_phase645_tests PRIVATE
  sin_pc_original_memory_wrappers_phase645
  sin_pc_original_symbol_growth_phase644
  sin_pc_original_symbol_enumeration_phase643
  sin_pc_original_symbol_slot_allocation_phase639
  sin_pc_original_symbol_tree_link_phase640
  sin_pc_original_symbol_rebalance_phase642
  sin_pc_original_symbol_rotations_phase641
  sin_pc_original_symbol_forward_phase638
  sin_pc_original_elite_range_registration_phase637)
set_target_properties(sin_pc_original_memory_wrappers_phase645_tests
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_memory_wrappers_phase645_tests
  sin_pc_original_memory_wrappers_phase645_tests)
if(MSVC)
  target_compile_options(sin_pc_original_memory_wrappers_phase645_tests PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_memory_wrappers_phase645_tests PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_memory_wrappers_phase645
  COMMAND $<TARGET_FILE:sin_pc_original_memory_wrappers_phase645_tests>)

# Phase646 native Win32 x86 original SiN comparator/constructor/factory,
# integrated with compiled original symbol registration Phase638 through 645.
add_library(sin_pc_original_symbol_comparator_phase646 STATIC
  ../../src/original_symbol_comparator_phase646.cpp)
target_include_directories(sin_pc_original_symbol_comparator_phase646 PUBLIC ../../include)
set_target_properties(sin_pc_original_symbol_comparator_phase646 PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_symbol_comparator_phase646
  sin_pc_original_symbol_comparator_phase646)
add_executable(sin_pc_original_symbol_comparator_phase646_tests
  ../../tests/test_original_symbol_comparator_phase646.cpp)
target_link_libraries(sin_pc_original_symbol_comparator_phase646_tests PRIVATE
  sin_pc_original_symbol_comparator_phase646
  sin_pc_original_memory_wrappers_phase645
  sin_pc_original_symbol_growth_phase644
  sin_pc_original_symbol_enumeration_phase643
  sin_pc_original_symbol_slot_allocation_phase639
  sin_pc_original_symbol_tree_link_phase640
  sin_pc_original_symbol_rebalance_phase642
  sin_pc_original_symbol_rotations_phase641
  sin_pc_original_symbol_forward_phase638
  sin_pc_original_elite_range_registration_phase637)
set_target_properties(sin_pc_original_symbol_comparator_phase646_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_symbol_comparator_phase646_tests
  sin_pc_original_symbol_comparator_phase646_tests)
if(MSVC)
  target_compile_options(sin_pc_original_symbol_comparator_phase646_tests PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_symbol_comparator_phase646_tests PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_symbol_comparator_phase646
  COMMAND $<TARGET_FILE:sin_pc_original_symbol_comparator_phase646_tests>)

# Phase647 original SiN nested map (81B), slot initialization (90B),
# release (41B), plus full native MSVC Win32 Phase638-647 chain tests.
add_library(sin_pc_original_nested_map_phase647 STATIC
  ../../src/original_nested_map_phase647.cpp)
target_include_directories(sin_pc_original_nested_map_phase647 PUBLIC ../../include)
set_target_properties(sin_pc_original_nested_map_phase647 PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_nested_map_phase647
  sin_pc_original_nested_map_phase647)
add_executable(sin_pc_original_nested_map_phase647_tests
  ../../tests/test_original_nested_map_phase647.cpp)
target_link_libraries(sin_pc_original_nested_map_phase647_tests PRIVATE
  sin_pc_original_nested_map_phase647)
set_target_properties(sin_pc_original_nested_map_phase647_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_nested_map_phase647_tests
  sin_pc_original_nested_map_phase647_tests)
if(MSVC)
  target_compile_options(sin_pc_original_nested_map_phase647_tests PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_nested_map_phase647_tests PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_nested_map_phase647
  COMMAND $<TARGET_FILE:sin_pc_original_nested_map_phase647_tests>)
add_executable(sin_pc_original_nested_map_chain_phase647_tests
  ../../tests/test_original_nested_map_chain_phase647.cpp)
target_link_libraries(sin_pc_original_nested_map_chain_phase647_tests PRIVATE
  sin_pc_original_nested_map_phase647
  sin_pc_original_symbol_comparator_phase646
  sin_pc_original_memory_wrappers_phase645
  sin_pc_original_symbol_growth_phase644
  sin_pc_original_symbol_enumeration_phase643
  sin_pc_original_symbol_slot_allocation_phase639
  sin_pc_original_symbol_tree_link_phase640
  sin_pc_original_symbol_rebalance_phase642
  sin_pc_original_symbol_rotations_phase641
  sin_pc_original_symbol_forward_phase638
  sin_pc_original_elite_range_registration_phase637)
set_target_properties(sin_pc_original_nested_map_chain_phase647_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_nested_map_chain_phase647_tests
  sin_pc_original_nested_map_chain_phase647_tests)
if(MSVC)
  target_compile_options(sin_pc_original_nested_map_chain_phase647_tests PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_nested_map_chain_phase647_tests PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_nested_map_chain_phase647
  COMMAND $<TARGET_FILE:sin_pc_original_nested_map_chain_phase647_tests>)

# Phase648 native Win32: original nested string comparison and Phase638-648.
add_library(sin_pc_original_nested_comparators_phase648 STATIC
  ../../src/original_nested_comparators_phase648.cpp)
target_include_directories(sin_pc_original_nested_comparators_phase648 PUBLIC ../../include)
set_target_properties(sin_pc_original_nested_comparators_phase648 PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_nested_comparators_phase648
  sin_pc_original_nested_comparators_phase648)
add_executable(sin_pc_original_nested_comparators_phase648_tests
  ../../tests/test_original_nested_comparators_phase648.cpp)
target_link_libraries(sin_pc_original_nested_comparators_phase648_tests PRIVATE
  sin_pc_original_nested_comparators_phase648 sin_pc_original_nested_map_phase647)
set_target_properties(sin_pc_original_nested_comparators_phase648_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_nested_comparators_phase648_tests
  sin_pc_original_nested_comparators_phase648_tests)
if(MSVC)
  target_compile_options(sin_pc_original_nested_comparators_phase648_tests PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_nested_comparators_phase648_tests PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_nested_comparators_phase648
  COMMAND $<TARGET_FILE:sin_pc_original_nested_comparators_phase648_tests>)
add_executable(sin_pc_original_nested_comparator_chain_phase648_tests
  ../../tests/test_original_nested_comparator_chain_phase648.cpp)
target_link_libraries(sin_pc_original_nested_comparator_chain_phase648_tests PRIVATE
  sin_pc_original_nested_comparators_phase648
  sin_pc_original_nested_map_phase647
  sin_pc_original_symbol_comparator_phase646
  sin_pc_original_memory_wrappers_phase645
  sin_pc_original_symbol_growth_phase644
  sin_pc_original_symbol_enumeration_phase643
  sin_pc_original_symbol_slot_allocation_phase639
  sin_pc_original_symbol_tree_link_phase640
  sin_pc_original_symbol_rebalance_phase642
  sin_pc_original_symbol_rotations_phase641
  sin_pc_original_symbol_forward_phase638
  sin_pc_original_elite_range_registration_phase637)
set_target_properties(sin_pc_original_nested_comparator_chain_phase648_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_nested_comparator_chain_phase648_tests
  sin_pc_original_nested_comparator_chain_phase648_tests)
if(MSVC)
  target_compile_options(sin_pc_original_nested_comparator_chain_phase648_tests PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_nested_comparator_chain_phase648_tests PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_nested_comparator_chain_phase648
  COMMAND $<TARGET_FILE:sin_pc_original_nested_comparator_chain_phase648_tests>)

# Phase649 Win32 x86: mode-switched original SiN string comparison
# and Phase638-649 full synthetic nested symbol registration chain.
add_library(sin_pc_original_string_casefold_phase649 STATIC
  ../../src/original_string_casefold_phase649.cpp)
target_include_directories(sin_pc_original_string_casefold_phase649 PUBLIC ../../include)
set_target_properties(sin_pc_original_string_casefold_phase649 PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_string_casefold_phase649
  sin_pc_original_string_casefold_phase649)
add_executable(sin_pc_original_string_casefold_phase649_tests
  ../../tests/test_original_string_casefold_phase649.cpp)
target_link_libraries(sin_pc_original_string_casefold_phase649_tests PRIVATE
  sin_pc_original_string_casefold_phase649
  sin_pc_original_nested_comparators_phase648)
set_target_properties(sin_pc_original_string_casefold_phase649_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_string_casefold_phase649_tests
  sin_pc_original_string_casefold_phase649_tests)
if(MSVC)
  target_compile_options(sin_pc_original_string_casefold_phase649_tests PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_string_casefold_phase649_tests PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_string_casefold_phase649
  COMMAND $<TARGET_FILE:sin_pc_original_string_casefold_phase649_tests>)
add_executable(sin_pc_original_string_casefold_chain_phase649_tests
  ../../tests/test_original_string_casefold_chain_phase649.cpp)
target_link_libraries(sin_pc_original_string_casefold_chain_phase649_tests PRIVATE
  sin_pc_original_string_casefold_phase649
  sin_pc_original_nested_comparators_phase648
  sin_pc_original_nested_map_phase647
  sin_pc_original_symbol_comparator_phase646
  sin_pc_original_memory_wrappers_phase645
  sin_pc_original_symbol_growth_phase644
  sin_pc_original_symbol_enumeration_phase643
  sin_pc_original_symbol_slot_allocation_phase639
  sin_pc_original_symbol_tree_link_phase640
  sin_pc_original_symbol_rebalance_phase642
  sin_pc_original_symbol_rotations_phase641
  sin_pc_original_symbol_forward_phase638
  sin_pc_original_elite_range_registration_phase637)
set_target_properties(sin_pc_original_string_casefold_chain_phase649_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_string_casefold_chain_phase649_tests
  sin_pc_original_string_casefold_chain_phase649_tests)
if(MSVC)
  target_compile_options(sin_pc_original_string_casefold_chain_phase649_tests PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_string_casefold_chain_phase649_tests PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_string_casefold_chain_phase649
  COMMAND $<TARGET_FILE:sin_pc_original_string_casefold_chain_phase649_tests>)

# Phase650 Win32 x86 original SiN nested lookup, tree comparator and
# original 12B-node block string allocation/reference regression tests.
add_library(sin_pc_original_string_lookup_phase650 STATIC
  ../../src/original_string_lookup_phase650.cpp)
target_include_directories(sin_pc_original_string_lookup_phase650 PUBLIC ../../include)
set_target_properties(sin_pc_original_string_lookup_phase650 PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_string_lookup_phase650
  sin_pc_original_string_lookup_phase650)
add_executable(sin_pc_original_string_lookup_phase650_tests
  ../../tests/test_original_string_lookup_phase650.cpp)
target_link_libraries(sin_pc_original_string_lookup_phase650_tests PRIVATE
  sin_pc_original_string_lookup_phase650
  sin_pc_original_nested_comparators_phase648
  sin_pc_original_string_casefold_phase649)
set_target_properties(sin_pc_original_string_lookup_phase650_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_string_lookup_phase650_tests
  sin_pc_original_string_lookup_phase650_tests)
if(MSVC)
  target_compile_options(sin_pc_original_string_lookup_phase650_tests PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_string_lookup_phase650_tests PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_string_lookup_phase650
  COMMAND $<TARGET_FILE:sin_pc_original_string_lookup_phase650_tests>)

# Phase651 native Win32 original SiN block-pointer insertion and
# nested string reference tree-insert chain with Phase648-650 C++ sources.
add_library(sin_pc_original_nested_insertion_phase651 STATIC
  ../../src/original_nested_insertion_phase651.cpp)
target_include_directories(sin_pc_original_nested_insertion_phase651 PUBLIC ../../include)
set_target_properties(sin_pc_original_nested_insertion_phase651 PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_nested_insertion_phase651
  sin_pc_original_nested_insertion_phase651)
add_executable(sin_pc_original_nested_insertion_phase651_tests
  ../../tests/test_original_nested_insertion_phase651.cpp)
target_link_libraries(sin_pc_original_nested_insertion_phase651_tests PRIVATE
  sin_pc_original_nested_insertion_phase651)
set_target_properties(sin_pc_original_nested_insertion_phase651_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_nested_insertion_phase651_tests
  sin_pc_original_nested_insertion_phase651_tests)
if(MSVC)
  target_compile_options(sin_pc_original_nested_insertion_phase651_tests PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_nested_insertion_phase651_tests PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_nested_insertion_phase651
  COMMAND $<TARGET_FILE:sin_pc_original_nested_insertion_phase651_tests>)
add_executable(sin_pc_original_nested_insertion_chain_phase651_tests
  ../../tests/test_original_nested_insertion_chain_phase651.cpp)
target_link_libraries(sin_pc_original_nested_insertion_chain_phase651_tests PRIVATE
  sin_pc_original_nested_insertion_phase651
  sin_pc_original_string_lookup_phase650
  sin_pc_original_nested_comparators_phase648
  sin_pc_original_string_casefold_phase649)
set_target_properties(sin_pc_original_nested_insertion_chain_phase651_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_nested_insertion_chain_phase651_tests
  sin_pc_original_nested_insertion_chain_phase651_tests)
if(MSVC)
  target_compile_options(sin_pc_original_nested_insertion_chain_phase651_tests PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_nested_insertion_chain_phase651_tests PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_nested_insertion_chain_phase651
  COMMAND $<TARGET_FILE:sin_pc_original_nested_insertion_chain_phase651_tests>)

# Phase652 Win32 x86: original nested parent search, 12-byte slot acquisition
# and link/rebalance-dispatch source models with actual Phase651 TreeInsert.
add_library(sin_pc_original_nested_tree_core_phase652 STATIC
  ../../src/original_nested_tree_core_phase652.cpp)
target_include_directories(sin_pc_original_nested_tree_core_phase652 PUBLIC ../../include)
set_target_properties(sin_pc_original_nested_tree_core_phase652 PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_nested_tree_core_phase652
  sin_pc_original_nested_tree_core_phase652)
add_executable(sin_pc_original_nested_tree_core_phase652_tests
  ../../tests/test_original_nested_tree_core_phase652.cpp)
target_link_libraries(sin_pc_original_nested_tree_core_phase652_tests PRIVATE
  sin_pc_original_nested_tree_core_phase652)
set_target_properties(sin_pc_original_nested_tree_core_phase652_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_nested_tree_core_phase652_tests
  sin_pc_original_nested_tree_core_phase652_tests)
if(MSVC)
  target_compile_options(sin_pc_original_nested_tree_core_phase652_tests PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_nested_tree_core_phase652_tests PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_nested_tree_core_phase652
  COMMAND $<TARGET_FILE:sin_pc_original_nested_tree_core_phase652_tests>)
add_executable(sin_pc_original_nested_tree_chain_phase652_tests
  ../../tests/test_original_nested_tree_chain_phase652.cpp)
target_link_libraries(sin_pc_original_nested_tree_chain_phase652_tests PRIVATE
  sin_pc_original_nested_tree_core_phase652
  sin_pc_original_nested_insertion_phase651)
set_target_properties(sin_pc_original_nested_tree_chain_phase652_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_nested_tree_chain_phase652_tests
  sin_pc_original_nested_tree_chain_phase652_tests)
if(MSVC)
  target_compile_options(sin_pc_original_nested_tree_chain_phase652_tests PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_nested_tree_chain_phase652_tests PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_nested_tree_chain_phase652
  COMMAND $<TARGET_FILE:sin_pc_original_nested_tree_chain_phase652_tests>)

# Phase653 native Win32 original 12B-node growth and static sentinel resolver.
add_library(sin_pc_original_nested_slot_growth_phase653 STATIC
  ../../src/original_nested_slot_growth_phase653.cpp)
target_include_directories(sin_pc_original_nested_slot_growth_phase653 PUBLIC ../../include)
set_target_properties(sin_pc_original_nested_slot_growth_phase653 PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_nested_slot_growth_phase653
  sin_pc_original_nested_slot_growth_phase653)
add_executable(sin_pc_original_nested_slot_growth_phase653_tests
  ../../tests/test_original_nested_slot_growth_phase653.cpp)
target_link_libraries(sin_pc_original_nested_slot_growth_phase653_tests PRIVATE
  sin_pc_original_nested_slot_growth_phase653)
set_target_properties(sin_pc_original_nested_slot_growth_phase653_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_nested_slot_growth_phase653_tests
  sin_pc_original_nested_slot_growth_phase653_tests)
if(MSVC)
  target_compile_options(sin_pc_original_nested_slot_growth_phase653_tests PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_nested_slot_growth_phase653_tests PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_nested_slot_growth_phase653
  COMMAND $<TARGET_FILE:sin_pc_original_nested_slot_growth_phase653_tests>)
add_executable(sin_pc_original_nested_slot_growth_chain_phase653_tests
  ../../tests/test_original_nested_slot_growth_chain_phase653.cpp)
target_link_libraries(sin_pc_original_nested_slot_growth_chain_phase653_tests PRIVATE
  sin_pc_original_nested_slot_growth_phase653
  sin_pc_original_nested_tree_core_phase652)
set_target_properties(sin_pc_original_nested_slot_growth_chain_phase653_tests PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
sin_pc_configure(sin_pc_original_nested_slot_growth_chain_phase653_tests
  sin_pc_original_nested_slot_growth_chain_phase653_tests)
if(MSVC)
  target_compile_options(sin_pc_original_nested_slot_growth_chain_phase653_tests PRIVATE /UNDEBUG)
else()
  target_compile_options(sin_pc_original_nested_slot_growth_chain_phase653_tests PRIVATE -UNDEBUG)
endif()
add_test(NAME sin_pc_original_nested_slot_growth_chain_phase653
  COMMAND $<TARGET_FILE:sin_pc_original_nested_slot_growth_chain_phase653_tests>)
