cmake_minimum_required(VERSION 3.20)
project(sin_independent_slot0 LANGUAGES CXX)
if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "MSVC native Windows x86 required")
endif()
enable_testing()
foreach(role IN ITEMS client server)
  add_library(sin_slot0_${role} SHARED slot0.cpp original_ammo_definitions.cpp weapon_network_contract.cpp native_weapon_datatables.cpp)
  if(role STREQUAL "client")
    target_sources(sin_slot0_${role} PRIVATE native_client_overlay_watchers.cpp native_client_overlay_history.cpp native_client_overlay_interpolate.cpp native_client_overlay_pool.cpp native_client_overlay_events.cpp)
  endif()
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

# No original binary or retail process: exercise exact phase selection and
# refusal of forged, stale, incomplete or cross-session owner evidence.
add_test(NAME independent_owner_filesystem_gate_synthetic
  COMMAND powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass
    -File "${CMAKE_CURRENT_SOURCE_DIR}/Test-OwnerFileSystemRetailGate.ps1"
    -Gate "${CMAKE_CURRENT_SOURCE_DIR}/Invoke-OwnerFileSystemRetailGate.ps1")

# One-click user package: public Windows CI verifies only dry-run and
# original-game safety preflight. Never copy/run owner game binaries in CI.
add_test(NAME independent_owner_oneclick_package_synthetic
  COMMAND powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass
    -File "${CMAKE_CURRENT_SOURCE_DIR}/Test-SiNOneClickPackage.ps1"
    -Runner "${CMAKE_CURRENT_SOURCE_DIR}/Start-SiNFileSystemTest.ps1")

