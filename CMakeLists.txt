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

# Source-authenticated SiN-only campaign trigger behavior. Registered only
# when the chosen native engine host provides genuine CBaseTrigger objects,
# entity lifetime, an event manager and queued removal callbacks.
# Missing in the owner's HL2 map-port unknown-class logs (88 + 44 reports).
add_library(sin_original_gameplay_triggers STATIC
  src/original_sin_trigger_entities.cpp)
target_include_directories(sin_original_gameplay_triggers PUBLIC include third_party/research)
target_compile_options(sin_original_gameplay_triggers PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_gameplay_triggers_tests
    tests/test_original_sin_trigger_entities.cpp)
  target_link_libraries(sin_original_gameplay_triggers_tests PRIVATE
    sin_original_gameplay_triggers)
  target_compile_options(sin_original_gameplay_triggers_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_gameplay_triggers_tests
    COMMAND sin_original_gameplay_triggers_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_gameplay_triggers_binary_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_sin_original_gameplay_triggers.py
        --self-test)
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

# Phase529 native-host adapter: a real Source engine must allocate a
# CBaseTrigger first, then route authored SiN trigger game code through
# native origin/flag snapshots and genuine map KeyValues callbacks.
# Does NOT add physics/model/prop reading or claim playable game DLL boot.
add_library(sin_original_trigger_map_bridge STATIC
  src/original_sin_trigger_map_bridge.cpp)
target_include_directories(sin_original_trigger_map_bridge PUBLIC
  include third_party/research)
target_link_libraries(sin_original_trigger_map_bridge PUBLIC
  sin_original_gameplay_triggers sin_source_map_runtime)
target_compile_options(sin_original_trigger_map_bridge PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_trigger_map_bridge_tests
    tests/test_original_sin_trigger_map_bridge.cpp)
  target_link_libraries(sin_original_trigger_map_bridge_tests PRIVATE
    sin_original_trigger_map_bridge)
  target_compile_options(sin_original_trigger_map_bridge_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_trigger_map_bridge_tests
    COMMAND sin_original_trigger_map_bridge_tests)
endif()

# Phase530: original retail NPC factory metadata SHA audit. CI uses ONLY
# original-free synthetic PE-shaped opcodes and SHA refusal; owner's actual
# unmodified DLL verification is private and never part of public Actions.
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_npc_factory_17_registration_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_sin_original_npc_factories.py
        --self-test)
  endif()
endif()

# Phase531 original SHA-authenticated 3-alias Druglab Grunt constructor.
# Host-neutral model only: no invented Source native allocations, weapons
# or AI; MSVC x86/Android code cannot use retail PE32 vtable addresses.
add_library(sin_original_druglab_grunt_factory STATIC
  src/original_druglab_grunt_factory.cpp)
target_include_directories(sin_original_druglab_grunt_factory PUBLIC include)
target_compile_options(sin_original_druglab_grunt_factory PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_druglab_grunt_factory_tests
    tests/test_original_druglab_grunt_factory.cpp)
  target_link_libraries(sin_original_druglab_grunt_factory_tests PRIVATE
    sin_original_druglab_grunt_factory)
  target_compile_options(sin_original_druglab_grunt_factory_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_druglab_grunt_factory_tests
    COMMAND sin_original_druglab_grunt_factory_tests)
endif()

# Phase532: authentic CDruglabGrunt::Spawn original virtual slot25
# selects variant weapon names, network field+0x3B0 and per-class
# x87-float-to-int fields+0x98/+0x9C, after inherited NPC calls.
# A true native engine NPC host remains required for live gameplay.
add_library(sin_original_druglab_grunt_spawn STATIC
  src/original_druglab_grunt_spawn.cpp)
target_include_directories(sin_original_druglab_grunt_spawn PUBLIC include)
target_link_libraries(sin_original_druglab_grunt_spawn PUBLIC
  sin_original_druglab_grunt_factory)
target_compile_options(sin_original_druglab_grunt_spawn PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_druglab_grunt_spawn_tests
    tests/test_original_druglab_grunt_spawn.cpp)
  target_link_libraries(sin_original_druglab_grunt_spawn_tests PRIVATE
    sin_original_druglab_grunt_spawn)
  target_compile_options(sin_original_druglab_grunt_spawn_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_druglab_grunt_spawn_tests
    COMMAND sin_original_druglab_grunt_spawn_tests)
  add_executable(sin_original_druglab_grunt_factory_spawn_integration_tests
    tests/test_original_druglab_grunt_factory_spawn_integration.cpp)
  target_link_libraries(sin_original_druglab_grunt_factory_spawn_integration_tests PRIVATE
    sin_original_druglab_grunt_spawn sin_original_druglab_grunt_factory)
  target_compile_options(sin_original_druglab_grunt_factory_spawn_integration_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_druglab_grunt_factory_spawn_integration_tests
    COMMAND sin_original_druglab_grunt_factory_spawn_integration_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_druglab_grunt_spawn_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_sin_original_druglab_grunt_spawn.py
        --self-test)
  endif()
endif()

# Phase533: original 2006 Grunt post-factory native entity/edict creation
# reference, with source-backed bit9/bit10 path and critical +0x160
# networkable ownership. Real engine must create actual CBaseNPC/edict.
add_library(sin_original_druglab_grunt_entity_init STATIC
  src/original_druglab_grunt_entity_init.cpp)
target_include_directories(sin_original_druglab_grunt_entity_init PUBLIC include)
target_compile_options(sin_original_druglab_grunt_entity_init PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_druglab_grunt_entity_init_tests
    tests/test_original_druglab_grunt_entity_init.cpp)
  target_link_libraries(sin_original_druglab_grunt_entity_init_tests PRIVATE
    sin_original_druglab_grunt_entity_init)
  target_compile_options(sin_original_druglab_grunt_entity_init_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_druglab_grunt_entity_init_tests
    COMMAND sin_original_druglab_grunt_entity_init_tests)
  add_executable(sin_original_druglab_grunt_full_lifecycle_tests
    tests/test_original_druglab_grunt_full_lifecycle.cpp)
  target_link_libraries(sin_original_druglab_grunt_full_lifecycle_tests PRIVATE
    sin_original_druglab_grunt_entity_init
    sin_original_druglab_grunt_spawn
    sin_original_druglab_grunt_factory)
  target_compile_options(sin_original_druglab_grunt_full_lifecycle_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_druglab_grunt_full_lifecycle_tests
    COMMAND sin_original_druglab_grunt_full_lifecycle_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_druglab_grunt_entity_edict_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_sin_original_grunt_entity_edict.py
        --self-test)
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

# Phase 114: original 2006 SiN 0x220ED290 name-based parent bone merging;
# studiohdr 216-byte metadata, sorted name lookup and quat/world matrices.
add_library(sin_original_parent_bone_merge STATIC
  src/original_parent_bone_merge.cpp)
target_include_directories(sin_original_parent_bone_merge PUBLIC include)
target_link_libraries(sin_original_parent_bone_merge PUBLIC
  sin_original_skeletal_setup)
target_compile_options(sin_original_parent_bone_merge PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_parent_bone_merge_tests
    tests/test_original_parent_bone_merge.cpp)
  target_link_libraries(sin_original_parent_bone_merge_tests PRIVATE
    sin_original_parent_bone_merge)
  target_compile_options(sin_original_parent_bone_merge_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_parent_bone_merge_tests
    COMMAND sin_original_parent_bone_merge_tests)

  add_executable(sin_original_parent_bone_merge_studio_parser_tests
    tests/test_original_parent_bone_merge_studio_parser.cpp)
  target_link_libraries(sin_original_parent_bone_merge_studio_parser_tests PRIVATE
    sin_original_parent_bone_merge)
  target_compile_options(sin_original_parent_bone_merge_studio_parser_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_parent_bone_merge_studio_parser_tests
    COMMAND sin_original_parent_bone_merge_studio_parser_tests)

  add_executable(sin_original_parent_bone_merge_setup_bridge_tests
    tests/test_original_parent_bone_merge_setup_bridge.cpp)
  target_link_libraries(sin_original_parent_bone_merge_setup_bridge_tests PRIVATE
    sin_original_parent_bone_merge)
  target_compile_options(sin_original_parent_bone_merge_setup_bridge_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_parent_bone_merge_setup_bridge_tests
    COMMAND sin_original_parent_bone_merge_setup_bridge_tests)

  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_parent_bone_merge_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/research/tests/test_audit_sin_original_parent_bone_merge.py)
  endif()
endif()

# Phase 115: 2006 SiN original 0x220FE0C0 masked ordinary bone builder.
# Supports all-bones and single-ancestor-chain modes plus seeded skip slots.
add_library(sin_original_ordinary_bone STATIC src/original_ordinary_bone.cpp)
target_include_directories(sin_original_ordinary_bone PUBLIC include)
target_link_libraries(sin_original_ordinary_bone PUBLIC
  sin_original_parent_bone_merge)
