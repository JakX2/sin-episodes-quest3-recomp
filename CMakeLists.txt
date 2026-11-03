cmake_minimum_required(VERSION 3.16)
project(sin_episodes_quest3_compat VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_library(sin_weapon_runtime STATIC src/weapon_runtime.cpp src/weapon_entity_io.cpp src/weapon_viewmodel_io.cpp src/weapon_viewmodel_lifecycle.cpp src/weapon_session.cpp)
target_include_directories(sin_weapon_runtime PUBLIC include third_party/research)
target_compile_options(sin_weapon_runtime PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
include(CTest)
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