# The owner observed 0x53494E6B from real retail IFileSystem; only the
# disposable-copy cleanup was refused. Synthetic WIN32 tests of all recovery
# paths use fabricated client/server bytes with replacement SHA constants.
add_test(NAME independent_owner_disposable_recovery_synthetic
  COMMAND powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass
    -File "${CMAKE_CURRENT_SOURCE_DIR}/Test-SiNRetainedCopyRecovery.ps1"
    -RecoveryScript "${CMAKE_CURRENT_SOURCE_DIR}/Recover-SiNRetainedTestCopy.ps1")

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
  add_test(NAME independent_client_class_inventory_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/Inspect-OwnerOriginalClientClasses.py"
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
  add_test(NAME independent_original_filesystem_steam_vtable_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/audit_sin_original_filesystem_steam.py"
      --self-test)
  add_test(NAME independent_original_stats_registry_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/audit_sin_original_stats_registry.py"
      --self-test)
  add_test(NAME independent_original_stats_message_hooks_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/audit_sin_original_stats_message_hooks.py"
      --self-test)
  add_test(NAME independent_original_stat_value_factory_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/audit_sin_original_stat_value_factory.py"
      --self-test)
  add_test(NAME independent_original_stat_value_mutators_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/audit_sin_original_stat_value_mutators.py"
      --self-test)
  add_test(NAME independent_original_stats_dynamic_lookup_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/audit_sin_original_stats_dynamic_lookup.py"
      --self-test)
  add_test(NAME independent_original_stats_reset_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/audit_sin_original_stats_reset.py"
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
add_executable(sin_client_stats_asset_loader_test test_client_stats_asset_loader.cpp)
target_compile_features(sin_client_stats_asset_loader_test PRIVATE cxx_std_17)
add_test(NAME independent_client_stats_asset_loader_native
  COMMAND sin_client_stats_asset_loader_test)

add_executable(sin_client_stats_live_asset_probe_test test_client_stats_live_asset_probe.cpp)
target_compile_features(sin_client_stats_live_asset_probe_test PRIVATE cxx_std_17)
add_test(NAME independent_client_stats_live_asset_probe_native
  COMMAND sin_client_stats_live_asset_probe_test)

add_executable(sin_client_engine_fs_read_test test_client_engine_filesystem_read.cpp)
target_compile_features(sin_client_engine_fs_read_test PRIVATE cxx_std_17)
add_test(NAME independent_client_engine_filesystem_read_native
  COMMAND sin_client_engine_fs_read_test)

add_executable(sin_client_stats_engine_fs_coordinator_test test_client_stats_engine_fs_coordinator.cpp)
target_compile_features(sin_client_stats_engine_fs_coordinator_test PRIVATE cxx_std_17)
add_test(NAME independent_client_stats_engine_fs_coordinator_native
  COMMAND sin_client_stats_engine_fs_coordinator_test)
add_executable(sin_client_stats_script_parser_test test_client_stats_script_parser.cpp)
target_compile_features(sin_client_stats_script_parser_test PRIVATE cxx_std_17)
add_test(NAME independent_client_stats_script_parser_native
  COMMAND sin_client_stats_script_parser_test)
add_executable(sin_client_stats_coordinator_test test_client_stats_coordinator.cpp)
target_compile_features(sin_client_stats_coordinator_test PRIVATE cxx_std_17)
add_test(NAME independent_client_stats_coordinator_native
  COMMAND sin_client_stats_coordinator_test)
add_executable(sin_client_stats_catalog_init_test test_client_stats_catalog_init.cpp)
target_compile_features(sin_client_stats_catalog_init_test PRIVATE cxx_std_17)
add_test(NAME independent_client_stats_catalog_init_native
  COMMAND sin_client_stats_catalog_init_test)
add_executable(sin_client_stats_original_id_map_test
  test_client_stats_original_id_map.cpp)
target_compile_features(sin_client_stats_original_id_map_test PRIVATE cxx_std_17)
add_test(NAME independent_client_stats_original_id_map_native
  COMMAND sin_client_stats_original_id_map_test)
# Owner-original 2006 CUserMessages::HookMessage registrations for
# StaticStat (27), DynamicStat (28), ResetHUD (5), and bf_read 16-bit IDs.
# Synthetic MSVC x86 only: no raw game packets or original handlers.
add_executable(sin_client_stats_user_message_hooks_test
  test_client_stats_user_message_hooks.cpp)
target_compile_features(sin_client_stats_user_message_hooks_test PRIVATE cxx_std_17)
add_test(NAME independent_client_stats_user_message_hooks_native
  COMMAND sin_client_stats_user_message_hooks_test)

# No proprietary bitbuf object is cast/invoked. An independent bounded,
# Source LSB-first cursor inspects recovered 16-bit Stats user-message IDs
# even when the initial packet cursor is not byte-aligned.
add_executable(sin_client_stats_bitbuf_cursor_test
  test_client_stats_bitbuf_cursor.cpp)
target_compile_features(sin_client_stats_bitbuf_cursor_test PRIVATE cxx_std_17)
add_test(NAME independent_client_stats_bitbuf_cursor_native
  COMMAND sin_client_stats_bitbuf_cursor_test)

# Original SHA-grounded CStat value-type factory contains 16 cases
# with different +0x4c virtual-method to reader-slot forwarding
# routes. Native synthetic test rejects unknown type discriminants.
add_executable(sin_client_stats_value_factory_test
  test_client_stats_value_factory_contract.cpp)
target_compile_features(sin_client_stats_value_factory_test PRIVATE cxx_std_17)
add_test(NAME independent_client_stats_value_factory_native
  COMMAND sin_client_stats_value_factory_test)
# Original CStatReader virtual slots each request one or two known
# Source bf_read 32-bit primitive fields. The test extracts bounded raw
# frames and does NOT call the original vtables or change game stats.
add_executable(sin_client_stats_wire_value_decoder_test
  test_client_stats_wire_value_decoder.cpp)
target_compile_features(sin_client_stats_wire_value_decoder_test PRIVATE cxx_std_17)
add_test(NAME independent_client_stats_wire_value_decoder_native
  COMMAND sin_client_stats_wire_value_decoder_test)
# Verify all 66 ID/type associations recovered from the original
# compiled 12-byte stat registry, plus end-to-end bounded wire reads.
add_executable(sin_client_stats_original_type_registry_test
  test_client_stats_original_type_registry.cpp)
target_compile_features(sin_client_stats_original_type_registry_test PRIVATE cxx_std_17)
add_test(NAME independent_client_stats_original_type_registry_native
  COMMAND sin_client_stats_original_type_registry_test)
# SHA-pinned 16 original value-object setters and their data-field
# mutation semantics, implemented ONLY in independent owned state.
# Verify all 55 static records with x87 narrowing, pairs and extrema.
add_executable(sin_client_stats_owned_value_mutation_test
  test_client_stats_owned_value_mutation.cpp)
target_compile_features(sin_client_stats_owned_value_mutation_test PRIVATE cxx_std_17)
add_test(NAME independent_client_stats_owned_value_mutation_native
  COMMAND sin_client_stats_owned_value_mutation_test)

# Original DynamicStat uses a different name-keyed hierarchy. Verify
# exact 256-byte "%s%i" keys, missing/create semantics, atomic updates.
add_executable(sin_client_stats_owned_dynamic_values_test
  test_client_stats_owned_dynamic_values.cpp)
target_compile_features(sin_client_stats_owned_dynamic_values_test PRIVATE cxx_std_17)
add_test(NAME independent_client_stats_owned_dynamic_values_native
  COMMAND sin_client_stats_owned_dynamic_values_test)

# Original SHA-pinned ResetHUD cleanup/rebuild sequence. Pure owned
# StaticStat/DynamicStat->ResetHUD->update simulation, never original
# game message dispatch or raw bf_read native pointer ABI.
add_executable(sin_client_stats_owned_message_runtime_test
  test_client_stats_owned_message_runtime.cpp)
target_compile_features(sin_client_stats_owned_message_runtime_test PRIVATE cxx_std_17)
add_test(NAME independent_client_stats_owned_message_runtime_native
  COMMAND sin_client_stats_owned_message_runtime_test)

# Owned-only delivery boundary: the original dispatcher gives each hook a
# separate 24-byte bf_read copy. This fixture uses independent Cursor
# copies, NEVER the 2006 bf_read object or live VClient011 slot33.
add_executable(sin_client_stats_owned_dispatcher_test
  test_client_stats_owned_dispatcher.cpp)
target_compile_features(sin_client_stats_owned_dispatcher_test PRIVATE cxx_std_17)
add_test(NAME independent_client_stats_owned_dispatcher_native
  COMMAND sin_client_stats_owned_dispatcher_test)

# Original SHA-grounded 24-byte Win32 bf_read candidate and owned-only
# trusted-storage snapshot boundary. No actual game bf_read* is cast,
# dereferenced or passed to VClient011::DispatchUserMessage.
add_executable(sin_client_bf_read_candidate_layout_test
  test_client_bf_read_candidate_layout.cpp)
target_compile_features(sin_client_bf_read_candidate_layout_test PRIVATE cxx_std_17)
add_test(NAME independent_client_bf_read_candidate_layout_native
  COMMAND sin_client_bf_read_candidate_layout_test)

# Public CI cannot access commercial game DLLs: only synthetic PE-section
# mapping tests run here. The SHA-pinned full original audit is private.
if(Python3_Interpreter_FOUND)
  add_test(NAME independent_original_bf_read_candidate_audit_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/audit_sin_original_bf_read_candidate_layout.py"
      --self-test)
endif()

# Test-only MSVC x86 VClient011 slot33 implementation with explicit
# trusted owned reader. This target is NEVER deployed as client.dll.
# Production sin_slot0_client remains fail-closed and is separately
# loaded by the fixture to verify the boundary has not been loosened.
add_library(sin_slot33_owned_fixture SHARED slot0.cpp)
target_compile_features(sin_slot33_owned_fixture PRIVATE cxx_std_17)
target_compile_definitions(sin_slot33_owned_fixture PRIVATE
  SIN_ROLE_client=1 SIN_SYNTHETIC_SLOT33_OWNED=1)
set_target_properties(sin_slot33_owned_fixture PROPERTIES
  OUTPUT_NAME "sin_slot33_owned_fixture" PREFIX ""
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/fixture-bf-read")
add_executable(sin_slot33_owned_fixture_test
  test_client_slot33_synthetic_reader.cpp)
target_compile_features(sin_slot33_owned_fixture_test PRIVATE cxx_std_17)
add_test(NAME independent_client_slot33_trusted_synthetic_native
  COMMAND sin_slot33_owned_fixture_test
    "$<TARGET_FILE:sin_slot33_owned_fixture>"
    "$<TARGET_FILE:sin_slot0_client>")

# SHA-pinned original CClientShadowMgr::Init top-level side effects.
# Independent callbacks preserve order and original numeric bit patterns;
# no actual renderer or original engine callback gets invoked.
add_executable(sin_client_shadow_mgr_known_init_sequence_test
  test_client_shadow_mgr_known_init_sequence.cpp)
target_compile_features(sin_client_shadow_mgr_known_init_sequence_test PRIVATE cxx_std_17)
add_test(NAME independent_client_shadow_mgr_known_init_sequence_native
  COMMAND sin_client_shadow_mgr_known_init_sequence_test)
if(Python3_Interpreter_FOUND)
  add_test(NAME independent_original_shadow_mgr_known_init_audit_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/audit_sin_original_shadow_mgr_known_init.py"
      --self-test)
endif()

# SHA-authenticated original VClient011 post-AutoGameSystem registration
# tail callsites: 14 direct + 12 virtual, with 2 explicit failure gates.
# Only independently injected callbacks; never original pointers.
add_executable(sin_client_original_init_tail_gates_test
  test_client_original_init_tail_gates.cpp)
target_compile_features(sin_client_original_init_tail_gates_test PRIVATE cxx_std_17)
add_test(NAME independent_client_original_init_tail_gates_native
  COMMAND sin_client_original_init_tail_gates_test)
if(Python3_Interpreter_FOUND)
  add_test(NAME independent_original_client_init_tail_audit_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/audit_sin_original_client_init_tail_gates.py"
      --self-test)
endif()

# Original SHA-pinned C_SoundscapeSystem::Init manifest contract,
# including the shipped misspelled root and seven file records.
# Native fixture only, no original soundscape resources shipped in CI.
add_executable(sin_client_soundscape_manifest_owned_test
  test_client_soundscape_manifest_owned.cpp)
target_compile_features(sin_client_soundscape_manifest_owned_test PRIVATE cxx_std_17)
add_test(NAME independent_client_soundscape_manifest_owned_native
  COMMAND sin_client_soundscape_manifest_owned_test)
if(Python3_Interpreter_FOUND)
  add_test(NAME independent_original_soundscape_manifest_audit_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/audit_sin_original_soundscape_manifest.py"
      --self-test)
endif()

# Owned soundscape definition trees + playsoundscape cross-file graph:
# native synthetic test plus private SHA-original inventory's public
# --self-test (no retail scripts or game DLLs in CI).
add_executable(sin_client_soundscape_catalog_owned_test
  test_client_soundscape_catalog_owned.cpp)
target_compile_features(sin_client_soundscape_catalog_owned_test PRIVATE cxx_std_17)
add_test(NAME independent_client_soundscape_catalog_owned_native
  COMMAND sin_client_soundscape_catalog_owned_test)
if(Python3_Interpreter_FOUND)
  add_test(NAME independent_original_soundscape_catalog_audit_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/audit_sin_original_soundscape_catalog.py"
      --self-test)
endif()

# Original 2006 soundscape loader: the manifest uses rb/GAME,
# but the per-script helper uses rb/null pathID. Two original complete
# method SHA checks (343B + 233B) have source-free fabricated CI tests.
if(Python3_Interpreter_FOUND)
  add_test(NAME independent_original_soundscape_dual_pathid_audit_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/audit_sin_original_soundscape_pathid.py"
      --self-test)
endif()

# Original KeyValues::LoadFromFile dual-pathID soundscape through
# SDK2004 VFileSystem012 slots, tested ONLY with fabricated Win32 objects.
# Never wire into production VClient011::Init without live ABI validation.
add_executable(sin_client_soundscape_engine_fs_bridge_test
  test_client_soundscape_engine_fs_bridge.cpp)
target_compile_features(sin_client_soundscape_engine_fs_bridge_test PRIVATE cxx_std_17)
add_test(NAME independent_client_soundscape_engine_fs_bridge_native
  COMMAND sin_client_soundscape_engine_fs_bridge_test)

# Phase 495: independent source-equivalent original C_SoundscapeSystem
# Init policy. Returns true on missing manifest or partial resource
# loads, handles optional per-map soundscape and suppresses duplicated
# fallback, WITHOUT changing the strict owner diagnostic gate.
add_executable(sin_client_soundscape_original_bootstrap_owned_test
  test_client_soundscape_original_bootstrap_owned.cpp)
target_compile_features(sin_client_soundscape_original_bootstrap_owned_test PRIVATE cxx_std_17)
add_test(NAME independent_client_soundscape_original_bootstrap_owned_native
  COMMAND sin_client_soundscape_original_bootstrap_owned_test)

# Phase 496: source-authenticated original C_SoundscapeSystem vtable
# slot3 caches snd_soundmixer and dsp_volume through original
# VEngineCvar002 FindVar virtual slot3. Fixture uses a fake engine.
add_executable(sin_client_soundscape_original_convar_cache_test
  test_client_soundscape_original_convar_cache.cpp)
target_compile_features(sin_client_soundscape_original_convar_cache_test PRIVATE cxx_std_17)
add_test(NAME independent_client_soundscape_original_convar_cache_native
  COMMAND sin_client_soundscape_original_convar_cache_test)
if(Python3_Interpreter_FOUND)
  add_test(NAME independent_original_soundscape_cvar_slot3_audit_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/audit_sin_original_soundscape_cvar_slot3.py"
      --self-test)
endif()

# Phase 499: original 2006 C_SoundscapeSystem 120-byte constructor
# (20 known zero DWORDs, two -1 fields, embedded vptr), full
# 12-slot vtable, original 12-byte slot7 global+4 copy. Source-free
# Win32 fixture and SHA method/slot audit, NO retail engine objects.
add_executable(sin_client_soundscape_original_object_ctor_test
  test_client_soundscape_original_object_ctor.cpp)
target_compile_features(sin_client_soundscape_original_object_ctor_test PRIVATE cxx_std_17)
add_test(NAME independent_client_soundscape_original_object_ctor_native
  COMMAND sin_client_soundscape_original_object_ctor_test)
if(Python3_Interpreter_FOUND)
  add_test(NAME independent_original_soundscape_object_ctor_audit_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/audit_sin_original_soundscape_object_ctor.py"
      --self-test)
endif()

# Phase 500: authenticated original C_SoundscapeSystem 223-byte
# auxiliary teardown and 5-byte vtable slot5 direct tail JMP.
# Bounded native two-phase fake handle/buffer cleanup, NEVER
# the original game's handle table/allocator/embedded virtual calls.
add_executable(sin_client_soundscape_original_aux_cleanup_owned_test
  test_client_soundscape_original_aux_cleanup_owned.cpp)
target_compile_features(sin_client_soundscape_original_aux_cleanup_owned_test PRIVATE cxx_std_17)
add_test(NAME independent_client_soundscape_original_aux_cleanup_owned_native
  COMMAND sin_client_soundscape_original_aux_cleanup_owned_test)
if(Python3_Interpreter_FOUND)
  add_test(NAME independent_original_soundscape_aux_cleanup_audit_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/audit_sin_original_soundscape_aux_cleanup.py"
      --self-test)
endif()

# SHA-authenticated original 2006 SoundscapeSystem virtual slot2
# shutdown-init-tail call ordering and slot11 deleting-destructor
# child/base lifetime with optional flag-bit0 allocator call.
# Owned synthetic events ONLY, never invoke real Source function pointers.
add_executable(sin_client_soundscape_original_vtable_lifecycle_owned_test
  test_client_soundscape_original_vtable_lifecycle_owned.cpp)
target_compile_features(sin_client_soundscape_original_vtable_lifecycle_owned_test PRIVATE cxx_std_17)
add_test(NAME independent_client_soundscape_original_vtable_lifecycle_owned_native
  COMMAND sin_client_soundscape_original_vtable_lifecycle_owned_test)
if(Python3_Interpreter_FOUND)
  add_test(NAME independent_original_soundscape_vtable_lifecycle_audit_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/audit_sin_original_soundscape_vtable_lifecycle.py"
      --self-test)
endif()

# Three SHA-verified 2006 C_SoundscapeSystem child-array teardown
# functions, identical 83-byte method bodies except allocator
# free CALL rel32 at two positions. Native owned field-policy
# regression and synthetic whole-method-normalization audit.
add_executable(sin_client_soundscape_original_child_array_owned_test
  test_client_soundscape_original_child_array_owned.cpp)
target_compile_features(sin_client_soundscape_original_child_array_owned_test PRIVATE cxx_std_17)
add_test(NAME independent_client_soundscape_original_child_array_owned_native
  COMMAND sin_client_soundscape_original_child_array_owned_test)
if(Python3_Interpreter_FOUND)
  add_test(NAME independent_original_soundscape_child_cleanup_trio_audit_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/audit_sin_original_soundscape_child_cleanup_trio.py"
      --self-test)
endif()

# Source-original SoundscapeSystem shared field alias contract:
# auxiliary cleanup 0x240F3D10 and child helper 83B at +A0/+B4
# must never independently free the same owned resource twice.
add_executable(sin_client_soundscape_original_array_overlap_owned_test
  test_client_soundscape_original_array_overlap_owned.cpp)
target_compile_features(sin_client_soundscape_original_array_overlap_owned_test PRIVATE cxx_std_17)
add_test(NAME independent_client_soundscape_original_array_overlap_owned_native
  COMMAND sin_client_soundscape_original_array_overlap_owned_test)

# Original 2006 AutoGameSystem base destructor writes its base
# vptr and removes FIRST matching pointer from global registry.
# Authenticated 26-byte dtor and 90-byte erase method; synthetic
# owned registry only, no real game pointer calls or allocations.
add_executable(sin_client_original_autogamesystem_registry_remove_test
  test_client_original_autogamesystem_registry_remove.cpp)
target_compile_features(sin_client_original_autogamesystem_registry_remove_test PRIVATE cxx_std_17)
add_test(NAME independent_client_original_autogamesystem_registry_remove_native
  COMMAND sin_client_original_autogamesystem_registry_remove_test)
if(Python3_Interpreter_FOUND)
  add_test(NAME independent_original_autogamesystem_registry_remove_audit_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/audit_sin_original_autogamesystem_registry_remove.py"
      --self-test)
endif()

# 2006 source original 100-byte AutoGameSystem ordered registry
# insert and 17-byte append wrapper (uses count as index).
# Real allocator growth is NOT modeled; independent owner capacity
# is bounded. Native Win32 fixture plus source-free method auditor.
add_executable(sin_client_original_autogamesystem_registry_insert_test
  test_client_original_autogamesystem_registry_insert.cpp)
target_compile_features(sin_client_original_autogamesystem_registry_insert_test PRIVATE cxx_std_17)
add_test(NAME independent_client_original_autogamesystem_registry_insert_native
  COMMAND sin_client_original_autogamesystem_registry_insert_test)
if(Python3_Interpreter_FOUND)
  add_test(NAME independent_original_autogamesystem_registry_insert_audit_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/audit_sin_original_autogamesystem_registry_insert.py"
      --self-test)
endif()

# Original 2006 VClient011::Init final mandatory physics factory
# helper 0x24182C60, complete 97B SHA-pinned in Phase 180.
# Tests only real function-pointer query ABI with FAKE Win32 engine.
# No unknown 0x241927A0 physics coupling or positive game Init.
add_executable(sin_client_original_physics_factory_probe_test
  test_client_original_physics_factory_probe.cpp)
target_compile_features(sin_client_original_physics_factory_probe_test PRIVATE cxx_std_17)
add_test(NAME independent_client_original_physics_factory_probe_native
  COMMAND sin_client_original_physics_factory_probe_test)

# Original 2006 physics surfaceprops::Init at 0x241927A0,
# 247-byte SHA-pinned source manifest rb/GAME, then two source
# scripts rb/nullptr via 0x24191EC0. Native source-free parser
# and real-call-capable fake-engine VFileSystem012 tests. NEVER
# invoke original VPhysicsSurfaceProps001 registration slot1.
add_executable(sin_client_original_surfaceprops_manifest_owned_test
  test_client_original_surfaceprops_manifest_owned.cpp)
target_compile_features(sin_client_original_surfaceprops_manifest_owned_test PRIVATE cxx_std_17)
add_test(NAME independent_client_original_surfaceprops_manifest_owned_native
  COMMAND sin_client_original_surfaceprops_manifest_owned_test)
add_executable(sin_client_original_surfaceprops_engine_fs_bridge_test
  test_client_original_surfaceprops_engine_fs_bridge.cpp)
target_compile_features(sin_client_original_surfaceprops_engine_fs_bridge_test PRIVATE cxx_std_17)
add_test(NAME independent_client_original_surfaceprops_engine_fs_bridge_native
  COMMAND sin_client_original_surfaceprops_engine_fs_bridge_test)

# Original SHA-verified CPerfVisualBenchmark::Init: 22 ordered
# descriptors inserted through a six-DWORD x86 vector helper.
# Entirely independently owned, no real engine ConVars or graphics.
add_executable(sin_client_perf_visual_benchmark_owned_test
  test_client_perf_visual_benchmark_owned.cpp)
target_compile_features(sin_client_perf_visual_benchmark_owned_test PRIVATE cxx_std_17)
add_test(NAME independent_client_perf_visual_benchmark_owned_native
  COMMAND sin_client_perf_visual_benchmark_owned_test)
if(Python3_Interpreter_FOUND)
  add_test(NAME independent_original_perf_visual_benchmark_audit_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/audit_sin_original_perf_visual_benchmark.py"
      --self-test)
endif()

# Optional owner-retail phase after Stats 27:28:5 proof: read original
# soundscape scripts through Source2004 VFileSystem012, validate 7/301/242/3.
# Public CI uses synthetic native engine/vtable and synthetic documents.
add_executable(sin_client_soundscape_retail_gate_test
  test_client_soundscape_retail_gate.cpp)
target_compile_features(sin_client_soundscape_retail_gate_test PRIVATE cxx_std_17)
add_test(NAME independent_client_soundscape_retail_gate_native
  COMMAND sin_client_soundscape_retail_gate_test)

# SHA-derived original 301-node acyclic soundscape reference graph.
# Win32 fixture is fully invented, preserving no original audio bytes.
add_executable(sin_client_soundscape_graph_owned_test
  test_client_soundscape_graph_owned.cpp)
target_compile_features(sin_client_soundscape_graph_owned_test PRIVATE cxx_std_17)
add_test(NAME independent_client_soundscape_graph_owned_native
  COMMAND sin_client_soundscape_graph_owned_test)

if(Python3_Interpreter_FOUND)
  add_test(NAME independent_original_soundscape_graph_audit_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/audit_sin_original_soundscape_graph.py"
      --self-test)
endif()

# Independently owned soundscape wave declarations: original SHA-derived
# 281 loop blocks, 150 random groups, 1224 WAV/MP3 path occurrences.
# Native fixture uses invented audio names only. No mixer is called.
add_executable(sin_client_soundscape_wave_inventory_owned_test
  test_client_soundscape_wave_inventory_owned.cpp)
target_compile_features(sin_client_soundscape_wave_inventory_owned_test PRIVATE cxx_std_17)
add_test(NAME independent_client_soundscape_wave_inventory_owned_native
  COMMAND sin_client_soundscape_wave_inventory_owned_test)
if(Python3_Interpreter_FOUND)
  add_test(NAME independent_original_soundscape_waves_audit_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/audit_sin_original_soundscape_waves.py"
      --self-test)
endif()

# Phase 489: SHA-authenticated original 2006 soundscape Shutdown
# (40-byte entries, reverse stop loop, dual engine routes, KeyValues cleanup).
# No engine sound methods are called. All native fixture state is invented.
add_executable(sin_client_soundscape_original_shutdown_owned_test
  test_client_soundscape_original_shutdown_owned.cpp)
target_compile_features(sin_client_soundscape_original_shutdown_owned_test PRIVATE cxx_std_17)
add_test(NAME independent_client_soundscape_original_shutdown_owned_native
  COMMAND sin_client_soundscape_original_shutdown_owned_test)
if(Python3_Interpreter_FOUND)
  add_test(NAME independent_original_soundscape_shutdown_audit_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/audit_sin_original_soundscape_shutdown.py"
      --self-test)
endif()

# Phase 490: exact original CClientShadowMgr::Init 146-byte method:
# decals/simpleshadow resource, 3 virtual settings and callback slot116.
# Public CI never calls the renderer or original game DLL.
add_executable(sin_client_shadow_original_init_owned_test
  test_client_shadow_original_init_owned.cpp)
target_compile_features(sin_client_shadow_original_init_owned_test PRIVATE cxx_std_17)
add_test(NAME independent_client_shadow_original_init_owned_native
  COMMAND sin_client_shadow_original_init_owned_test)
if(Python3_Interpreter_FOUND)
  add_test(NAME independent_original_client_shadow_init_audit_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/audit_sin_original_client_shadow_init.py"
      --self-test)
endif()

# Phase 491: source-authenticated ShadowMgr::refresh helper at
# 0x24115E80 (125B) iterates 120-byte linked shadow records with 16-bit
# next indices. Pure fake state, no original renderer or actual callbacks.
add_executable(sin_client_shadow_original_refresh_owned_test
  test_client_shadow_original_refresh_owned.cpp)
target_compile_features(sin_client_shadow_original_refresh_owned_test PRIVATE cxx_std_17)
add_test(NAME independent_client_shadow_original_refresh_owned_native
  COMMAND sin_client_shadow_original_refresh_owned_test)
if(Python3_Interpreter_FOUND)
  add_test(NAME independent_original_shadow_refresh_audit_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/audit_sin_original_shadow_refresh.py"
      --self-test)
endif()

# Owner-retail 0x53494E76/77 soundscape refusal reason reporting:
# compiled exact formatter has no path/resource/pointer string inputs.
add_executable(sin_client_soundscape_diagnostic_formatter_test
  test_client_soundscape_diagnostic_formatter.cpp)
target_compile_features(sin_client_soundscape_diagnostic_formatter_test PRIVATE cxx_std_17)
add_test(NAME independent_client_soundscape_diagnostic_formatter_native
  COMMAND sin_client_soundscape_diagnostic_formatter_test)

add_executable(sin_client_stats_two_stage_test
  test_client_stats_two_stage_init.cpp)
target_compile_features(sin_client_stats_two_stage_test PRIVATE cxx_std_17)
add_test(NAME independent_client_stats_two_stage_native
  COMMAND sin_client_stats_two_stage_test)
add_executable(sin_client_stats_atomic_publication_test
  test_client_stats_atomic_publication.cpp)
target_compile_features(sin_client_stats_atomic_publication_test PRIVATE cxx_std_17)
add_test(NAME independent_client_stats_atomic_publication_native
  COMMAND sin_client_stats_atomic_publication_test)
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


# Phase 515: real MSVC x86 vtable-slot1 surface data handoff ABI.
# Runs only invented FileSystem and PhysicsSurfaceProps objects in CI;
# original engine mutation remains an explicit owner-side opt-in.
add_executable(sin_client_surfaceprops_registration_test
  test_client_original_surfaceprops_registration.cpp)
target_compile_features(sin_client_surfaceprops_registration_test PRIVATE cxx_std_17)
add_test(NAME independent_client_surfaceprops_registration_native
  COMMAND sin_client_surfaceprops_registration_test)


# Phase 516: test-only SDK2004 VPhysicsSurfaceProps001 vslot2/vslot3
# readback with independently invented data, no retail engine calls.
add_executable(sin_client_surfaceprops_readback_test
  test_client_original_surfaceprops_readback.cpp)
target_compile_features(sin_client_surfaceprops_readback_test PRIVATE cxx_std_17)
add_test(NAME independent_client_surfaceprops_readback_native
  COMMAND sin_client_surfaceprops_readback_test)
# Phase522: Source2004 5-float surfacephysicsparams_t and both original
# SiN DLLs vslot5; read-only data from an already populated engine table.
add_executable(sin_client_surfacephysics_data_test
  test_client_original_surfacephysics_data_probe.cpp)
target_compile_features(sin_client_surfacephysics_data_test PRIVATE cxx_std_17)
add_test(NAME independent_client_surfacephysics_data_readonly_native
  COMMAND sin_client_surfacephysics_data_test)


# SHA-witness audit of owner-authenticated 2006 client vslot3 callsites.
# Public CI exercises only invented bytecode witnesses and mutations.
if(Python3_Interpreter_FOUND)
  add_test(NAME independent_original_surfaceprops_index_calls_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/audit_sin_original_surfaceprops_index_calls.py"
      --self-test)
endif()


# Phase 517: exact original 2006 server physics factory order differs from
# client. Native fake factory only; no real engine or vphysics mutation in CI.
add_executable(sin_server_original_physics_factory_test
  test_server_original_physics_factory_probe.cpp)
target_compile_features(sin_server_original_physics_factory_test PRIVATE cxx_std_17)
add_test(NAME independent_server_original_physics_factory_native
  COMMAND sin_server_original_physics_factory_test)
# Original server 2006 rb/GAME surface manifest and two rb/null scripts
# via the independent, role-neutral, bounded VFileSystem012 parser.
add_executable(sin_server_original_surfaceprops_readonly_test
  test_server_original_surfaceprops_readonly.cpp)
target_compile_features(sin_server_original_surfaceprops_readonly_test PRIVATE cxx_std_17)
add_test(NAME independent_server_original_surfaceprops_readonly_native
  COMMAND sin_server_original_surfaceprops_readonly_test)
if(Python3_Interpreter_FOUND)
  add_test(NAME independent_original_server_physics_bootstrap_audit_synthetic
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/audit_sin_original_server_physics_bootstrap.py"
      --self-test)
endif()

# Phase666: link one genuine owned difficulty service into the shipped
# non-forwarding diagnostic server.dll. No original PE32 address binding.
target_sources(sin_slot0_server PRIVATE
  server_fixed_difficulty_service.cpp
  ../../src/fixed_difficulty_phase663.cpp
  ../../src/original_sin_trigger_entities.cpp
  ../../src/fixed_difficulty_trigger_phase663.cpp
  ../../src/original_ar_grenade_runtime.cpp
  ../../src/original_ar_grenade_touch.cpp)
target_include_directories(sin_slot0_server PRIVATE ../../include ../../third_party/research)
add_executable(sin_native_fixed_difficulty_service_test
  test_server_fixed_difficulty_service.cpp
  ../../src/source_bsp_entities.cpp
  ../../src/source_map_runtime.cpp)
target_include_directories(sin_native_fixed_difficulty_service_test PRIVATE ../../include)
target_compile_features(sin_native_fixed_difficulty_service_test PRIVATE cxx_std_17)
add_dependencies(sin_native_fixed_difficulty_service_test sin_slot0_server)
add_test(NAME independent_native_fixed_difficulty_service
  COMMAND sin_native_fixed_difficulty_service_test "$<TARGET_FILE:sin_slot0_server>")

# Phase691: compare both actual loaded PE32 DLLs' shared ammo metadata.
add_executable(sin_native_ammo_definition_pair_test test_original_ammo_definition_pair.cpp)
target_compile_features(sin_native_ammo_definition_pair_test PRIVATE cxx_std_17)
add_dependencies(sin_native_ammo_definition_pair_test sin_slot0_client sin_slot0_server)
add_dependencies(sin_native_fixed_difficulty_service_test sin_native_ammo_definition_pair_test)
add_test(NAME independent_native_ammo_definition_pair
  COMMAND sin_native_ammo_definition_pair_test "$<TARGET_FILE:sin_slot0_client>" "$<TARGET_FILE:sin_slot0_server>")

# Phase990: persistent raw scene owner drives actual loaded native ensure.
add_executable(sin_raw_scene_cache_owner_test test_server_raw_scene_cache_owner.cpp)
target_compile_features(sin_raw_scene_cache_owner_test PRIVATE cxx_std_17)
add_dependencies(sin_raw_scene_cache_owner_test sin_slot0_server)
add_test(NAME independent_native_raw_scene_cache_owner
 COMMAND sin_raw_scene_cache_owner_test "$<TARGET_FILE:sin_slot0_server>")

add_executable(sin_client_overlay_watchers_test test_native_client_overlay_watchers.cpp)
target_compile_features(sin_client_overlay_watchers_test PRIVATE cxx_std_17)
add_test(NAME independent_client_overlay_watchers_native COMMAND sin_client_overlay_watchers_test "$<TARGET_FILE:sin_slot0_client>" "$<TARGET_FILE:sin_slot0_server>")

add_executable(sin_client_overlay_history_test test_native_client_overlay_history.cpp)
target_compile_features(sin_client_overlay_history_test PRIVATE cxx_std_17)
add_test(NAME independent_client_overlay_history_native COMMAND sin_client_overlay_history_test "$<TARGET_FILE:sin_slot0_client>" "$<TARGET_FILE:sin_slot0_server>")

add_executable(sin_client_overlay_interpolate_test test_native_client_overlay_interpolate.cpp)
target_compile_features(sin_client_overlay_interpolate_test PRIVATE cxx_std_17)
add_test(NAME independent_client_overlay_interpolate_native COMMAND sin_client_overlay_interpolate_test "$<TARGET_FILE:sin_slot0_client>" "$<TARGET_FILE:sin_slot0_server>")

add_executable(sin_client_overlay_pool_test test_native_client_overlay_pool.cpp)
target_compile_features(sin_client_overlay_pool_test PRIVATE cxx_std_17)
add_test(NAME independent_client_overlay_pool_native COMMAND sin_client_overlay_pool_test "$<TARGET_FILE:sin_slot0_client>" "$<TARGET_FILE:sin_slot0_server>")

add_executable(sin_client_overlay_entity_test test_native_client_overlay_entity.cpp)
target_compile_features(sin_client_overlay_entity_test PRIVATE cxx_std_17)
add_test(NAME independent_client_overlay_entity_native COMMAND sin_client_overlay_entity_test "$<TARGET_FILE:sin_slot0_client>" "$<TARGET_FILE:sin_slot0_server>")

add_executable(sin_client_overlay_events_test test_native_client_overlay_events.cpp)
target_compile_features(sin_client_overlay_events_test PRIVATE cxx_std_17)
add_test(NAME independent_client_overlay_events_native COMMAND sin_client_overlay_events_test "$<TARGET_FILE:sin_slot0_client>" "$<TARGET_FILE:sin_slot0_server>")