target_compile_options(sin_original_ordinary_bone PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  foreach(name original_ordinary_bone original_ordinary_bone_studio_parser
               original_ordinary_bone_setup_bridge)
    add_executable(sin_${name}_tests tests/test_${name}.cpp)
    target_link_libraries(sin_${name}_tests PRIVATE
      sin_original_ordinary_bone)
    target_compile_options(sin_${name}_tests PRIVATE
      $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
    add_test(NAME sin_${name}_tests COMMAND sin_${name}_tests)
  endforeach()
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_ordinary_bone_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/research/tests/test_audit_sin_original_ordinary_bone.py)
  endif()
endif()

# Phase 116: source-observed SiN 2006 pose-driver reset and FIRST command pass.
# Full animation second pass and original x86 runtime are not implemented.
add_library(sin_original_pose_driver_first_pass STATIC
  src/original_pose_driver_first_pass.cpp)
target_include_directories(sin_original_pose_driver_first_pass PUBLIC include)
target_link_libraries(sin_original_pose_driver_first_pass PUBLIC
  sin_original_ordinary_bone)
target_compile_options(sin_original_pose_driver_first_pass PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  foreach(name original_pose_driver_first_pass original_pose_driver_record_parser)
    add_executable(sin_${name}_tests tests/test_${name}.cpp)
    target_link_libraries(sin_${name}_tests PRIVATE
      sin_original_pose_driver_first_pass)
    target_compile_options(sin_${name}_tests PRIVATE
      $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
    add_test(NAME sin_${name}_tests COMMAND sin_${name}_tests)
  endforeach()
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_pose_driver_first_pass_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/research/tests/test_audit_sin_original_pose_driver_first_pass.py)
  endif()
endif()

# Phase 117: original pose driver recursive per-bone matrix path 0x22100F30.
add_library(sin_original_pose_bone_matrix STATIC
  src/original_pose_bone_matrix.cpp)
target_include_directories(sin_original_pose_bone_matrix PUBLIC include)
target_link_libraries(sin_original_pose_bone_matrix PUBLIC
  sin_original_parent_bone_merge)
target_compile_options(sin_original_pose_bone_matrix PRIVATE
  -Wall -Wextra -Wpedantic -Werror)
add_executable(original_pose_bone_matrix
  tests/test_original_pose_bone_matrix.cpp)
target_link_libraries(original_pose_bone_matrix PRIVATE
  sin_original_pose_bone_matrix)
target_compile_options(original_pose_bone_matrix PRIVATE
  -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME original_pose_bone_matrix COMMAND original_pose_bone_matrix)
add_executable(original_pose_bone_matrix_driver_bridge
  tests/test_original_pose_bone_matrix_driver_bridge.cpp)
target_link_libraries(original_pose_bone_matrix_driver_bridge PRIVATE
  sin_original_pose_bone_matrix sin_original_pose_driver_first_pass)
target_compile_options(original_pose_bone_matrix_driver_bridge PRIVATE
  -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME original_pose_bone_matrix_driver_bridge
  COMMAND original_pose_bone_matrix_driver_bridge)
if(Python3_Interpreter_FOUND)
  add_test(NAME original_pose_bone_matrix_audit_synthetic
    COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_SOURCE_DIR}/research/tests/test_audit_sin_original_pose_bone_matrix.py)
endif()


# Phase 118: second pose-pass eligibility, exact cap flag and sequence history.
add_library(sin_original_pose_second_pass_state STATIC
  src/original_pose_second_pass_state.cpp)
target_include_directories(sin_original_pose_second_pass_state PUBLIC include)
target_link_libraries(sin_original_pose_second_pass_state PUBLIC
  sin_original_pose_driver_first_pass sin_original_pose_bone_matrix)
target_compile_options(sin_original_pose_second_pass_state PRIVATE
  -Wall -Wextra -Wpedantic -Werror)
foreach(name original_pose_second_pass_state original_pose_second_track_parser
             original_pose_second_pass_pipeline)
  add_executable(${name} tests/test_${name}.cpp)
  target_link_libraries(${name} PRIVATE sin_original_pose_second_pass_state
    sin_original_pose_bone_matrix)
  target_compile_options(${name} PRIVATE
    -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
  add_test(NAME ${name} COMMAND ${name})
endforeach()
if(Python3_Interpreter_FOUND)
  add_test(NAME original_pose_second_pass_state_audit_synthetic
    COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_SOURCE_DIR}/research/tests/test_audit_sin_original_pose_second_pass_state.py)
endif()


# Original SiN 2006 CPropSinVehicle map-input and monitor-camera behavior.
# Engine-neutral reconstruction only; the eventual host must provide the actual
# Source vehicle physics, entity references, client replication and rendering.
add_library(sin_original_vehicle_runtime STATIC
  src/original_vehicle_runtime.cpp)
target_include_directories(sin_original_vehicle_runtime PUBLIC include)
target_compile_options(sin_original_vehicle_runtime PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING OR (ANDROID AND SIN_BUILD_ANDROID_SMOKE))
  if(ANDROID)
    set(sin_vehicle_runtime_test_target
      sin_android_original_vehicle_runtime_tests)
  else()
    set(sin_vehicle_runtime_test_target
      sin_original_vehicle_runtime_tests)
  endif()
  add_executable(${sin_vehicle_runtime_test_target}
    tests/test_original_vehicle_runtime.cpp)
  target_link_libraries(${sin_vehicle_runtime_test_target} PRIVATE
    sin_original_vehicle_runtime)
  target_compile_options(${sin_vehicle_runtime_test_target} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  if(NOT ANDROID)
    add_test(NAME sin_original_vehicle_runtime_tests
      COMMAND ${sin_vehicle_runtime_test_target})
  endif()
endif()


# Original SiN common combat-character alive-health reduction. This is a
# portable host adapter for the verified final integer-health stage, not a
# complete TakeDamage/RadiusDamage path or a native Source game module.
add_library(sin_original_alive_damage STATIC
  src/original_alive_damage.cpp)
target_include_directories(sin_original_alive_damage PUBLIC include)
target_compile_options(sin_original_alive_damage PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
set_target_properties(sin_original_alive_damage PROPERTIES
  POSITION_INDEPENDENT_CODE ON)
if(BUILD_TESTING OR (ANDROID AND SIN_BUILD_ANDROID_SMOKE))
  if(ANDROID)
    set(sin_alive_damage_test_target sin_android_original_alive_damage_tests)
  else()
    set(sin_alive_damage_test_target sin_original_alive_damage_tests)
  endif()
  add_executable(${sin_alive_damage_test_target}
    tests/test_original_alive_damage.cpp)
  target_link_libraries(${sin_alive_damage_test_target} PRIVATE
    sin_original_alive_damage)
  target_compile_options(${sin_alive_damage_test_target} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  if(NOT ANDROID)
    add_test(NAME sin_original_alive_damage_tests
      COMMAND ${sin_alive_damage_test_target})
  endif()
endif()


# Partial original SiN grunt schedule selector, reconstructed from Phase 7
# registration pairs and confirmed 2006 control-flow branches. Unknown gaps
# remain explicit host callbacks; this is not a native Source NPC.
add_library(sin_original_grunt_ai STATIC
  src/original_grunt_schedule_selector.cpp
  src/original_grunt_schedule_translation.cpp
  src/original_grunt_help_up_task.cpp
  src/original_grunt_relationship.cpp
  src/original_grunt_beserk_behavior.cpp
  src/original_grunt_kick.cpp
  src/original_grunt_map_inputs.cpp)
target_include_directories(sin_original_grunt_ai PUBLIC include)
target_compile_options(sin_original_grunt_ai PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
set_target_properties(sin_original_grunt_ai PROPERTIES
  POSITION_INDEPENDENT_CODE ON)
if(BUILD_TESTING OR (ANDROID AND SIN_BUILD_ANDROID_SMOKE))
  if(ANDROID)
    set(sin_grunt_selector_test_target
      sin_android_original_grunt_schedule_selector_tests)
  else()
    set(sin_grunt_selector_test_target
      sin_original_grunt_schedule_selector_tests)
  endif()
  add_executable(${sin_grunt_selector_test_target}
    tests/test_original_grunt_schedule_selector.cpp)
  target_link_libraries(${sin_grunt_selector_test_target} PRIVATE
    sin_original_grunt_ai)
  target_compile_options(${sin_grunt_selector_test_target} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  if(NOT ANDROID)
    add_test(NAME sin_original_grunt_schedule_selector_tests
      COMMAND ${sin_grunt_selector_test_target})
  endif()
endif()

if(BUILD_TESTING OR (ANDROID AND SIN_BUILD_ANDROID_SMOKE))
  if(ANDROID)
    set(sin_grunt_translation_test_target
      sin_android_original_grunt_schedule_translation_tests)
  else()
    set(sin_grunt_translation_test_target
      sin_original_grunt_schedule_translation_tests)
  endif()
  add_executable(${sin_grunt_translation_test_target}
    tests/test_original_grunt_schedule_translation.cpp)
  target_link_libraries(${sin_grunt_translation_test_target} PRIVATE
    sin_original_grunt_ai)
  target_compile_options(${sin_grunt_translation_test_target} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  if(NOT ANDROID)
    add_test(NAME sin_original_grunt_schedule_translation_tests
      COMMAND ${sin_grunt_translation_test_target})
  endif()
endif()

# Original SiN 2006 TASK_GRUNT_HELP_UP start/update paths. The unresolved
# wounded-object helper and task-completion operation remain host callbacks.
if(BUILD_TESTING OR (ANDROID AND SIN_BUILD_ANDROID_SMOKE))
  if(ANDROID)
    set(sin_grunt_help_up_test_target
      sin_android_original_grunt_help_up_task_tests)
  else()
    set(sin_grunt_help_up_test_target
      sin_original_grunt_help_up_task_tests)
  endif()
  add_executable(${sin_grunt_help_up_test_target}
    tests/test_original_grunt_help_up_task.cpp)
  target_link_libraries(${sin_grunt_help_up_test_target} PRIVATE
    sin_original_grunt_ai)
  target_compile_options(${sin_grunt_help_up_test_target} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  if(NOT ANDROID)
    add_test(NAME sin_original_grunt_help_up_task_tests
      COMMAND ${sin_grunt_help_up_test_target})
  endif()
endif()

# Phase 135: original grunt hostility-mode relationship and condition gather.
# The +0xDB1 flag and +0x898 collection are host supplied; +0x678 remains an
# explicit unresolved callback, not a guessed engine operation.
if(BUILD_TESTING OR (ANDROID AND SIN_BUILD_ANDROID_SMOKE))
  if(ANDROID)
    set(sin_grunt_relationship_test_target
      sin_android_original_grunt_relationship_tests)
  else()
    set(sin_grunt_relationship_test_target
      sin_original_grunt_relationship_tests)
  endif()
  add_executable(${sin_grunt_relationship_test_target}
    tests/test_original_grunt_relationship.cpp)
  target_link_libraries(${sin_grunt_relationship_test_target} PRIVATE
    sin_original_grunt_ai)
  target_compile_options(${sin_grunt_relationship_test_target} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  if(NOT ANDROID)
    add_test(NAME sin_original_grunt_relationship_tests
      COMMAND ${sin_grunt_relationship_test_target})
  endif()
endif()

# Phase 136: original embedded CAI_BeserkBehavior timer and separate schedule gate.
if(BUILD_TESTING OR (ANDROID AND SIN_BUILD_ANDROID_SMOKE))
  if(ANDROID)
    set(sin_grunt_beserk_test_target
      sin_android_original_grunt_beserk_behavior_tests)
  else()
    set(sin_grunt_beserk_test_target
      sin_original_grunt_beserk_behavior_tests)
  endif()
  add_executable(${sin_grunt_beserk_test_target}
    tests/test_original_grunt_beserk_behavior.cpp)
  target_link_libraries(${sin_grunt_beserk_test_target} PRIVATE
    sin_original_grunt_ai)
  target_compile_options(${sin_grunt_beserk_test_target} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  if(NOT ANDROID)
    add_test(NAME sin_original_grunt_beserk_behavior_tests
      COMMAND ${sin_grunt_beserk_test_target})
  endif()
endif()

# Phase 137: original grunt grenade-kick eligibility, task start and event 3.
if(BUILD_TESTING OR (ANDROID AND SIN_BUILD_ANDROID_SMOKE))
  if(ANDROID)
    set(sin_grunt_kick_test_target sin_android_original_grunt_kick_tests)
  else()
    set(sin_grunt_kick_test_target sin_original_grunt_kick_tests)
  endif()
  add_executable(${sin_grunt_kick_test_target}
    tests/test_original_grunt_kick.cpp)
  target_link_libraries(${sin_grunt_kick_test_target} PRIVATE
    sin_original_grunt_ai)
  target_compile_options(${sin_grunt_kick_test_target} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  if(NOT ANDROID)
    add_test(NAME sin_original_grunt_kick_tests
      COMMAND ${sin_grunt_kick_test_target})
  endif()
endif()

# Phase 138: original CSceneEntity and CAI_ScriptedSequence map-output routing.
# The engine host owns Source output objects and the FireOutput ABI.
add_library(sin_original_scene_io STATIC
  src/original_scene_io.cpp)
target_include_directories(sin_original_scene_io PUBLIC include)
target_compile_options(sin_original_scene_io PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
set_target_properties(sin_original_scene_io PROPERTIES
  POSITION_INDEPENDENT_CODE ON)
if(BUILD_TESTING OR (ANDROID AND SIN_BUILD_ANDROID_SMOKE))
  if(ANDROID)
    set(sin_scene_io_test_target sin_android_original_scene_io_tests)
  else()
    set(sin_scene_io_test_target sin_original_scene_io_tests)
  endif()
  add_executable(${sin_scene_io_test_target}
    tests/test_original_scene_io.cpp)
  target_link_libraries(${sin_scene_io_test_target} PRIVATE
    sin_original_scene_io)
  target_compile_options(${sin_scene_io_test_target} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  if(NOT ANDROID)
    add_test(NAME sin_original_scene_io_tests
      COMMAND ${sin_scene_io_test_target})
  endif()
endif()

# Phase 139: original CSceneEntity Start/Pause/Resume/Cancel input behavior.
# The host owns actual playback and native engine state; no x86 ABI is reused.
add_library(sin_original_scene_playback STATIC
  src/original_scene_playback.cpp)
target_include_directories(sin_original_scene_playback PUBLIC include)
target_link_libraries(sin_original_scene_playback PUBLIC sin_original_scene_io)
target_compile_options(sin_original_scene_playback PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
set_target_properties(sin_original_scene_playback PROPERTIES
  POSITION_INDEPENDENT_CODE ON)
if(BUILD_TESTING OR (ANDROID AND SIN_BUILD_ANDROID_SMOKE))
  if(ANDROID)
    set(sin_scene_playback_test_target
      sin_android_original_scene_playback_tests)
  else()
    set(sin_scene_playback_test_target
      sin_original_scene_playback_tests)
  endif()
  add_executable(${sin_scene_playback_test_target}
    tests/test_original_scene_playback.cpp)
  target_link_libraries(${sin_scene_playback_test_target} PRIVATE
    sin_original_scene_playback)
  target_compile_options(${sin_scene_playback_test_target} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  if(NOT ANDROID)
    add_test(NAME sin_original_scene_playback_tests
      COMMAND ${sin_scene_playback_test_target})
  endif()
endif()


# Phase 140: original CNPC_GruntBase map-input behavior.
if(BUILD_TESTING OR (ANDROID AND SIN_BUILD_ANDROID_SMOKE))
  if(ANDROID)
    set(sin_grunt_map_inputs_test_target
      sin_android_original_grunt_map_inputs_tests)
  else()
    set(sin_grunt_map_inputs_test_target
      sin_original_grunt_map_inputs_tests)
  endif()
  add_executable(${sin_grunt_map_inputs_test_target}
    tests/test_original_grunt_map_inputs.cpp)
  target_link_libraries(${sin_grunt_map_inputs_test_target} PRIVATE
    sin_original_grunt_ai)
  target_compile_options(${sin_grunt_map_inputs_test_target} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  if(NOT ANDROID)
    add_test(NAME sin_original_grunt_map_inputs_tests
      COMMAND ${sin_grunt_map_inputs_test_target})
  endif()
endif()


# Phase 142: original SiN bullet target damage-filter and TraceAttack routing.
# The host owns all native entities and maps the verified class route to its
# own engine methods; no original x86 vtable or object offset is reused.
add_library(sin_original_bullet_target_dispatch STATIC
  src/original_bullet_target_dispatch.cpp)
target_include_directories(sin_original_bullet_target_dispatch PUBLIC include)
target_compile_options(sin_original_bullet_target_dispatch PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
set_target_properties(sin_original_bullet_target_dispatch PROPERTIES
  POSITION_INDEPENDENT_CODE ON)
if(BUILD_TESTING)
  add_executable(sin_original_bullet_target_dispatch_tests
    tests/test_original_bullet_target_dispatch.cpp)
  target_link_libraries(sin_original_bullet_target_dispatch_tests PRIVATE
    sin_original_bullet_target_dispatch)
  target_compile_options(sin_original_bullet_target_dispatch_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_bullet_target_dispatch_tests
    COMMAND sin_original_bullet_target_dispatch_tests)
endif()


# Phase 143: original CWeaponMagnum DUAG impact-effect branch.
# The host supplies native trace state and owns actual decal/effect dispatch.
add_library(sin_original_magnum_impact_effects STATIC
  src/original_magnum_impact_effects.cpp)
target_include_directories(sin_original_magnum_impact_effects PUBLIC include)
target_compile_options(sin_original_magnum_impact_effects PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
set_target_properties(sin_original_magnum_impact_effects PROPERTIES
  POSITION_INDEPENDENT_CODE ON)
if(BUILD_TESTING OR (ANDROID AND SIN_BUILD_ANDROID_SMOKE))
  if(ANDROID)
    set(sin_magnum_impact_test_target
      sin_android_original_magnum_impact_effects_tests)
  else()
    set(sin_magnum_impact_test_target
      sin_original_magnum_impact_effects_tests)
  endif()
  add_executable(${sin_magnum_impact_test_target}
    tests/test_original_magnum_impact_effects.cpp)
  target_link_libraries(${sin_magnum_impact_test_target} PRIVATE
    sin_original_magnum_impact_effects)
  target_compile_options(${sin_magnum_impact_test_target} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  if(NOT ANDROID)
    add_test(NAME sin_original_magnum_impact_effects_tests
      COMMAND ${sin_magnum_impact_test_target})
  endif()
endif()

# Phase 144: original client.dll C_BaseViewModel origin-offset composition.
# This is one portable render-math stage; the host owns RecvTable and rendering.
add_library(sin_original_viewmodel_origin_offset STATIC
  src/original_viewmodel_origin_offset.cpp)
target_include_directories(sin_original_viewmodel_origin_offset PUBLIC include)
target_compile_options(sin_original_viewmodel_origin_offset PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
set_target_properties(sin_original_viewmodel_origin_offset PROPERTIES
  POSITION_INDEPENDENT_CODE ON)
if(BUILD_TESTING OR (ANDROID AND SIN_BUILD_ANDROID_SMOKE))
  if(ANDROID)
    set(sin_viewmodel_offset_test_target
      sin_android_original_viewmodel_origin_offset_tests)
  else()
    set(sin_viewmodel_offset_test_target
      sin_original_viewmodel_origin_offset_tests)
  endif()
  add_executable(${sin_viewmodel_offset_test_target}
    tests/test_original_viewmodel_origin_offset.cpp)
  target_link_libraries(${sin_viewmodel_offset_test_target} PRIVATE
    sin_original_viewmodel_origin_offset)
  target_compile_options(${sin_viewmodel_offset_test_target} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  if(NOT ANDROID)
    add_test(NAME sin_original_viewmodel_origin_offset_tests
      COMMAND ${sin_viewmodel_offset_test_target})
  endif()
endif()


# Phase 145: read Source BSP entity lump into ordered, value-owned map records.
# This is a file parser only; no map geometry or engine entity ABI is assumed.
add_library(sin_source_bsp_entities STATIC
  src/source_bsp_entities.cpp)
target_include_directories(sin_source_bsp_entities PUBLIC include)
target_compile_options(sin_source_bsp_entities PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
set_target_properties(sin_source_bsp_entities PROPERTIES
  POSITION_INDEPENDENT_CODE ON)
if(BUILD_TESTING OR (ANDROID AND SIN_BUILD_ANDROID_SMOKE))
  if(ANDROID)
    set(sin_bsp_entities_test_target sin_android_source_bsp_entities_tests)
  else()
    set(sin_bsp_entities_test_target sin_source_bsp_entities_tests)
  endif()
  add_executable(${sin_bsp_entities_test_target}
    tests/test_source_bsp_entities.cpp)
  target_link_libraries(${sin_bsp_entities_test_target} PRIVATE
    sin_source_bsp_entities)
  target_compile_options(${sin_bsp_entities_test_target} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  if(NOT ANDROID)
    add_test(NAME sin_source_bsp_entities_tests
      COMMAND ${sin_bsp_entities_test_target})
  endif()
endif()

# Phase 146: instantiate the ordinary entity subset parsed from a Source BSP.
# The native host supplies its real classname factories and engine callbacks.
add_library(sin_source_map_runtime STATIC
  src/source_map_runtime.cpp)
target_include_directories(sin_source_map_runtime PUBLIC include)
target_link_libraries(sin_source_map_runtime PUBLIC sin_source_bsp_entities)
target_compile_options(sin_source_map_runtime PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
set_target_properties(sin_source_map_runtime PROPERTIES
  POSITION_INDEPENDENT_CODE ON)
if(BUILD_TESTING OR (ANDROID AND SIN_BUILD_ANDROID_SMOKE))
  if(ANDROID)
    set(sin_source_map_runtime_test_target sin_android_source_map_runtime_tests)
  else()
    set(sin_source_map_runtime_test_target sin_source_map_runtime_tests)
  endif()
  add_executable(${sin_source_map_runtime_test_target}
    tests/test_source_map_runtime.cpp)
  target_link_libraries(${sin_source_map_runtime_test_target} PRIVATE
    sin_source_map_runtime)
  target_compile_options(${sin_source_map_runtime_test_target} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  if(NOT ANDROID)
    add_test(NAME sin_source_map_runtime_tests
      COMMAND ${sin_source_map_runtime_test_target})
  endif()
endif()

# Phase527 integrated synthetic Source BSP -> independent SourceMapRuntime
# -> legitimate host-registered SiN trigger behavior -> Spawn descriptor11
# -> Touch descriptor12. Neither original retail map nor engine is loaded.
if(BUILD_TESTING)
  add_executable(sin_original_sin_trigger_bsp_integration_tests
    tests/test_original_sin_trigger_bsp_integration.cpp)
  target_link_libraries(sin_original_sin_trigger_bsp_integration_tests PRIVATE
    sin_source_map_runtime sin_original_gameplay_triggers)
  target_compile_options(sin_original_sin_trigger_bsp_integration_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_sin_trigger_bsp_integration_tests
    COMMAND sin_original_sin_trigger_bsp_integration_tests)
endif()

# Phase 148: exact original CStatAdvisor virtual-slot evaluation recovery.
# Uses Phase 76 client/server advisor wire contract; no engine ABI.
if(BUILD_TESTING)
  add_executable(sin_original_stat_advisor_evaluators_tests
    tests/test_original_stat_advisor_evaluators.cpp)
  target_include_directories(sin_original_stat_advisor_evaluators_tests PRIVATE include)
  target_compile_features(sin_original_stat_advisor_evaluators_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_stat_advisor_evaluators_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_stat_advisor_evaluators_tests
    COMMAND sin_original_stat_advisor_evaluators_tests)
endif()

# Phase149: original SiN CStatAdvisor 2006 sample callback and routing model.
# Uses existing Phase148 evaluators, never guessed engine-owned vtables.
if(BUILD_TESTING)
  add_executable(sin_original_stat_advisor_sample_runtime_tests
    tests/test_original_stat_advisor_sample_runtime.cpp)
  target_include_directories(sin_original_stat_advisor_sample_runtime_tests PRIVATE include)
  target_compile_features(sin_original_stat_advisor_sample_runtime_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_stat_advisor_sample_runtime_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_stat_advisor_sample_runtime_tests
    COMMAND sin_original_stat_advisor_sample_runtime_tests)
endif()

# Phase150: original SiN CShortTermStatGate and Conjugate listener classes,
# grounded in the original 2006 server.dll RTTI, vtables and method bodies.
if(BUILD_TESTING)
  add_executable(sin_original_short_term_stat_gates_tests
    tests/test_original_short_term_stat_gates.cpp)
  target_include_directories(sin_original_short_term_stat_gates_tests PRIVATE include)
  target_compile_features(sin_original_short_term_stat_gates_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_short_term_stat_gates_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_short_term_stat_gates_tests
    COMMAND sin_original_short_term_stat_gates_tests)
endif()

# Phase 151: original SiN CFloatShortTermStat source for Phase150 gates.
# Host-neutral reference only; no x86 Source engine binary ABI.
if(BUILD_TESTING)
  add_executable(sin_original_float_short_term_stat_tests
    tests/test_original_float_short_term_stat.cpp)
  target_include_directories(sin_original_float_short_term_stat_tests PRIVATE include)
  target_compile_features(sin_original_float_short_term_stat_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_float_short_term_stat_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_float_short_term_stat_tests
    COMMAND sin_original_float_short_term_stat_tests)
endif()

# Phase 153: original 2006 client CHudPoisonTimer state and rendering gates.
# Portable reference; not the native VGUI class or x86 engine ABI.
if(BUILD_TESTING)
  add_executable(sin_original_client_poison_timer_hud_tests
    tests/test_original_client_poison_timer_hud.cpp)
  target_include_directories(sin_original_client_poison_timer_hud_tests PRIVATE include)
  target_compile_features(sin_original_client_poison_timer_hud_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_poison_timer_hud_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_client_poison_timer_hud_tests
    COMMAND sin_original_client_poison_timer_hud_tests)
endif()

# Phase153b: original 2006 client/server poison-timer nested netprop offsets.
# This C++ reference models original x86 metadata, not ARM64 object ABI.
if(BUILD_TESTING)
  add_executable(sin_original_poison_timer_network_tests
    tests/test_original_poison_timer_network_contract.cpp)
  target_include_directories(sin_original_poison_timer_network_tests PRIVATE include)
  target_compile_features(sin_original_poison_timer_network_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_poison_timer_network_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_poison_timer_network_tests
    COMMAND sin_original_poison_timer_network_tests)
endif()

# Phase154: original CHudPoisonTimer fade event selection in 2006 client.dll.
# Host-neutral replay of exact x87 compare branches and sound-token order.
if(BUILD_TESTING)
  add_executable(sin_original_client_poison_timer_fade_tests
    tests/test_original_client_poison_timer_fade.cpp)
  target_include_directories(sin_original_client_poison_timer_fade_tests PRIVATE include)
  target_compile_features(sin_original_client_poison_timer_fade_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_poison_timer_fade_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_client_poison_timer_fade_tests
    COMMAND sin_original_client_poison_timer_fade_tests)
endif()

# Phase155: original SiN 2006 client CHudBreathTimer and dual-DLL
# DT_HL2Local.m_breathTimer contract. Host-neutral, not VGUI ABI.
if(BUILD_TESTING)
  add_executable(sin_original_client_breath_timer_hud_tests
    tests/test_original_client_breath_timer_hud.cpp)
  target_include_directories(sin_original_client_breath_timer_hud_tests PRIVATE include)
  target_compile_features(sin_original_client_breath_timer_hud_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_breath_timer_hud_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_client_breath_timer_hud_tests
    COMMAND sin_original_client_breath_timer_hud_tests)
endif()

# Phase156: original 2006 client shared x87 HUD coordinate scaler and
# two 32-bit global dimension getters. Reference, not the VGUI ABI.
if(BUILD_TESTING)
  add_executable(sin_original_client_hud_coordinate_scale_tests
    tests/test_original_client_hud_coordinate_scale.cpp)
  target_include_directories(sin_original_client_hud_coordinate_scale_tests PRIVATE include)
  target_compile_features(sin_original_client_hud_coordinate_scale_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_hud_coordinate_scale_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_client_hud_coordinate_scale_tests
    COMMAND sin_original_client_hud_coordinate_scale_tests)
endif()

# Phase157: original SiN 2006 client HUD renderer entry-stage and
# two resource rectangle extent getters (not a full VGUI renderer).
if(BUILD_TESTING)
  add_executable(sin_original_client_hud_renderer_preflight_tests
    tests/test_original_client_hud_renderer_preflight.cpp)
  target_include_directories(sin_original_client_hud_renderer_preflight_tests PRIVATE include)
  target_compile_features(sin_original_client_hud_renderer_preflight_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_hud_renderer_preflight_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_client_hud_renderer_preflight_tests
    COMMAND sin_original_client_hud_renderer_preflight_tests)
endif()

# Phase158: original SiN 2006 client draw-resource seven-to-nine arg bridge,
# surface clipping/UV preflight; not a complete VGUI texture renderer.
if(BUILD_TESTING)
  add_executable(sin_original_client_vgui_draw_bridge_tests
    tests/test_original_client_vgui_draw_bridge.cpp)
  target_include_directories(sin_original_client_vgui_draw_bridge_tests PRIVATE include)
  target_compile_features(sin_original_client_vgui_draw_bridge_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_vgui_draw_bridge_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_client_vgui_draw_bridge_tests
    COMMAND sin_original_client_vgui_draw_bridge_tests)
endif()

# Phase159: native original SiN Episodes 2006 client x87 FISTP
# float-to-signed-int HUD adapter and 9-word draw worker packet.
if(BUILD_TESTING)
  add_executable(sin_original_client_vgui_integer_draw_adapter_tests
    tests/test_original_client_vgui_integer_draw_adapter.cpp)
  target_include_directories(sin_original_client_vgui_integer_draw_adapter_tests PRIVATE include)
  target_compile_features(sin_original_client_vgui_integer_draw_adapter_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_vgui_integer_draw_adapter_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_client_vgui_integer_draw_adapter_tests
    COMMAND sin_original_client_vgui_integer_draw_adapter_tests)
endif()

# Phase160: original SiN 2006 client CHudElement constructor, vtable
# accessors/setters and global HUD suppression predicate (reference C++).
if(BUILD_TESTING)
  add_executable(sin_original_client_hud_element_visibility_tests
    tests/test_original_client_hud_element_visibility.cpp)
  target_include_directories(sin_original_client_hud_element_visibility_tests PRIVATE include)
  target_compile_features(sin_original_client_hud_element_visibility_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_hud_element_visibility_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_client_hud_element_visibility_tests
    COMMAND sin_original_client_hud_element_visibility_tests)
endif()

# Phase161: verified original 2006 client breath/poison constructor stores,
# VGUI panel offset thunks and distinct CHudElement visibility masks.
if(BUILD_TESTING)
  add_executable(sin_original_client_timer_constructor_contract_tests
    tests/test_original_client_timer_constructor_contract.cpp)
  target_include_directories(sin_original_client_timer_constructor_contract_tests PRIVATE include)
  target_compile_features(sin_original_client_timer_constructor_contract_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_timer_constructor_contract_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_client_timer_constructor_contract_tests
    COMMAND sin_original_client_timer_constructor_contract_tests)
endif()

# Phase162: exact original 2006 Breath/Poison/MutagenBar VGUI
# Color/string property registration metadata and once-guard behavior.
if(BUILD_TESTING)
  add_executable(sin_original_client_timer_property_registrar_tests
    tests/test_original_client_timer_property_registrars.cpp)
  target_include_directories(sin_original_client_timer_property_registrar_tests PRIVATE include)
  target_compile_features(sin_original_client_timer_property_registrar_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_timer_property_registrar_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_client_timer_property_registrar_tests
    COMMAND sin_original_client_timer_property_registrar_tests)
endif()

# Phase163: original SiN 2006 client 24-byte VGUI property descriptor
# insertion, memmove and native dynamic-array growth policy.
if(BUILD_TESTING)
  add_executable(sin_original_client_property_descriptor_array_tests
    tests/test_original_client_property_descriptor_array.cpp)
  target_include_directories(sin_original_client_property_descriptor_array_tests PRIVATE include)
  target_compile_features(sin_original_client_property_descriptor_array_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_property_descriptor_array_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_client_property_descriptor_array_tests
    COMMAND sin_original_client_property_descriptor_array_tests)
endif()

# Phase164: real 2006 SiN client VGUI panel-animation class-owner registry,
# Q_strnicmp namespace handling, lazy singleton and find-or-create control flow.
if(BUILD_TESTING)
  add_executable(sin_original_client_panel_animation_registry_tests
    tests/test_original_client_panel_animation_registry.cpp)
  target_include_directories(sin_original_client_panel_animation_registry_tests PRIVATE include)
  target_compile_features(sin_original_client_panel_animation_registry_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_panel_animation_registry_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_client_panel_animation_registry_tests
    COMMAND sin_original_client_panel_animation_registry_tests)
endif()

# Phase165: original SiN Episodes server.dll CWeaponScatterGun
# ammo snapshot/pending state and secondary clip decrement.
if(BUILD_TESTING)
  add_executable(sin_original_server_scattergun_ammo_tests
    tests/test_original_server_scattergun_ammo.cpp)
  target_include_directories(sin_original_server_scattergun_ammo_tests PRIVATE include)
  target_compile_features(sin_original_server_scattergun_ammo_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_server_scattergun_ammo_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_server_scattergun_ammo_tests
    COMMAND sin_original_server_scattergun_ammo_tests)
endif()

# Phase166: original 2006 SiN server CWeaponScatterGun deferred ammo
# event, deadline gate and three 32-bit tracked-float notification setters.
if(BUILD_TESTING)
  add_executable(sin_original_server_scattergun_deferred_tests
    tests/test_original_server_scattergun_deferred.cpp)
  target_include_directories(sin_original_server_scattergun_deferred_tests PRIVATE include)
  target_compile_features(sin_original_server_scattergun_deferred_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_server_scattergun_deferred_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_original_server_scattergun_deferred_tests
    COMMAND sin_original_server_scattergun_deferred_tests)
endif()

# Phase167: original 2006 server.dll ScatterGun secondary-fire host control flow.
# This references the genuine original x86 routine, but does not link the
# Source engine or implement original MSVC weapon objects or pellet hit physics.
if(BUILD_TESTING)
  add_executable(sin_original_server_scattergun_secondary_fire_tests
    tests/test_original_server_scattergun_secondary_fire.cpp)
  target_include_directories(sin_original_server_scattergun_secondary_fire_tests
    PRIVATE include)
  target_compile_options(sin_original_server_scattergun_secondary_fire_tests
    PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_scattergun_secondary_fire_tests
    COMMAND sin_original_server_scattergun_secondary_fire_tests)
endif()

# Phase168: SHA-verified original 2006 server.dll flak spawn/origin host model.
# The engine's factory, handle registry, parent matrices, and original native
# ABI are external dependencies, not replaced by this standalone test.
if(BUILD_TESTING)
  add_executable(sin_original_server_flak_origin_tests
    tests/test_original_server_flak_origin.cpp)
  target_include_directories(sin_original_server_flak_origin_tests PRIVATE include)
  target_compile_options(sin_original_server_flak_origin_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_flak_origin_tests
    COMMAND sin_original_server_flak_origin_tests)
endif()

# Phase169: original 2006 server.dll absolute-angle and three x87 vector
# routine behavior models. Not the original Source engine/MSVC object ABI.
if(BUILD_TESTING)
  add_executable(sin_original_server_entity_angles_tests
    tests/test_original_server_entity_angles.cpp)
  target_include_directories(sin_original_server_entity_angles_tests PRIVATE include)
  target_compile_options(sin_original_server_entity_angles_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_entity_angles_tests
    COMMAND sin_original_server_entity_angles_tests)
endif()

# Phase170: original 2006 server.dll classname factory and registry reference.
# Host-neutral branch model; original Source engine allocator and DLL ABI
# remain separate reconstruction targets.
if(BUILD_TESTING)
  add_executable(sin_original_server_entity_factory_tests
    tests/test_original_server_entity_factory.cpp)
  target_include_directories(sin_original_server_entity_factory_tests PRIVATE include)
  target_compile_options(sin_original_server_entity_factory_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_entity_factory_tests
    COMMAND sin_original_server_entity_factory_tests)
endif()

# Phase171: actual 2006 original server registry classname lookup and
# distinct create/destroy wrappers; engine allocator/ABI remain host-owned.
if(BUILD_TESTING)
  add_executable(sin_original_server_factory_dispatch_tests
    tests/test_original_server_factory_dispatch.cpp)
  target_include_directories(sin_original_server_factory_dispatch_tests PRIVATE include)
  target_compile_options(sin_original_server_factory_dispatch_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_factory_dispatch_tests
    COMMAND sin_original_server_factory_dispatch_tests)
endif()

# Phase172: original 2006 server.dll registration slot, provider writers,
# and signed-byte classname hash. Native engine allocator remains separate.
if(BUILD_TESTING)
  add_executable(sin_original_server_registry_registration_tests
    tests/test_original_server_registry_registration.cpp)
  target_include_directories(sin_original_server_registry_registration_tests
    PRIVATE include)
  target_compile_options(sin_original_server_registry_registration_tests
    PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_registry_registration_tests
    COMMAND sin_original_server_registry_registration_tests)
endif()

# Phase173: original 2006 server.dll registry insertion, 16-bit slot free
# list, native string-pool first-fit and string interning branch models.
# Original MSVC x86 engine allocator, interned map ABI remain host callbacks.
if(BUILD_TESTING)
  add_executable(sin_original_server_registry_intern_insert_tests
    tests/test_original_server_registry_intern_insert.cpp)
  target_include_directories(sin_original_server_registry_intern_insert_tests
    PRIVATE include)
  target_compile_options(sin_original_server_registry_intern_insert_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_registry_intern_insert_tests
    COMMAND sin_original_server_registry_intern_insert_tests)
endif()

# Phase174: original 2006 SiN server.dll 16-byte insertion tree,
# 12-byte symbol search tree, slot registration, guarded cleanup.
# Engine comparators/allocators are host callbacks, not native DLL ABI.
if(BUILD_TESTING)
  add_executable(sin_original_server_registry_tree_search_tests
    tests/test_original_server_registry_tree_search.cpp)
  target_include_directories(sin_original_server_registry_tree_search_tests
    PRIVATE include)
  target_compile_options(sin_original_server_registry_tree_search_tests
    PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_registry_tree_search_tests
    COMMAND sin_original_server_registry_tree_search_tests)
endif()

# Phase175: owner-original x86 12-byte node storage, free-list and tree-link
# reference. Original Source/MSVC allocation and balancing remain external.
if(BUILD_TESTING)
  add_executable(sin_original_server_symbol_tree_storage_tests
    tests/test_original_server_symbol_tree_storage.cpp)
  target_include_directories(sin_original_server_symbol_tree_storage_tests
    PRIVATE include)
  target_compile_options(sin_original_server_symbol_tree_storage_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_symbol_tree_storage_tests
    COMMAND sin_original_server_symbol_tree_storage_tests)
endif()

# Phase176: SHA-verified original SiN 2006 server.dll 12-byte / 4-byte
# buffer growth, symbol insertion traversal and guarded storage reset.
# The native MSVC x86 allocator/comparator remain explicit host callbacks.
if(BUILD_TESTING)
  add_executable(sin_original_server_symbol_growth_traversal_tests
    tests/test_original_server_symbol_growth_traversal.cpp)
  target_include_directories(sin_original_server_symbol_growth_traversal_tests
    PRIVATE include)
  target_compile_options(sin_original_server_symbol_growth_traversal_tests
    PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_symbol_growth_traversal_tests
    COMMAND sin_original_server_symbol_growth_traversal_tests)
endif()


# Phase177: verified original 2006 server/client CreateInterface entrypoints,
# MSVC x86 three-DWORD interface registrations and synthetic host behavior.
if(BUILD_TESTING)
  add_executable(sin_original_dll_interface_abi_tests
    tests/test_original_dll_interface_abi.cpp)
  target_include_directories(sin_original_dll_interface_abi_tests PRIVATE include)
  target_compile_options(sin_original_dll_interface_abi_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_dll_interface_abi_tests
    COMMAND sin_original_dll_interface_abi_tests)
endif()

# This is a self-contained synthetic *loader ABI* probe, not an original game
# DLL nor a Source engine interface implementation. The .def makes the export
# explicitly undecorated as in the actual 2006 PE32 game modules.
if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 4 AND BUILD_TESTING)
  add_library(sin_original_win32_interface_probe SHARED
    tests/original_win32_loader_probe.cpp
    tests/original_win32_loader_probe.def)
  target_include_directories(sin_original_win32_interface_probe PRIVATE include)
  set_target_properties(sin_original_win32_interface_probe PROPERTIES
    OUTPUT_NAME "sin_original_abi_probe")
  add_executable(sin_original_win32_loader_tests
    tests/test_original_win32_loader.cpp)
  add_dependencies(sin_original_win32_loader_tests
    sin_original_win32_interface_probe)
  add_test(NAME sin_original_win32_loader_tests
    COMMAND sin_original_win32_loader_tests
      $<TARGET_FILE:sin_original_win32_interface_probe>)
endif()


# Phase178: genuine 2006 original client/server singleton factory, MSVC32
# RTTI and original vtable-prefix address metadata. No original engine ABI.
if(BUILD_TESTING)
  add_executable(sin_original_dll_engine_interface_objects_tests
    tests/test_original_dll_engine_interface_objects.cpp)
  target_include_directories(sin_original_dll_engine_interface_objects_tests
    PRIVATE include)
  target_compile_options(sin_original_dll_engine_interface_objects_tests
    PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_dll_engine_interface_objects_tests
    COMMAND sin_original_dll_engine_interface_objects_tests)
endif()


# Phase179: both SHA-pinned original 2006 server/client Init function bodies
# with 37 mandatory early engine interface requests and isolated late queries.
# Synthetic host model, not a real Source engine module init implementation.
if(BUILD_TESTING)
  add_executable(sin_original_dll_initialization_tests
    tests/test_original_dll_initialization.cpp)
  target_include_directories(sin_original_dll_initialization_tests PRIVATE include)
  target_compile_options(sin_original_dll_initialization_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_dll_initialization_tests
    COMMAND sin_original_dll_initialization_tests)
endif()


# Phase180: exact original client.dll post-Init app list creation/cleanup
# and triple VPhysics interface negotiation; host-neutral, not game ABI.
if(BUILD_TESTING)
  add_executable(sin_original_client_postinit_tests
    tests/test_original_client_postinit.cpp)
  target_include_directories(sin_original_client_postinit_tests PRIVATE include)
  target_compile_options(sin_original_client_postinit_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_postinit_tests
    COMMAND sin_original_client_postinit_tests)
endif()


# Phase181: the real original client KeyValues surfaceproperties manifest
# and callback-based post-physics Init reference. Not engine owned objects.
if(BUILD_TESTING)
  add_executable(sin_original_client_surface_manifest_tests
    tests/test_original_client_surface_manifest.cpp)
  target_include_directories(sin_original_client_surface_manifest_tests PRIVATE include)
  target_compile_options(sin_original_client_surface_manifest_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_surface_manifest_tests
    COMMAND sin_original_client_surface_manifest_tests)
endif()


# Phase182: exact original 2006 client KeyValues GetString tagged conversion,
# filesystem surface-file loading and three surfaceprops virtual dispatches.
# Original Source engine file/physics implementations remain opaque callbacks.
if(BUILD_TESTING)
  add_executable(sin_original_client_surface_file_tests
    tests/test_original_client_surface_file.cpp)
  target_include_directories(sin_original_client_surface_file_tests PRIVATE include)
  target_compile_options(sin_original_client_surface_file_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_surface_file_tests
    COMMAND sin_original_client_surface_file_tests)
endif()


# Phase183: original 2006 client KeyValues path find/create and numeric,
# narrow/wide value setters. References only; not native Source objects.
if(BUILD_TESTING)
  add_executable(sin_original_client_keyvalues_setters_tests
    tests/test_original_client_keyvalues_setters.cpp)
  target_include_directories(sin_original_client_keyvalues_setters_tests PRIVATE include)
  target_compile_options(sin_original_client_keyvalues_setters_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_keyvalues_setters_tests
    COMMAND sin_original_client_keyvalues_setters_tests)
endif()


# Phase184: SHA-pinned original 2006 client KeyValues constructor and
# recursive scalar-deleting lifetime behavior, with synthetic host callbacks.
if(BUILD_TESTING)
  add_executable(sin_original_client_keyvalues_lifetime_tests
    tests/test_original_client_keyvalues_lifetime.cpp)
  target_include_directories(sin_original_client_keyvalues_lifetime_tests PRIVATE include)
  target_compile_options(sin_original_client_keyvalues_lifetime_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_keyvalues_lifetime_tests
    COMMAND sin_original_client_keyvalues_lifetime_tests)
endif()


# Phase185: native 2006 SiN client.dll tier0 g_pMemAlloc forwarding,
# debug-source caching, and Phase184 KeyValues destructor/free integration.
# Host callbacks only, no original Source engine allocator implementation.
if(BUILD_TESTING)
  add_executable(sin_original_client_memalloc_bridge_tests
    tests/test_original_client_memalloc_bridge.cpp)
  target_include_directories(sin_original_client_memalloc_bridge_tests PRIVATE include)
  target_compile_options(sin_original_client_memalloc_bridge_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_memalloc_bridge_tests
    COMMAND sin_original_client_memalloc_bridge_tests)
endif()


# Phase186: original 2006 client.dll Win32 module filename resolver and
# three cache-based tier0 allocation dispatches. Callbacks remain synthetic.
if(BUILD_TESTING)
  add_executable(sin_original_client_debug_owner_tests
    tests/test_original_client_debug_owner.cpp)
  target_include_directories(sin_original_client_debug_owner_tests PRIVATE include)
  target_compile_options(sin_original_client_debug_owner_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_debug_owner_tests
    COMMAND sin_original_client_debug_owner_tests)
endif()


# Phase187: server.dll post-emitter AutoGameSystem registration and three
# mandatory Init gates. Original repository parsers remain opaque callbacks.
if(BUILD_TESTING)
  add_executable(sin_original_server_post_emitter_tests
    tests/test_original_server_post_emitter.cpp)
  target_include_directories(sin_original_server_post_emitter_tests PRIVATE include)
  target_compile_options(sin_original_server_post_emitter_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_post_emitter_tests
    COMMAND sin_original_server_post_emitter_tests)
endif()


# Phase188: original scene.cache / modelsounds.cache Init and reload wrappers.
# Host filesystem/record processing remains synthetic until engine ABI work.
if(BUILD_TESTING)
  add_executable(sin_original_server_cache_repositories_tests
    tests/test_original_server_cache_repositories.cpp)
  target_include_directories(sin_original_server_cache_repositories_tests PRIVATE include)
  target_compile_options(sin_original_server_cache_repositories_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_cache_repositories_tests
    COMMAND sin_original_server_cache_repositories_tests)
endif()


# Phase189: integration of the independently verified server system and cache
# gates. Synthetic callbacks; not original-engine DLL initialization.
if(BUILD_TESTING)
  add_executable(sin_original_server_startup_composition_tests
    tests/test_original_server_startup_composition.cpp)
  target_include_directories(sin_original_server_startup_composition_tests PRIVATE include)
  target_compile_options(sin_original_server_startup_composition_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_startup_composition_tests
    COMMAND sin_original_server_startup_composition_tests)
endif()


# Phase190: native-cache record identity branch models (synthetic callbacks).
if(BUILD_TESTING)
  add_executable(sin_original_server_cache_record_identity_tests
    tests/test_original_server_cache_record_identity.cpp)
  target_include_directories(sin_original_server_cache_record_identity_tests PRIVATE include)
  target_compile_options(sin_original_server_cache_record_identity_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_cache_record_identity_tests
    COMMAND sin_original_server_cache_record_identity_tests)
endif()


# Phase191: SHA-grounded paired native server cache tree search reference.
# All comparators/sentinels are synthetic, NOT the Source engine ABI.
if(BUILD_TESTING)
  add_executable(sin_original_server_cache_tree_lookup_tests
    tests/test_original_server_cache_tree_lookup.cpp)
  target_include_directories(sin_original_server_cache_tree_lookup_tests PRIVATE include)
  target_compile_options(sin_original_server_cache_tree_lookup_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_cache_tree_lookup_tests
    COMMAND sin_original_server_cache_tree_lookup_tests)
endif()


# Phase192: original 2006 server cached-record unsigned comparators, as
# synthetic host-neutral C++17 behavior and Phase191 tree integration.
if(BUILD_TESTING)
  add_executable(sin_original_server_cache_key_comparator_tests
    tests/test_original_server_cache_key_comparators.cpp)
  target_include_directories(sin_original_server_cache_key_comparator_tests PRIVATE include)
  target_compile_options(sin_original_server_cache_key_comparator_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_cache_key_comparator_tests
    COMMAND sin_original_server_cache_key_comparator_tests)
endif()


# Phase193: original paired server cache tree insertion 16-byte key copy.
# Host-neutral model, not engine ABI or an original game DLL.
if(BUILD_TESTING)
  add_executable(sin_original_server_cache_tree_insert_tests
    tests/test_original_server_cache_tree_insert.cpp)
  target_include_directories(sin_original_server_cache_tree_insert_tests PRIVATE include)
  target_compile_options(sin_original_server_cache_tree_insert_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_cache_tree_insert_tests
    COMMAND sin_original_server_cache_tree_insert_tests)
endif()


# Phase194: original server cache-tree slot allocator reference.
# Host-neutral synthetic memory; not the original Windows engine ABI.
if(BUILD_TESTING)
  add_executable(sin_original_server_cache_slot_allocation_tests
    tests/test_original_server_cache_slot_allocation.cpp)
  target_include_directories(sin_original_server_cache_slot_allocation_tests PRIVATE include)
  target_compile_options(sin_original_server_cache_slot_allocation_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_cache_slot_allocation_tests
    COMMAND sin_original_server_cache_slot_allocation_tests)
endif()


# Phase195: original server cache node resolver and Phase194 free-list handoff.
# Safe synthetic host pointers, no original Source engine ABI.
if(BUILD_TESTING)
  add_executable(sin_original_server_cache_node_resolver_tests
    tests/test_original_server_cache_node_resolver.cpp)
  target_include_directories(sin_original_server_cache_node_resolver_tests PRIVATE include)
  target_compile_options(sin_original_server_cache_node_resolver_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_cache_node_resolver_tests
    COMMAND sin_original_server_cache_node_resolver_tests)
endif()


# Phase196: binary-grounded original scene/model-sounds cache tree linker.
# Balancing is a synthetic callback, not a native engine implementation.
if(BUILD_TESTING)
  add_executable(sin_original_server_cache_tree_link_tests
    tests/test_original_server_cache_tree_link.cpp)
  target_include_directories(sin_original_server_cache_tree_link_tests PRIVATE include)
  target_compile_options(sin_original_server_cache_tree_link_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_cache_tree_link_tests
    COMMAND sin_original_server_cache_tree_link_tests)
endif()


# Phase197: original SiN server cache balancing safe-prefix semantics.
# The 1,387-byte original algorithms' rotations remain deliberately gated.
if(BUILD_TESTING)
  add_executable(sin_original_server_cache_balance_preflight_tests
    tests/test_original_server_cache_balance_preflight.cpp)
  target_include_directories(sin_original_server_cache_balance_preflight_tests PRIVATE include)
  target_compile_options(sin_original_server_cache_balance_preflight_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_cache_balance_preflight_tests
    COMMAND sin_original_server_cache_balance_preflight_tests)
endif()


# Phase198: original scene/model-sounds paired left/right tree rotations.
# Original resolver and remaining Phase197 balancing flow are separate.
if(BUILD_TESTING)
  add_executable(sin_original_server_cache_tree_rotations_tests
    tests/test_original_server_cache_tree_rotations.cpp)
  target_include_directories(sin_original_server_cache_tree_rotations_tests PRIVATE include)
  target_compile_options(sin_original_server_cache_tree_rotations_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_cache_tree_rotations_tests
    COMMAND sin_original_server_cache_tree_rotations_tests)
endif()


# Phase199: composed red-black fixup for SHA-reviewed original cache tree
# layout/rotations. Reference algorithm, NOT proven complete native x86 ABI.
if(BUILD_TESTING)
  add_executable(sin_original_server_cache_balance_composition_tests
    tests/test_original_server_cache_balance_composition.cpp)
  target_include_directories(sin_original_server_cache_balance_composition_tests PRIVATE include)
  target_compile_options(sin_original_server_cache_balance_composition_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_cache_balance_composition_tests
    COMMAND sin_original_server_cache_balance_composition_tests)
endif()


# Phase200: independent 64-node unsigned-key stress for the canonical
# Phase199 red-black fixup candidate (not a native game DLL parity claim).
if(BUILD_TESTING)
  add_executable(sin_original_server_cache_balance_stress_tests
    tests/test_original_server_cache_balance_stress.cpp)
  target_include_directories(sin_original_server_cache_balance_stress_tests PRIVATE include)
  target_compile_options(sin_original_server_cache_balance_stress_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_cache_balance_stress_tests
    COMMAND sin_original_server_cache_balance_stress_tests)
endif()


# Phase201: native original scene/model-sounds vector growth and descriptor init.
# Native allocator calls are modeled as synthetic callbacks, not heap ABI.
if(BUILD_TESTING)
  add_executable(sin_original_server_cache_array_growth_tests
    tests/test_original_server_cache_array_growth.cpp)
  target_include_directories(sin_original_server_cache_array_growth_tests PRIVATE include)
  target_compile_options(sin_original_server_cache_array_growth_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_cache_array_growth_tests
    COMMAND sin_original_server_cache_array_growth_tests)
endif()


# Phase202: original 2006 server.dll allocator context/dispatch wrappers.
# Host-neutral fake memory ownership only, not a live engine heap ABI.
if(BUILD_TESTING)
  add_executable(sin_original_server_allocator_dispatch_tests
    tests/test_original_server_allocator_dispatch.cpp)
  target_include_directories(sin_original_server_allocator_dispatch_tests PRIVATE include)
  target_compile_options(sin_original_server_allocator_dispatch_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_allocator_dispatch_tests
    COMMAND sin_original_server_allocator_dispatch_tests)
endif()


# Phase205: exact original 2006 client/server tier0 allocator twin contract.
# Original x86 register argument wrappers and shared per-DLL lazy caches.
if(BUILD_TESTING)
  add_executable(sin_original_cross_dll_tier0_allocator_tests
    tests/test_original_cross_dll_tier0_allocator.cpp)
  target_include_directories(sin_original_cross_dll_tier0_allocator_tests PRIVATE include)
  target_compile_options(sin_original_cross_dll_tier0_allocator_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_cross_dll_tier0_allocator_tests
    COMMAND sin_original_cross_dll_tier0_allocator_tests)
endif()


# Phase206: paired original 2006 server/client zero-fill allocator and
# opaque tier0 virtual-call dispatch. Synthetic host backing only.
if(BUILD_TESTING)
  add_executable(sin_original_tier0_zero_allocation_tests
    tests/test_original_tier0_zero_allocation.cpp)
  target_include_directories(sin_original_tier0_zero_allocation_tests PRIVATE include)
  target_compile_options(sin_original_tier0_zero_allocation_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_tier0_zero_allocation_tests
    COMMAND sin_original_tier0_zero_allocation_tests)
endif()


# Phase207: original 2006 SiN server/client alias allocation entrypoints,
# shared primary cache and opaque imported tier0 +0x1C two-arg bridge.
if(BUILD_TESTING)
  add_executable(sin_original_tier0_allocator_aliases_tests
    tests/test_original_tier0_allocator_aliases.cpp)
  target_include_directories(sin_original_tier0_allocator_aliases_tests PRIVATE include)
  target_compile_options(sin_original_tier0_allocator_aliases_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_tier0_allocator_aliases_tests
    COMMAND sin_original_tier0_allocator_aliases_tests)
endif()


# Phase208: source-grounded original SiN server/client tier0 allocator
# three/four-argument bridges, skipped-parameter debug allocation, and
# module-local cached allocation aliases. Host-owned synthetic memory only.
if(BUILD_TESTING)
  add_executable(sin_original_tier0_allocator_bridges_tests
    tests/test_original_tier0_allocator_bridges.cpp)
  target_include_directories(sin_original_tier0_allocator_bridges_tests PRIVATE include)
  target_compile_options(sin_original_tier0_allocator_bridges_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_tier0_allocator_bridges_tests
    COMMAND sin_original_tier0_allocator_bridges_tests)
endif()


# Phase209: full-SHA-verified original server/client ALLOCATOR CALLERS,
# including an object virtual random string sender and 91 client buffers.
if(BUILD_TESTING)
  add_executable(sin_original_allocator_game_callers_tests
    tests/test_original_allocator_game_callers.cpp)
  target_include_directories(sin_original_allocator_game_callers_tests PRIVATE include)
  target_compile_options(sin_original_allocator_game_callers_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_allocator_game_callers_tests
    COMMAND sin_original_allocator_game_callers_tests)
endif()


# Phase210: verified original client prediction/SaveData caller and 90-slot
# snapshot loop, composed with Phase209's 91-buffer host model.
if(BUILD_TESTING)
  add_executable(sin_original_client_prediction_savedata_tests
    tests/test_original_client_prediction_savedata.cpp)
  target_include_directories(sin_original_client_prediction_savedata_tests PRIVATE include)
  target_compile_options(sin_original_client_prediction_savedata_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_prediction_savedata_tests
    COMMAND sin_original_client_prediction_savedata_tests)
endif()


# Phase211: complete original 2006 client.dll 52-byte prediction argpack,
# global-counter snapshot dispatcher, and linked-list record walker.
if(BUILD_TESTING)
  add_executable(sin_original_client_prediction_snapshot_dispatch_tests
    tests/test_original_client_prediction_snapshot_dispatch.cpp)
  target_include_directories(sin_original_client_prediction_snapshot_dispatch_tests PRIVATE include)
  target_compile_options(sin_original_client_prediction_snapshot_dispatch_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_prediction_snapshot_dispatch_tests
    COMMAND sin_original_client_prediction_snapshot_dispatch_tests)
endif()


# Phase212: SHA-verified original 1036-byte client snapshot record writer.
# Portable tests exercise exactly 27 type selectors, 13 case branches,
# and x86 mode/flag filtering, not real game record serialization.
if(BUILD_TESTING)
  add_executable(sin_original_client_snapshot_record_router_tests
    tests/test_original_client_snapshot_record_router.cpp)
  target_include_directories(sin_original_client_snapshot_record_router_tests PRIVATE include)
  target_compile_options(sin_original_client_snapshot_record_router_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_snapshot_record_router_tests
    COMMAND sin_original_client_snapshot_record_router_tests)
endif()


# Phase213: original complete client.dll field copy helpers from snapshot
# type 1, 2, 5 and 6, including byte/DWORD REP MOVS and NUL-inclusive text.
if(BUILD_TESTING)
  add_executable(sin_original_client_snapshot_field_copy_tests
    tests/test_original_client_snapshot_field_copy.cpp)
  target_include_directories(sin_original_client_snapshot_field_copy_tests PRIVATE include)
  target_compile_options(sin_original_client_snapshot_field_copy_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_snapshot_field_copy_tests
    COMMAND sin_original_client_snapshot_field_copy_tests)
endif()

# Phase215: original SiN 2006 client type-2 snapshot equality/trace/preview.
# Three SHA-verified native methods, modeled with synthetic buffer callbacks.
if(BUILD_TESTING)
  add_executable(sin_original_client_snapshot_type2_tests
    tests/test_original_client_snapshot_type2.cpp)
  target_include_directories(sin_original_client_snapshot_type2_tests PRIVATE include)
  target_compile_options(sin_original_client_snapshot_type2_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_snapshot_type2_tests
    COMMAND sin_original_client_snapshot_type2_tests)
endif()

# Phase216: original 2006 client prediction type-1 x87 float comparison,
# numeric tolerance, diagnostics and preview (clean-room host reference).
if(BUILD_TESTING)
  add_executable(sin_original_client_snapshot_type1_tests
    tests/test_original_client_snapshot_type1.cpp)
  target_include_directories(sin_original_client_snapshot_type1_tests PRIVATE include)
  target_compile_options(sin_original_client_snapshot_type1_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_snapshot_type1_tests
    COMMAND sin_original_client_snapshot_type1_tests)
endif()

# Phase217: recovered original 2006 SiN client prediction type8/9 binary
# and integer specialists. Original-free C++17 host reference tests.
if(BUILD_TESTING)
  add_executable(sin_original_client_snapshot_binary_integer_tests
    tests/test_original_client_snapshot_binary_integer.cpp)
  target_include_directories(sin_original_client_snapshot_binary_integer_tests
    PRIVATE include)
  target_compile_options(sin_original_client_snapshot_binary_integer_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_snapshot_binary_integer_tests
    COMMAND sin_original_client_snapshot_binary_integer_tests)
endif()

# Phase218: original 2006 client prediction type7 16-bit short specialists.
# Original-free C++17 host reference; not real retail game module loading.
if(BUILD_TESTING)
  add_executable(sin_original_client_snapshot_short_tests
    tests/test_original_client_snapshot_short.cpp)
  target_include_directories(sin_original_client_snapshot_short_tests
    PRIVATE include)
  target_compile_options(sin_original_client_snapshot_short_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_snapshot_short_tests
    COMMAND sin_original_client_snapshot_short_tests)
endif()

# Phase219 original 2006 client prediction type6 Boolean comparison and trace.
# Synthetic host reference only; NOT original Windows engine module loading.
if(BUILD_TESTING)
  add_executable(sin_original_client_snapshot_boolean_tests
    tests/test_original_client_snapshot_boolean.cpp)
  target_include_directories(sin_original_client_snapshot_boolean_tests PRIVATE include)
  target_compile_options(sin_original_client_snapshot_boolean_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_snapshot_boolean_tests
    COMMAND sin_original_client_snapshot_boolean_tests)
endif()

# Phase220 original client type5 DWORD compare and symbol-preview references.
if(BUILD_TESTING)
  add_executable(sin_original_client_snapshot_dword_tests
    tests/test_original_client_snapshot_dword.cpp)
  target_include_directories(sin_original_client_snapshot_dword_tests PRIVATE include)
  target_compile_options(sin_original_client_snapshot_dword_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_snapshot_dword_tests
    COMMAND sin_original_client_snapshot_dword_tests)
endif()

# Phase221: authentic 2006 client type3/type4 stride12/16 copy host tests.
# These CTests do not execute the retail Source engine.
if(BUILD_TESTING)
  add_executable(sin_original_client_snapshot_vector_copy_tests
    tests/test_original_client_snapshot_vector_copy.cpp)
  target_include_directories(sin_original_client_snapshot_vector_copy_tests PRIVATE include)
  target_compile_options(sin_original_client_snapshot_vector_copy_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_snapshot_vector_copy_tests
    COMMAND sin_original_client_snapshot_vector_copy_tests)
endif()

# Phase222: three authentic original SiN 2006 type3 vec3 x87 and trace
# methods (synthetic host reference; no retail engine module loaded).
if(BUILD_TESTING)
  add_executable(sin_original_client_snapshot_vec3_tests
    tests/test_original_client_snapshot_vec3.cpp)
  target_include_directories(sin_original_client_snapshot_vec3_tests PRIVATE include)
  target_compile_options(sin_original_client_snapshot_vec3_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_snapshot_vec3_tests
    COMMAND sin_original_client_snapshot_vec3_tests)
endif()

# Phase223: original quaternion x87 scratch-sensitive helper host reference.
if(BUILD_TESTING)
  add_executable(sin_original_client_snapshot_quaternion_tests
    tests/test_original_client_snapshot_quaternion.cpp)
  target_include_directories(sin_original_client_snapshot_quaternion_tests PRIVATE include)
  target_compile_options(sin_original_client_snapshot_quaternion_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_snapshot_quaternion_tests
    COMMAND sin_original_client_snapshot_quaternion_tests)
endif()

# Phase224: original SiN 2006 client prediction type13 EHandle specialist
# host reference tests. Does not run actual retail gameplay DLLs.
if(BUILD_TESTING)
  add_executable(sin_original_client_snapshot_ehandle_tests
    tests/test_original_client_snapshot_ehandle.cpp)
  target_include_directories(sin_original_client_snapshot_ehandle_tests PRIVATE include)
  target_compile_options(sin_original_client_snapshot_ehandle_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_snapshot_ehandle_tests
    COMMAND sin_original_client_snapshot_ehandle_tests)
endif()

# Phase225: common original 2006 SiN client snapshot trace wrappers and
# field-type table. Clean-room host tests; no original game engine loading.
if(BUILD_TESTING)
  add_executable(sin_original_client_shared_trace_tests
    tests/test_original_client_shared_trace.cpp)
  target_include_directories(sin_original_client_shared_trace_tests PRIVATE include)
  target_compile_options(sin_original_client_shared_trace_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_shared_trace_tests
    COMMAND sin_original_client_shared_trace_tests)
endif()

# Phase226: native factory singleton return identities and PE32 relocation
# contracts (host-only reference, not executable retail game objects).
if(BUILD_TESTING)
  add_executable(sin_original_core_factory_thunks_tests
    tests/test_original_core_factory_thunks.cpp)
  target_include_directories(sin_original_core_factory_thunks_tests PRIVATE include)
  target_compile_options(sin_original_core_factory_thunks_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_core_factory_thunks_tests
    COMMAND sin_original_core_factory_thunks_tests)
endif()

# Phase227: original runtime singleton ctor/vptr/teardown event-reference
# regression; never loads actual retail Source engine game modules.
if(BUILD_TESTING)
  add_executable(sin_original_runtime_singleton_initializers_tests
    tests/test_original_runtime_singleton_initializers.cpp)
  target_include_directories(sin_original_runtime_singleton_initializers_tests PRIVATE include)
  target_compile_options(sin_original_runtime_singleton_initializers_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_runtime_singleton_initializers_tests
    COMMAND sin_original_runtime_singleton_initializers_tests)
endif()

# Phase228: original HLTV and client entity-list embedded-interface
# constructor/teardown metadata. This is a host-only event reference.
if(BUILD_TESTING)
  add_executable(sin_original_embedded_interface_initializers_tests
    tests/test_original_embedded_interface_initializers.cpp)
  target_include_directories(sin_original_embedded_interface_initializers_tests PRIVATE include)
  target_compile_options(sin_original_embedded_interface_initializers_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_embedded_interface_initializers_tests
    COMMAND sin_original_embedded_interface_initializers_tests)
endif()

# Phase229: source-grounded HLTVDirector and VClientEntityList embedded
# virtual accessors. Independent host reference, not original DLL loader.
if(BUILD_TESTING)
  add_executable(sin_original_embedded_virtual_accessors_tests
    tests/test_original_embedded_virtual_accessors.cpp)
  target_include_directories(sin_original_embedded_virtual_accessors_tests PRIVATE include)
  target_compile_options(sin_original_embedded_virtual_accessors_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_embedded_virtual_accessors_tests
    COMMAND sin_original_embedded_virtual_accessors_tests)
endif()

# Phase230: original VClientEntityList003 handle/index/virtual dispatch
# and parent highest-live slot scan. Host-only clean-room regressions.
if(BUILD_TESTING)
  add_executable(sin_original_client_entitylist_handle_dispatch_tests
    tests/test_original_client_entitylist_handle_dispatch.cpp)
  target_include_directories(sin_original_client_entitylist_handle_dispatch_tests PRIVATE include)
  target_compile_options(sin_original_client_entitylist_handle_dispatch_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_entitylist_handle_dispatch_tests
    COMMAND sin_original_client_entitylist_handle_dispatch_tests)
endif()

# Phase231: original entity-list containing-object vtable slots 0-2.
# Host event/field reference only; NOT a real reconstructed gameplay DLL.
if(BUILD_TESTING)
  add_executable(sin_original_client_entitylist_parent_lifecycle_tests
    tests/test_original_client_entitylist_parent_lifecycle.cpp)
  target_include_directories(sin_original_client_entitylist_parent_lifecycle_tests PRIVATE include)
  target_compile_options(sin_original_client_entitylist_parent_lifecycle_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_entitylist_parent_lifecycle_tests
    COMMAND sin_original_client_entitylist_parent_lifecycle_tests)
endif()

# Phase232: original client.dll PVS notifier/auxiliary 8-byte linked-list
# reference and synthetic native instruction regression tests.
if(BUILD_TESTING)
  add_executable(sin_original_client_pvs_aux_helpers_tests
    tests/test_original_client_pvs_aux_helpers.cpp)
  target_include_directories(sin_original_client_pvs_aux_helpers_tests PRIVATE include)
  target_compile_options(sin_original_client_pvs_aux_helpers_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_pvs_aux_helpers_tests
    COMMAND sin_original_client_pvs_aux_helpers_tests)
endif()

# Phase233: original client PVS sixteen-byte record append/unlink
# independent C++17 host reference, not original Source game objects.
if(BUILD_TESTING)
  add_executable(sin_original_client_pvs_sixteen_list_tests
    tests/test_original_client_pvs_sixteen_list.cpp)
  target_include_directories(sin_original_client_pvs_sixteen_list_tests PRIVATE include)
  target_compile_options(sin_original_client_pvs_sixteen_list_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_pvs_sixteen_list_tests
    COMMAND sin_original_client_pvs_sixteen_list_tests)
endif()


# Phase234 original 2006 client PVS 8/16-byte free-list index allocator paths.
# Source-grounded host model only; this does not instantiate engine objects.
if(BUILD_TESTING)
  add_executable(sin_original_client_pvs_index_allocators_tests
    tests/test_original_client_pvs_index_allocators.cpp)
  target_include_directories(sin_original_client_pvs_index_allocators_tests
    PRIVATE include)
  target_compile_features(sin_original_client_pvs_index_allocators_tests
    PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_pvs_index_allocators_tests
    PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_pvs_index_allocators_tests
    COMMAND sin_original_client_pvs_index_allocators_tests)
endif()


# Phase235 original 2006 SiN client.dll 8/16-byte PVS storage-growth models.
# Portable host tests only. No original Source game DLL objects are loaded.
if(BUILD_TESTING)
  add_executable(sin_original_client_pvs_storage_growth_tests
    tests/test_original_client_pvs_storage_growth.cpp)
  target_include_directories(sin_original_client_pvs_storage_growth_tests
    PRIVATE include)
  target_compile_features(sin_original_client_pvs_storage_growth_tests
    PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_pvs_storage_growth_tests
    PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_pvs_storage_growth_tests
    COMMAND sin_original_client_pvs_storage_growth_tests)
endif()


# Phase236: original client PVS map insert/lookup/release frontend reference.
# Synthetic callbacks only; does NOT link or load original game DLL classes.
if(BUILD_TESTING)
  add_executable(sin_original_client_pvs_map_frontends_tests
    tests/test_original_client_pvs_map_frontends.cpp)
  target_include_directories(sin_original_client_pvs_map_frontends_tests
    PRIVATE include)
  target_compile_features(sin_original_client_pvs_map_frontends_tests
    PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_pvs_map_frontends_tests
    PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_pvs_map_frontends_tests
    COMMAND sin_original_client_pvs_map_frontends_tests)
endif()


# Phase237 original 2006 client PVS tree placement and free-index allocator.
# Independent C++17 callback model, not an engine-owned game DLL.
if(BUILD_TESTING)
  add_executable(sin_original_client_pvs_map_search_index_tests
    tests/test_original_client_pvs_map_search_index.cpp)
  target_include_directories(sin_original_client_pvs_map_search_index_tests
    PRIVATE include)
  target_compile_features(sin_original_client_pvs_map_search_index_tests
    PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_pvs_map_search_index_tests
    PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_pvs_map_search_index_tests
    COMMAND sin_original_client_pvs_map_search_index_tests)
endif()


# Phase238 original client PVS map node resolution and 16-byte growth.
# Clean C++17 host composition; not reconstructed engine gameplay modules.
if(BUILD_TESTING)
  add_executable(sin_original_client_pvs_map_node_growth_tests
    tests/test_original_client_pvs_map_node_growth.cpp)
  target_include_directories(sin_original_client_pvs_map_node_growth_tests
    PRIVATE include)
  target_compile_features(sin_original_client_pvs_map_node_growth_tests
    PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_pvs_map_node_growth_tests
    PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_pvs_map_node_growth_tests
    COMMAND sin_original_client_pvs_map_node_growth_tests)
endif()


# Phase239 original 2006 client PVS map tree-attachment host reference.
# Original 0x24106E30 balancing is an opaque callback, not decompiled here.
if(BUILD_TESTING)
  add_executable(sin_original_client_pvs_tree_attach_tests
    tests/test_original_client_pvs_tree_attach.cpp)
  target_include_directories(sin_original_client_pvs_tree_attach_tests
    PRIVATE include)
  target_compile_features(sin_original_client_pvs_tree_attach_tests
    PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_pvs_tree_attach_tests
    PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_pvs_tree_attach_tests
    COMMAND sin_original_client_pvs_tree_attach_tests)
endif()


# Phase240: independent original-client PVS red-black fixup/rotation reference.
# This host CTest never executes the original game or proprietary DLLs.
if(BUILD_TESTING)
  add_executable(sin_original_client_pvs_rb_fixup_tests
    tests/test_original_client_pvs_rb_fixup.cpp)
  target_include_directories(sin_original_client_pvs_rb_fixup_tests
    PRIVATE include)
  target_compile_features(sin_original_client_pvs_rb_fixup_tests
    PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_pvs_rb_fixup_tests
    PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_pvs_rb_fixup_tests
    COMMAND sin_original_client_pvs_rb_fixup_tests)
endif()


# Phase241: original-client PVS tree-erasure frontend with opaque native
# deletion fixup. Independent C++17 host model only, no retail game boot.
if(BUILD_TESTING)
  add_executable(sin_original_client_pvs_tree_erase_tests
    tests/test_original_client_pvs_tree_erase.cpp)
  target_include_directories(sin_original_client_pvs_tree_erase_tests
    PRIVATE include)
  target_compile_features(sin_original_client_pvs_tree_erase_tests
    PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_pvs_tree_erase_tests
    PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_pvs_tree_erase_tests
    COMMAND sin_original_client_pvs_tree_erase_tests)
endif()


# Phase242 original client PVS red-black deletion balancing. Independent
# C++17 host test, not an engine-owned native gameplay DLL.
if(BUILD_TESTING)
  add_executable(sin_original_client_pvs_delete_fixup_tests
    tests/test_original_client_pvs_delete_fixup.cpp)
  target_include_directories(sin_original_client_pvs_delete_fixup_tests
    PRIVATE include)
  target_compile_features(sin_original_client_pvs_delete_fixup_tests
    PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_pvs_delete_fixup_tests
    PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_pvs_delete_fixup_tests
    COMMAND sin_original_client_pvs_delete_fixup_tests)
endif()


# Phase243 original client PVS comparator, global thunk and owner key adapter.
# C++17 host reference only, not reconstructed retail gameplay module startup.
if(BUILD_TESTING)
  add_executable(sin_original_client_pvs_comparator_dispatch_tests
    tests/test_original_client_pvs_comparator_dispatch.cpp)
  target_include_directories(sin_original_client_pvs_comparator_dispatch_tests
    PRIVATE include)
  target_compile_features(sin_original_client_pvs_comparator_dispatch_tests
    PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_pvs_comparator_dispatch_tests
    PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_pvs_comparator_dispatch_tests
    COMMAND sin_original_client_pvs_comparator_dispatch_tests)
endif()


# Phase244: retail-original client PVS node accessors and auxiliary free reset.
# Independent host C++17 test, not an original gameplay DLL binary.
if(BUILD_TESTING)
  add_executable(sin_original_client_pvs_node_fields_list_reset_tests
    tests/test_original_client_pvs_node_fields_list_reset.cpp)
  target_include_directories(sin_original_client_pvs_node_fields_list_reset_tests
    PRIVATE include)
  target_compile_features(sin_original_client_pvs_node_fields_list_reset_tests
    PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_pvs_node_fields_list_reset_tests
    PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_pvs_node_fields_list_reset_tests
    COMMAND sin_original_client_pvs_node_fields_list_reset_tests)
endif()


# Phase245 original client entity-list destructor/drain and PVS index wrappers.
# Native source-grounded C++17 host model, not a game-engine runtime boot.
if(BUILD_TESTING)
  add_executable(sin_original_client_entitylist_pvs_lifecycle_wrappers_tests
    tests/test_original_client_entitylist_pvs_lifecycle_wrappers.cpp)
  target_include_directories(sin_original_client_entitylist_pvs_lifecycle_wrappers_tests
    PRIVATE include)
  target_compile_features(sin_original_client_entitylist_pvs_lifecycle_wrappers_tests
    PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_entitylist_pvs_lifecycle_wrappers_tests
    PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_entitylist_pvs_lifecycle_wrappers_tests
    COMMAND sin_original_client_entitylist_pvs_lifecycle_wrappers_tests)
endif()


# Phase247 original client entity-list base destructor/handle-removal helpers.
# Portable independent C++17 host tests, not retail gameplay DLL execution.
if(BUILD_TESTING)
  add_executable(sin_original_client_entitylist_base_removal_tests
    tests/test_original_client_entitylist_base_removal.cpp)
  target_include_directories(sin_original_client_entitylist_base_removal_tests
    PRIVATE include)
  target_compile_features(sin_original_client_entitylist_base_removal_tests
    PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_entitylist_base_removal_tests
    PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_entitylist_base_removal_tests
    COMMAND sin_original_client_entitylist_base_removal_tests)
endif()


# Phase248: original client CBaseEntityList constructor, explicit and
# non-networked registration, and vstdlib RandomInt(0,32767) serial wrapper.
# Independent C++17 host reference: NOT retail Windows gameplay module boot.
if(BUILD_TESTING)
  add_executable(sin_original_client_entitylist_base_register_tests
    tests/test_original_client_entitylist_base_register.cpp)
  target_include_directories(sin_original_client_entitylist_base_register_tests
    PRIVATE include)
  target_compile_features(sin_original_client_entitylist_base_register_tests
    PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_entitylist_base_register_tests
    PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_entitylist_base_register_tests
    COMMAND sin_original_client_entitylist_base_register_tests)
endif()


# Phase249: original 2006 client CBaseEntityList intrusive-list operations
# and genuine base virtual slot0/slot1 no-op RET8 interface contracts.
# Independent C++17 host model; NOT a retail gameplay client.dll boot.
if(BUILD_TESTING)
  add_executable(sin_original_client_entitylist_intrusive_list_vtable_tests
    tests/test_original_client_entitylist_intrusive_list_vtable.cpp)
  target_include_directories(sin_original_client_entitylist_intrusive_list_vtable_tests
    PRIVATE include)
  target_compile_features(sin_original_client_entitylist_intrusive_list_vtable_tests
    PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_entitylist_intrusive_list_vtable_tests
    PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_entitylist_intrusive_list_vtable_tests
    COMMAND sin_original_client_entitylist_intrusive_list_vtable_tests)
endif()


# Phase250 original client EntityParticleTrailInfo RecvTable and CRT glue.
# Independent C++17 host model, NOT original gameplay DLL startup.
if(BUILD_TESTING)
  add_executable(sin_original_client_particle_trail_recvtable_crt_tests
    tests/test_original_client_particle_trail_recvtable_crt.cpp)
  target_include_directories(sin_original_client_particle_trail_recvtable_crt_tests
    PRIVATE include)
  target_compile_features(sin_original_client_particle_trail_recvtable_crt_tests
    PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_particle_trail_recvtable_crt_tests
    PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_particle_trail_recvtable_crt_tests
    COMMAND sin_original_client_particle_trail_recvtable_crt_tests)
endif()


# Phase251: original 2006 client recv-proxy, RecvProp and RecvTable factories.
# Host C++17 references only; not independently rebuilt gameplay DLL boot.
if(BUILD_TESTING)
  add_executable(sin_original_client_recvprop_proxies_factories_tests
    tests/test_original_client_recvprop_proxies_factories.cpp)
  target_include_directories(sin_original_client_recvprop_proxies_factories_tests
    PRIVATE include)
  target_compile_features(sin_original_client_recvprop_proxies_factories_tests
    PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_recvprop_proxies_factories_tests
    PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_recvprop_proxies_factories_tests
    COMMAND sin_original_client_recvprop_proxies_factories_tests)
endif()

# Portable independent particle storage test; not a retail allocator ABI.
if(BUILD_TESTING)
  add_executable(sin_independent_particle_pool_tests pc/independent_slot0/test_client_particle_pool_core.cpp)
  target_compile_features(sin_independent_particle_pool_tests PRIVATE cxx_std_17)
  target_compile_options(sin_independent_particle_pool_tests PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME sin_independent_particle_pool_tests COMMAND sin_independent_particle_pool_tests)
endif()

# Phase534: original MobGrunt native CAI_* factories and post-constructor
# wiring after Phase533 entity/edict initialization. Metadata-only ABI
# reference, not a substitute for real engine-owned Source AI classes.
add_library(sin_original_grunt_ai_subsystems STATIC
  src/original_grunt_ai_subsystems.cpp)
target_include_directories(sin_original_grunt_ai_subsystems PUBLIC include)
target_compile_features(sin_original_grunt_ai_subsystems PUBLIC cxx_std_17)
target_compile_options(sin_original_grunt_ai_subsystems PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_grunt_ai_subsystems_tests
    tests/test_original_grunt_ai_subsystems.cpp)
  target_link_libraries(sin_original_grunt_ai_subsystems_tests PRIVATE
    sin_original_grunt_ai_subsystems)
  target_compile_options(sin_original_grunt_ai_subsystems_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_ai_subsystems_tests
    COMMAND sin_original_grunt_ai_subsystems_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_ai_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_ai_subsystems.py"
        --self-test)
  endif()
endif()

# Phase534 integration: exercise original NPC CAI setup inside the existing
# Phase533 factory -> edict -> postinit -> Spawn synthetic-host contract.
if(BUILD_TESTING)
  target_link_libraries(sin_original_druglab_grunt_full_lifecycle_tests PRIVATE
    sin_original_grunt_ai_subsystems)
endif()

# Phase535: RTTI-authenticated CNPC_MobGrunt embedded AI behavior collection,
# owner links and original NPC sentence-parameter key; host-only implementation.
add_library(sin_original_grunt_behavior_postinit STATIC
  src/original_grunt_behavior_postinit.cpp)
target_include_directories(sin_original_grunt_behavior_postinit PUBLIC include)
target_compile_features(sin_original_grunt_behavior_postinit PUBLIC cxx_std_17)
target_compile_options(sin_original_grunt_behavior_postinit PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)

# The original Grunt +0x3F8 calls the seven core factories before it
# registers the eight embedded behaviors, then configures sentence state.
add_library(sin_original_grunt_full_ai_postinit STATIC
  src/original_grunt_full_ai_postinit.cpp)
target_include_directories(sin_original_grunt_full_ai_postinit PUBLIC include)
target_link_libraries(sin_original_grunt_full_ai_postinit PUBLIC
  sin_original_grunt_ai_subsystems sin_original_grunt_behavior_postinit)
target_compile_features(sin_original_grunt_full_ai_postinit PUBLIC cxx_std_17)
target_compile_options(sin_original_grunt_full_ai_postinit PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  # Extend Phase533 factory -> edict -> NPC postinit -> weapon Spawn test
  # with actual Phase535 behavior stages instead of the previous placeholder.
  target_link_libraries(sin_original_druglab_grunt_full_lifecycle_tests PRIVATE
    sin_original_grunt_full_ai_postinit)
  add_executable(sin_original_grunt_behavior_postinit_tests
    tests/test_original_grunt_behavior_postinit.cpp)
  target_link_libraries(sin_original_grunt_behavior_postinit_tests PRIVATE
    sin_original_grunt_behavior_postinit)
  target_compile_options(sin_original_grunt_behavior_postinit_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_behavior_postinit_tests
    COMMAND sin_original_grunt_behavior_postinit_tests)

  add_executable(sin_original_grunt_full_ai_postinit_tests
    tests/test_original_grunt_full_ai_postinit.cpp)
  target_link_libraries(sin_original_grunt_full_ai_postinit_tests PRIVATE
    sin_original_grunt_full_ai_postinit)
  target_compile_options(sin_original_grunt_full_ai_postinit_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_full_ai_postinit_tests
    COMMAND sin_original_grunt_full_ai_postinit_tests)

  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_behavior_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_behavior_postinit.py"
        --self-test)
  endif()
endif()

# Phase536 original Grunt mobility-sidestep (+0x848) eligibility gate,
# native +0x850=3 selector branch and exact original schedule 0x55 return.
# Independent, source-authenticated host model. No retail DLL executable ABI.
add_library(sin_original_grunt_sidestep_gate STATIC
  src/original_grunt_sidestep_gate.cpp)
target_include_directories(sin_original_grunt_sidestep_gate PUBLIC include)
target_compile_features(sin_original_grunt_sidestep_gate PUBLIC cxx_std_17)
target_compile_options(sin_original_grunt_sidestep_gate PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_grunt_sidestep_gate_tests
    tests/test_original_grunt_sidestep_gate.cpp)
  target_link_libraries(sin_original_grunt_sidestep_gate_tests PRIVATE
    sin_original_grunt_sidestep_gate)
  target_compile_options(sin_original_grunt_sidestep_gate_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_sidestep_gate_tests
    COMMAND sin_original_grunt_sidestep_gate_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_sidestep_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_sidestep_gate.py"
        --self-test)
  endif()
endif()

# Phase537: decompilation-first original Grunt vslot +0x844 control-flow
# auditor. Offline self-test uses synthetic CALLs; original PE32 DLLs
# are checked only by an opt-in owner-local --bin-zip invocation.
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_fallback_844_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_fallback_844.py"
        --self-test)
  endif()
endif()
# Phase538: original 2006 VClient011::Init interface request-order audit.
# Synthetic-only CTest; SHA-pinned owner-local original PE32 audit is opt-in.
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_client_init_manifest_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_client_init_interface_manifest.py"
        --self-test)
  endif()
endif()

# Phase539: original 2006 client Init tail and native game-system helper
# authentication. Offline CTest only; owner original DLL inputs are private.
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_client_init_tail_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_client_init_tail.py"
        --self-test)
  endif()
endif()

# Phase540: authenticate owner-original 2006 client VGUI/HUD init helper.
# Offline synthetic test requires no copyrighted game DLL binaries.
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_client_vgui_hud_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_client_vgui_hud_startup.py"
        --self-test)
  endif()
endif()

# Phase541: 2006 original client factory-array VGUI interface resolver.
# Only synthetic verifier in hosted CTest. Original DLL checks owner-local.
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_client_vgui_factory_resolver_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_client_vgui_factory_resolver.py"
        --self-test)
  endif()
endif()

# Phase542: original client CHudTextureHandle type converter intern and
# linked-list registry decompilation (synthetic-only hosted regression).
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_client_converter_registry_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_client_converter_registry.py"
        --self-test)
  endif()
endif()

# Original 2006 converter tree: exact 16-bit token ordering and 24-byte
# entry layout; tests never link or execute original proprietary DLLs.
if(BUILD_TESTING)
  add_executable(sin_original_client_converter_tree_tests
    tests/test_original_client_converter_tree.cpp)
  target_include_directories(sin_original_client_converter_tree_tests PRIVATE include)
  target_compile_features(sin_original_client_converter_tree_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_converter_tree_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_converter_tree_tests
    COMMAND sin_original_client_converter_tree_tests)
endif()

# Phase544 original red-black converter tree insertion and rebalancing,
# backed by exact 2006 client PE32 SHA-proven method metadata. The host
# tests are original-free and do NOT execute any retail engine object.
if(BUILD_TESTING)
  add_executable(sin_original_client_converter_insertion_tests
    tests/test_original_client_converter_insertion.cpp)
  target_include_directories(sin_original_client_converter_insertion_tests PRIVATE include)
  target_compile_features(sin_original_client_converter_insertion_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_converter_insertion_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_converter_insertion_tests
    COMMAND sin_original_client_converter_insertion_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_client_converter_insertion_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_client_converter_insertion.py"
        --self-test)
  endif()
endif()

# Phase545: original 2006 client string-token 12-byte red-black tree.
# C++ tests run entirely without original game code/binaries; audit needs
# the owner's exact original DLL pair only for opt-in local --bin-zip.
if(BUILD_TESTING)
  add_executable(sin_original_client_string_token_tree_tests
    tests/test_original_client_string_token_tree.cpp)
  target_include_directories(sin_original_client_string_token_tree_tests PRIVATE include)
  target_compile_features(sin_original_client_string_token_tree_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_string_token_tree_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_string_token_tree_tests
    COMMAND sin_original_client_string_token_tree_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_client_string_token_tree_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_client_string_token_tree.py"
        --self-test)
  endif()
endif()

# Phase546: 2006 client string-token compare variants, selected constructor
# callback and CRT/ASCII fallback proof. Hosted regressions use synthetic
# data only; the owner-only SHA-pinned original DLL audit is opt-in.
if(BUILD_TESTING)
  add_executable(sin_original_client_token_comparator_tests
    tests/test_original_client_string_token_compare.cpp)
  target_include_directories(sin_original_client_token_comparator_tests PRIVATE include)
  target_compile_features(sin_original_client_token_comparator_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_token_comparator_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_token_comparator_tests
    COMMAND sin_original_client_token_comparator_tests)
  add_executable(sin_original_client_token_comparator_tree_integration_tests
    tests/test_original_client_string_token_compare_integration.cpp)
  target_include_directories(sin_original_client_token_comparator_tree_integration_tests PRIVATE include)
  target_compile_features(sin_original_client_token_comparator_tree_integration_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_token_comparator_tree_integration_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_token_comparator_tree_integration_tests
    COMMAND sin_original_client_token_comparator_tree_integration_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_client_token_comparator_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_client_token_comparators.py"
        --self-test)
  endif()
endif()

# Phase547: owner-authenticated client string-token sentinel, capacity and
# page-space search. Native original PE32 methods are verified only by the
# opt-in owner-local --bin-zip script. Hosted regression is synthetic-only.
if(BUILD_TESTING)
  add_executable(sin_original_client_token_storage_tests
    tests/test_original_client_token_storage.cpp)
  target_include_directories(sin_original_client_token_storage_tests PRIVATE include)
  target_compile_features(sin_original_client_token_storage_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_token_storage_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_token_storage_tests
    COMMAND sin_original_client_token_storage_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_client_token_storage_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_client_token_storage.py"
        --self-test)
  endif()
endif()

# Phase548: original 2006 global client token singleton, distinct comparator
# default, source-verified free-list reset and string page ownership.
# Original DLLs required only for optional owner-local --bin-zip auditing.
if(BUILD_TESTING)
  add_executable(sin_original_client_token_lifecycle_tests
    tests/test_original_client_token_lifecycle.cpp)
  target_include_directories(sin_original_client_token_lifecycle_tests PRIVATE include)
  target_compile_features(sin_original_client_token_lifecycle_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_token_lifecycle_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_token_lifecycle_tests
    COMMAND sin_original_client_token_lifecycle_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_client_token_lifecycle_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_client_token_lifecycle.py"
        --self-test)
  endif()
endif()

# Phase549: authentic 2006 client VClient011 level lifecycle and teardown.
# The original DLL verification is owner-local; hosted CI uses synthetic
# branch-order tests and mutation checks only.
if(BUILD_TESTING)
  add_executable(sin_original_client_level_lifecycle_tests
    tests/test_original_client_level_lifecycle.cpp)
  target_include_directories(sin_original_client_level_lifecycle_tests PRIVATE include)
  target_compile_features(sin_original_client_level_lifecycle_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_level_lifecycle_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_level_lifecycle_tests
    COMMAND sin_original_client_level_lifecycle_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_client_level_lifecycle_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_client_level_lifecycle.py"
        --self-test)
  endif()
endif()

# Phase550: original CVoiceStatus map gate, server command and 1-indexed
# 32-bit voice-mask accessors; synthetic hosted regressions only.
if(BUILD_TESTING)
  add_executable(sin_original_client_voice_status_tests
    tests/test_original_client_voice_status.cpp)
  target_include_directories(sin_original_client_voice_status_tests PRIVATE include)
  target_compile_features(sin_original_client_voice_status_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_voice_status_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_voice_status_tests
    COMMAND sin_original_client_voice_status_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_client_voice_status_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_client_voice_status.py"
        --self-test)
  endif()
endif()

# Phase551: original client CVoiceStatus 32-byte identity hash and
# 256-bucket membership logic. Hosted tests contain NO original DLLs.
if(BUILD_TESTING)
  add_executable(sin_original_client_voice_block_hash_tests
    tests/test_original_client_voice_block_hash.cpp)
  target_include_directories(sin_original_client_voice_block_hash_tests PRIVATE include)
  target_compile_features(sin_original_client_voice_block_hash_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_voice_block_hash_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_voice_block_hash_tests
    COMMAND sin_original_client_voice_block_hash_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_client_voice_block_hash_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_voice_block_hash.py"
        --self-test)
  endif()
endif()

# Phase552: 2006 CVoiceStatus native RTTI, singleton, registered callbacks,
# original one-second update gate and GUI handle dispatch. All hosted C++
# and Python tests use independently authored synthetic data only.
if(BUILD_TESTING)
  add_executable(sin_original_client_voice_lifecycle_tests
    tests/test_original_client_voice_lifecycle.cpp)
  target_include_directories(sin_original_client_voice_lifecycle_tests PRIVATE include)
  target_compile_features(sin_original_client_voice_lifecycle_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_client_voice_lifecycle_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_voice_lifecycle_tests
    COMMAND sin_original_client_voice_lifecycle_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_client_voice_lifecycle_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_voice_lifecycle.py"
        --self-test)
  endif()
endif()

# Phase553: original 2006 CVoiceStatus voice_ban.dt persistent v1 format.
# Hosted tests exercise independent data only. Original DLLs are owner-local.
if(BUILD_TESTING)
  add_executable(sin_original_voice_ban_persistence_tests
    tests/test_original_voice_ban_persistence.cpp)
  target_include_directories(sin_original_voice_ban_persistence_tests PRIVATE include)
  target_compile_features(sin_original_voice_ban_persistence_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_voice_ban_persistence_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_voice_ban_persistence_tests
    COMMAND sin_original_voice_ban_persistence_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_voice_ban_persistence_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_voice_ban_persistence.py"
        --self-test)
  endif()
endif()

# Phase554: 2006 VoiceMask original 32+32+8 bit reader, source-bounded
# original-free tests. Authentic private owner DLL input is never in CI.
if(BUILD_TESTING)
  add_executable(sin_original_voice_mask_wire_tests
    tests/test_original_voice_mask_wire.cpp)
  target_include_directories(sin_original_voice_mask_wire_tests PRIVATE include)
  target_compile_features(sin_original_voice_mask_wire_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_voice_mask_wire_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_voice_mask_wire_tests
    COMMAND sin_original_voice_mask_wire_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_voice_mask_wire_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_voice_mask_wire.py"
        --self-test)
  endif()
endif()

# Phase555: original 2006 server VoiceMask producer authenticated across
# both game DLLs. Hosted checks never execute or distribute original binaries.
if(BUILD_TESTING)
  add_executable(sin_original_server_voice_mask_producer_tests
    tests/test_original_server_voice_mask_wire.cpp)
  target_include_directories(sin_original_server_voice_mask_producer_tests PRIVATE include)
  target_compile_features(sin_original_server_voice_mask_producer_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_server_voice_mask_producer_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_voice_mask_producer_tests
    COMMAND sin_original_server_voice_mask_producer_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_server_voice_mask_producer_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_server_voice_mask_producer.py"
        --self-test)
  endif()
endif()

# Phase556: original 2006 CVoiceGameMgr command processor, vban/VModEnable,
# per-player voice bitfields and authentic native RTTI.
# Hosted C++ + Python selftests never require private original game DLLs.
if(BUILD_TESTING)
  add_executable(sin_original_server_voice_clientcommand_tests
    tests/test_original_server_voice_commands.cpp)
  target_include_directories(sin_original_server_voice_clientcommand_tests PRIVATE include)
  target_compile_features(sin_original_server_voice_clientcommand_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_server_voice_clientcommand_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_voice_clientcommand_tests
    COMMAND sin_original_server_voice_clientcommand_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_server_voice_clientcommand_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_server_voice_commands.py"
        --self-test)
  endif()
endif()

# Phase557: original CRecipientFilter / IRecipientFilter RTTI and the exact
# recipient selection helpers called from the original 9-byte VoiceMask sender.
# Original binary verification stays owner-local; CI uses synthetic inputs.
if(BUILD_TESTING)
  add_executable(sin_original_server_voice_recipient_filter_tests
    tests/test_original_server_voice_recipient_filter.cpp)
  target_include_directories(sin_original_server_voice_recipient_filter_tests PRIVATE include)
  target_compile_features(sin_original_server_voice_recipient_filter_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_server_voice_recipient_filter_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_voice_recipient_filter_tests
    COMMAND sin_original_server_voice_recipient_filter_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_server_voice_recipient_filter_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_server_voice_recipients.py"
        --self-test)
  endif()
endif()

# Phase558: original 2006 server Grunt CAI_Sentence mode gate, command
# dispatch and queue pipeline. No commercial binaries in hosted tests.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_sentence_phase558_tests
    tests/test_original_grunt_sentence_phase558.cpp)
  target_include_directories(sin_original_grunt_sentence_phase558_tests PRIVATE include)
  target_compile_features(sin_original_grunt_sentence_phase558_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_grunt_sentence_phase558_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_sentence_phase558_tests
    COMMAND sin_original_grunt_sentence_phase558_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_sentence_phase558_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_sentence_phase558.py"
        --self-test)
  endif()
endif()

# Phase559: original 2006 Grunt prioritized combat selector and linked-group
# eligibility logic. Hosted tests contain only original-free synthetic data.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_combat_phase559_tests
    tests/test_original_grunt_combat_phase559.cpp)
  target_include_directories(sin_original_grunt_combat_phase559_tests PRIVATE include)
  target_compile_features(sin_original_grunt_combat_phase559_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_grunt_combat_phase559_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_combat_phase559_tests
    COMMAND sin_original_grunt_combat_phase559_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_combat_phase559_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_combat_phase559.py"
        --self-test)
  endif()
endif()

# Phase560: authenticated original 2006 Grunt linked-group reservation,
# member eligibility/iteration and owner condition admission. Host-only tests.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_group_phase560_tests
    tests/test_original_grunt_group_phase560.cpp)
  target_include_directories(sin_original_grunt_group_phase560_tests PRIVATE include)
  target_compile_features(sin_original_grunt_group_phase560_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_grunt_group_phase560_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_group_phase560_tests
    COMMAND sin_original_grunt_group_phase560_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_group_phase560_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_group_phase560.py"
        --self-test)
  endif()
endif()

# Phase561: original Grunt 30-second linked-group bitmap maintenance and
# set/clear/test helpers; only synthetic authored host code runs in CI.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_bitmap_phase561_tests
    tests/test_original_grunt_bitmap_phase561.cpp)
  target_include_directories(sin_original_grunt_bitmap_phase561_tests PRIVATE include)
  target_compile_features(sin_original_grunt_bitmap_phase561_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_grunt_bitmap_phase561_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_bitmap_phase561_tests
    COMMAND sin_original_grunt_bitmap_phase561_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_bitmap_phase561_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_bitmap_phase561.py"
        --self-test)
  endif()
endif()

# Phase562: original 2006 Grunt group allocation, attach/detach, and
# member notifications; original binaries remain private.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_group_lifecycle_phase562_tests
    tests/test_original_grunt_group_lifecycle_phase562.cpp)
  target_include_directories(sin_original_grunt_group_lifecycle_phase562_tests PRIVATE include)
  target_compile_features(sin_original_grunt_group_lifecycle_phase562_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_grunt_group_lifecycle_phase562_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_group_lifecycle_phase562_tests
    COMMAND sin_original_grunt_group_lifecycle_phase562_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_group_lifecycle_phase562_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_group_lifecycle_phase562.py"
        --self-test)
  endif()
endif()

# Phase563: authenticate original 2006 group inline member-handle array,
# serial identity search, forced eviction and flags broadcast. No binaries in CI.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_handle_array_phase563_tests
    tests/test_original_grunt_handle_array_phase563.cpp)
  target_include_directories(sin_original_grunt_handle_array_phase563_tests PRIVATE include)
  target_compile_features(sin_original_grunt_handle_array_phase563_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_grunt_handle_array_phase563_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_handle_array_phase563_tests
    COMMAND sin_original_grunt_handle_array_phase563_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_handle_array_phase563_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_handle_array_phase563.py"
        --self-test)
  endif()
endif()

# Phase564: authenticated original Grunt group virtuals and squad sentence
# dispatch. Host-only model and synthetic PE authentication run in CI.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_virtuals_phase564_tests
    tests/test_original_grunt_virtuals_phase564.cpp)
  target_include_directories(sin_original_grunt_virtuals_phase564_tests PRIVATE include)
  target_compile_features(sin_original_grunt_virtuals_phase564_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_grunt_virtuals_phase564_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_virtuals_phase564_tests
    COMMAND sin_original_grunt_virtuals_phase564_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_virtuals_phase564_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_virtuals_phase564.py"
        --self-test)
  endif()
endif()

# Phase565: original 2006 Grunt flank, pain/taunt, and death sentence
# selectors. Host tests are authored models, not executable retail DLL code.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_reactions_phase565_tests
    tests/test_original_grunt_reactions_phase565.cpp)
  target_include_directories(sin_original_grunt_reactions_phase565_tests PRIVATE include)
  target_compile_features(sin_original_grunt_reactions_phase565_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_grunt_reactions_phase565_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_reactions_phase565_tests
    COMMAND sin_original_grunt_reactions_phase565_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_reactions_phase565_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_reactions_phase565.py"
        --self-test)
  endif()
endif()

# Phase566: original 2006 Grunt flank threshold is cumulative route-waypoint
# length, with cached per-node XY/XYZ distance. Synthetic hosted tests only.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_flank_path_phase566_tests
    tests/test_original_grunt_flank_path_phase566.cpp)
  target_include_directories(sin_original_grunt_flank_path_phase566_tests PRIVATE include)
  target_compile_features(sin_original_grunt_flank_path_phase566_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_grunt_flank_path_phase566_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_flank_path_phase566_tests
    COMMAND sin_original_grunt_flank_path_phase566_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_flank_path_phase566_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_flank_path_phase566.py"
        --self-test)
  endif()
endif()

# Phase567: original Grunt NPC+0x8F8 navigation-component constructor,
# native waypoint route ownership, and original-free synthetic model tests.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_nav_phase567_tests
    tests/test_original_grunt_nav_phase567.cpp)
  target_include_directories(sin_original_grunt_nav_phase567_tests PRIVATE include)
  target_compile_features(sin_original_grunt_nav_phase567_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_grunt_nav_phase567_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_nav_phase567_tests
    COMMAND sin_original_grunt_nav_phase567_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_nav_phase567_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_nav_phase567.py"
        --self-test)
  endif()
endif()

# Phase568: authentic original 2006 Grunt waypoint replacement, advancement,
# link repair, cache invalidation and raw state transitions. Synthetic tests.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_route_mutation_phase568_tests
    tests/test_original_grunt_route_mutation_phase568.cpp)
  target_include_directories(sin_original_grunt_route_mutation_phase568_tests PRIVATE include)
  target_compile_features(sin_original_grunt_route_mutation_phase568_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_grunt_route_mutation_phase568_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_route_mutation_phase568_tests
    COMMAND sin_original_grunt_route_mutation_phase568_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_route_mutation_phase568_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_route_mutation_phase568.py"
        --self-test)
  endif()
endif()

# Phase569: original Grunt navigator +0x28 candidate -> +0x24 active route
# promotion. Host tests do not include game DLLs or original executable code.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_route_promotion_phase569_tests
    tests/test_original_grunt_route_promotion_phase569.cpp)
  target_include_directories(sin_original_grunt_route_promotion_phase569_tests PRIVATE include)
  target_compile_features(sin_original_grunt_route_promotion_phase569_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_grunt_route_promotion_phase569_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_route_promotion_phase569_tests
    COMMAND sin_original_grunt_route_promotion_phase569_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_route_promotion_phase569_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_route_promotion_phase569.py"
        --self-test)
  endif()
endif()

# Phase570: original 2006 waypoint pool linked-chain release and identity-
# sensitive native splice; C++ host deliberately limits matched-ID behavior.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_waypoint_pool_phase570_tests
    tests/test_original_grunt_waypoint_pool_phase570.cpp)
  target_include_directories(sin_original_grunt_waypoint_pool_phase570_tests PRIVATE include)
  target_compile_features(sin_original_grunt_waypoint_pool_phase570_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_grunt_waypoint_pool_phase570_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_waypoint_pool_phase570_tests
    COMMAND sin_original_grunt_waypoint_pool_phase570_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_waypoint_pool_phase570_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_waypoint_pool_phase570.py"
        --self-test)
  endif()
endif()

# Phase571: original matched-identity waypoint splice and two constructors
# including native REP STOSD last-write correction. Synthetic hosted tests.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_waypoint_identity_phase571_tests
    tests/test_original_grunt_waypoint_identity_phase571.cpp)
  target_include_directories(sin_original_grunt_waypoint_identity_phase571_tests PRIVATE include)
  target_compile_features(sin_original_grunt_waypoint_identity_phase571_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_grunt_waypoint_identity_phase571_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_waypoint_identity_phase571_tests
    COMMAND sin_original_grunt_waypoint_identity_phase571_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_waypoint_identity_phase571_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_waypoint_identity_phase571.py"
        --self-test)
  endif()
endif()

# Phase572: original 894-byte Grunt nav+0x54 waypoint candidate builder,
# entry metric gate and nonempty candidate commit. Host-only partial model.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_route_generator_phase572_tests
    tests/test_original_grunt_route_generator_phase572.cpp)
  target_include_directories(sin_original_grunt_route_generator_phase572_tests PRIVATE include)
  target_compile_features(sin_original_grunt_route_generator_phase572_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_grunt_route_generator_phase572_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_route_generator_phase572_tests
    COMMAND sin_original_grunt_route_generator_phase572_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_route_generator_phase572_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_route_generator_phase572.py"
        --self-test)
  endif()
endif()

# Phase573: original 612-byte waypoint-kind 4-way spatial dispatcher and
# native signed result status contract. No generic physics or assets in CI.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_spatial_dispatch_phase573_tests
    tests/test_original_grunt_spatial_dispatch_phase573.cpp)
  target_include_directories(sin_original_grunt_spatial_dispatch_phase573_tests PRIVATE include)
  target_compile_features(sin_original_grunt_spatial_dispatch_phase573_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_grunt_spatial_dispatch_phase573_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_spatial_dispatch_phase573_tests
    COMMAND sin_original_grunt_spatial_dispatch_phase573_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_spatial_dispatch_phase573_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_spatial_dispatch_phase573.py"
        --self-test)
  endif()
endif()

# Phase574: original SiN 2006 Grunt waypoint-kind-0 scaled clearance,
# navigation-handle scale selection and native status -2 rejection.
# CI runs only original-free C++17 models and synthetic PE control-edge tests.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_mode0_phase574_tests
    tests/test_original_grunt_mode0_phase574.cpp)
  target_include_directories(sin_original_grunt_mode0_phase574_tests PRIVATE include)
  target_compile_features(sin_original_grunt_mode0_phase574_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_grunt_mode0_phase574_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_mode0_phase574_tests
    COMMAND sin_original_grunt_mode0_phase574_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_mode0_phase574_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_mode0_phase574.py"
        --self-test)
  endif()
endif()

# Phase575: original Grunt 0x2206EF20 two-trace clearance, native x87
# parity-bit decisions and source-vs-trace XYZ return. Host-only tests;
# original game binaries and physics are not executed or emulated here.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_clearance_phase575_tests
    tests/test_original_grunt_clearance_phase575.cpp)
  target_include_directories(sin_original_grunt_clearance_phase575_tests PRIVATE include)
  target_compile_features(sin_original_grunt_clearance_phase575_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_grunt_clearance_phase575_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_clearance_phase575_tests
    COMMAND sin_original_grunt_clearance_phase575_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_clearance_phase575_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_clearance_phase575.py"
        --self-test)
  endif()
