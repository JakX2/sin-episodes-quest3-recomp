cmake_minimum_required(VERSION 3.16)
project(sin_episodes_quest3_compat VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_library(sin_weapon_runtime STATIC src/weapon_runtime.cpp src/weapon_entity_io.cpp src/weapon_viewmodel_io.cpp src/weapon_viewmodel_lifecycle.cpp src/weapon_session.cpp)
target_include_directories(sin_weapon_runtime PUBLIC include third_party/research)
target_compile_options(sin_weapon_runtime PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
include(CTest)

# Original grenade touch collision guard from genuine 2006 server.dll:
# raw +0x11C, +0x120 mask 0x4, virtual +0xFC reference, separate +0x795
# flag setter and Detonate slot250. No SourceVR physics method is called.
add_library(sin_original_ar_grenade_touch STATIC
  src/original_ar_grenade_touch.cpp)
target_include_directories(sin_original_ar_grenade_touch PUBLIC include)
target_link_libraries(sin_original_ar_grenade_touch PUBLIC
  sin_original_ar_grenade_runtime)
target_compile_options(sin_original_ar_grenade_touch PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING OR (ANDROID AND SIN_BUILD_ANDROID_SMOKE))
  if(ANDROID)
    set(sin_touch_test_target sin_android_original_ar_grenade_touch_tests)
  else()
    set(sin_touch_test_target sin_original_ar_grenade_touch_tests)
  endif()
  add_executable(${sin_touch_test_target}
    tests/test_original_ar_grenade_touch.cpp)
  target_link_libraries(${sin_touch_test_target} PRIVATE
    sin_original_ar_grenade_touch)
  target_compile_options(${sin_touch_test_target} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  if(NOT ANDROID)
    add_test(NAME sin_original_ar_grenade_touch_tests
      COMMAND ${sin_touch_test_target})
  endif()
endif()

# Original SiN 2006 grenade_ar gameplay behavior reconstructed from server.dll.
# Builds as a portable static component for future native gameplay host; no
# SourceVR engine, map assets or Android runtime is linked into this module.
add_library(sin_original_ar_grenade_runtime STATIC
  src/original_ar_grenade_runtime.cpp)
target_include_directories(sin_original_ar_grenade_runtime PUBLIC include)
target_compile_options(sin_original_ar_grenade_runtime PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_ar_grenade_runtime_tests
    tests/test_original_ar_grenade_runtime.cpp)
  target_link_libraries(sin_original_ar_grenade_runtime_tests PRIVATE
    sin_original_ar_grenade_runtime)
  target_compile_options(sin_original_ar_grenade_runtime_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_ar_grenade_runtime_tests
    COMMAND sin_original_ar_grenade_runtime_tests)
endif()
# Original 2006 SiN server entity registrations. This actually constructs the
# independently reconstructed rifle WeaponSession and CGrenadeAR Runtime when
# the native host supplies a live entity ref. No guessed SourceVR game ABI.
add_library(sin_original_gameplay_entity_factory STATIC
  src/original_gameplay_entity_factory.cpp)
target_include_directories(sin_original_gameplay_entity_factory PUBLIC include)
target_link_libraries(sin_original_gameplay_entity_factory PUBLIC
  sin_weapon_runtime sin_original_ar_grenade_touch sin_original_ar_grenade_runtime)
target_compile_options(sin_original_gameplay_entity_factory PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING OR (ANDROID AND SIN_BUILD_ANDROID_SMOKE))
  if(ANDROID)
    set(sin_entity_factory_test_target sin_android_original_gameplay_entity_factory_tests)
  else()
    set(sin_entity_factory_test_target sin_original_gameplay_entity_factory_tests)
  endif()
  add_executable(${sin_entity_factory_test_target}
    tests/test_original_gameplay_entity_factory.cpp)
  target_link_libraries(${sin_entity_factory_test_target} PRIVATE
    sin_original_gameplay_entity_factory)
  target_compile_options(${sin_entity_factory_test_target} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  if(NOT ANDROID)
    add_test(NAME sin_original_gameplay_entity_factory_tests
      COMMAND ${sin_entity_factory_test_target})
  endif()
endif()

# Original 2006 rifle secondary-fire to CGrenadeAR gameplay integration:
# authoritative player secondary ammo, source-backed player/NPC velocity
# multipliers, generation-safe weapon selection and atomic host publication.
# Engine-neutral only. NOT a SourceVR weapon or native game-server entity.
add_library(sin_original_rifle_grenade_launch STATIC
  src/original_rifle_grenade_launch.cpp)
target_include_directories(sin_original_rifle_grenade_launch PUBLIC include)
target_link_libraries(sin_original_rifle_grenade_launch PUBLIC
  sin_original_ar_grenade_runtime sin_weapon_runtime)
target_compile_options(sin_original_rifle_grenade_launch PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING OR (ANDROID AND SIN_BUILD_ANDROID_SMOKE))
  if(ANDROID)
    set(sin_rifle_grenade_test_target
      sin_android_original_rifle_grenade_launch_tests)
  else()
    set(sin_rifle_grenade_test_target
      sin_original_rifle_grenade_launch_tests)
  endif()
  add_executable(${sin_rifle_grenade_test_target}
    tests/test_original_rifle_grenade_launch.cpp)
  target_link_libraries(${sin_rifle_grenade_test_target} PRIVATE
    sin_original_rifle_grenade_launch)
  target_compile_options(${sin_rifle_grenade_test_target} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  if(NOT ANDROID)
    add_test(NAME sin_original_rifle_grenade_launch_tests
      COMMAND ${sin_rifle_grenade_test_target})
  endif()
endif()

# Source-derived SiN assault-rifle settings consumed by the existing
# engine-neutral weapon state machine (not a native Source game entity).
if(BUILD_TESTING)
  add_executable(sin_original_assault_rifle_profile_tests
    tests/test_original_assault_rifle_profile.cpp)
  target_link_libraries(sin_original_assault_rifle_profile_tests PRIVATE sin_weapon_runtime)
  add_test(NAME sin_original_assault_rifle_profile_tests
    COMMAND sin_original_assault_rifle_profile_tests)
endif()
# Source-derived original MobGrunt assault rally point ranking reference.
# Tests verify priority + squared-distance tie-break only, not native NPC AI.
if(BUILD_TESTING)
  add_executable(sin_mobgrunt_rally_rank_tests tests/test_mobgrunt_rally_rank.cpp)
  target_include_directories(sin_mobgrunt_rally_rank_tests PRIVATE include)
  target_compile_options(sin_mobgrunt_rally_rank_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_mobgrunt_rally_rank_tests COMMAND sin_mobgrunt_rally_rank_tests)
endif()

# A synthetic two-MobGrunt integration test joins ranking, exclusive
# rally ownership and callback-driven original assault task progression.
# These simulated NPCs do not instantiate any Source engine entities.
if(BUILD_TESTING)
  add_executable(sin_assault_simulated_encounter_tests
    tests/test_assault_simulated_encounter.cpp)
  target_include_directories(sin_assault_simulated_encounter_tests PRIVATE include)
  target_compile_options(sin_assault_simulated_encounter_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_assault_simulated_encounter_tests
    COMMAND sin_assault_simulated_encounter_tests)
endif()

# Engine-neutral host-driven assault task runner. Original tasks and interrupt
# declarations are source-verified; callbacks are a new, explicit host boundary.
# No native SourceVR AI schedule, NPC factory or game module registration.
if(BUILD_TESTING)
  add_executable(sin_assault_schedule_runner_tests tests/test_assault_schedule_runner.cpp)
  target_include_directories(sin_assault_schedule_runner_tests PRIVATE include)
  target_compile_options(sin_assault_schedule_runner_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_assault_schedule_runner_tests COMMAND sin_assault_schedule_runner_tests)
endif()

# Host-side orchestration of original rally ranking, lease and cue outcomes.
# This test does not construct a SourceVR NPC or touch game assets.
if(BUILD_TESTING)
  add_executable(sin_assault_rally_binding_tests
    tests/test_assault_rally_binding.cpp)
  target_include_directories(sin_assault_rally_binding_tests PRIVATE include)
  target_compile_options(sin_assault_rally_binding_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_assault_rally_binding_tests
    COMMAND sin_assault_rally_binding_tests)
endif()

# Original x86 CAI_AssaultBehavior::SetParameters outcome distinctions:
# no rally, unresolved rally handle, successful link + cue reset.
# Only a portable source-grounded reference, not SourceVR NPC integration.
if(BUILD_TESTING)
  add_executable(sin_original_assault_setparameters_outcomes_tests
    tests/test_original_assault_setparameters_outcomes.cpp)
  target_include_directories(sin_original_assault_setparameters_outcomes_tests PRIVATE include)
  target_compile_options(sin_original_assault_setparameters_outcomes_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_assault_setparameters_outcomes_tests
    COMMAND sin_original_assault_setparameters_outcomes_tests)
endif()

# Original 2006 Source condition registry IDs (names and x86 integer values).
# Keep historic condition/task/schedule ID namespaces strictly separated.
if(BUILD_TESTING)
  add_executable(sin_original_assault_condition_ids_tests
    tests/test_original_assault_condition_ids.cpp)
  target_include_directories(sin_original_assault_condition_ids_tests PRIVATE include)
  target_compile_options(sin_original_assault_condition_ids_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_assault_condition_ids_tests
    COMMAND sin_original_assault_condition_ids_tests)
endif()

# Original ai_goal_assault BeginAssault input: setup, per-actor cue=1
# and native NPC condition ID 0x18, plus direct MobGrunt mode-3 distinction.
# Engine host owns real Source actor list, state reset and condition setting.
if(BUILD_TESTING)
  add_executable(sin_original_goal_assault_input_tests
    tests/test_original_goal_assault_input.cpp)
  target_include_directories(sin_original_goal_assault_input_tests PRIVATE include)
  target_compile_options(sin_original_goal_assault_input_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_goal_assault_input_tests
    COMMAND sin_original_goal_assault_input_tests)
endif()

# Original SiN 2006 AssaultCue five-way predicate and deterministic x87
# decisecond wait completion: engine-neutral typed reference and host test.
if(BUILD_TESTING)
  add_executable(sin_original_assault_cue_wait_tests
    tests/test_original_assault_cue_wait.cpp)
  target_include_directories(sin_original_assault_cue_wait_tests PRIVATE include)
  target_compile_options(sin_original_assault_cue_wait_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_assault_cue_wait_tests
    COMMAND sin_original_assault_cue_wait_tests)
endif()

# Direct source-derived behavior effects for arrival outputs, map-configured
# waits and deferred schedule-selection timestamp; not engine task handlers.
if(BUILD_TESTING)
  add_executable(sin_original_assault_task_effects_tests
    tests/test_original_assault_task_effects.cpp)
  target_include_directories(sin_original_assault_task_effects_tests PRIVATE include)
  target_compile_options(sin_original_assault_task_effects_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_assault_task_effects_tests
    COMMAND sin_original_assault_task_effects_tests)
endif()

# Original 2006 SiN 11 task IDs and the separate schedule ID namespace;
# typed metadata only, never native SourceVR task/schedule registration.
if(BUILD_TESTING)
  add_executable(sin_original_assault_task_ids_tests
    tests/test_original_assault_task_ids.cpp)
  target_include_directories(sin_original_assault_task_ids_tests PRIVATE include)
  target_compile_options(sin_original_assault_task_ids_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_assault_task_ids_tests
    COMMAND sin_original_assault_task_ids_tests)
endif()

# Original 2006 assault schedule task/interrupt skeletons recovered directly
# from server.dll text; reference metadata only, no native AI task execution.
if(BUILD_TESTING)
  add_executable(sin_original_assault_task_tests tests/test_original_assault_tasks.cpp)
  target_include_directories(sin_original_assault_task_tests PRIVATE include)
  target_compile_options(sin_original_assault_task_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_assault_task_tests COMMAND sin_original_assault_task_tests)
endif()

# Recovered 2006 assault schedule registry. Strong task vs schedule ID types
# prevent using legacy Windows x86 numeric IDs as new native engine schedules.
if(BUILD_TESTING)
  add_executable(sin_original_assault_schedule_tests tests/test_original_assault_schedule.cpp)
  target_include_directories(sin_original_assault_schedule_tests PRIVATE include)
  target_compile_options(sin_original_assault_schedule_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_assault_schedule_tests COMMAND sin_original_assault_schedule_tests)
endif()

# Original SiN 2006 CAssaultRallyPoint::Lock/Unlock reference, backed by
# real x86 call-graph research. No engine-owned EHANDLE pointers exported.
if(BUILD_TESTING)
  add_executable(sin_rally_point_lease_tests tests/test_rally_point_lease.cpp)
  target_include_directories(sin_rally_point_lease_tests PRIVATE include)
  target_compile_options(sin_rally_point_lease_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_rally_point_lease_tests COMMAND sin_rally_point_lease_tests)
endif()

if(BUILD_TESTING)
  add_executable(sin_weapon_runtime_tests tests/test_weapon_runtime.cpp)
  target_link_libraries(sin_weapon_runtime_tests PRIVATE sin_weapon_runtime)
  add_test(NAME sin_weapon_runtime_tests COMMAND sin_weapon_runtime_tests)
  add_executable(sin_weapon_entity_io_tests tests/test_weapon_entity_io.cpp)
  target_link_libraries(sin_weapon_entity_io_tests PRIVATE sin_weapon_runtime)
  add_test(NAME sin_weapon_entity_io_tests COMMAND sin_weapon_entity_io_tests)
endif()

if(BUILD_TESTING)
  add_executable(sin_weapon_viewmodel_io_tests tests/test_weapon_viewmodel_io.cpp)
  target_link_libraries(sin_weapon_viewmodel_io_tests PRIVATE sin_weapon_runtime)
  add_test(NAME sin_weapon_viewmodel_io_tests COMMAND sin_weapon_viewmodel_io_tests)
endif()

if(BUILD_TESTING)
  add_executable(sin_weapon_viewmodel_lifecycle_tests tests/test_weapon_viewmodel_lifecycle.cpp)
  target_link_libraries(sin_weapon_viewmodel_lifecycle_tests PRIVATE sin_weapon_runtime)
  add_test(NAME sin_weapon_viewmodel_lifecycle_tests COMMAND sin_weapon_viewmodel_lifecycle_tests)
endif()

if(BUILD_TESTING)
  add_executable(sin_weapon_session_tests tests/test_weapon_session.cpp)
  target_link_libraries(sin_weapon_session_tests PRIVATE sin_weapon_runtime)
  add_test(NAME sin_weapon_session_tests COMMAND sin_weapon_session_tests)
endif()

# A testable C ABI shared module for a future engine to call. This is NOT a
# SourceVR game DLL/plugin, which requires real engine-specific headers/loader.
set_target_properties(sin_weapon_runtime PROPERTIES POSITION_INDEPENDENT_CODE ON CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
add_library(sin_compat_host_abi SHARED src/host_abi_v1.cpp)
target_link_libraries(sin_compat_host_abi PRIVATE sin_weapon_runtime)
target_include_directories(sin_compat_host_abi PUBLIC include)
# The Android NDK's static libc++ library can inject hundreds of public
# C++ dynamic exports. Bind the ELF symbol surface to the intentional v1 ABI.
# This is also applied to Linux host tests so CI catches accidental ABI growth.
if(UNIX AND NOT APPLE)
  target_link_options(sin_compat_host_abi PRIVATE
    "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/src/host_abi_exports.map")
  set_property(TARGET sin_compat_host_abi APPEND PROPERTY LINK_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/host_abi_exports.map")
endif()
set_target_properties(sin_compat_host_abi PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)

# Original SiN rifle metadata + portable C session bridge. This is NOT a
# registered native Source entity; SourceVR factory integration remains open.
add_library(sin_original_rifle_bridge SHARED src/original_rifle_c_api_v1.cpp)
target_include_directories(sin_original_rifle_bridge PUBLIC include)
target_link_libraries(sin_original_rifle_bridge PRIVATE sin_compat_host_abi sin_weapon_runtime)
set_target_properties(sin_original_rifle_bridge PROPERTIES
    CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
if(UNIX AND NOT APPLE)
    target_link_options(sin_original_rifle_bridge PRIVATE
      "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/src/original_rifle_c_api_exports.map")
    set_property(TARGET sin_original_rifle_bridge APPEND PROPERTY LINK_DEPENDS
      "${CMAKE_CURRENT_SOURCE_DIR}/src/original_rifle_c_api_exports.map")
endif()
if(BUILD_TESTING)
    add_executable(sin_original_rifle_c_bridge_tests tests/test_original_rifle_c_api_v1.c)
    target_include_directories(sin_original_rifle_c_bridge_tests PRIVATE include)
    target_link_libraries(sin_original_rifle_c_bridge_tests
      PRIVATE sin_original_rifle_bridge sin_compat_host_abi)
    set_target_properties(sin_original_rifle_c_bridge_tests PROPERTIES
      C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
    add_test(NAME sin_original_rifle_c_bridge_tests
      COMMAND sin_original_rifle_c_bridge_tests)
endif()

if(BUILD_TESTING AND UNIX)
  add_executable(sin_host_abi_dlopen_tests tests/test_host_abi.c)
  target_include_directories(sin_host_abi_dlopen_tests PRIVATE include)
  set_target_properties(sin_host_abi_dlopen_tests PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
  target_link_libraries(sin_host_abi_dlopen_tests PRIVATE ${CMAKE_DL_LIBS})
  add_test(NAME sin_host_abi_dlopen_tests COMMAND sin_host_abi_dlopen_tests $<TARGET_FILE:sin_compat_host_abi>)
endif()

# Independently compiled C ABI layout checks, executable on host. The same C
# source is also compiled freestanding for AArch64 Android in Phase 68 CI.
if(BUILD_TESTING)
  add_executable(sin_host_abi_layout_tests tests/abi_layout_contract.c)
  target_include_directories(sin_host_abi_layout_tests PRIVATE include)
  set_target_properties(sin_host_abi_layout_tests PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
  add_test(NAME sin_host_abi_layout_tests COMMAND sin_host_abi_layout_tests)
endif()

# Phase 70: build a *real Android executable* which dlopens the Android .so.
# Unlike an SDK-less compile, this is linked against Android Bionic by the NDK.
# Do not conflate the executable's build with running it on a physical device.
option(SIN_BUILD_ANDROID_SMOKE "Build a standalone Android dlopen smoke executable" OFF)
if(SIN_BUILD_ANDROID_SMOKE AND NOT ANDROID)
  message(FATAL_ERROR "SIN_BUILD_ANDROID_SMOKE requires the official Android NDK toolchain")
endif()
if(SIN_BUILD_ANDROID_SMOKE AND ANDROID)
  add_executable(sin_android_host_abi_smoke tests/test_host_abi.c)
  target_include_directories(sin_android_host_abi_smoke PRIVATE include)
  target_compile_options(sin_android_host_abi_smoke PRIVATE -Wall -Wextra -Werror)
  target_link_libraries(sin_android_host_abi_smoke PRIVATE dl)
  set_target_properties(sin_android_host_abi_smoke PROPERTIES
    C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
  add_dependencies(sin_android_host_abi_smoke sin_compat_host_abi)
endif()


# Cross-DLL dynamic-difficulty typed-call reference only. This is deliberately
# separate from any SourceVR/Android engine module or network bitstream codec.
if(BUILD_TESTING)
  add_executable(sin_difficulty_message_contract_tests tests/test_difficulty_message_contract.cpp)
  target_include_directories(sin_difficulty_message_contract_tests PRIVATE include)
  target_compile_options(sin_difficulty_message_contract_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
  add_test(NAME sin_difficulty_message_contract_tests COMMAND sin_difficulty_message_contract_tests)
endif()

# Phase 75: original 2006 DDInfo typed-call reference and client flag override.
if(BUILD_TESTING)
  add_executable(sin_difficulty_info_contract_tests tests/test_difficulty_info_contract.cpp)
  target_include_directories(sin_difficulty_info_contract_tests PRIVATE include)
  target_compile_options(sin_difficulty_info_contract_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
  add_test(NAME sin_difficulty_info_contract_tests COMMAND sin_difficulty_info_contract_tests)
endif()


# Phase 76: original SiN advisor score/channel typed operation reference.
if(BUILD_TESTING)
  add_executable(sin_difficulty_advisors_contract_tests
    tests/test_difficulty_advisors_contract.cpp)
  target_include_directories(sin_difficulty_advisors_contract_tests PRIVATE include)
  target_compile_options(sin_difficulty_advisors_contract_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
  add_test(NAME sin_difficulty_advisors_contract_tests
    COMMAND sin_difficulty_advisors_contract_tests)
endif()


# Phase 77: original SiN dynamic-difficulty manager archive-state contract.
if(BUILD_TESTING)
  add_executable(sin_difficulty_state_archive_tests
    tests/test_difficulty_state_archive_contract.cpp)
  target_include_directories(sin_difficulty_state_archive_tests PRIVATE include)
  target_compile_options(sin_difficulty_state_archive_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
  add_test(NAME sin_difficulty_state_archive_tests
    COMMAND sin_difficulty_state_archive_tests)
endif()

# Phase 78: original SiN named difficulty metric math and modifier clamps.
if(BUILD_TESTING)
  add_executable(sin_difficulty_metric_math_tests
    tests/test_difficulty_metric_math.cpp)
  target_include_directories(sin_difficulty_metric_math_tests PRIVATE include)
  target_compile_options(sin_difficulty_metric_math_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
  add_test(NAME sin_difficulty_metric_math_tests
    COMMAND sin_difficulty_metric_math_tests)
endif()


# Phase 79: verified SiN difficulty gameplay metric consumption reference.
if(BUILD_TESTING)
  add_executable(sin_difficulty_metric_consumers_tests
    research/tests/sin_metric_consumers_reference.cpp)
  target_include_directories(sin_difficulty_metric_consumers_tests PRIVATE include)
  target_compile_options(sin_difficulty_metric_consumers_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
  add_test(NAME sin_difficulty_metric_consumers_tests
    COMMAND sin_difficulty_metric_consumers_tests)
endif()

# Phase 80: verified SiN dd_challenge ConVar and advisor preset math.
if(BUILD_TESTING)
  add_executable(sin_difficulty_advisor_preset_tests
    research/tests/sin_difficulty_advisor_presets_reference.cpp)
  target_include_directories(sin_difficulty_advisor_preset_tests PRIVATE include)
  target_compile_options(sin_difficulty_advisor_preset_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
  add_test(NAME sin_difficulty_advisor_preset_tests
    COMMAND sin_difficulty_advisor_preset_tests)
endif()

# Phase 81: original SiN CStatAdvisor state, listener notification and
# DDAdvisorScores/DDAdvisorChannels integration reference.
if(BUILD_TESTING)
  add_executable(sin_difficulty_advisor_state_flow_tests
    research/tests/sin_difficulty_advisor_state_flow_reference.cpp)
  target_include_directories(sin_difficulty_advisor_state_flow_tests PRIVATE include)
  target_compile_options(sin_difficulty_advisor_state_flow_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
  add_test(NAME sin_difficulty_advisor_state_flow_tests
    COMMAND sin_difficulty_advisor_state_flow_tests)
endif()


# Phase 84: independent original SiN x86 CreateInterface registry behavior
# contract; not a SourceVR CreateInterface game module ABI implementation.
if(BUILD_TESTING)
  add_executable(sin_original_factory_contract_tests
    research/tests/sin_original_factory_reference.cpp)
  target_include_directories(sin_original_factory_contract_tests PRIVATE include)
  target_compile_options(sin_original_factory_contract_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
  add_test(NAME sin_original_factory_contract_tests
    COMMAND sin_original_factory_contract_tests)
endif()


# Phase 85: original SiN 2006 client.dll/server.dll ordered engine
# factory handshake reference. Not a SourceVR-native module Init() ABI.
if(BUILD_TESTING)
  add_executable(sin_original_init_contract_tests
    research/tests/sin_original_init_contract_reference.cpp)
  target_include_directories(sin_original_init_contract_tests PRIVATE include)
  target_compile_options(sin_original_init_contract_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
  add_test(NAME sin_original_init_contract_tests
    COMMAND sin_original_init_contract_tests)
endif()


# Phase 86: original client.dll/server.dll post-core Init gates and
# nonfatal factory/allocation paths, not an Android SourceVR module.
if(BUILD_TESTING)
  add_executable(sin_original_postinit_contract_tests
    research/tests/test_original_postinit_contract.cpp)
  target_include_directories(sin_original_postinit_contract_tests PRIVATE include)
  target_compile_options(sin_original_postinit_contract_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
  add_test(NAME sin_original_postinit_contract_tests
    COMMAND sin_original_postinit_contract_tests)
endif()


# Phase 87: original client/server registration and subsystem-startup
# behavioral reference; not the Android SourceVR game module ABI.
if(BUILD_TESTING)
  add_executable(sin_original_subsystem_startup_tests
    research/tests/sin_original_subsystem_startup_reference.cpp)
  target_include_directories(sin_original_subsystem_startup_tests PRIVATE include)
  target_compile_options(sin_original_subsystem_startup_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
  add_test(NAME sin_original_subsystem_startup_tests
    COMMAND sin_original_subsystem_startup_tests)
endif()


# Phase 88: original 2006 client/server shutdown and cache lifecycle contract.
# Host test only, not the actual SourceVR ARM64 game engine interface.
if(BUILD_TESTING)
  add_executable(sin_original_shutdown_contract_tests
    research/tests/sin_original_shutdown_reference.cpp)
  target_include_directories(sin_original_shutdown_contract_tests PRIVATE include)
  target_compile_options(sin_original_shutdown_contract_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
  add_test(NAME sin_original_shutdown_contract_tests
    COMMAND sin_original_shutdown_contract_tests)
endif()


# Phase 89: original 2006 SiN server cache read-header guards and
# client release/teardown reference; no actual filesystem or SourceVR ABI.
if(BUILD_TESTING)
  add_executable(sin_original_cache_header_client_cleanup_tests
    research/tests/sin_original_cache_header_client_cleanup_reference.cpp)
  target_include_directories(sin_original_cache_header_client_cleanup_tests PRIVATE include)
  target_compile_options(sin_original_cache_header_client_cleanup_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_cache_header_client_cleanup_tests
    COMMAND sin_original_cache_header_client_cleanup_tests)
endif()


# Phase 90: original SiN two-cache in-memory record addressing and
# client late shutdown virtual-dispatch ordering (not SourceVR ABI).
if(BUILD_TESTING)
  add_executable(sin_original_cache_record_dispatch_tests
    research/tests/sin_original_cache_record_dispatch_reference.cpp)
  target_include_directories(sin_original_cache_record_dispatch_tests PRIVATE include)
  target_compile_options(sin_original_cache_record_dispatch_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_cache_record_dispatch_tests
    COMMAND sin_original_cache_record_dispatch_tests)
endif()


# Phase 91: original SiN client HUD shutdown class RTTI and server cache
# concrete RTTI handler vtables, host-only C++17 reference.
if(BUILD_TESTING)
  add_executable(sin_original_rtti_handler_tests
    research/tests/sin_original_rtti_handler_reference.cpp)
  target_include_directories(sin_original_rtti_handler_tests PRIVATE include)
  target_compile_options(sin_original_rtti_handler_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_rtti_handler_tests
    COMMAND sin_original_rtti_handler_tests)
endif()


# Phase 92: original SiN 2006 client HUD release shapes and server
# cache virtual read/write call ordering. Host-only reference, not SourceVR ABI.
if(BUILD_TESTING)
  add_executable(sin_original_cache_io_hud_lifecycle_tests
    research/tests/sin_original_cache_io_hud_lifecycle_reference.cpp)
  target_include_directories(sin_original_cache_io_hud_lifecycle_tests PRIVATE include)
  target_compile_options(sin_original_cache_io_hud_lifecycle_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_cache_io_hud_lifecycle_tests
    COMMAND sin_original_cache_io_hud_lifecycle_tests)
endif()


# Phase 93: original 2006 SiN HUD constructor x86 layout and server cache
# virtual slot-2 filtering. Host mock only, not a SourceVR game module.
if(BUILD_TESTING)
  add_executable(sin_original_hud_ctor_cache_slot2_tests
    research/tests/sin_original_hud_ctor_cache_slot2_reference.cpp)
  target_include_directories(sin_original_hud_ctor_cache_slot2_tests PRIVATE include)
  target_compile_options(sin_original_hud_ctor_cache_slot2_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_hud_ctor_cache_slot2_tests
    COMMAND sin_original_hud_ctor_cache_slot2_tests)
endif()


# Phase94: original 2006 client loading disc and server model sound
# event mapping. Host-only reference; NOT original SourceVR native ABI.
if(BUILD_TESTING)
  add_executable(sin_original_loadingdisc_modelsound_tests
    research/tests/sin_original_loadingdisc_modelsound_reference.cpp)
  target_include_directories(sin_original_loadingdisc_modelsound_tests PRIVATE include)
  target_compile_options(sin_original_loadingdisc_modelsound_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_loadingdisc_modelsound_tests
    COMMAND sin_original_loadingdisc_modelsound_tests)
endif()


# Phase 95: original SiN 2006 nested CLoadingDiscPanel/vgui::Label and
# server VSoundEmitter001 mapping. Host-only contract, not Android game ABI.
if(BUILD_TESTING)
  add_executable(sin_original_loading_panel_emitter_tests
    research/tests/sin_original_loading_panel_sound_emitter_reference.cpp)
  target_include_directories(sin_original_loading_panel_emitter_tests PRIVATE include)
  target_compile_options(sin_original_loading_panel_emitter_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_loading_panel_emitter_tests
    COMMAND sin_original_loading_panel_emitter_tests)
endif()


# Phase 96: original SiN 2006 PE32 vs SourceVR 0.5.5 ARM64
# CreateInterface VERSION preflight. This is not a native game module.
if(BUILD_TESTING)
  add_executable(sin_sourcevr_factory_preflight_tests
    research/tests/sin_sourcevr_factory_preflight_reference.cpp)
  target_include_directories(sin_sourcevr_factory_preflight_tests PRIVATE include)
  target_compile_options(sin_sourcevr_factory_preflight_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_sourcevr_factory_preflight_tests
    COMMAND sin_sourcevr_factory_preflight_tests)
endif()


# Reconstructed original SiN 2006 server.dll/client.dll required factory
# lookups and common null-failure edges. Preflight only, no game module.
if(BUILD_TESTING)
  add_executable(sin_original_dllinit_factory_contract_tests
    tests/test_original_dllinit_factory_contract.cpp)
  target_include_directories(sin_original_dllinit_factory_contract_tests PRIVATE include)
  target_compile_options(sin_original_dllinit_factory_contract_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_dllinit_factory_contract_tests
    COMMAND sin_original_dllinit_factory_contract_tests)
endif()

# Engine-backed native client/server startup dispatch evidence.
# C++ fixture checks metadata only and NEVER invokes unknown engine ABI.
if(BUILD_TESTING)
  add_executable(sin_sourcevr_game_startup_abi_tests
    research/tests/sin_sourcevr_game_startup_abi_reference.cpp)
  target_include_directories(sin_sourcevr_game_startup_abi_tests PRIVATE include)
  target_compile_options(sin_sourcevr_game_startup_abi_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_sourcevr_game_startup_abi_tests
    COMMAND sin_sourcevr_game_startup_abi_tests)
endif()

# Recovered concrete client/server native RTTI vtables (47/78 slots).
# No C++ game dispatch: actual method signatures and lifecycle remain blocked.
if(BUILD_TESTING)
  add_executable(sin_sourcevr_game_vtable_layout_tests
    research/tests/sin_sourcevr_game_vtable_layout_reference.cpp)
  target_include_directories(sin_sourcevr_game_vtable_layout_tests PRIVATE include)
  target_compile_options(sin_sourcevr_game_vtable_layout_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_sourcevr_game_vtable_layout_tests
    COMMAND sin_sourcevr_game_vtable_layout_tests)
endif()

# Actual seven-profile SourceVR 0.5.5 ARM64 game module factory names,
# separately proved against original SiN server/client and native registrar.
# Both factory interfaces are DISALLOWED until a real compatibility ABI exists.
if(BUILD_TESTING)
  add_executable(sin_sourcevr_game_factory_preflight_tests
    research/tests/sin_sourcevr_game_factory_preflight_reference.cpp)
  target_include_directories(sin_sourcevr_game_factory_preflight_tests PRIVATE include)
  target_compile_options(sin_sourcevr_game_factory_preflight_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_sourcevr_game_factory_preflight_tests
    COMMAND sin_sourcevr_game_factory_preflight_tests)
endif()

# Phase 97: exact original SiN vs SourceVR 0.5.5 virtual-slot proof.
# Host-only C++17 safety gate; not a native SiN libclient/libserver module.
if(BUILD_TESTING)
  add_executable(sin_sourcevr_method_slot_tests
    research/tests/sin_sourcevr_method_slot_reference.cpp)
  target_include_directories(sin_sourcevr_method_slot_tests PRIVATE include)
  target_compile_options(sin_sourcevr_method_slot_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_sourcevr_method_slot_tests
    COMMAND sin_sourcevr_method_slot_tests)
endif()


# Phase98: original 2006 SoundEmitterSystem.dll and vgui2.dll compared
# with actual SourceVR 0.5.5 ARM64 methods; no unverified runtime alias.
if(BUILD_TESTING)
  add_executable(sin_original_engine_sound_vgui_tests
    research/tests/sin_original_engine_sound_vgui_reference.cpp)
  target_include_directories(sin_original_engine_sound_vgui_tests PRIVATE include)
  target_compile_options(sin_original_engine_sound_vgui_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_engine_sound_vgui_tests
    COMMAND sin_original_engine_sound_vgui_tests)
endif()


# Phase99: original 2006 VGUI_System010 key translation and separate
# original/native sound token namespace. No unverified ARM64 vtable calls.
if(BUILD_TESTING)
  add_executable(sin_legacy_sound_vgui_prebridge_tests
    research/tests/sin_legacy_sound_vgui_prebridge_reference.cpp)
  target_include_directories(sin_legacy_sound_vgui_prebridge_tests PRIVATE include)
  target_compile_options(sin_legacy_sound_vgui_prebridge_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_legacy_sound_vgui_prebridge_tests
    COMMAND sin_legacy_sound_vgui_prebridge_tests)
endif()


# Phase100: real standalone legacy keyboard/sound C ABI; NOT SourceVR game DLL.
add_library(sin_legacy_bridge_abi SHARED src/legacy_bridge_c_api.cpp)
target_include_directories(sin_legacy_bridge_abi PUBLIC include)
target_compile_definitions(sin_legacy_bridge_abi PRIVATE SIN100_BUILD_SHARED=1)
target_compile_options(sin_legacy_bridge_abi PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
set_target_properties(sin_legacy_bridge_abi PROPERTIES
  CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
if(UNIX AND NOT APPLE)
  target_link_options(sin_legacy_bridge_abi PRIVATE
    "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/src/legacy_bridge_exports.map")
  set_property(TARGET sin_legacy_bridge_abi APPEND PROPERTY LINK_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/legacy_bridge_exports.map")
endif()
if(BUILD_TESTING)
  add_executable(sin100_sha256_tests tests/sin100_sha256_tests.cpp)
  target_include_directories(sin100_sha256_tests PRIVATE include)
  target_compile_options(sin100_sha256_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin100_sha256_tests COMMAND sin100_sha256_tests)
  if(UNIX AND NOT ANDROID)
    add_executable(sin100_dlopen_smoke tests/sin100_dlopen_smoke.c)
    target_include_directories(sin100_dlopen_smoke PRIVATE include)
    set_target_properties(sin100_dlopen_smoke PROPERTIES
      C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
    target_compile_options(sin100_dlopen_smoke PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_link_libraries(sin100_dlopen_smoke PRIVATE ${CMAKE_DL_LIBS})
    add_test(NAME sin100_dlopen_smoke
      COMMAND sin100_dlopen_smoke $<TARGET_FILE:sin_legacy_bridge_abi>)
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
      add_test(NAME sin100_python_ctypes
        COMMAND ${Python3_EXECUTABLE}
          ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_phase100_ctypes.py
          $<TARGET_FILE:sin_legacy_bridge_abi>)
    endif()
  endif()
endif()
if(SIN_BUILD_ANDROID_SMOKE AND ANDROID)
  add_executable(sin_android_legacy_bridge_smoke tests/sin100_dlopen_smoke.c)
  target_include_directories(sin_android_legacy_bridge_smoke PRIVATE include)
  target_link_libraries(sin_android_legacy_bridge_smoke PRIVATE dl)
  target_compile_options(sin_android_legacy_bridge_smoke PRIVATE -Wall -Wextra -Werror)
  set_target_properties(sin_android_legacy_bridge_smoke PROPERTIES
    C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
  add_dependencies(sin_android_legacy_bridge_smoke sin_legacy_bridge_abi)
endif()

# Phase101: separate provider C ABI; original Phase100 exports unchanged.
add_library(sin_source_provider_abi SHARED src/source_provider_c_api.cpp)
target_include_directories(sin_source_provider_abi PUBLIC include)
target_compile_definitions(sin_source_provider_abi PRIVATE SIN101_BUILD_SHARED=1)
target_compile_options(sin_source_provider_abi PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
set_target_properties(sin_source_provider_abi PROPERTIES
  CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
if(UNIX AND NOT APPLE)
  target_link_libraries(sin_source_provider_abi PRIVATE ${CMAKE_DL_LIBS})
  target_link_options(sin_source_provider_abi PRIVATE
    "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/src/source_provider_exports.map")
  set_property(TARGET sin_source_provider_abi APPEND PROPERTY LINK_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/source_provider_exports.map")
endif()
if(BUILD_TESTING)
  add_executable(sin101_provider_version_tests tests/sin101_provider_version.cpp)
  target_include_directories(sin101_provider_version_tests PRIVATE include)
  target_link_libraries(sin101_provider_version_tests PRIVATE sin_source_provider_abi)
  target_compile_options(sin101_provider_version_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin101_provider_version_tests COMMAND sin101_provider_version_tests)
endif()


# Phase101 cross-library provider tests and real Android Bionic executable.
if(BUILD_TESTING)
  foreach(test_name sin101_two_library_smoke sin101_mock_provider_roundtrip)
    add_executable(${test_name} tests/${test_name}.cpp)
    target_include_directories(${test_name} PRIVATE include)
    target_link_libraries(${test_name} PRIVATE sin_source_provider_abi)
    target_compile_options(${test_name} PRIVATE
      $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
    add_test(NAME ${test_name}
      COMMAND ${test_name} $<TARGET_FILE:sin_legacy_bridge_abi>)
  endforeach()
endif()
if(SIN_BUILD_ANDROID_SMOKE AND ANDROID)
  add_executable(sin_android_source_provider_smoke
    tests/sin101_mock_provider_roundtrip.cpp)
  target_include_directories(sin_android_source_provider_smoke PRIVATE include)
  target_link_libraries(sin_android_source_provider_smoke PRIVATE sin_source_provider_abi)
  target_compile_options(sin_android_source_provider_smoke PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_dependencies(sin_android_source_provider_smoke sin_legacy_bridge_abi)
endif()


# Phase102: provider-generation isolation, keeping verified VGUI keys on
# registration and rejecting stale legacy/native sound mappings.
if(BUILD_TESTING OR (SIN_BUILD_ANDROID_SMOKE AND ANDROID))
  find_package(Threads REQUIRED)
  add_executable(sin102_provider_lifecycle tests/sin102_provider_lifecycle.cpp)
  target_include_directories(sin102_provider_lifecycle PRIVATE include)
  target_link_libraries(sin102_provider_lifecycle PRIVATE
    sin_source_provider_abi Threads::Threads)
  target_compile_options(sin102_provider_lifecycle PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_dependencies(sin102_provider_lifecycle sin_legacy_bridge_abi)
  if(BUILD_TESTING AND NOT ANDROID)
    add_test(NAME sin102_provider_lifecycle
      COMMAND sin102_provider_lifecycle $<TARGET_FILE:sin_legacy_bridge_abi>)
  endif()
endif()
if(BUILD_TESTING AND NOT ANDROID)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin102_native_sound_abi_metadata_tests
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tests/test_phase102_native_abi.py)
  endif()
endif()


# November 7 scripted-character proof: host-only original JC/Jessica mapping.
# No real SourceVR/Android game entity factory is implemented by this target.
add_library(sin_scene_actor_bridge STATIC src/scene_actor_bridge.cpp)
target_include_directories(sin_scene_actor_bridge PUBLIC include)
set_target_properties(sin_scene_actor_bridge PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_options(sin_scene_actor_bridge PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_scene_actor_bridge_tests tests/test_scene_actor_bridge.cpp)
  target_link_libraries(sin_scene_actor_bridge_tests PRIVATE sin_scene_actor_bridge)
  add_test(NAME sin_scene_actor_bridge_tests COMMAND sin_scene_actor_bridge_tests)
  target_compile_options(sin_scene_actor_bridge_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-UNDEBUG>)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_scene_bsp_safety_tests
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/research/tests/test_sin_bsp_scene_actor_trial.py)
    add_test(NAME sin_scene_archive_preflight_tests
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/research/tests/test_sin_scene_archive_preflight.py)
    add_test(NAME sin_scene_actor_diagnostics_tests
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/research/tests/test_sin_scene_actor_diagnostics.py)
  endif()
endif()


# November 7: separate original assault NPC callback ABI. The future engine
# host supplies generation-aware NPC/point refs, actual rally locks, callbacks.
# This is not yet an implementation of a SourceVR entity game module.
add_library(sin_assault_npc_c_abi SHARED src/assault_npc_c_api.cpp)
target_include_directories(sin_assault_npc_c_abi PUBLIC include)
set_target_properties(sin_assault_npc_c_abi PROPERTIES
  CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
target_compile_options(sin_assault_npc_c_abi PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(UNIX AND NOT APPLE)
  target_link_options(sin_assault_npc_c_abi PRIVATE
    "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/src/assault_npc_exports.map")
  set_property(TARGET sin_assault_npc_c_abi APPEND PROPERTY LINK_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/assault_npc_exports.map")
endif()
if(BUILD_TESTING AND UNIX AND NOT ANDROID)
  add_executable(sin_assault_npc_c_abi_smoke tests/test_assault_npc_c_abi.c)
  target_include_directories(sin_assault_npc_c_abi_smoke PRIVATE include)
  set_target_properties(sin_assault_npc_c_abi_smoke PROPERTIES
    C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
  target_compile_options(sin_assault_npc_c_abi_smoke PRIVATE
    -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
  target_link_libraries(sin_assault_npc_c_abi_smoke PRIVATE ${CMAKE_DL_LIBS})
  add_test(NAME sin_assault_npc_c_abi_smoke
    COMMAND sin_assault_npc_c_abi_smoke $<TARGET_FILE:sin_assault_npc_c_abi>)
endif()
if(ANDROID AND SIN_BUILD_ANDROID_SMOKE)
  add_executable(sin_android_assault_npc_c_abi_smoke tests/test_assault_npc_c_abi.c)
  target_include_directories(sin_android_assault_npc_c_abi_smoke PRIVATE include)
  set_target_properties(sin_android_assault_npc_c_abi_smoke PROPERTIES
    C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
  target_compile_options(sin_android_assault_npc_c_abi_smoke PRIVATE
    -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
  target_link_libraries(sin_android_assault_npc_c_abi_smoke PRIVATE dl)
  add_dependencies(sin_android_assault_npc_c_abi_smoke sin_assault_npc_c_abi)
endif()

# Engine-independent right-handed tracking-space -> Source room-space mapping.
# Math-only bridge for future verified OpenXR/Quest host, no headset/runtime ABI.
if(BUILD_TESTING AND NOT ANDROID)
  add_executable(sin_xr_source_tracking_tests tests/test_xr_source_tracking.cpp)
  target_include_directories(sin_xr_source_tracking_tests PRIVATE include)
  target_compile_options(sin_xr_source_tracking_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_xr_source_tracking_tests COMMAND sin_xr_source_tracking_tests)
endif()
if(ANDROID AND SIN_BUILD_ANDROID_SMOKE)
  add_executable(sin_android_xr_source_tracking_tests tests/test_xr_source_tracking.cpp)
  target_include_directories(sin_android_xr_source_tracking_tests PRIVATE include)
  target_compile_options(sin_android_xr_source_tracking_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
endif()

# Host-side ai_goal_assault fanout over the independently compiled native NPC
# module. Uses a FAKE level and generation-aware actor/rally registry only.
if(BUILD_TESTING AND NOT ANDROID)
  add_executable(sin_assault_goal_fanout_tests
    tests/test_assault_goal_fanout.cpp)
  target_include_directories(sin_assault_goal_fanout_tests PRIVATE include)
  target_link_libraries(sin_assault_goal_fanout_tests PRIVATE sin_assault_npc_c_abi)
  target_compile_options(sin_assault_goal_fanout_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_assault_goal_fanout_tests
    COMMAND sin_assault_goal_fanout_tests)
endif()
# Same compiled host integration fixture running on Android Bionic x86_64;
# this still does not create a SourceVR game entity or register map inputs.
if(ANDROID AND SIN_BUILD_ANDROID_SMOKE)
  add_executable(sin_android_assault_goal_fanout_tests
    tests/test_assault_goal_fanout.cpp)
  target_include_directories(sin_android_assault_goal_fanout_tests PRIVATE include)
  target_link_libraries(sin_android_assault_goal_fanout_tests PRIVATE sin_assault_npc_c_abi)
  target_compile_options(sin_android_assault_goal_fanout_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
endif()

# First genuinely game-NAMED Android module-loading probe. These separately
# linked libserver.so / libclient.so contain only an inert CreateInterface,
# never unverified ServerGameDLL/VClient virtual method objects.
foreach(probe_role IN ITEMS server client)
  add_library(sin_sourcevr_probe_${probe_role} SHARED
    src/sourcevr_module_probe.c)
  target_include_directories(sin_sourcevr_probe_${probe_role} PUBLIC include)
  set_target_properties(sin_sourcevr_probe_${probe_role} PROPERTIES
    OUTPUT_NAME "${probe_role}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/sourcevr-probe"
    C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF
    C_VISIBILITY_PRESET hidden)
  if(probe_role STREQUAL "server")
    target_compile_definitions(sin_sourcevr_probe_${probe_role}
      PRIVATE SIN_SOURCEVR_PROBE_BUILD_SERVER=1)
  else()
    target_compile_definitions(sin_sourcevr_probe_${probe_role}
      PRIVATE SIN_SOURCEVR_PROBE_BUILD_CLIENT=1)
  endif()
  target_compile_options(sin_sourcevr_probe_${probe_role} PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  if(UNIX AND NOT APPLE)
    target_link_options(sin_sourcevr_probe_${probe_role} PRIVATE
      "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/src/sourcevr_module_probe_exports.map")
    set_property(TARGET sin_sourcevr_probe_${probe_role}
      APPEND PROPERTY LINK_DEPENDS
      "${CMAKE_CURRENT_SOURCE_DIR}/src/sourcevr_module_probe_exports.map")
  endif()
endforeach()
if((BUILD_TESTING AND UNIX AND NOT ANDROID) OR
   (ANDROID AND SIN_BUILD_ANDROID_SMOKE))
  if(ANDROID)
    set(sin_module_probe_smoke_target sin_android_sourcevr_module_probe_tests)
  else()
    set(sin_module_probe_smoke_target sin_sourcevr_module_probe_tests)
  endif()
  add_executable(${sin_module_probe_smoke_target}
    tests/test_sourcevr_module_probe.c)
  target_include_directories(${sin_module_probe_smoke_target} PRIVATE include)
  set_target_properties(${sin_module_probe_smoke_target} PROPERTIES
    C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
  target_compile_options(${sin_module_probe_smoke_target} PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  target_link_libraries(${sin_module_probe_smoke_target} PRIVATE ${CMAKE_DL_LIBS})
  add_dependencies(${sin_module_probe_smoke_target}
    sin_sourcevr_probe_server sin_sourcevr_probe_client)
  if(NOT ANDROID)
    add_test(NAME sin_sourcevr_module_probe_tests
      COMMAND ${sin_module_probe_smoke_target}
      "${CMAKE_CURRENT_BINARY_DIR}/sourcevr-probe")
  endif()
endif()

# Native C ABI wrapper around scene actor bridge. Still NOT Source/Quest game DLL.
add_library(sin_scene_actor_c_abi SHARED src/scene_actor_c_api.cpp)
target_link_libraries(sin_scene_actor_c_abi PRIVATE sin_scene_actor_bridge)
target_include_directories(sin_scene_actor_c_abi PUBLIC include)
set_target_properties(sin_scene_actor_c_abi PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
target_compile_options(sin_scene_actor_c_abi PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(UNIX AND NOT APPLE)
  target_link_options(sin_scene_actor_c_abi PRIVATE
    "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/src/scene_actor_exports.map")
  set_property(TARGET sin_scene_actor_c_abi APPEND PROPERTY LINK_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/scene_actor_exports.map")
endif()
# Release -DNDEBUG must NOT disable tests written with assert().
if(BUILD_TESTING AND UNIX AND NOT ANDROID)
  add_executable(sin_scene_actor_c_abi_smoke tests/test_scene_actor_c_abi.c)
  target_include_directories(sin_scene_actor_c_abi_smoke PRIVATE include)
  set_target_properties(sin_scene_actor_c_abi_smoke PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
  target_compile_options(sin_scene_actor_c_abi_smoke PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
  target_link_libraries(sin_scene_actor_c_abi_smoke PRIVATE ${CMAKE_DL_LIBS})
  add_test(NAME sin_scene_actor_c_abi_smoke
    COMMAND sin_scene_actor_c_abi_smoke $<TARGET_FILE:sin_scene_actor_c_abi>)
endif()
# A real Bionic dlopen executable. Uses synthetic callbacks; not game integration.
if(ANDROID AND SIN_BUILD_ANDROID_SMOKE)
  add_executable(sin_android_scene_actor_c_abi_smoke tests/test_scene_actor_c_abi.c)
  target_include_directories(sin_android_scene_actor_c_abi_smoke PRIVATE include)
  set_target_properties(sin_android_scene_actor_c_abi_smoke PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
  target_compile_options(sin_android_scene_actor_c_abi_smoke PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
  target_link_libraries(sin_android_scene_actor_c_abi_smoke PRIVATE dl)
  add_dependencies(sin_android_scene_actor_c_abi_smoke sin_scene_actor_c_abi)
endif()

# Portable declarative assault task runner, host callback behavior only.
if(BUILD_TESTING)
  add_executable(sin_assault_task_runner_tests tests/test_assault_task_runner.cpp)
  target_include_directories(sin_assault_task_runner_tests PRIVATE include)
  target_compile_options(sin_assault_task_runner_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_assault_task_runner_tests COMMAND sin_assault_task_runner_tests)
endif()

# Reconstructed 2006 CNPC_Jessica scripted item-throw behavior.
# This is an original-free C++17 host reference, NOT a registered engine NPC.
add_library(sin_original_jessica_throw STATIC
  src/original_jessica_throw.cpp)
target_include_directories(sin_original_jessica_throw PUBLIC include)
target_compile_options(sin_original_jessica_throw PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_jessica_throw_tests
    tests/test_original_jessica_throw.cpp)
  target_link_libraries(sin_original_jessica_throw_tests PRIVATE
    sin_original_jessica_throw)
  target_compile_options(sin_original_jessica_throw_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_jessica_throw_tests
    COMMAND sin_original_jessica_throw_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_jessica_binary_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/research/tests/test_audit_sin_original_jessica_throw.py)
  endif()
endif()

# Exact original 2006 Jessica x87 ballistic toss, reconstructed as a portable
# C++17 math helper. No original source or proprietary binary is embedded.
add_library(sin_original_jessica_toss STATIC src/original_jessica_toss.cpp)
target_include_directories(sin_original_jessica_toss PUBLIC include)
target_compile_options(sin_original_jessica_toss PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
target_link_libraries(sin_original_jessica_throw PUBLIC
  sin_original_jessica_toss)
if(BUILD_TESTING)
  add_executable(sin_original_jessica_toss_tests
    tests/test_original_jessica_toss.cpp)
  target_link_libraries(sin_original_jessica_toss_tests PRIVATE
    sin_original_jessica_toss)
  target_compile_options(sin_original_jessica_toss_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_jessica_toss_tests
    COMMAND sin_original_jessica_toss_tests)

  add_executable(sin_original_jessica_toss_host_tests
    tests/test_original_jessica_toss_host.cpp)
  target_link_libraries(sin_original_jessica_toss_host_tests PRIVATE
    sin_original_jessica_throw sin_original_jessica_toss)
  target_compile_options(sin_original_jessica_toss_host_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_jessica_toss_host_tests
    COMMAND sin_original_jessica_toss_host_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_jessica_toss_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/research/tests/test_audit_sin_original_jessica_toss.py)
  endif()
endif()

# Original 2006 server.dll 0x2214BA20 additive velocity impulse.
# Both physics type-6 and non-physics absolute velocity paths are modeled
# behind a host contract. No private engine ABI is assumed.
add_library(sin_original_jessica_impulse STATIC
  src/original_jessica_impulse.cpp)
target_include_directories(sin_original_jessica_impulse PUBLIC include)
target_compile_options(sin_original_jessica_impulse PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
target_link_libraries(sin_original_jessica_throw PUBLIC
  sin_original_jessica_impulse)
if(BUILD_TESTING)
  add_executable(sin_original_jessica_impulse_tests
    tests/test_original_jessica_impulse.cpp)
  target_link_libraries(sin_original_jessica_impulse_tests PRIVATE
    sin_original_jessica_impulse)
  target_compile_options(sin_original_jessica_impulse_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_jessica_impulse_tests
    COMMAND sin_original_jessica_impulse_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_jessica_impulse_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/research/tests/test_audit_sin_original_jessica_impulse.py)
  endif()
endif()

# Phase 106: original 2006 SiN player WorldSpaceCenter at vtable +0x214.
# Native game-engine transforms/observations remain host-owned.
add_library(sin_original_jessica_target_center STATIC
  src/original_jessica_target_center.cpp)
target_include_directories(sin_original_jessica_target_center PUBLIC include)
target_compile_options(sin_original_jessica_target_center PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_jessica_target_center_tests
    tests/test_original_jessica_target_center.cpp)
  target_link_libraries(sin_original_jessica_target_center_tests PRIVATE
    sin_original_jessica_target_center sin_original_jessica_toss)
  target_compile_options(sin_original_jessica_target_center_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_jessica_target_center_tests
    COMMAND sin_original_jessica_target_center_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_jessica_target_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/research/tests/test_audit_sin_original_jessica_target.py)
  endif()
endif()

# Phase 107: 2006 SiN server.dll's parent-aware absolute velocity cache.
# Original functions 0x22141A00, 0x2213D750, rotation helpers, invalidation.
# Engine-owned entity pointers and EHANDLE slots remain outside this library.
add_library(sin_original_absolute_velocity STATIC
  src/original_absolute_velocity.cpp)
target_include_directories(sin_original_absolute_velocity PUBLIC include)
target_compile_options(sin_original_absolute_velocity PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_absolute_velocity_tests
    tests/test_original_absolute_velocity.cpp)
  target_link_libraries(sin_original_absolute_velocity_tests PRIVATE
    sin_original_absolute_velocity)
  target_compile_options(sin_original_absolute_velocity_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_absolute_velocity_tests
    COMMAND sin_original_absolute_velocity_tests)

  # Host bridge: Phase 105 impulse -> Phase 107 parent-aware setter/refresh.
  add_executable(sin_original_absolute_impulse_adapter_tests
    tests/test_original_absolute_impulse_adapter.cpp)
  target_link_libraries(sin_original_absolute_impulse_adapter_tests PRIVATE
    sin_original_absolute_velocity sin_original_jessica_impulse)
  target_compile_options(sin_original_absolute_impulse_adapter_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_absolute_impulse_adapter_tests
    COMMAND sin_original_absolute_impulse_adapter_tests)

  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_absolute_velocity_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/research/tests/test_audit_sin_original_absolute_velocity.py)
  endif()
endif()

# Phase 108: original SiN world transform refresh 0x2213FA60.
# All EHANDLE / attachment and engine-side callbacks are host controlled.
add_library(sin_original_world_transform STATIC
  src/original_world_transform.cpp)
target_include_directories(sin_original_world_transform PUBLIC include)
target_compile_options(sin_original_world_transform PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_world_transform_tests
    tests/test_original_world_transform.cpp)
  target_link_libraries(sin_original_world_transform_tests PRIVATE
    sin_original_world_transform)
  target_compile_options(sin_original_world_transform_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_world_transform_tests
    COMMAND sin_original_world_transform_tests)

  add_executable(sin_world_transform_velocity_bridge_tests
    tests/test_original_world_transform_velocity_bridge.cpp)
  target_link_libraries(sin_world_transform_velocity_bridge_tests PRIVATE
    sin_original_world_transform sin_original_absolute_velocity
    sin_original_jessica_impulse)
  target_compile_options(sin_world_transform_velocity_bridge_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_world_transform_velocity_bridge_tests
    COMMAND sin_world_transform_velocity_bridge_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_world_transform_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/research/tests/test_audit_sin_original_world_transform.py)
  endif()
endif()

# Phase 109: original SiN 2006 +0xEC parent attachment / world matrix
# selection. The 2006 engine owns EHANDLE, animation vtables and lifetime.
add_library(sin_original_parent_attachment STATIC
  src/original_parent_attachment.cpp)
target_include_directories(sin_original_parent_attachment PUBLIC include)
target_link_libraries(sin_original_parent_attachment PUBLIC
  sin_original_world_transform)
target_compile_options(sin_original_parent_attachment PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_parent_attachment_tests
    tests/test_original_parent_attachment.cpp)
  target_link_libraries(sin_original_parent_attachment_tests PRIVATE
    sin_original_parent_attachment)
  target_compile_options(sin_original_parent_attachment_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_parent_attachment_tests
    COMMAND sin_original_parent_attachment_tests)

  add_executable(sin_original_parent_attachment_world_bridge_tests
    tests/test_original_parent_attachment_world_bridge.cpp)
  target_link_libraries(sin_original_parent_attachment_world_bridge_tests PRIVATE
    sin_original_parent_attachment)
  target_compile_options(sin_original_parent_attachment_world_bridge_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_parent_attachment_world_bridge_tests
    COMMAND sin_original_parent_attachment_world_bridge_tests)

  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_parent_attachment_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/research/tests/test_audit_sin_original_parent_attachment.py)
  endif()
endif()

# Phase 110: original SiN CBaseAnimating attachment matrix / bone resolve,
# with a native-host-only studiohdr and MDL cache boundary.
add_library(sin_original_studio_attachment STATIC
  src/original_studio_attachment.cpp)
target_include_directories(sin_original_studio_attachment PUBLIC include)
target_link_libraries(sin_original_studio_attachment PUBLIC
  sin_original_parent_attachment sin_original_world_transform)
target_compile_options(sin_original_studio_attachment PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_studio_attachment_tests
    tests/test_original_studio_attachment.cpp)
  target_link_libraries(sin_original_studio_attachment_tests PRIVATE
    sin_original_studio_attachment)
  target_compile_options(sin_original_studio_attachment_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_studio_attachment_tests
    COMMAND sin_original_studio_attachment_tests)

  add_executable(sin_original_studio_attachment_world_bridge_tests
    tests/test_original_studio_attachment_world_bridge.cpp)
  target_link_libraries(sin_original_studio_attachment_world_bridge_tests PRIVATE
    sin_original_studio_attachment)
  target_compile_options(sin_original_studio_attachment_world_bridge_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_studio_attachment_world_bridge_tests
    COMMAND sin_original_studio_attachment_world_bridge_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_studio_attachment_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/research/tests/test_audit_sin_original_studio_attachment.py)
  endif()
endif()

# Phase 111: original SiN compact animated bone cache remap and entity
# world-matrix fallback (0x220ED1B0/0x221004C0/0x220EBE70).
add_library(sin_original_bone_cache STATIC src/original_bone_cache.cpp)
target_include_directories(sin_original_bone_cache PUBLIC include)
target_link_libraries(sin_original_bone_cache PUBLIC
  sin_original_studio_attachment)
target_compile_options(sin_original_bone_cache PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_bone_cache_tests
    tests/test_original_bone_cache.cpp)
  target_link_libraries(sin_original_bone_cache_tests PRIVATE
    sin_original_bone_cache)
  target_compile_options(sin_original_bone_cache_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_bone_cache_tests COMMAND sin_original_bone_cache_tests)

  add_executable(sin_original_bone_cache_studio_bridge_tests
    tests/test_original_bone_cache_studio_bridge.cpp)
  target_link_libraries(sin_original_bone_cache_studio_bridge_tests PRIVATE
    sin_original_bone_cache)
  target_compile_options(sin_original_bone_cache_studio_bridge_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_bone_cache_studio_bridge_tests
    COMMAND sin_original_bone_cache_studio_bridge_tests)

  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_bone_cache_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/research/tests/test_audit_sin_original_bone_cache.py)
  endif()
endif()

# Phase 112: original SiN 2006 CBaseAnimating bone cache lifecycle
# 0x220EB8D0, 0x22100450, 0x220FC5A0; no native x86 ABI calls.
add_library(sin_original_bone_preparation STATIC
  src/original_bone_preparation.cpp)
target_include_directories(sin_original_bone_preparation PUBLIC include)
target_link_libraries(sin_original_bone_preparation PUBLIC
  sin_original_bone_cache)
target_compile_options(sin_original_bone_preparation PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_bone_preparation_tests
    tests/test_original_bone_preparation.cpp)
  target_link_libraries(sin_original_bone_preparation_tests PRIVATE
    sin_original_bone_preparation)
  target_compile_options(sin_original_bone_preparation_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_bone_preparation_tests
    COMMAND sin_original_bone_preparation_tests)

  add_executable(sin_original_bone_preparation_studio_bridge_tests
    tests/test_original_bone_preparation_studio_bridge.cpp)
  target_link_libraries(sin_original_bone_preparation_studio_bridge_tests PRIVATE
    sin_original_bone_preparation)
  target_compile_options(sin_original_bone_preparation_studio_bridge_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_bone_preparation_studio_bridge_tests
    COMMAND sin_original_bone_preparation_studio_bridge_tests)

  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_bone_preparation_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/research/tests/test_audit_sin_original_bone_preparation.py)
  endif()
endif()

# Phase 113: original SiN 2006 CBaseAnimating +0x2B4 skeletal setup
# coordinator, 0x220EF510, with native pose/MDL callbacks owned by host.
add_library(sin_original_skeletal_setup STATIC
  src/original_skeletal_setup.cpp)
target_include_directories(sin_original_skeletal_setup PUBLIC include)
target_link_libraries(sin_original_skeletal_setup PUBLIC
  sin_original_bone_preparation)
target_compile_options(sin_original_skeletal_setup PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_skeletal_setup_tests
    tests/test_original_skeletal_setup.cpp)
  target_link_libraries(sin_original_skeletal_setup_tests PRIVATE
    sin_original_skeletal_setup)
  target_compile_options(sin_original_skeletal_setup_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_skeletal_setup_tests
    COMMAND sin_original_skeletal_setup_tests)

  add_executable(sin_original_skeletal_setup_cache_bridge_tests
    tests/test_original_skeletal_setup_cache_bridge.cpp)
  target_link_libraries(sin_original_skeletal_setup_cache_bridge_tests PRIVATE
    sin_original_skeletal_setup)
  target_compile_options(sin_original_skeletal_setup_cache_bridge_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_skeletal_setup_cache_bridge_tests
    COMMAND sin_original_skeletal_setup_cache_bridge_tests)

  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_skeletal_setup_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/research/tests/test_audit_sin_original_skeletal_setup.py)
  endif()
endif()
