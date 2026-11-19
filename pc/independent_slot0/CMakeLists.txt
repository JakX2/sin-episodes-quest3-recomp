cmake_minimum_required(VERSION 3.20)
project(sin_independent_slot0 LANGUAGES CXX)
if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "MSVC native Windows x86 required")
endif()
enable_testing()
foreach(role IN ITEMS client server)
  add_library(sin_slot0_${role} SHARED slot0.cpp)
  target_compile_features(sin_slot0_${role} PRIVATE cxx_std_17)
  target_compile_definitions(sin_slot0_${role} PRIVATE SIN_ROLE_${role}=1)
  set_target_properties(sin_slot0_${role} PROPERTIES OUTPUT_NAME "${role}" PREFIX "" RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
  add_executable(sin_slot0_${role}_test test_slot0.cpp)
  target_compile_features(sin_slot0_${role}_test PRIVATE cxx_std_17)
  target_compile_definitions(sin_slot0_${role}_test PRIVATE SIN_ROLE_${role}=1)
  add_test(NAME independent_slot0_${role} COMMAND sin_slot0_${role}_test "$<TARGET_FILE:sin_slot0_${role}>")
endforeach()


# Owner-side reversible independent-DLL startup probe: syntax and fail-closed
# validation ONLY. Public CI does not launch a retail game or stage originals.
add_test(NAME independent_owner_probe_guardrails
  COMMAND powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass
    -File "${CMAKE_CURRENT_SOURCE_DIR}/Test-OwnerIndependentEngineProbe.ps1"
    -Launcher "${CMAKE_CURRENT_SOURCE_DIR}/Invoke-OwnerIndependentEngineProbe.ps1"
    -IndependentBin "$<TARGET_FILE_DIR:sin_slot0_client>")

# Session-isolated owner-engine failure classification; synthetic logs only.
add_test(NAME independent_owner_startup_verdict
  COMMAND powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass
    -File "${CMAKE_CURRENT_SOURCE_DIR}/Test-OwnerIndependentStartupVerdict.ps1"
    -Classifier "${CMAKE_CURRENT_SOURCE_DIR}/Get-OwnerIndependentStartupVerdict.ps1")

# Actual owner game exit classification, exercised only with synthetic statuses
# in public CI. Real retail process exit is never fabricated.
add_test(NAME independent_owner_engine_exit_diagnosis
  COMMAND powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass
    -File "${CMAKE_CURRENT_SOURCE_DIR}/Test-OwnerEngineExitDiagnosis.ps1"
    -Classifier "${CMAKE_CURRENT_SOURCE_DIR}/Get-OwnerEngineExitDiagnosis.ps1"
    -Launcher "${CMAKE_CURRENT_SOURCE_DIR}/Invoke-OwnerIndependentEngineProbe.ps1")

# Original server.dll is owner-private. CI tests only the SHA-gated
# metadata parser with an invented PE32 image, never commercial game code.
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
  add_test(NAME independent_client_console_inventory_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/Inspect-OwnerOriginalClientConsole.py"
      --self-test)
  add_test(NAME independent_server_extended_cvar_metadata_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/Inspect-OwnerOriginalExtendedCvars.py"
      --self-test)
  add_test(NAME independent_server_batch_cvar_metadata_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/Inspect-OwnerOriginalBatchCvars.py"
      --self-test)
  add_test(NAME independent_server_console_inventory_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/Inspect-OwnerOriginalConsole.py"
      --self-test)
  add_test(NAME independent_server_send_table_inventory_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/Inspect-OwnerOriginalSendTables.py"
      --self-test)
  add_test(NAME independent_server_class_inventory_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/Inspect-OwnerOriginalServerClasses.py"
      --self-test)
  add_test(NAME independent_user_message_metadata_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/Inspect-OwnerOriginalUserMessages.py"
      --self-test)
  add_test(NAME independent_server_original_metadata_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/Inspect-OwnerOriginalServerPE.py"
      --self-test)
  add_test(NAME independent_client_original_metadata_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/Inspect-OwnerOriginalClientPE.py"
      --self-test)
endif()

# Source SDK 2004-compatible ServerGameEnts001 is now represented as a
# seven-slot typed MSVC x86 ABI, validated with a synthetic object only.
# Do NOT attach the unimplemented interface to independent server.dll yet.
add_executable(sin_server_gameents_abi_test test_server_gameents_abi.cpp)
target_compile_features(sin_server_gameents_abi_test PRIVATE cxx_std_17)
add_test(NAME independent_server_gameents_7slot_native_abi
  COMMAND sin_server_gameents_abi_test)

# The first independently reconstructed real server entity-method behavior,
# validated against synthetic x86 Source object layouts. Not exported yet:
# MarkEntitiesAsTouching, FreeContainingEntity and CheckTransmit still need
# the full game-side entity graph before ServerGameEnts001 is safe to expose.
add_executable(sin_server_entity_conversion_test
  test_server_entity_conversion_core.cpp)
target_compile_features(sin_server_entity_conversion_test PRIVATE cxx_std_17)
add_test(NAME independent_server_entity_conversion_native
  COMMAND sin_server_entity_conversion_test)

# Original-guided FreeContainingEntity detach and cleanup callback-order core.
# Unit test uses synthetic edicts; original cleanup routines are still
# unidentified, so this is not exported as a playable ServerGameEnts001.
add_executable(sin_server_entity_release_test test_server_entity_release_core.cpp)
target_compile_features(sin_server_entity_release_test PRIVATE cxx_std_17)
add_test(NAME independent_server_entity_release_native
  COMMAND sin_server_entity_release_test)

# Original contact cleanup uses a generation-checked entity handle lookup.
# This portable independent component also checks the PE32 record shape
# when compiled as native MSVC x86. Does not export a new game interface.
add_executable(sin_server_entity_handle_test test_server_entity_handle_core.cpp)
target_compile_features(sin_server_entity_handle_test PRIVATE cxx_std_17)
add_test(NAME independent_server_entity_handle_native
  COMMAND sin_server_entity_handle_test)

# Independently owned touch/ground cleanup traversal (no retail object layout).
add_executable(sin_server_contact_cleanup_test test_server_contact_cleanup_core.cpp)
target_compile_features(sin_server_contact_cleanup_test PRIVATE cxx_std_17)
add_test(NAME independent_server_contact_cleanup_native
  COMMAND sin_server_contact_cleanup_test)

# Reciprocal unlink and contact callbacks in a native independently-owned list.
add_executable(sin_server_reciprocal_contact_test test_server_reciprocal_contact_core.cpp)
target_compile_features(sin_server_reciprocal_contact_test PRIVATE cxx_std_17)
add_test(NAME independent_server_reciprocal_contact_native
  COMMAND sin_server_reciprocal_contact_test)

# Exercise bulk and reciprocal cleanup together, including shared counters.
add_executable(sin_server_contact_pair_test test_server_contact_pair_cleanup.cpp)
target_compile_features(sin_server_contact_pair_test PRIVATE cxx_std_17)
add_test(NAME independent_server_contact_pair_native
  COMMAND sin_server_contact_pair_test)

# Independent immediate-deletion lifecycle and deferred routing.
add_executable(sin_server_immediate_removal_test test_server_entity_immediate_removal_core.cpp)
target_compile_features(sin_server_immediate_removal_test PRIVATE cxx_std_17)
add_test(NAME independent_server_immediate_removal_native
  COMMAND sin_server_immediate_removal_test)

# Full detach + reciprocal contact cleanup + deletion in an owned synthetic world.
add_executable(sin_server_full_teardown_test test_server_entity_full_teardown.cpp)
target_compile_features(sin_server_full_teardown_test PRIVATE cxx_std_17)
add_test(NAME independent_server_full_teardown_native
  COMMAND sin_server_full_teardown_test)

# Deferred entity queue with FIFO releases and generation invalidation.
add_executable(sin_server_deferred_queue_test test_server_deferred_entity_queue.cpp)
target_compile_features(sin_server_deferred_queue_test PRIVATE cxx_std_17)
add_test(NAME independent_server_deferred_queue_native
  COMMAND sin_server_deferred_queue_test)

# Independently owned contact creation, refresh, unlink and recreation.
add_executable(sin_server_touch_mark_test test_server_touch_mark_core.cpp)
target_compile_features(sin_server_touch_mark_test PRIVATE cxx_std_17)
add_test(NAME independent_server_touch_mark_native
  COMMAND sin_server_touch_mark_test)

# Bounded original-guided entity transmit decisions with synthetic PVS.
add_executable(sin_server_transmit_policy_test test_server_transmit_policy_core.cpp)
target_compile_features(sin_server_transmit_policy_test PRIVATE cxx_std_17)
add_test(NAME independent_server_transmit_policy_native
  COMMAND sin_server_transmit_policy_test)

# Opt-in extended server factory boundary; all terminal guards tested in
# synthetic child processes, never in a retail executable.
add_executable(sin_server_factory_probe_test test_server_factory_probe.cpp)
target_compile_features(sin_server_factory_probe_test PRIVATE cxx_std_17)
# The test-only vstdlib provider MUST be outside bin/ and never packaged.
add_library(sin_test_commandline_fixture SHARED test_server_commandline_fixture.cpp)
target_compile_features(sin_test_commandline_fixture PRIVATE cxx_std_17)
set_target_properties(sin_test_commandline_fixture PROPERTIES
  OUTPUT_NAME "vstdlib" PREFIX ""
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/fixture-only")
add_test(NAME independent_server_factory_probe_native
  COMMAND sin_server_factory_probe_test "$<TARGET_FILE:sin_slot0_server>"
    "$<TARGET_FILE:sin_test_commandline_fixture>")
set_tests_properties(independent_server_factory_probe_native PROPERTIES TIMEOUT 180)

# First real post-factory virtual call, exercised by owned fixtures only.
# Neither synthetic Connect DLL is a distributable owner game binary.
add_executable(sin_sound_connect_core_test test_sound_emitter_connect_core.cpp)
target_compile_features(sin_sound_connect_core_test PRIVATE cxx_std_17)
add_test(NAME independent_sound_connect_core_native COMMAND sin_sound_connect_core_test)
foreach(role IN ITEMS client server)
  add_library(sin_fixture_connect_${role} SHARED slot0.cpp)
  target_compile_features(sin_fixture_connect_${role} PRIVATE cxx_std_17)
  target_compile_definitions(sin_fixture_connect_${role} PRIVATE
    SIN_ROLE_${role}=1 SIN_SYNTHETIC_SOUND_CONNECT=1)
  set_target_properties(sin_fixture_connect_${role} PROPERTIES
    OUTPUT_NAME "sin_fixture_connect_${role}" PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/fixture-only")
  add_executable(sin_fixture_connect_${role}_test test_slot0.cpp)
  target_compile_features(sin_fixture_connect_${role}_test PRIVATE cxx_std_17)
  target_compile_definitions(sin_fixture_connect_${role}_test PRIVATE
    SIN_ROLE_${role}=1 SIN_SYNTHETIC_SOUND_CONNECT=1)
  add_test(NAME independent_${role}_synthetic_full_prefix_connect
    COMMAND sin_fixture_connect_${role}_test "$<TARGET_FILE:sin_fixture_connect_${role}>")
endforeach()

# Production server opt-in only, controlled synthetic child fixtures.
add_executable(sin_server_sound_probe_test test_server_sound_probe.cpp)
target_compile_features(sin_server_sound_probe_test PRIVATE cxx_std_17)
add_test(NAME independent_server_sound_probe_native
  COMMAND sin_server_sound_probe_test "$<TARGET_FILE:sin_slot0_server>")
set_tests_properties(independent_server_sound_probe_native PROPERTIES TIMEOUT 90)

# Original x86 data-table property construction and SE1Proxy fixture.
add_executable(sin_server_send_datatable_test test_server_send_datatable_core.cpp)
target_compile_features(sin_server_send_datatable_test PRIVATE cxx_std_17)
add_test(NAME independent_server_send_datatable_native COMMAND sin_server_send_datatable_test)

# Scalar independently owned startup math tables, not original SIMD pointers.
add_executable(sin_server_math_tables_test test_server_math_tables_core.cpp)
target_compile_features(sin_server_math_tables_test PRIVATE cxx_std_17)
add_test(NAME independent_server_math_tables_native COMMAND sin_server_math_tables_test)

# Independently owned console registration after post-Connect event load.
add_executable(sin_server_cvar_registration_test test_server_cvar_registration_core.cpp)
target_compile_features(sin_server_cvar_registration_test PRIVATE cxx_std_17)
add_test(NAME independent_server_cvar_registration_native COMMAND sin_server_cvar_registration_test)

add_executable(sin_server_console_value_test test_server_console_value_core.cpp)
target_compile_features(sin_server_console_value_test PRIVATE cxx_std_17)
add_test(NAME independent_server_console_value_native COMMAND sin_server_console_value_test)

# Original console-object x86 layouts, actual vtable calls, owned fixture.
add_executable(sin_server_console_abi_test test_server_console_object_abi.cpp)
target_compile_features(sin_server_console_abi_test PRIVATE cxx_std_17)
add_test(NAME independent_server_console_abi_native COMMAND sin_server_console_abi_test)

# Original VEngineCvar002 slot order and native independently owned roots.
add_executable(sin_native_cvar_registration_test test_server_native_cvar_registration.cpp)
target_compile_features(sin_native_cvar_registration_test PRIVATE cxx_std_17)
add_test(NAME independent_server_native_cvar_registration_native COMMAND sin_native_cvar_registration_test)

# Original save block and game-system startup ordering, native ABI.
add_executable(sin_server_save_system_lifecycle_test test_server_save_system_lifecycle.cpp)
target_compile_features(sin_server_save_system_lifecycle_test PRIVATE cxx_std_17)
add_test(NAME independent_server_save_system_lifecycle_native COMMAND sin_server_save_system_lifecycle_test)

# Test-only VGUI provider, never included in owner artifacts.
add_library(sin_test_vgui_fixture SHARED test_client_vgui_fixture.cpp)
target_compile_features(sin_test_vgui_fixture PRIVATE cxx_std_17)
set_target_properties(sin_test_vgui_fixture PROPERTIES ARCHIVE_OUTPUT_NAME "vgui2_fixture_import"
  OUTPUT_NAME "vgui2" PREFIX ""
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/fixture-only")
# Loaded-module refusal fixture: deliberately exports no CreateInterface.
add_library(sin_test_vgui_missing_factory SHARED test_client_vgui_missing_factory.cpp)
target_compile_features(sin_test_vgui_missing_factory PRIVATE cxx_std_17)
set_target_properties(sin_test_vgui_missing_factory PROPERTIES ARCHIVE_OUTPUT_NAME "vgui2_missing_factory_import"
  OUTPUT_NAME "vgui2" PREFIX ""
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/fixture-missing")
add_executable(sin_client_vgui_vtable_probe_test test_client_vgui_vtable_core.cpp)
target_compile_features(sin_client_vgui_vtable_probe_test PRIVATE cxx_std_17)
add_test(NAME independent_client_vgui_vtable_probe_native COMMAND
  sin_client_vgui_vtable_probe_test "$<TARGET_FILE:sin_test_vgui_fixture>")
add_executable(sin_client_engine_vgui_slot1_test test_client_engine_vgui_slot1.cpp)
target_compile_features(sin_client_engine_vgui_slot1_test PRIVATE cxx_std_17)
add_test(NAME independent_client_engine_vgui_slot1_native COMMAND
  sin_client_engine_vgui_slot1_test)
add_executable(sin_client_vgui_controls_bootstrap_test test_client_vgui_controls_bootstrap.cpp)
target_compile_features(sin_client_vgui_controls_bootstrap_test PRIVATE cxx_std_17)
add_test(NAME independent_client_vgui_controls_bootstrap_native COMMAND
  sin_client_vgui_controls_bootstrap_test)
add_executable(sin_client_mat_surface_query_test test_client_mat_surface_query.cpp)
target_compile_features(sin_client_mat_surface_query_test PRIVATE cxx_std_17)
add_test(NAME independent_client_mat_surface_query_native
  COMMAND sin_client_mat_surface_query_test)
add_executable(sin_client_dynamic_difficulty_init_test test_client_dynamic_difficulty_init.cpp)
target_compile_features(sin_client_dynamic_difficulty_init_test PRIVATE cxx_std_17)
add_test(NAME independent_client_dynamic_difficulty_init_native
  COMMAND sin_client_dynamic_difficulty_init_test)
add_executable(sin_client_original_gamesystem_sequence_test
  test_client_original_gamesystem_sequence.cpp)
target_compile_features(sin_client_original_gamesystem_sequence_test PRIVATE cxx_std_17)
add_test(NAME independent_client_original_gamesystem_sequence_native
  COMMAND sin_client_original_gamesystem_sequence_test)
add_executable(sin_client_known_constructed_gamesystem_init_test
  test_client_known_constructed_gamesystem_init.cpp)
target_compile_features(sin_client_known_constructed_gamesystem_init_test PRIVATE cxx_std_17)
add_test(NAME independent_client_known_constructed_gamesystem_init_native
  COMMAND sin_client_known_constructed_gamesystem_init_test)
add_executable(sin_client_known_gamesystem_init_test test_client_known_gamesystem_init.cpp)
target_compile_features(sin_client_known_gamesystem_init_test PRIVATE cxx_std_17)
add_test(NAME independent_client_known_gamesystem_init_native
  COMMAND sin_client_known_gamesystem_init_test)
add_executable(sin_client_shadow_mgr_init_test test_client_shadow_mgr_init.cpp)
target_compile_features(sin_client_shadow_mgr_init_test PRIVATE cxx_std_17)
add_test(NAME independent_client_shadow_mgr_init_native
  COMMAND sin_client_shadow_mgr_init_test)
add_executable(sin_client_stats_storage_init_test test_client_stats_storage_init.cpp)
target_compile_features(sin_client_stats_storage_init_test PRIVATE cxx_std_17)
add_test(NAME independent_client_stats_storage_init_native
  COMMAND sin_client_stats_storage_init_test)
add_executable(sin_client_stats_system_init_test test_client_stats_system_init.cpp)
target_compile_features(sin_client_stats_system_init_test PRIVATE cxx_std_17)
add_test(NAME independent_client_stats_system_init_native
  COMMAND sin_client_stats_system_init_test)
add_executable(sin_client_auto_gamesystem_init_test test_client_auto_gamesystem_init_core.cpp)
target_compile_features(sin_client_auto_gamesystem_init_test PRIVATE cxx_std_17)
add_test(NAME independent_client_auto_gamesystem_init_native
  COMMAND sin_client_auto_gamesystem_init_test)
add_executable(sin_client_hud_texture_registration_test test_client_hud_texture_registration.cpp)
target_compile_features(sin_client_hud_texture_registration_test PRIVATE cxx_std_17)
add_test(NAME independent_client_hud_texture_registration_native
  COMMAND sin_client_hud_texture_registration_test)
add_executable(sin_client_hud_texture_converter_abi_test test_client_hud_texture_converter_abi.cpp)
target_compile_features(sin_client_hud_texture_converter_abi_test PRIVATE cxx_std_17)
add_test(NAME independent_client_hud_texture_converter_ms_vtable_native
  COMMAND sin_client_hud_texture_converter_abi_test)
add_executable(sin_client_hud_texture_converter_test test_client_hud_texture_converter_core.cpp)
target_compile_features(sin_client_hud_texture_converter_test PRIVATE cxx_std_17)
add_test(NAME independent_client_hud_texture_converter_core_native
  COMMAND sin_client_hud_texture_converter_test)
add_executable(sin_client_sound_probe_test test_client_sound_probe.cpp)
target_compile_features(sin_client_sound_probe_test PRIVATE cxx_std_17)
add_test(NAME independent_client_sound_probe_native
  COMMAND sin_client_sound_probe_test "$<TARGET_FILE:sin_slot0_client>"
    "$<TARGET_FILE:sin_test_commandline_fixture>" "$<TARGET_FILE:sin_test_vgui_fixture>"
    "$<TARGET_FILE:sin_test_vgui_missing_factory>")
set_tests_properties(independent_client_sound_probe_native PROPERTIES TIMEOUT 180)

add_executable(sin_client_screen_random_test test_client_screen_random_core.cpp)
target_compile_features(sin_client_screen_random_test PRIVATE cxx_std_17)
add_test(NAME independent_client_screen_random_native COMMAND sin_client_screen_random_test)

add_executable(sin_server_extended_cvar_catalog_test test_server_extended_cvar_catalog.cpp)
target_compile_features(sin_server_extended_cvar_catalog_test PRIVATE cxx_std_17)
add_test(NAME independent_server_extended_cvar_catalog_native COMMAND sin_server_extended_cvar_catalog_test)

add_executable(sin_client_object_initializer_test test_client_object_initializer_core.cpp)
target_compile_features(sin_client_object_initializer_test PRIVATE cxx_std_17)
add_test(NAME independent_client_object_initializer_native COMMAND sin_client_object_initializer_test)

add_executable(sin_client_console_registration_test test_client_console_registration_core.cpp)
target_compile_features(sin_client_console_registration_test PRIVATE cxx_std_17)
add_test(NAME independent_client_console_registration_native COMMAND sin_client_console_registration_test)

# Independently owned particle-bucket boundary; synthetic backend only.
add_executable(sin_client_particle_bucket_test test_client_particle_bucket_core.cpp)
target_compile_features(sin_client_particle_bucket_test PRIVATE cxx_std_17)
add_test(NAME independent_client_particle_bucket_native COMMAND sin_client_particle_bucket_test)

# Combined independent native post-console startup fixture (no engine callbacks).
add_executable(sin_client_post_console_startup_test test_client_post_console_startup_core.cpp)
target_compile_features(sin_client_post_console_startup_test PRIVATE cxx_std_17)
target_include_directories(sin_client_post_console_startup_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../include")
add_test(NAME independent_client_post_console_startup_native COMMAND sin_client_post_console_startup_test)

add_executable(sin_client_particle_pool_test test_client_particle_pool_core.cpp)
target_compile_features(sin_client_particle_pool_test PRIVATE cxx_std_17)
add_test(NAME independent_client_particle_pool_native COMMAND sin_client_particle_pool_test)

add_executable(sin_client_post_console_pool_test test_client_post_console_pool.cpp)
target_compile_features(sin_client_post_console_pool_test PRIVATE cxx_std_17)
target_include_directories(sin_client_post_console_pool_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../include")
add_test(NAME independent_client_post_console_pool_native COMMAND sin_client_post_console_pool_test)