endif()

# Phase576: authentic 2006 SiN Grunt post-clearance candidate selector,
# planar-normalization and virtual+0xF4 entity gate. Original-free tests.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_candidate_filter_phase576_tests
    tests/test_original_grunt_candidate_filter_phase576.cpp)
  target_include_directories(sin_original_grunt_candidate_filter_phase576_tests PRIVATE include)
  target_compile_features(sin_original_grunt_candidate_filter_phase576_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_grunt_candidate_filter_phase576_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_candidate_filter_phase576_tests
    COMMAND sin_original_grunt_candidate_filter_phase576_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_candidate_filter_phase576_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_candidate_filter_phase576.py"
        --self-test)
  endif()
endif()

# Gameplay Phase580: original SiN Grunt animation event IDs 0x50..0x54,
# Grunt announce/alert/refind/lost speech and authentic x87 status ordering.
# Synthetic hosted C++17 model, not retail DLL execution or HL2 physics.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_events_phase580_tests
    tests/test_original_grunt_events_phase580.cpp)
  target_include_directories(sin_original_grunt_events_phase580_tests PRIVATE include)
  target_compile_features(sin_original_grunt_events_phase580_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_grunt_events_phase580_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_events_phase580_tests
    COMMAND sin_original_grunt_events_phase580_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_events_phase580_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_events_phase580.py"
        --self-test)
  endif()
