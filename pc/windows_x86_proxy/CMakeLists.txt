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
