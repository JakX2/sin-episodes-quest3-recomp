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