endif()

# Gameplay Phase581: original 1330-byte Grunt animation event override,
# 9 dedicated event IDs, 113 delegated IDs and native 0x8C alert odds.
# Synthetic C++17 model and original-free PE mutation tests only.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_event_switch_phase581_tests
    tests/test_original_grunt_event_switch_phase581.cpp)
  target_include_directories(sin_original_grunt_event_switch_phase581_tests PRIVATE include)
  target_compile_features(sin_original_grunt_event_switch_phase581_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_grunt_event_switch_phase581_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_event_switch_phase581_tests
    COMMAND sin_original_grunt_event_switch_phase581_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_event_switch_phase581_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_event_switch_phase581.py"
        --self-test)
  endif()
endif()

# Gameplay Phase582: original SiN NPC numeric signal 0x22049C20,
# 16-bit x87 angle wrap 0x22028200, Grunt events 0x8D and 0x92.
# Host-only authored tests, no generic HL2 physics nor original DLLs.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_event_signals_phase582_tests
    tests/test_original_grunt_event_signals_phase582.cpp)
  target_include_directories(sin_original_grunt_event_signals_phase582_tests PRIVATE include)
  target_compile_features(sin_original_grunt_event_signals_phase582_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_grunt_event_signals_phase582_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_event_signals_phase582_tests
    COMMAND sin_original_grunt_event_signals_phase582_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_event_signals_phase582_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_event_signals_phase582.py"
        --self-test)
  endif()
endif()

# Gameplay Phase583: authentic SiN numeric request mapping 0x22049A60,
# mapped schedule fallback 10->6, current-request gate 0x22049B50.
# Host-only exact-order regression; no original DLL or generic HL2 physics.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_signal_route_phase583_tests
    tests/test_original_grunt_signal_route_phase583.cpp)
  target_include_directories(sin_original_grunt_signal_route_phase583_tests PRIVATE include)
  target_compile_features(sin_original_grunt_signal_route_phase583_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_grunt_signal_route_phase583_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_signal_route_phase583_tests
    COMMAND sin_original_grunt_signal_route_phase583_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_signal_route_phase583_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_signal_route_phase583.py"
        --self-test)
  endif()
endif()

# Gameplay Phase584: original SiN request resolver (+0x338/+0x33C),
# entity-context action lookup and NPC action-transition callback order.
# Original-free SHA auditor and independent portable C++17 reference tests.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_action_translation_phase584_tests
    tests/test_original_grunt_action_translation_phase584.cpp)
  target_include_directories(sin_original_grunt_action_translation_phase584_tests PRIVATE include)
  target_compile_features(sin_original_grunt_action_translation_phase584_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_grunt_action_translation_phase584_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_action_translation_phase584_tests
    COMMAND sin_original_grunt_action_translation_phase584_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_action_translation_phase584_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_action_translation_phase584.py"
        --self-test)
  endif()
endif()

# Gameplay Phase585: original 2006 SiN weighted action-registration
# lookup, signed weight key selection and registry refresh; host-only C++17.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_weighted_lookup_phase585_tests
    tests/test_original_grunt_weighted_lookup_phase585.cpp)
  target_include_directories(sin_original_grunt_weighted_lookup_phase585_tests PRIVATE include)
  target_compile_features(sin_original_grunt_weighted_lookup_phase585_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_grunt_weighted_lookup_phase585_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_weighted_lookup_phase585_tests
    COMMAND sin_original_grunt_weighted_lookup_phase585_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_weighted_lookup_phase585_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_weighted_lookup_phase585.py"
        --self-test)
  endif()
endif()

# Gameplay Phase586: original SiN 2006 registry count, 212-byte record
# address, revision, name-to-ID resolver and full refresh. Host-only tests.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_registry_refresh_phase586_tests
    tests/test_original_grunt_registry_refresh_phase586.cpp)
  target_include_directories(sin_original_grunt_registry_refresh_phase586_tests PRIVATE include)
  target_compile_features(sin_original_grunt_registry_refresh_phase586_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_grunt_registry_refresh_phase586_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_registry_refresh_phase586_tests
    COMMAND sin_original_grunt_registry_refresh_phase586_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_registry_refresh_phase586_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_registry_refresh_phase586.py"
        --self-test)
  endif()
endif()

# Gameplay Phase587: authenticated original Grunt ACT activity vtable slots,
# kneeling/crouching/aim activity conversions, and serial-handle delegates.
# Original-free C++17 models and synthetic mutation regressions only.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_activity_phase587_tests
    tests/test_original_grunt_activity_phase587.cpp)
  target_include_directories(sin_original_grunt_activity_phase587_tests PRIVATE include)
  target_compile_features(sin_original_grunt_activity_phase587_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_grunt_activity_phase587_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_activity_phase587_tests
    COMMAND sin_original_grunt_activity_phase587_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_activity_phase587_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_activity_phase587.py"
        --self-test)
  endif()
endif()

# Gameplay Phase588: original Jetpack Grunt-derived activity vtables,
# source-pinned RTTI class names, strict 50-unit three-vector threshold.
# Uses original-free Phase587 inherited Grunt C++ reference.
if(BUILD_TESTING)
  add_executable(sin_original_jetpack_activity_phase588_tests
    tests/test_original_jetpack_activity_phase588.cpp)
  target_include_directories(sin_original_jetpack_activity_phase588_tests PRIVATE include)
  target_compile_features(sin_original_jetpack_activity_phase588_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_jetpack_activity_phase588_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_jetpack_activity_phase588_tests
    COMMAND sin_original_jetpack_activity_phase588_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_jetpack_activity_phase588_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_jetpack_activity_phase588.py"
        --self-test)
  endif()
endif()

# Gameplay Phase589: original Jetpack Grunt +0x1024 flying attachment,
# ai_base_physics_flying native name/registration, reciprocal owner handle
# and safe serial-generation lifecycle reference. No general physics work.
if(BUILD_TESTING)
  add_executable(sin_original_jetpack_attachment_phase589_tests
    tests/test_original_jetpack_attachment_phase589.cpp)
  target_include_directories(sin_original_jetpack_attachment_phase589_tests PRIVATE include)
  target_compile_features(sin_original_jetpack_attachment_phase589_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_jetpack_attachment_phase589_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_jetpack_attachment_phase589_tests
    COMMAND sin_original_jetpack_attachment_phase589_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_jetpack_attachment_phase589_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_jetpack_attachment_phase589.py"
        --self-test)
  endif()
endif()

# Gameplay Phase590: original Jetpack NPC virtual+0x444 142-case animation
# event override, original SHA-pinned two-level tables and 26 call edges.
# Host-only C++17 partial model, no physics or game binary execution.
if(BUILD_TESTING)
  add_executable(sin_original_jetpack_events_phase590_tests
    tests/test_original_jetpack_events_phase590.cpp)
  target_include_directories(sin_original_jetpack_events_phase590_tests PRIVATE include)
  target_compile_features(sin_original_jetpack_events_phase590_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_jetpack_events_phase590_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_jetpack_events_phase590_tests
    COMMAND sin_original_jetpack_events_phase590_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_jetpack_events_phase590_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_jetpack_events_phase590.py"
        --self-test)
  endif()
endif()

# Gameplay Phase591: original flying-entity +0xC50 owner-backhandle
# event filter, six special IDs, native true/false returns and Jetpack
# to inherited Grunt 0x54 fallback. No generic physics runtime.
if(BUILD_TESTING)
  add_executable(sin_original_flying_event_filter_phase591_tests
    tests/test_original_flying_event_filter_phase591.cpp)
  target_include_directories(sin_original_flying_event_filter_phase591_tests PRIVATE include)
  target_compile_features(sin_original_flying_event_filter_phase591_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_flying_event_filter_phase591_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_flying_event_filter_phase591_tests
    COMMAND sin_original_flying_event_filter_phase591_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_flying_event_filter_phase591_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_flying_event_filter_phase591.py"
        --self-test)
  endif()
endif()

# Gameplay Phase592: original Grunt inherited +0x444 event switch with
# 156-entry native table and owner+0xC18 marker-gated event intermediary.
# Independent C++17 tests and original-free PE SHA mutation checks.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_event_parent_phase592_tests
    tests/test_original_grunt_event_parent_phase592.cpp)
  target_include_directories(sin_original_grunt_event_parent_phase592_tests PRIVATE include)
  target_compile_features(sin_original_grunt_event_parent_phase592_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_grunt_event_parent_phase592_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_event_parent_phase592_tests
    COMMAND sin_original_grunt_event_parent_phase592_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_event_parent_phase592_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_event_parent_phase592.py"
        --self-test)
  endif()
endif()

# Gameplay Phase593: original CNPC_EliteGrunt virtual+0x444 event 0x03,
# 0x43, 0x47, and inheritance of event 0x54 to Grunt Phase592.
# C++17 host-only source model, original-free native PE mutation audit.
if(BUILD_TESTING)
  add_executable(sin_original_elite_grunt_event_phase593_tests
    tests/test_original_elite_grunt_event_phase593.cpp)
  target_include_directories(sin_original_elite_grunt_event_phase593_tests PRIVATE include)
  target_compile_features(sin_original_elite_grunt_event_phase593_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_elite_grunt_event_phase593_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_elite_grunt_event_phase593_tests
    COMMAND sin_original_elite_grunt_event_phase593_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_elite_grunt_event_phase593_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_elite_grunt_event_phase593.py"
        --self-test)
  endif()
endif()


# Gameplay Phase594: regression composition across authenticated Jetpack
# phase590, flying event filter phase591, Grunt phase592, Elite phase593.
# This is NOT a new native DLL byte audit or a playable engine integration.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_event_chain_phase594_tests
    tests/test_original_grunt_event_chain_phase594.cpp)
  target_include_directories(sin_original_grunt_event_chain_phase594_tests PRIVATE include)
  target_compile_features(sin_original_grunt_event_chain_phase594_tests PRIVATE cxx_std_17)
  target_compile_options(sin_original_grunt_event_chain_phase594_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_event_chain_phase594_tests
    COMMAND sin_original_grunt_event_chain_phase594_tests)
endif()

# Gameplay Phase595: SHA-gated read-only Grunt +0xC18 provenance probe.
# The hosted test uses ONLY a synthetic x86 PE. No original retail binaries.
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_c18_component_phase595_probe_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/probe_sin_original_grunt_component_c18_phase595.py"
        --self-test)
  endif()
endif()

# Phase596: original-free Grunt component C18 candidate report triage.
# The input is synthetic in CI; no retail binary or Ghidra runtime required.
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_c18_phase596_triage_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/triage_sin_original_grunt_c18_phase596.py"
        --self-test)
  endif()
endif()

# Phase598: original-free synthetic Grunt behavior vtable pointer/RTTI verification.
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_behavior_vtable_phase598_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/probe_sin_original_grunt_behavior_vtables_phase598.py"
        --self-test)
  endif()
endif()

# Phase599: binary-free exact-source Grunt behavior handler target metadata audit.
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_behavior_dispatch_phase599_triage_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/triage_sin_original_grunt_behavior_vtables_phase599.py"
        --self-test)
  endif()
endif()

# Phase600: no-retail-binary synthetic headless Ghidra launcher and provenance audit.
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_ghidra_headless_phase600_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/launch_sin_original_grunt_ghidra_phase600.py"
        --self-test)
    add_test(NAME sin_original_grunt_ghidra_triage_phase600_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/triage_sin_original_grunt_ghidra_phase600.py"
        --self-test)
  endif()
endif()

# Phase601: original-free 8-event-native-method and marker-clear audit mutation test.
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_behavior_event54_phase601_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_behavior_event54_phase601.py"
        --self-test)
  endif()
endif()

# Phase602: original-free SHA-pinned Grunt AI behavior activity method audit.
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_behavior_activity_phase602_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_behavior_activities_phase602.py"
        --self-test)
  endif()
endif()

# Phase603: original-free Grunt CAI_BehaviorHost ctor and 2006 RTTI self-test.
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_behavior_host_phase603_ctor_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_behavior_host_ctor_phase603.py"
        --self-test)
  endif()
endif()

# Phase604 original-free Grunt behavior selection/transition source audit.
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_behavior_selection_phase604_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_behavior_selection_phase604.py"
        --self-test)
  endif()
endif()

# Phase605: retail-source Grunt AI_Behaviors Save/Restore method SHA and callsite self-test.
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_save_restore_phase605_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_behavior_save_restore_phase605.py"
        --self-test)
  endif()
endif()

# Phase606: source-backed Grunt runtime behavior priority, portable test,
# and original-free SHA-pinned native function audit.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_priority_phase606_tests
    tests/test_original_grunt_behavior_priority_phase606.cpp)
  target_include_directories(sin_original_grunt_priority_phase606_tests
    PRIVATE include)
  target_compile_options(sin_original_grunt_priority_phase606_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_priority_phase606_tests
    COMMAND sin_original_grunt_priority_phase606_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_priority_phase606_native_audit_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_runtime_behavior_priority_phase606.py"
        --self-test)
  endif()
endif()

# Phase607: authentic 2006 Grunt eight behavior eligibility method boundaries.
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_eligibility_phase607_native_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_grunt_eligibility_phase607.py"
        --self-test)
  endif()
endif()

# Phase608: original Follow +0xE4 handle/serial/bit15 native body and host parity.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_follow_eligibility_phase608_tests
    tests/test_original_grunt_follow_eligibility_phase608.cpp)
  target_include_directories(sin_original_grunt_follow_eligibility_phase608_tests
    PRIVATE include)
  target_compile_options(sin_original_grunt_follow_eligibility_phase608_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_follow_eligibility_phase608_tests
    COMMAND sin_original_grunt_follow_eligibility_phase608_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_follow_eligibility_phase608_native_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_follow_handle_predicate_phase608.py"
        --self-test)
  endif()
endif()

# Phase609: original Assault two serial-checked handle helper/body audit.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_assault_handle_phase609_tests
    tests/test_original_grunt_assault_handle_guards_phase609.cpp)
  target_include_directories(sin_original_grunt_assault_handle_phase609_tests
    PRIVATE include)
  target_compile_options(sin_original_grunt_assault_handle_phase609_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_assault_handle_phase609_tests
    COMMAND sin_original_grunt_assault_handle_phase609_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_assault_handle_phase609_native_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_assault_handle_guards_phase609.py"
        --self-test)
  endif()
endif()

# Phase610: source-checked Assault complete finite-time state machine.
if(BUILD_TESTING)
  add_executable(sin_original_assault_finite_eligibility_phase610_tests
    tests/test_original_grunt_assault_eligibility_phase610.cpp)
  target_include_directories(sin_original_assault_finite_eligibility_phase610_tests
    PRIVATE include)
  target_compile_options(sin_original_assault_finite_eligibility_phase610_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_assault_finite_eligibility_phase610_tests
    COMMAND sin_original_assault_finite_eligibility_phase610_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_assault_x87_timer_phase610_native_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_assault_timer_state_phase610.py"
        --self-test)
  endif()
endif()

# Phase611: authentic Assault vtable+0x20 deactivation and link cleanup.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_assault_deactivation_phase611_tests
    tests/test_original_grunt_assault_deactivate_phase611.cpp)
  target_include_directories(sin_original_grunt_assault_deactivation_phase611_tests
    PRIVATE include)
  target_compile_options(sin_original_grunt_assault_deactivation_phase611_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_assault_deactivation_phase611_tests
    COMMAND sin_original_grunt_assault_deactivation_phase611_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_grunt_assault_deactivation_phase611_native_selftest
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_assault_deactivate_unlink_phase611.py"
        --self-test)
  endif()
endif()

# Phase612: integrated source-backed Grunt priority/Assault eligibility/shutdown.
if(BUILD_TESTING)
  add_executable(sin_original_grunt_assault_lifecycle_phase612_tests
    tests/test_original_grunt_assault_priority_lifecycle_phase612.cpp)
  target_include_directories(sin_original_grunt_assault_lifecycle_phase612_tests
    PRIVATE include)
  target_compile_options(sin_original_grunt_assault_lifecycle_phase612_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_grunt_assault_lifecycle_phase612_tests
    COMMAND sin_original_grunt_assault_lifecycle_phase612_tests)
endif()

# Phase613: genuinely loadable versioned Grunt behavior gameplay C ABI.
# Uses only original-free portable C++ (Phases606/610/611), NOT retail DLLs.
add_library(sin_grunt_behavior_c_abi SHARED src/grunt_behavior_c_api_v1.cpp)
target_include_directories(sin_grunt_behavior_c_abi PUBLIC include)
set_target_properties(sin_grunt_behavior_c_abi PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
target_compile_options(sin_grunt_behavior_c_abi PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-fvisibility=hidden>)
if(UNIX AND NOT APPLE)
  target_link_options(sin_grunt_behavior_c_abi PRIVATE
    "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/src/grunt_behavior_exports.map")
  set_property(TARGET sin_grunt_behavior_c_abi APPEND PROPERTY LINK_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/grunt_behavior_exports.map")
endif()
if(BUILD_TESTING)
  add_executable(sin_grunt_behavior_c_abi_phase613_tests
    tests/test_grunt_behavior_c_api_v1.cpp)
  target_include_directories(sin_grunt_behavior_c_abi_phase613_tests PRIVATE include)
  target_link_libraries(sin_grunt_behavior_c_abi_phase613_tests
    PRIVATE sin_grunt_behavior_c_abi)
  set_target_properties(sin_grunt_behavior_c_abi_phase613_tests PROPERTIES
    CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
  target_compile_options(sin_grunt_behavior_c_abi_phase613_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>
    $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>)
  add_test(NAME sin_grunt_behavior_c_abi_phase613_tests
    COMMAND sin_grunt_behavior_c_abi_phase613_tests)

  add_executable(sin_grunt_behavior_dynload_phase613_tests
    tests/test_grunt_behavior_dynload_phase613.cpp)
  target_include_directories(sin_grunt_behavior_dynload_phase613_tests PRIVATE include)
  target_link_libraries(sin_grunt_behavior_dynload_phase613_tests
    PRIVATE ${CMAKE_DL_LIBS})
  set_target_properties(sin_grunt_behavior_dynload_phase613_tests PROPERTIES
    CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
  target_compile_options(sin_grunt_behavior_dynload_phase613_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>
    $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>)
  add_dependencies(sin_grunt_behavior_dynload_phase613_tests
    sin_grunt_behavior_c_abi)
  add_test(NAME sin_grunt_behavior_dynload_phase613_tests
    COMMAND sin_grunt_behavior_dynload_phase613_tests
      $<TARGET_FILE:sin_grunt_behavior_c_abi>)
endif()

# Phase614: attach the loadable Grunt C ABI only to host-confirmed native NPCs.
# This registry owns sessions, never Source/SiN entity objects or factories.
add_library(sin_grunt_npc_session_registry STATIC
  src/original_grunt_npc_session_registry_phase614.cpp)
target_include_directories(sin_grunt_npc_session_registry PUBLIC include)
target_link_libraries(sin_grunt_npc_session_registry PUBLIC
  sin_grunt_behavior_c_abi)
set_target_properties(sin_grunt_npc_session_registry PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
target_compile_options(sin_grunt_npc_session_registry PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_grunt_npc_session_registry_phase614_tests
    tests/test_original_grunt_npc_session_registry_phase614.cpp)
  target_include_directories(sin_grunt_npc_session_registry_phase614_tests PRIVATE include)
  target_link_libraries(sin_grunt_npc_session_registry_phase614_tests
    PRIVATE sin_grunt_npc_session_registry)
  set_target_properties(sin_grunt_npc_session_registry_phase614_tests PROPERTIES
    CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
  target_compile_options(sin_grunt_npc_session_registry_phase614_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>
    $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>)
  add_test(NAME sin_grunt_npc_session_registry_phase614_tests
    COMMAND sin_grunt_npc_session_registry_phase614_tests)
endif()

# Phase617: real 2006 Assault lifecycle +0x1C RET, +0x24 bridge
# virtual0, +0x28 RET: independent host parity and SHA-locked source audit.
if(BUILD_TESTING)
  add_executable(sin_original_assault_activation_phase617_tests
    tests/test_original_grunt_assault_activation_phase617.cpp)
  target_include_directories(sin_original_assault_activation_phase617_tests
    PRIVATE include)
  set_target_properties(sin_original_assault_activation_phase617_tests PROPERTIES
    CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
  target_compile_options(sin_original_assault_activation_phase617_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>
    $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>)
  add_test(NAME sin_original_assault_activation_phase617_tests
    COMMAND sin_original_assault_activation_phase617_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_assault_activation_phase617_native_selftest
      COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/research/tools/audit_sin_original_assault_activation_phase617.py" --self-test)
  endif()
endif()

# Phase619: a SEPARATE C ABI for attaching reconstructed behavior sessions
# to a native Source host's already existing Grunt NPC. No CreateInterface
# replacement, Source object allocation, or engine callback interception.
set_target_properties(sin_grunt_npc_session_registry PROPERTIES
  POSITION_INDEPENDENT_CODE ON)
add_library(sin_grunt_native_host_c_abi SHARED
  src/grunt_native_host_c_api_v1.cpp)
target_include_directories(sin_grunt_native_host_c_abi PUBLIC include)
target_link_libraries(sin_grunt_native_host_c_abi PRIVATE
  sin_grunt_npc_session_registry)
set_target_properties(sin_grunt_native_host_c_abi PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF
  CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
target_compile_options(sin_grunt_native_host_c_abi PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(MSVC)
  target_sources(sin_grunt_native_host_c_abi PRIVATE
    src/grunt_native_host_win32.def)
endif()
if(UNIX AND NOT APPLE)
  target_link_options(sin_grunt_native_host_c_abi PRIVATE
    "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/src/grunt_native_host_exports.map")
  set_property(TARGET sin_grunt_native_host_c_abi APPEND PROPERTY LINK_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/grunt_native_host_exports.map")
endif()
if(BUILD_TESTING)
  add_executable(sin_grunt_native_host_c_api_phase619_tests
    tests/test_grunt_native_host_c_api_phase619.c)
  target_include_directories(sin_grunt_native_host_c_api_phase619_tests
    PRIVATE include)
  target_link_libraries(sin_grunt_native_host_c_api_phase619_tests
    PRIVATE sin_grunt_native_host_c_abi)
  set_target_properties(sin_grunt_native_host_c_api_phase619_tests PROPERTIES
    C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
  target_compile_options(sin_grunt_native_host_c_api_phase619_tests PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>
    $<$<C_COMPILER_ID:MSVC>:/UNDEBUG>)
  add_test(NAME sin_grunt_native_host_c_api_phase619_tests
    COMMAND sin_grunt_native_host_c_api_phase619_tests)

  add_executable(sin_grunt_native_host_dynload_phase619_tests
    tests/test_grunt_native_host_dynload_phase619.cpp)
  target_include_directories(sin_grunt_native_host_dynload_phase619_tests
    PRIVATE include)
  target_link_libraries(sin_grunt_native_host_dynload_phase619_tests
    PRIVATE ${CMAKE_DL_LIBS})
  set_target_properties(sin_grunt_native_host_dynload_phase619_tests PROPERTIES
    CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
  target_compile_options(sin_grunt_native_host_dynload_phase619_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>
    $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>)
  add_dependencies(sin_grunt_native_host_dynload_phase619_tests
    sin_grunt_native_host_c_abi)
  add_test(NAME sin_grunt_native_host_dynload_phase619_tests
    COMMAND sin_grunt_native_host_dynload_phase619_tests
      $<TARGET_FILE:sin_grunt_native_host_c_abi>)
endif()

# Phase621: the default npc_druglab_grunt is a FOURTH original
# CEntityFactory<CNPC_MobGrunt> alias, with no weapon-specific Spawn edits.
# CI runs synthetic x86 control-flow regressions; owner PE32 hashes/bytes
# are checked locally and never checked into the public repository.
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_default_mobgrunt_factory_spawn_audit_phase621_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_base_grunt_factory_phase621.py
        --self-test)
  endif()
endif()

# Phase622: source-authenticated original 2006 CNPC_EliteGrunt factory.
# It uses an independently modeled host callback sequence and a different
# native object size/constructor from the original CNPC_MobGrunt family.
# No live Source entities, native vtable cloning, physics or rendering.
add_library(sin_original_elite_grunt_factory_phase622 STATIC
  src/original_elite_grunt_factory_phase622.cpp)
target_include_directories(sin_original_elite_grunt_factory_phase622 PUBLIC include)
target_compile_options(sin_original_elite_grunt_factory_phase622 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_elite_grunt_factory_phase622_tests
    tests/test_original_elite_grunt_factory_phase622.cpp)
  target_link_libraries(sin_original_elite_grunt_factory_phase622_tests PRIVATE
    sin_original_elite_grunt_factory_phase622)
  target_compile_options(sin_original_elite_grunt_factory_phase622_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_elite_grunt_factory_phase622_tests
    COMMAND sin_original_elite_grunt_factory_phase622_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_elite_grunt_factory_phase622_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_elite_grunt_factory_phase622.py
        --self-test)
  endif()
endif()

# Phase623: authentic original EliteGrunt::Spawn 505-byte control flow.
# Engine-neutral original-source reference, not a Source CBaseNPC or
# replacement game DLL. No physics, surfaces, models or prop work.
add_library(sin_original_elite_grunt_spawn_phase623 STATIC
  src/original_elite_grunt_spawn_phase623.cpp)
target_include_directories(sin_original_elite_grunt_spawn_phase623 PUBLIC include)
target_compile_options(sin_original_elite_grunt_spawn_phase623 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_elite_grunt_spawn_phase623_tests
    tests/test_original_elite_grunt_spawn_phase623.cpp)
  target_link_libraries(sin_original_elite_grunt_spawn_phase623_tests PRIVATE
    sin_original_elite_grunt_spawn_phase623)
  target_compile_options(sin_original_elite_grunt_spawn_phase623_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_elite_grunt_spawn_phase623_tests
    COMMAND sin_original_elite_grunt_spawn_phase623_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_elite_grunt_spawn_phase623_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_elite_grunt_spawn_phase623.py
        --self-test)
  endif()
endif()

# Phase626: independent native Elite C ABI for already engine-owned
# npc_druglab_grunt_elite. Links original-source Spawn logic and bridges
# its +0xFDC serial handle to the authenticated event 0x43 path.
# No native Source factory/object, vtable injection or game callbacks.
set_target_properties(sin_original_elite_grunt_spawn_phase623
  PROPERTIES POSITION_INDEPENDENT_CODE ON)
add_library(sin_elite_native_host_c_abi SHARED
  src/elite_native_host_c_api_v1.cpp)
target_include_directories(sin_elite_native_host_c_abi PUBLIC include)
target_link_libraries(sin_elite_native_host_c_abi PRIVATE
  sin_original_elite_grunt_spawn_phase623)
target_compile_options(sin_elite_native_host_c_abi PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
set_target_properties(sin_elite_native_host_c_abi PROPERTIES
  CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
if(UNIX AND NOT APPLE)
  target_link_options(sin_elite_native_host_c_abi PRIVATE
    "LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/src/elite_native_host_exports.map")
endif()
if(WIN32 AND MSVC)
  target_sources(sin_elite_native_host_c_abi PRIVATE
    src/elite_native_host_win32.def)
endif()
if(BUILD_TESTING)
  add_executable(sin_elite_native_host_c_api_phase626_tests
    tests/test_elite_native_host_c_api_phase626.c)
  target_include_directories(sin_elite_native_host_c_api_phase626_tests
    PRIVATE include)
  target_link_libraries(sin_elite_native_host_c_api_phase626_tests
    PRIVATE sin_elite_native_host_c_abi)
  target_compile_options(sin_elite_native_host_c_api_phase626_tests
    PRIVATE $<$<C_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_elite_native_host_c_api_phase626_tests
    COMMAND sin_elite_native_host_c_api_phase626_tests)

  add_executable(sin_elite_native_host_dynload_phase626_tests
    tests/test_elite_native_host_dynload_phase626.cpp)
  target_include_directories(sin_elite_native_host_dynload_phase626_tests
    PRIVATE include)
  target_link_libraries(sin_elite_native_host_dynload_phase626_tests
    PRIVATE ${CMAKE_DL_LIBS})
  target_compile_options(sin_elite_native_host_dynload_phase626_tests
    PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_dependencies(sin_elite_native_host_dynload_phase626_tests
    sin_elite_native_host_c_abi)
  add_test(NAME sin_elite_native_host_dynload_phase626_tests
    COMMAND sin_elite_native_host_dynload_phase626_tests
      $<TARGET_FILE:sin_elite_native_host_c_abi>)
endif()

# Phase629: authentic original 2006 event43 weapon gate and paired
# 1-second state progression from whole source SHA-verified x86 methods.
# Pure host-owned model; never a native Source vtable or game executable.
add_library(sin_original_elite_event43_chaingun_gate_phase629 STATIC
  src/original_elite_event43_chaingun_gate_phase629.cpp)
target_include_directories(sin_original_elite_event43_chaingun_gate_phase629 PUBLIC include)
target_compile_options(sin_original_elite_event43_chaingun_gate_phase629 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_elite_event43_chaingun_gate_phase629_tests
    tests/test_original_elite_event43_chaingun_gate_phase629.cpp)
  target_link_libraries(sin_original_elite_event43_chaingun_gate_phase629_tests
    PRIVATE sin_original_elite_event43_chaingun_gate_phase629)
  target_compile_options(sin_original_elite_event43_chaingun_gate_phase629_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_elite_event43_chaingun_gate_phase629_tests
    COMMAND sin_original_elite_event43_chaingun_gate_phase629_tests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_elite_event43_chaingun_gate_phase629_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_elite_event43_chaingun_gate_phase629.py
        --self-test)
  endif()
endif()

# Gameplay Phase630: preserve the exact eight-export Elite native v1 ABI.
# A separate THREE-export C shared library binds original 2006 Phase629
# source-state gate to Phase626's actor-verified original event43 path.
set_target_properties(sin_original_elite_event43_chaingun_gate_phase629
  PROPERTIES POSITION_INDEPENDENT_CODE ON)
add_library(sin_elite_source_gate_phase630 SHARED
  src/elite_native_source_gate_phase630.cpp)
target_include_directories(sin_elite_source_gate_phase630 PUBLIC include)
target_link_libraries(sin_elite_source_gate_phase630 PRIVATE
  sin_elite_native_host_c_abi
  sin_original_elite_event43_chaingun_gate_phase629)
target_compile_options(sin_elite_source_gate_phase630 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
set_target_properties(sin_elite_source_gate_phase630 PROPERTIES
  CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
if(UNIX AND NOT APPLE)
  target_link_options(sin_elite_source_gate_phase630 PRIVATE
    "LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/src/elite_native_source_gate_phase630.exports.map")
endif()
if(WIN32 AND MSVC)
  target_sources(sin_elite_source_gate_phase630 PRIVATE
    src/elite_native_source_gate_phase630.win32.def)
endif()
if(BUILD_TESTING)
  add_executable(sin_elite_source_gate_phase630_tests
    tests/test_elite_native_source_gate_phase630.cpp)
  target_include_directories(sin_elite_source_gate_phase630_tests
    PRIVATE include)
  target_link_libraries(sin_elite_source_gate_phase630_tests PRIVATE
    sin_elite_source_gate_phase630 sin_elite_native_host_c_abi)
  target_compile_options(sin_elite_source_gate_phase630_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_elite_source_gate_phase630_tests
    COMMAND sin_elite_source_gate_phase630_tests)
  add_executable(sin_elite_source_gate_phase630_dynload_tests
    tests/test_elite_native_source_gate_dynload_phase630.cpp)
  target_include_directories(sin_elite_source_gate_phase630_dynload_tests
    PRIVATE include)
  target_link_libraries(sin_elite_source_gate_phase630_dynload_tests PRIVATE
    ${CMAKE_DL_LIBS})
  target_compile_options(sin_elite_source_gate_phase630_dynload_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_dependencies(sin_elite_source_gate_phase630_dynload_tests
    sin_elite_source_gate_phase630)
  add_test(NAME sin_elite_source_gate_phase630_dynload_tests
    COMMAND sin_elite_source_gate_phase630_dynload_tests
      $<TARGET_FILE:sin_elite_source_gate_phase630>)
endif()

# Gameplay Phase631: complete source-authenticated original Elite virtual
# slot +0x424 invokes the weapon state updater with double serial resolution
# and then inherited Grunt tick. Pure host callback model, not a game vtable.
add_library(sin_original_elite_weapon_tick_phase631 STATIC
  src/original_elite_weapon_tick_phase631.cpp)
target_include_directories(sin_original_elite_weapon_tick_phase631 PUBLIC include)
target_compile_options(sin_original_elite_weapon_tick_phase631 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_elite_weapon_tick_phase631_tests
    tests/test_original_elite_weapon_tick_phase631.cpp)
  target_link_libraries(sin_original_elite_weapon_tick_phase631_tests PRIVATE
    sin_original_elite_weapon_tick_phase631)
  target_compile_options(sin_original_elite_weapon_tick_phase631_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_elite_weapon_tick_phase631_tests
    COMMAND sin_original_elite_weapon_tick_phase631_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_elite_weapon_tick_phase631_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_elite_weapon_tick_phase631.py
        --self-test)
  endif()
endif()

# Gameplay Phase632: independently SHA256-authenticated 232-byte original
# Elite vtable +0x44C event handler with separate 0x43 and 0x47 branches.
# Host callback model only: NO guessed Source object ABI or engine calls.
add_library(sin_original_elite_secondary_event_phase632 STATIC
  src/original_elite_secondary_event_phase632.cpp)
target_include_directories(sin_original_elite_secondary_event_phase632 PUBLIC include)
target_compile_options(sin_original_elite_secondary_event_phase632 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_elite_secondary_event_phase632_tests
    tests/test_original_elite_secondary_event_phase632.cpp)
  target_link_libraries(sin_original_elite_secondary_event_phase632_tests PRIVATE
    sin_original_elite_secondary_event_phase632)
  target_compile_options(sin_original_elite_secondary_event_phase632_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_elite_secondary_event_phase632_tests
    COMMAND sin_original_elite_secondary_event_phase632_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_elite_secondary_event_phase632_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_elite_secondary_event_phase632.py
        --self-test)
  endif()
endif()

# Gameplay Phase633: three authenticated whole original Elite event47
# methods (clock +0x9DC predicate, actor +0x78C serial-handle getter,
# resolved-target DWORD +0x4EC dirty-state setter). Test against the
# actual independently compiled Phase632 event handler, not dummy booleans.
add_library(sin_original_elite_event47_helpers_phase633 STATIC
  src/original_elite_event47_helpers_phase633.cpp)
target_include_directories(sin_original_elite_event47_helpers_phase633 PUBLIC include)
target_compile_options(sin_original_elite_event47_helpers_phase633 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_elite_event47_helpers_phase633_tests
    tests/test_original_elite_event47_helpers_phase633.cpp)
  target_link_libraries(sin_original_elite_event47_helpers_phase633_tests PRIVATE
    sin_original_elite_event47_helpers_phase633
    sin_original_elite_secondary_event_phase632)
  target_compile_options(sin_original_elite_event47_helpers_phase633_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_elite_event47_helpers_phase633_tests
    COMMAND sin_original_elite_event47_helpers_phase633_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_elite_event47_helpers_phase633_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_elite_event47_helpers_phase633.py
        --self-test)
  endif()
endif()

# Gameplay Phase634: two COMPLETE SHA256-authenticated original Elite terminal
# event helper methods. The integration test links actual compiled source
# references for Phase632->633->634, including 3/4/5 flag bit writes.
add_library(sin_original_elite_event47_terminal_phase634 STATIC
  src/original_elite_event47_terminal_phase634.cpp)
target_include_directories(sin_original_elite_event47_terminal_phase634 PUBLIC include)
target_compile_options(sin_original_elite_event47_terminal_phase634 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_elite_event47_terminal_phase634_tests
    tests/test_original_elite_event47_terminal_phase634.cpp)
  target_link_libraries(sin_original_elite_event47_terminal_phase634_tests PRIVATE
    sin_original_elite_event47_terminal_phase634
    sin_original_elite_event47_helpers_phase633
    sin_original_elite_secondary_event_phase632)
  target_compile_options(sin_original_elite_event47_terminal_phase634_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_elite_event47_terminal_phase634_tests
    COMMAND sin_original_elite_event47_terminal_phase634_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_elite_event47_terminal_phase634_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_elite_event47_terminal_phase634.py
        --self-test)
  endif()
endif()

# Phase635: original full 58-byte 2006 SiN Elite linked-range event-ID
# mapping algorithm, separately SHA256-pinned in the original server.dll.
# The synthetic integration CTest links REAL phase632->633->634->635 models.
add_library(sin_original_elite_id_mapping_phase635 STATIC
  src/original_elite_id_mapping_phase635.cpp)
target_include_directories(sin_original_elite_id_mapping_phase635 PUBLIC include)
target_compile_options(sin_original_elite_id_mapping_phase635 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_elite_id_mapping_phase635_tests
    tests/test_original_elite_id_mapping_phase635.cpp)
  target_link_libraries(sin_original_elite_id_mapping_phase635_tests PRIVATE
    sin_original_elite_id_mapping_phase635
    sin_original_elite_event47_terminal_phase634
    sin_original_elite_event47_helpers_phase633
    sin_original_elite_secondary_event_phase632)
  target_compile_options(sin_original_elite_id_mapping_phase635_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_elite_id_mapping_phase635_tests
    COMMAND sin_original_elite_id_mapping_phase635_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_elite_id_mapping_phase635_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_elite_id_mapping_phase635.py
        --self-test)
  endif()
endif()

# Phase636 source-verified original 2006 SiN Elite mapping node constructor,
# conditional successor linker and three-node owner registration. This
# unit verifies that Phase635 map conversion still skips constructed
# sentinel nodes until actual original runtime ID ranges are populated.
add_library(sin_original_elite_mapping_init_phase636 STATIC
  src/original_elite_mapping_init_phase636.cpp)
target_include_directories(sin_original_elite_mapping_init_phase636 PUBLIC include)
target_compile_options(sin_original_elite_mapping_init_phase636 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_elite_mapping_init_phase636_tests
    tests/test_original_elite_mapping_init_phase636.cpp)
  target_link_libraries(sin_original_elite_mapping_init_phase636_tests PRIVATE
    sin_original_elite_mapping_init_phase636
    sin_original_elite_id_mapping_phase635)
  target_compile_options(sin_original_elite_mapping_init_phase636_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_elite_mapping_init_phase636_tests
    COMMAND sin_original_elite_mapping_init_phase636_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_elite_mapping_init_phase636_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_elite_mapping_init_phase636.py
        --self-test)
  endif()
endif()

# Phase637: original 2006 SiN 217-byte LOCALID registrar and 31-byte
# task (owner+0x1C) wrapper. Native source behavior composes with the
# Phase636 constructor/linker and Phase635 true range converter.
add_library(sin_original_elite_range_registration_phase637 STATIC
  src/original_elite_range_registration_phase637.cpp)
target_include_directories(sin_original_elite_range_registration_phase637
  PUBLIC include)
target_compile_options(sin_original_elite_range_registration_phase637 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_elite_range_registration_phase637_tests
    tests/test_original_elite_range_registration_phase637.cpp)
  target_link_libraries(sin_original_elite_range_registration_phase637_tests PRIVATE
    sin_original_elite_range_registration_phase637
    sin_original_elite_mapping_init_phase636
    sin_original_elite_id_mapping_phase635)
  target_compile_options(sin_original_elite_range_registration_phase637_tests
    PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_elite_range_registration_phase637_tests
    COMMAND sin_original_elite_range_registration_phase637_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_elite_range_registration_phase637_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_elite_range_registration_phase637.py
        --self-test)
  endif()
endif()

# Phase638 complete original 2006 SiN condition wrapper and symbol slot writer.
add_library(sin_original_symbol_forward_phase638 STATIC
  src/original_symbol_forward_phase638.cpp)
target_include_directories(sin_original_symbol_forward_phase638 PUBLIC include)
target_compile_options(sin_original_symbol_forward_phase638 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_symbol_forward_phase638_tests
    tests/test_original_symbol_forward_phase638.cpp)
  target_link_libraries(sin_original_symbol_forward_phase638_tests PRIVATE
    sin_original_symbol_forward_phase638
    sin_original_elite_range_registration_phase637)
  target_compile_options(sin_original_symbol_forward_phase638_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_symbol_forward_phase638_tests
    COMMAND sin_original_symbol_forward_phase638_tests)
  if(${Python3_Interpreter_FOUND})
    add_test(NAME sin_original_symbol_forward_phase638_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_symbol_forward_phase638.py
        --self-test)
  endif()
endif()

# Phase639 native-source symbol-slot registrar 102B and acquire 98B.
add_library(sin_original_symbol_slot_allocation_phase639 STATIC
  src/original_symbol_slot_allocation_phase639.cpp)
target_include_directories(sin_original_symbol_slot_allocation_phase639 PUBLIC include)
target_compile_options(sin_original_symbol_slot_allocation_phase639 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_symbol_slot_allocation_phase639_tests
    tests/test_original_symbol_slot_allocation_phase639.cpp)
  target_link_libraries(sin_original_symbol_slot_allocation_phase639_tests PRIVATE
    sin_original_symbol_slot_allocation_phase639
    sin_original_symbol_forward_phase638
    sin_original_elite_range_registration_phase637)
  target_compile_options(sin_original_symbol_slot_allocation_phase639_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_symbol_slot_allocation_phase639_tests
    COMMAND sin_original_symbol_slot_allocation_phase639_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_symbol_slot_allocation_phase639_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_symbol_slot_allocation_phase639.py
        --self-test)
  endif()
endif()

# Phase640: authenticated original 106B symbol tree linker and 14B accessor.
# The 0x22281840 rebalancer remains an explicit mandatory host callback.
add_library(sin_original_symbol_tree_link_phase640 STATIC
  src/original_symbol_tree_link_phase640.cpp)
target_include_directories(sin_original_symbol_tree_link_phase640 PUBLIC include)
target_compile_options(sin_original_symbol_tree_link_phase640 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_symbol_tree_link_phase640_tests
    tests/test_original_symbol_tree_link_phase640.cpp)
  target_link_libraries(sin_original_symbol_tree_link_phase640_tests PRIVATE
    sin_original_symbol_tree_link_phase640
    sin_original_symbol_slot_allocation_phase639
    sin_original_symbol_forward_phase638
    sin_original_elite_range_registration_phase637)
  target_compile_options(sin_original_symbol_tree_link_phase640_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_symbol_tree_link_phase640_tests
    COMMAND sin_original_symbol_tree_link_phase640_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_symbol_tree_link_phase640_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_symbol_tree_link_phase640.py
        --self-test)
  endif()
endif()

# Phase641 original retail SiN tree rotation source references:
# two complete rotations (231+234 bytes) plus 81B sentinel-aware accessor.
add_library(sin_original_symbol_rotations_phase641 STATIC
  src/original_symbol_rotations_phase641.cpp)
target_include_directories(sin_original_symbol_rotations_phase641 PUBLIC include)
target_compile_options(sin_original_symbol_rotations_phase641 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_symbol_rotations_phase641_tests
    tests/test_original_symbol_rotations_phase641.cpp)
  target_link_libraries(sin_original_symbol_rotations_phase641_tests PRIVATE
    sin_original_symbol_rotations_phase641)
  target_compile_options(sin_original_symbol_rotations_phase641_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_symbol_rotations_phase641_tests
    COMMAND sin_original_symbol_rotations_phase641_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_symbol_rotations_phase641_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_symbol_rotations_phase641.py
        --self-test)
  endif()
endif()

# Phase642 original SiN red-black symbol tree rebalance, complete retail
# 1482-byte method source reference, links into original Phase641 rotations.
add_library(sin_original_symbol_rebalance_phase642 STATIC
  src/original_symbol_rebalance_phase642.cpp)
target_include_directories(sin_original_symbol_rebalance_phase642 PUBLIC include)
target_link_libraries(sin_original_symbol_rebalance_phase642 PUBLIC
  sin_original_symbol_rotations_phase641)
target_compile_options(sin_original_symbol_rebalance_phase642 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_symbol_rebalance_phase642_tests
    tests/test_original_symbol_rebalance_phase642.cpp)
  target_link_libraries(sin_original_symbol_rebalance_phase642_tests PRIVATE
    sin_original_symbol_rebalance_phase642
    sin_original_symbol_rotations_phase641
    sin_original_symbol_tree_link_phase640)
  target_compile_options(sin_original_symbol_rebalance_phase642_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_symbol_rebalance_phase642_tests
    COMMAND sin_original_symbol_rebalance_phase642_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_symbol_rebalance_phase642_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_symbol_rebalance_phase642.py
        --self-test)
  endif()
endif()

# Phase643: complete original 255-byte 2006 SiN symbol search/parent-side
# enumerator, with typed original comparator callback and real full-chain tests.
add_library(sin_original_symbol_enumeration_phase643 STATIC
  src/original_symbol_enumeration_phase643.cpp)
target_include_directories(sin_original_symbol_enumeration_phase643 PUBLIC include)
target_compile_options(sin_original_symbol_enumeration_phase643 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_symbol_enumeration_phase643_tests
    tests/test_original_symbol_enumeration_phase643.cpp)
  target_link_libraries(sin_original_symbol_enumeration_phase643_tests PRIVATE
    sin_original_symbol_enumeration_phase643
    sin_original_symbol_slot_allocation_phase639
    sin_original_symbol_tree_link_phase640
    sin_original_symbol_rebalance_phase642
    sin_original_symbol_rotations_phase641
    sin_original_symbol_forward_phase638
    sin_original_elite_range_registration_phase637)
  target_compile_options(sin_original_symbol_enumeration_phase643_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_symbol_enumeration_phase643_tests
    COMMAND sin_original_symbol_enumeration_phase643_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_symbol_enumeration_phase643_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_symbol_enumeration_phase643.py
        --self-test)
  endif()
endif()

# Phase644 original retail symbol-slot growth (110B) and release (39B).
# Tests compose the compiled Phase638-644 symbol tree with dynamic allocation.
add_library(sin_original_symbol_growth_phase644 STATIC
  src/original_symbol_growth_phase644.cpp)
target_include_directories(sin_original_symbol_growth_phase644 PUBLIC include)
target_compile_options(sin_original_symbol_growth_phase644 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_symbol_growth_phase644_tests
    tests/test_original_symbol_growth_phase644.cpp)
  target_link_libraries(sin_original_symbol_growth_phase644_tests PRIVATE
    sin_original_symbol_growth_phase644
    sin_original_symbol_enumeration_phase643
    sin_original_symbol_slot_allocation_phase639
    sin_original_symbol_tree_link_phase640
    sin_original_symbol_rebalance_phase642
    sin_original_symbol_rotations_phase641
    sin_original_symbol_forward_phase638
    sin_original_elite_range_registration_phase637)
  target_compile_options(sin_original_symbol_growth_phase644_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_symbol_growth_phase644_tests
    COMMAND sin_original_symbol_growth_phase644_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_symbol_growth_phase644_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_symbol_growth_phase644.py
        --self-test)
  endif()
endif()

# Phase645 original 2006 SiN allocator wrappers + debug module tag
# (277 complete retail executable bytes); actual memory manager host supplied.
add_library(sin_original_memory_wrappers_phase645 STATIC
  src/original_memory_wrappers_phase645.cpp)
target_include_directories(sin_original_memory_wrappers_phase645 PUBLIC include)
target_compile_options(sin_original_memory_wrappers_phase645 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_memory_wrappers_phase645_tests
    tests/test_original_memory_wrappers_phase645.cpp)
  target_link_libraries(sin_original_memory_wrappers_phase645_tests PRIVATE
    sin_original_memory_wrappers_phase645
    sin_original_symbol_growth_phase644
    sin_original_symbol_enumeration_phase643
    sin_original_symbol_slot_allocation_phase639
    sin_original_symbol_tree_link_phase640
    sin_original_symbol_rebalance_phase642
    sin_original_symbol_rotations_phase641
    sin_original_symbol_forward_phase638
    sin_original_elite_range_registration_phase637)
  target_compile_options(sin_original_memory_wrappers_phase645_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_memory_wrappers_phase645_tests
    COMMAND sin_original_memory_wrappers_phase645_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_memory_wrappers_phase645_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_memory_wrappers_phase645.py
        --self-test)
  endif()
endif()

# Phase646 authenticated original 2006 SiN 16-bit comparator and table
# constructor/factory (+251 complete original x86 executable bytes).
add_library(sin_original_symbol_comparator_phase646 STATIC
  src/original_symbol_comparator_phase646.cpp)
target_include_directories(sin_original_symbol_comparator_phase646 PUBLIC include)
target_compile_options(sin_original_symbol_comparator_phase646 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_symbol_comparator_phase646_tests
    tests/test_original_symbol_comparator_phase646.cpp)
  target_link_libraries(sin_original_symbol_comparator_phase646_tests PRIVATE
    sin_original_symbol_comparator_phase646
    sin_original_memory_wrappers_phase645
    sin_original_symbol_growth_phase644
    sin_original_symbol_enumeration_phase643
    sin_original_symbol_slot_allocation_phase639
    sin_original_symbol_tree_link_phase640
    sin_original_symbol_rebalance_phase642
    sin_original_symbol_rotations_phase641
    sin_original_symbol_forward_phase638
    sin_original_elite_range_registration_phase637)
  target_compile_options(sin_original_symbol_comparator_phase646_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_symbol_comparator_phase646_tests
    COMMAND sin_original_symbol_comparator_phase646_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_symbol_comparator_phase646_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_symbol_comparator_phase646.py
        --self-test)
  endif()
endif()

# Phase647 original 2006 SiN nested map/slot constructor and array release
# (212 complete retail bytes) with full Phase638..647 integration coverage.
add_library(sin_original_nested_map_phase647 STATIC
  src/original_nested_map_phase647.cpp)
target_include_directories(sin_original_nested_map_phase647 PUBLIC include)
target_compile_options(sin_original_nested_map_phase647 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_nested_map_phase647_tests
    tests/test_original_nested_map_phase647.cpp)
  target_link_libraries(sin_original_nested_map_phase647_tests PRIVATE
    sin_original_nested_map_phase647)
  target_compile_options(sin_original_nested_map_phase647_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_nested_map_phase647_tests
    COMMAND sin_original_nested_map_phase647_tests)
  add_executable(sin_original_nested_map_chain_phase647_tests
    tests/test_original_nested_map_chain_phase647.cpp)
  target_link_libraries(sin_original_nested_map_chain_phase647_tests PRIVATE
    sin_original_nested_map_phase647
    sin_original_symbol_comparator_phase646
    sin_original_memory_wrappers_phase645
    sin_original_symbol_growth_phase644
    sin_original_symbol_enumeration_phase643
    sin_original_symbol_slot_allocation_phase639
    sin_original_symbol_tree_link_phase640
    sin_original_symbol_rebalance_phase642
    sin_original_symbol_rotations_phase641
    sin_original_symbol_forward_phase638
    sin_original_elite_range_registration_phase637)
  target_compile_options(sin_original_nested_map_chain_phase647_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_nested_map_chain_phase647_tests
    COMMAND sin_original_nested_map_chain_phase647_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_nested_map_phase647_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_nested_map_phase647.py
        --self-test)
  endif()
endif()

# Phase648: two complete original 2006 SiN string comparators (270 x86 B).
# Keep original 0x223A5574 call host-supplied; no invented CRT behavior.
add_library(sin_original_nested_comparators_phase648 STATIC
  src/original_nested_comparators_phase648.cpp)
target_include_directories(sin_original_nested_comparators_phase648 PUBLIC include)
target_compile_options(sin_original_nested_comparators_phase648 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_nested_comparators_phase648_tests
    tests/test_original_nested_comparators_phase648.cpp)
  target_link_libraries(sin_original_nested_comparators_phase648_tests PRIVATE
    sin_original_nested_comparators_phase648 sin_original_nested_map_phase647)
  target_compile_options(sin_original_nested_comparators_phase648_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_nested_comparators_phase648_tests
    COMMAND sin_original_nested_comparators_phase648_tests)
  add_executable(sin_original_nested_comparator_chain_phase648_tests
    tests/test_original_nested_comparator_chain_phase648.cpp)
  target_link_libraries(sin_original_nested_comparator_chain_phase648_tests PRIVATE
    sin_original_nested_comparators_phase648
    sin_original_nested_map_phase647
    sin_original_symbol_comparator_phase646
    sin_original_memory_wrappers_phase645
    sin_original_symbol_growth_phase644
    sin_original_symbol_enumeration_phase643
    sin_original_symbol_slot_allocation_phase639
    sin_original_symbol_tree_link_phase640
    sin_original_symbol_rebalance_phase642
    sin_original_symbol_rotations_phase641
    sin_original_symbol_forward_phase638
    sin_original_elite_range_registration_phase637)
  target_compile_options(sin_original_nested_comparator_chain_phase648_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_nested_comparator_chain_phase648_tests
    COMMAND sin_original_nested_comparator_chain_phase648_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_nested_comparators_phase648_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_nested_comparators_phase648.py
        --self-test)
  endif()
endif()

# Phase649 original 2006 SiN mode-switched string comparator (67B),
# ASCII fallback (78B) and locale transforming helper (213B).
add_library(sin_original_string_casefold_phase649 STATIC
  src/original_string_casefold_phase649.cpp)
target_include_directories(sin_original_string_casefold_phase649 PUBLIC include)
target_compile_options(sin_original_string_casefold_phase649 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_string_casefold_phase649_tests
    tests/test_original_string_casefold_phase649.cpp)
  target_link_libraries(sin_original_string_casefold_phase649_tests PRIVATE
    sin_original_string_casefold_phase649
    sin_original_nested_comparators_phase648)
  target_compile_options(sin_original_string_casefold_phase649_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_string_casefold_phase649_tests
    COMMAND sin_original_string_casefold_phase649_tests)
  add_executable(sin_original_string_casefold_chain_phase649_tests
    tests/test_original_string_casefold_chain_phase649.cpp)
  target_link_libraries(sin_original_string_casefold_chain_phase649_tests PRIVATE
    sin_original_string_casefold_phase649
    sin_original_nested_comparators_phase648
    sin_original_nested_map_phase647
    sin_original_symbol_comparator_phase646
    sin_original_memory_wrappers_phase645
    sin_original_symbol_growth_phase644
    sin_original_symbol_enumeration_phase643
    sin_original_symbol_slot_allocation_phase639
    sin_original_symbol_tree_link_phase640
    sin_original_symbol_rebalance_phase642
    sin_original_symbol_rotations_phase641
    sin_original_symbol_forward_phase638
    sin_original_elite_range_registration_phase637)
  target_compile_options(sin_original_string_casefold_chain_phase649_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_string_casefold_chain_phase649_tests
    COMMAND sin_original_string_casefold_chain_phase649_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_string_casefold_phase649_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_string_casefold_phase649.py
        --self-test)
  endif()
endif()

# Phase650 original SiN 2006 nested-string bidirectional search, block fit
# and C-string lookup-or-insert: 578 complete original x86 method bytes.
add_library(sin_original_string_lookup_phase650 STATIC
  src/original_string_lookup_phase650.cpp)
target_include_directories(sin_original_string_lookup_phase650 PUBLIC include)
target_compile_options(sin_original_string_lookup_phase650 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_string_lookup_phase650_tests
    tests/test_original_string_lookup_phase650.cpp)
  target_link_libraries(sin_original_string_lookup_phase650_tests PRIVATE
    sin_original_string_lookup_phase650
    sin_original_nested_comparators_phase648
    sin_original_string_casefold_phase649)
  target_compile_options(sin_original_string_lookup_phase650_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_string_lookup_phase650_tests
    COMMAND sin_original_string_lookup_phase650_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_string_lookup_phase650_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_string_lookup_phase650.py
        --self-test)
  endif()
endif()

# Phase651 original 2006 SiN nested block pointer array growth (112B),
# indexed insertion (100B), and nested 12B-node tree insertion (94B).
# Original deeper node-parent search/link/memmove are mandatory host calls.
add_library(sin_original_nested_insertion_phase651 STATIC
  src/original_nested_insertion_phase651.cpp)
target_include_directories(sin_original_nested_insertion_phase651 PUBLIC include)
target_compile_options(sin_original_nested_insertion_phase651 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_nested_insertion_phase651_tests
    tests/test_original_nested_insertion_phase651.cpp)
  target_link_libraries(sin_original_nested_insertion_phase651_tests PRIVATE
    sin_original_nested_insertion_phase651)
  target_compile_options(sin_original_nested_insertion_phase651_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_nested_insertion_phase651_tests
    COMMAND sin_original_nested_insertion_phase651_tests)
  add_executable(sin_original_nested_insertion_chain_phase651_tests
    tests/test_original_nested_insertion_chain_phase651.cpp)
  target_link_libraries(sin_original_nested_insertion_chain_phase651_tests PRIVATE
    sin_original_nested_insertion_phase651
    sin_original_string_lookup_phase650
    sin_original_nested_comparators_phase648
    sin_original_string_casefold_phase649)
  target_compile_options(sin_original_nested_insertion_chain_phase651_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_nested_insertion_chain_phase651_tests
    COMMAND sin_original_nested_insertion_chain_phase651_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_nested_insertion_phase651_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_nested_insertion_phase651.py
        --self-test)
  endif()
endif()

# Phase652 original 2006 SiN nested tree search, 12B slot acquire and link.
# 456 original bytes; rebalance 0x2228EDF0 stays host supplied.
add_library(sin_original_nested_tree_core_phase652 STATIC
  src/original_nested_tree_core_phase652.cpp)
target_include_directories(sin_original_nested_tree_core_phase652 PUBLIC include)
target_compile_options(sin_original_nested_tree_core_phase652 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_nested_tree_core_phase652_tests
    tests/test_original_nested_tree_core_phase652.cpp)
  target_link_libraries(sin_original_nested_tree_core_phase652_tests PRIVATE
    sin_original_nested_tree_core_phase652)
  target_compile_options(sin_original_nested_tree_core_phase652_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_nested_tree_core_phase652_tests
    COMMAND sin_original_nested_tree_core_phase652_tests)
  add_executable(sin_original_nested_tree_chain_phase652_tests
    tests/test_original_nested_tree_chain_phase652.cpp)
  target_link_libraries(sin_original_nested_tree_chain_phase652_tests PRIVATE
    sin_original_nested_tree_core_phase652
    sin_original_nested_insertion_phase651)
  target_compile_options(sin_original_nested_tree_chain_phase652_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_nested_tree_chain_phase652_tests
    COMMAND sin_original_nested_tree_chain_phase652_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_nested_tree_core_phase652_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_nested_tree_core_phase652.py
        --self-test)
  endif()
endif()

# Phase653 original retail 2006 SiN 12-byte slot capacity grower (115B)
# and free-node resolver (84B), replacing two Phase652 host dependencies.
add_library(sin_original_nested_slot_growth_phase653 STATIC
  src/original_nested_slot_growth_phase653.cpp)
target_include_directories(sin_original_nested_slot_growth_phase653 PUBLIC include)
target_compile_options(sin_original_nested_slot_growth_phase653 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_nested_slot_growth_phase653_tests
    tests/test_original_nested_slot_growth_phase653.cpp)
  target_link_libraries(sin_original_nested_slot_growth_phase653_tests PRIVATE
    sin_original_nested_slot_growth_phase653)
  target_compile_options(sin_original_nested_slot_growth_phase653_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_nested_slot_growth_phase653_tests
    COMMAND sin_original_nested_slot_growth_phase653_tests)
  add_executable(sin_original_nested_slot_growth_chain_phase653_tests
    tests/test_original_nested_slot_growth_chain_phase653.cpp)
  target_link_libraries(sin_original_nested_slot_growth_chain_phase653_tests PRIVATE
    sin_original_nested_slot_growth_phase653
    sin_original_nested_tree_core_phase652)
  target_compile_options(sin_original_nested_slot_growth_chain_phase653_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_nested_slot_growth_chain_phase653_tests
    COMMAND sin_original_nested_slot_growth_chain_phase653_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_nested_slot_growth_phase653_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_nested_slot_growth_phase653.py
        --self-test)
  endif()
endif()

# Phase654 recovered original 2006 SiN RB-tree insertion fixup (1387B)
# and its exact rotate-left (235B) / rotate-right (238B) helpers.
add_library(sin_original_nested_rebalance_phase654 STATIC
  src/original_nested_rebalance_phase654.cpp)
target_include_directories(sin_original_nested_rebalance_phase654 PUBLIC include)
target_compile_features(sin_original_nested_rebalance_phase654 PUBLIC cxx_std_17)
target_compile_options(sin_original_nested_rebalance_phase654 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_nested_rebalance_phase654_tests
    tests/test_original_nested_rebalance_phase654.cpp)
  target_link_libraries(sin_original_nested_rebalance_phase654_tests PRIVATE
    sin_original_nested_rebalance_phase654)
  target_compile_options(sin_original_nested_rebalance_phase654_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_nested_rebalance_phase654_tests
    COMMAND sin_original_nested_rebalance_phase654_tests)
  add_executable(sin_original_nested_rebalance_chain_phase654_tests
    tests/test_original_nested_rebalance_chain_phase654.cpp)
  target_link_libraries(sin_original_nested_rebalance_chain_phase654_tests PRIVATE
    sin_original_nested_rebalance_phase654
    sin_original_nested_slot_growth_phase653
    sin_original_nested_tree_core_phase652)
  target_compile_options(sin_original_nested_rebalance_chain_phase654_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_nested_rebalance_chain_phase654_tests
    COMMAND sin_original_nested_rebalance_chain_phase654_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_nested_rebalance_phase654_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_nested_rebalance_phase654.py
        --self-test)
  endif()
endif()

# Phase655: original SiN node free-list reset (132B), nested name-block
# teardown (49B), and original name-view lookup adapter (61B).
add_library(sin_original_nested_clear_phase655 STATIC
  src/original_nested_clear_phase655.cpp)
target_include_directories(sin_original_nested_clear_phase655 PUBLIC include)
target_compile_features(sin_original_nested_clear_phase655 PUBLIC cxx_std_17)
target_compile_options(sin_original_nested_clear_phase655 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_nested_clear_phase655_tests
    tests/test_original_nested_clear_phase655.cpp)
  target_link_libraries(sin_original_nested_clear_phase655_tests PRIVATE
    sin_original_nested_clear_phase655)
  target_compile_options(sin_original_nested_clear_phase655_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_nested_clear_phase655_tests
    COMMAND sin_original_nested_clear_phase655_tests)
  add_executable(sin_original_nested_clear_chain_phase655_tests
    tests/test_original_nested_clear_chain_phase655.cpp)
  target_link_libraries(sin_original_nested_clear_chain_phase655_tests PRIVATE
    sin_original_nested_clear_phase655
    sin_original_nested_rebalance_phase654
    sin_original_nested_slot_growth_phase653
    sin_original_nested_tree_core_phase652
    sin_original_string_lookup_phase650)
  target_compile_options(sin_original_nested_clear_chain_phase655_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_nested_clear_chain_phase655_tests
    COMMAND sin_original_nested_clear_chain_phase655_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_nested_clear_phase655_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_nested_clear_phase655.py
        --self-test)
  endif()
endif()

# Phase656: end-to-end recovered nested symbol registration and deletion
# through original SiN name comparators, string storage, red-black tree,
# capacity growth, lookup, teardown and free-list recycling.
if(BUILD_TESTING)
  add_executable(sin_original_full_name_registry_chain_phase656_tests
    tests/test_original_full_name_registry_chain_phase656.cpp)
  target_link_libraries(sin_original_full_name_registry_chain_phase656_tests PRIVATE
    sin_original_nested_clear_phase655
    sin_original_nested_rebalance_phase654
    sin_original_nested_slot_growth_phase653
    sin_original_nested_tree_core_phase652
    sin_original_nested_insertion_phase651
    sin_original_string_lookup_phase650
    sin_original_nested_comparators_phase648
    sin_original_string_casefold_phase649)
  target_compile_options(sin_original_full_name_registry_chain_phase656_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_full_name_registry_chain_phase656_tests
    COMMAND sin_original_full_name_registry_chain_phase656_tests)
endif()

# Phase657 public invented-byte PE32 interface registry/ABI mutation test.
# Owner's original 2006 client.dll/server.dll are NOT required by public CI.
if(BUILD_TESTING AND Python3_Interpreter_FOUND)
  add_test(NAME sin_original_createinterface_abi_phase657_audit_synthetic
    COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_createinterface_phase657.py
      --self-test)
endif()

# Phase658: recovered original server/client engine-interface acquisition prefix.
# The full original startup tails remain intentionally unmodeled.
add_library(sin_original_module_init_phase658 STATIC
  src/original_module_init_phase658.cpp)
target_include_directories(sin_original_module_init_phase658 PUBLIC include)
target_compile_features(sin_original_module_init_phase658 PUBLIC cxx_std_17)
target_compile_options(sin_original_module_init_phase658 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_module_init_phase658_tests
    tests/test_original_module_init_phase658.cpp)
  target_link_libraries(sin_original_module_init_phase658_tests PRIVATE
    sin_original_module_init_phase658)
  target_compile_options(sin_original_module_init_phase658_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_module_init_phase658_tests
    COMMAND sin_original_module_init_phase658_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_module_init_phase658_synthetic_audit
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_module_init_phase658.py
        --self-test)
  endif()
endif()

# Phase659: original SiN client app-system 16-byte linked-list lifecycle.
add_library(sin_original_client_appsystem_phase659 STATIC
  src/original_client_appsystem_phase659.cpp)
target_include_directories(sin_original_client_appsystem_phase659 PUBLIC include)
target_compile_features(sin_original_client_appsystem_phase659 PUBLIC cxx_std_17)
target_compile_options(sin_original_client_appsystem_phase659 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_client_appsystem_phase659_tests
    tests/test_original_client_appsystem_phase659.cpp)
  target_link_libraries(sin_original_client_appsystem_phase659_tests PRIVATE
    sin_original_client_appsystem_phase659)
  target_compile_options(sin_original_client_appsystem_phase659_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_client_appsystem_phase659_tests
    COMMAND sin_original_client_appsystem_phase659_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_client_appsystem_phase659_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_client_appsystem_phase659.py
        --self-test)
  endif()
endif()

# Phase660: original 2006 server AutoGameSystem pending registration and
# initialization, including duplicate elimination and full-list order.
add_library(sin_original_server_autogamesystem_phase660 STATIC
  src/original_server_autogamesystem_phase660.cpp)
target_include_directories(sin_original_server_autogamesystem_phase660 PUBLIC include)
target_compile_features(sin_original_server_autogamesystem_phase660 PUBLIC cxx_std_17)
target_compile_options(sin_original_server_autogamesystem_phase660 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_server_autogamesystem_phase660_tests
    tests/test_original_server_autogamesystem_phase660.cpp)
  target_link_libraries(sin_original_server_autogamesystem_phase660_tests PRIVATE
    sin_original_server_autogamesystem_phase660)
  target_compile_options(sin_original_server_autogamesystem_phase660_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_autogamesystem_phase660_tests
    COMMAND sin_original_server_autogamesystem_phase660_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_server_autogamesystem_phase660_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_server_autogamesystem_phase660.py
        --self-test)
  endif()
endif()

# Phase661: compile four original server AutoGameSystem virtual Init routines
# with an integration regression through Phase660's compiled registration list.
add_library(sin_original_server_system_init_phase661 STATIC
  src/original_server_system_init_phase661.cpp)
target_include_directories(sin_original_server_system_init_phase661 PUBLIC include)
target_compile_features(sin_original_server_system_init_phase661 PUBLIC cxx_std_17)
target_compile_options(sin_original_server_system_init_phase661 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_server_system_init_phase661_tests
    tests/test_original_server_system_init_phase661.cpp)
  target_link_libraries(sin_original_server_system_init_phase661_tests PRIVATE
    sin_original_server_system_init_phase661
    sin_original_server_autogamesystem_phase660)
  target_compile_options(sin_original_server_system_init_phase661_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_system_init_phase661_tests
    COMMAND sin_original_server_system_init_phase661_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_server_system_init_phase661_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_server_system_init_phase661.py
        --self-test)
  endif()
endif()

# Phase662: original server auxiliary-object AutoGameSystem virtual Init;
# six 40-byte allocations, constructors and conditional owner stores.
add_library(sin_original_server_object_init_phase662 STATIC
  src/original_server_object_init_phase662.cpp)
target_include_directories(sin_original_server_object_init_phase662 PUBLIC include)
target_compile_features(sin_original_server_object_init_phase662 PUBLIC cxx_std_17)
target_compile_options(sin_original_server_object_init_phase662 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_original_server_object_init_phase662_tests
    tests/test_original_server_object_init_phase662.cpp)
  target_link_libraries(sin_original_server_object_init_phase662_tests PRIVATE
    sin_original_server_object_init_phase662
    sin_original_server_autogamesystem_phase660)
  target_compile_options(sin_original_server_object_init_phase662_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_original_server_object_init_phase662_tests
    COMMAND sin_original_server_object_init_phase662_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_original_server_object_init_phase662_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_server_object_init_phase662.py
        --self-test)
  endif()
endif()

# Phase663: opt-in fixed skill 1/2/3 compatibility policy over the 14
# original SiN gameplay metrics. Not a Source ConVar binding or an original
# game DLL startup claim.
add_library(sin_fixed_difficulty_phase663 STATIC src/fixed_difficulty_phase663.cpp)
target_include_directories(sin_fixed_difficulty_phase663 PUBLIC include)
target_compile_features(sin_fixed_difficulty_phase663 PUBLIC cxx_std_17)
target_compile_options(sin_fixed_difficulty_phase663 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_fixed_difficulty_phase663_tests
    tests/test_fixed_difficulty_phase663.cpp)
  target_link_libraries(sin_fixed_difficulty_phase663_tests PRIVATE
    sin_fixed_difficulty_phase663)
  target_compile_options(sin_fixed_difficulty_phase663_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_fixed_difficulty_phase663_tests
    COMMAND sin_fixed_difficulty_phase663_tests)
endif()

# Phase663: retain compiled original SiN campaign dynamic/secret trigger
# semantics while replacing only the adaptive difficulty manager callback.
add_library(sin_fixed_difficulty_trigger_phase663 STATIC
  src/fixed_difficulty_trigger_phase663.cpp)
target_include_directories(sin_fixed_difficulty_trigger_phase663 PUBLIC include)
target_link_libraries(sin_fixed_difficulty_trigger_phase663 PUBLIC
  sin_fixed_difficulty_phase663 sin_original_gameplay_triggers)
target_compile_features(sin_fixed_difficulty_trigger_phase663 PUBLIC cxx_std_17)
target_compile_options(sin_fixed_difficulty_trigger_phase663 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_fixed_difficulty_trigger_phase663_tests
    tests/test_fixed_difficulty_trigger_phase663.cpp)
  target_link_libraries(sin_fixed_difficulty_trigger_phase663_tests PRIVATE
    sin_fixed_difficulty_trigger_phase663)
  target_compile_options(sin_fixed_difficulty_trigger_phase663_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_fixed_difficulty_trigger_phase663_tests
    COMMAND sin_fixed_difficulty_trigger_phase663_tests)
endif()

# Owner-retail dd_enabled ConVar and guarded adaptive-call evidence.
# Public CTest only uses synthetic invented instruction witnesses.
if(BUILD_TESTING AND Python3_Interpreter_FOUND)
  add_test(NAME sin_fixed_difficulty_phase663_original_gate_synthetic
    COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_sin_fixed_difficulty_gate_phase663.py
      --self-test)
endif()

# Phase664: original 2006 server record->float setter, 14 name pointers and
# PE32 relocation-aware fixed skill publication with strict host validation.
add_library(sin_fixed_metric_bridge_phase664 STATIC
  src/fixed_metric_bridge_phase664.cpp)
target_include_directories(sin_fixed_metric_bridge_phase664 PUBLIC include)
target_link_libraries(sin_fixed_metric_bridge_phase664 PUBLIC
  sin_fixed_difficulty_phase663)
target_compile_features(sin_fixed_metric_bridge_phase664 PUBLIC cxx_std_17)
target_compile_options(sin_fixed_metric_bridge_phase664 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
if(BUILD_TESTING)
  add_executable(sin_fixed_metric_bridge_phase664_tests
    tests/test_fixed_metric_bridge_phase664.cpp)
  target_link_libraries(sin_fixed_metric_bridge_phase664_tests PRIVATE
    sin_fixed_metric_bridge_phase664)
  target_compile_options(sin_fixed_metric_bridge_phase664_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_fixed_metric_bridge_phase664_tests
    COMMAND sin_fixed_metric_bridge_phase664_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_fixed_metric_bridge_phase664_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_fixed_metric_bridge_phase664.py
        --self-test)
  endif()
endif()

# Phase665: original CInfoChallenge map inputs and fixed reporting adapter.
if(BUILD_TESTING)
  add_executable(sin_info_challenge_phase665_tests
    tests/test_info_challenge_phase665.cpp)
  target_link_libraries(sin_info_challenge_phase665_tests PRIVATE
    sin_fixed_difficulty_phase663)
  target_compile_options(sin_info_challenge_phase665_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>)
  add_test(NAME sin_info_challenge_phase665_tests
    COMMAND sin_info_challenge_phase665_tests)
  if(Python3_Interpreter_FOUND)
    add_test(NAME sin_info_challenge_phase665_audit_synthetic
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/research/tools/audit_original_info_challenge_phase665.py
        --self-test)
  endif()
endif()

# Phase843: portable VCD lexer, synthetic CI and optional owner corpus audit.
if(BUILD_TESTING)
  add_executable(sin_vcd_tokenizer_tests tests/test_vcd_tokenizer.cpp)
  target_include_directories(sin_vcd_tokenizer_tests PRIVATE include)
  target_compile_features(sin_vcd_tokenizer_tests PRIVATE cxx_std_17)
  target_compile_options(sin_vcd_tokenizer_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>
    $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>)
  add_test(NAME sin_vcd_tokenizer_tests COMMAND sin_vcd_tokenizer_tests)
endif()

# Phase944: shared indexed red-black deletion regression with Win32 macro guards.
if(BUILD_TESTING)
  add_executable(sin_choreography_indexed_rb_erase_tests tests/test_choreography_indexed_rb_erase.cpp)
  target_compile_features(sin_choreography_indexed_rb_erase_tests PRIVATE cxx_std_17)
  target_compile_options(sin_choreography_indexed_rb_erase_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>
    $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>)
  add_test(NAME sin_choreography_indexed_rb_erase_tests COMMAND sin_choreography_indexed_rb_erase_tests)
endif()

if(BUILD_TESTING)
  add_executable(sin_response_rule_matcher_tests tests/test_response_rule_matcher.cpp)
  target_compile_features(sin_response_rule_matcher_tests PRIVATE cxx_std_17)
  target_compile_options(sin_response_rule_matcher_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>
    $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>)
  add_test(NAME sin_response_rule_matcher_tests COMMAND sin_response_rule_matcher_tests)
endif()

if(BUILD_TESTING)
  add_executable(sin_response_rule_scoring_tests tests/test_response_rule_scoring.cpp)
  target_compile_features(sin_response_rule_scoring_tests PRIVATE cxx_std_17)
  target_compile_options(sin_response_rule_scoring_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>
    $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>)
  add_test(NAME sin_response_rule_scoring_tests COMMAND sin_response_rule_scoring_tests)
endif()

if(BUILD_TESTING)
  add_executable(sin_response_group_choice_tests tests/test_response_group_choice.cpp)
  target_compile_features(sin_response_group_choice_tests PRIVATE cxx_std_17)
  target_compile_options(sin_response_group_choice_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>
    $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>)
  add_test(NAME sin_response_group_choice_tests COMMAND sin_response_group_choice_tests)
endif()

if(BUILD_TESTING)
  add_executable(sin_response_script_parser_tests tests/test_response_script_parser.cpp)
  target_compile_features(sin_response_script_parser_tests PRIVATE cxx_std_17)
  target_compile_options(sin_response_script_parser_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>
    $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>)
  add_test(NAME sin_response_script_parser_tests COMMAND sin_response_script_parser_tests)
endif()

if(BUILD_TESTING)
  add_executable(sin_response_script_registry_tests tests/test_response_script_registry.cpp)
  target_compile_features(sin_response_script_registry_tests PRIVATE cxx_std_17)
  target_compile_options(sin_response_script_registry_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror;-UNDEBUG>
    $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>)
  add_test(NAME sin_response_script_registry_tests COMMAND sin_response_script_registry_tests)
endif()
