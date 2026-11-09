#!/usr/bin/env bash
# Official Android NDK x86_64 build for Phase 71 Bionic emulator execution.
# This does NOT build a Quest/ARM64 binary or integrate SourceVR.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
NDK="${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-}}"
if [[ -z "$NDK" || ! -f "$NDK/build/cmake/android.toolchain.cmake" ]]; then
  echo 'ERROR: ANDROID_NDK_HOME must refer to the official Android NDK.' >&2
  exit 2
fi
if [[ $# -gt 1 ]]; then echo "Usage: $0 [build-dir]" >&2; exit 2; fi
BUILD="${1:-$ROOT/build-android-emulator-x86_64}"
cmake -S "$ROOT" -B "$BUILD" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=x86_64 -DANDROID_PLATFORM=android-29 \
  -DANDROID_STL=c++_static -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF -DSIN_BUILD_ANDROID_SMOKE=ON
cmake --build "$BUILD" --target sin_compat_host_abi sin_android_host_abi_smoke sin_legacy_bridge_abi sin_android_legacy_bridge_smoke sin_source_provider_abi sin_android_source_provider_smoke sin102_provider_lifecycle sin_scene_actor_c_abi sin_android_scene_actor_c_abi_smoke sin_assault_npc_c_abi sin_android_assault_npc_c_abi_smoke sin_android_assault_goal_fanout_tests sin_android_xr_source_tracking_tests sin_sourcevr_probe_server sin_sourcevr_probe_client sin_android_sourcevr_module_probe_tests sin_original_ar_grenade_runtime sin_original_rifle_grenade_launch sin_android_original_rifle_grenade_launch_tests sin_original_ar_grenade_touch sin_android_original_ar_grenade_touch_tests sin_original_gameplay_entity_factory sin_android_original_gameplay_entity_factory_tests sin_original_alive_damage sin_android_original_alive_damage_tests sin_original_grunt_ai sin_android_original_grunt_help_up_task_tests sin_android_original_grunt_relationship_tests --parallel 2
LIB="$BUILD/libsin_compat_host_abi.so"
SMOKE="$BUILD/sin_android_host_abi_smoke"
for f in "$LIB" "$SMOKE"; do test -s "$f" || { echo "ERROR: missing $f" >&2; exit 1; }; done
READELF="${READELF:-readelf}"
for f in "$LIB" "$SMOKE"; do
  header="$($READELF -h "$f")"
  grep -q 'Class:.*ELF64' <<<"$header" || { echo "ERROR: not ELF64: $f" >&2; exit 1; }
  grep -q 'Machine:.*Advanced Micro Devices X86-64' <<<"$header" || { echo "ERROR: not x86_64: $f" >&2; exit 1; }
  grep -q 'Type:.*DYN' <<<"$header" || { echo "ERROR: not shared/PIE: $f" >&2; exit 1; }
done
$READELF -l "$SMOKE" | grep -q '/system/bin/linker64' || { echo 'ERROR: no Android linker64' >&2; exit 1; }
$READELF -d "$SMOKE" | grep -q 'NEEDED.*libdl.so' || { echo 'ERROR: loader not linked to Bionic libdl' >&2; exit 1; }
if $READELF -d "$SMOKE" | grep -q 'NEEDED.*libsin_compat_host_abi.so'; then
  echo 'ERROR: loader bypassed dlopen by linking the module' >&2; exit 1
fi
$READELF -d "$LIB" | grep -q 'SONAME.*libsin_compat_host_abi.so' || {
  echo 'ERROR: library has unexpected SONAME' >&2; exit 1;
}
if $READELF -d "$LIB" | grep -q 'NEEDED.*libc++_shared.so'; then
  echo 'ERROR: expected static libc++' >&2; exit 1
fi
HOST_TAG=linux-x86_64
if [[ "$(uname -s)" == Darwin ]]; then HOST_TAG=darwin-x86_64; fi
NM="$NDK/toolchains/llvm/prebuilt/$HOST_TAG/bin/llvm-nm"
[[ -x "$NM" ]] || { echo 'ERROR: missing NDK llvm-nm' >&2; exit 1; }
EXPECTED="$(mktemp)"; ACTUAL="$(mktemp)"; trap 'rm -f "$EXPECTED" "$ACTUAL"' EXIT
awk '/^[[:space:]]+sin_[A-Za-z0-9_]+;/{gsub(/;/,"",$1); print $1}' "$ROOT/src/host_abi_exports.map" | sort -u > "$EXPECTED"
"$NM" -D --defined-only --format=posix "$LIB" | awk 'NF{print $1}' | sort -u > "$ACTUAL"
diff -u "$EXPECTED" "$ACTUAL" || { echo 'ERROR: exported C ABI symbols changed' >&2; exit 1; }
if [[ "$(wc -l < "$ACTUAL")" -ne 13 ]]; then
  echo 'ERROR: public ABI must contain 13 symbols' >&2; exit 1
fi
printf 'PASS: NDK Android x86_64 API29 library and Bionic dlopen PIE linked and audited.\n'
printf 'Not yet executed: Android emulator job must call scripts/run_android_host_abi_smoke.sh.\n'

# Phase100: independent Bionic x86_64 C API .so and loader for actual
# Android emulator runtime execution. The original weapon v1 ABI is unchanged.
LEGACY="$BUILD/libsin_legacy_bridge_abi.so"
LEGACY_SMOKE="$BUILD/sin_android_legacy_bridge_smoke"
for f in "$LEGACY" "$LEGACY_SMOKE"; do
  test -s "$f" || { echo "ERROR: missing Phase100 emulator artifact: $f" >&2; exit 1; }
  "$READELF" -h "$f" | grep -q 'Machine:.*Advanced Micro Devices X86-64' || {
    echo "ERROR: Phase100 is not Android emulator x86_64: $f" >&2; exit 1; }
done
"$READELF" -d "$LEGACY_SMOKE" | grep -q 'NEEDED.*libdl.so' || {
  echo 'ERROR: Phase100 loader missing Bionic libdl' >&2; exit 1; }
if "$READELF" -d "$LEGACY_SMOKE" | grep -q 'NEEDED.*libsin_legacy_bridge_abi.so'; then
  echo 'ERROR: Phase100 Android test links directly rather than dlopen' >&2; exit 1
fi
LEGACY_EXPECTED="$(mktemp)"
LEGACY_ACTUAL="$(mktemp)"
trap 'rm -f "$EXPECTED" "$ACTUAL" "$LEGACY_EXPECTED" "$LEGACY_ACTUAL"' EXIT
sed -n '/^[[:space:]]*sin100_[a-z_]*;/s/[[:space:];]//gp' \
  "$ROOT/src/legacy_bridge_exports.map" | sort -u > "$LEGACY_EXPECTED"
"$NM" -D --defined-only --format=posix "$LEGACY" |
  awk 'NF{ sub(/@@.*/, "", $1); print $1 }' | sort -u > "$LEGACY_ACTUAL"
diff -u "$LEGACY_EXPECTED" "$LEGACY_ACTUAL" || {
  echo 'ERROR: Phase100 exported C ABI changed' >&2; exit 1; }
test "$(wc -l < "$LEGACY_ACTUAL")" -eq 11 || {
  echo 'ERROR: Phase100 must have 11 public exports' >&2; exit 1; }
printf 'PASS: Phase100 Bionic emulator shared module and dlopen runner built with 11 stable exports.\n'

# Phase101: verify a SECOND independent Source-provider callback shared library.
# The 11 Phase100 and 13 original host exports remain unchanged.
PROVIDER="$BUILD/libsin_source_provider_abi.so"
PROVIDER_SMOKE="$BUILD/sin_android_source_provider_smoke"
for path in "$PROVIDER" "$PROVIDER_SMOKE"; do
  test -s "$path" || { echo "ERROR: Phase101 Android output missing: $path" >&2; exit 1; }
done
EXPECTED_101="$(mktemp)"
ACTUAL_101="$(mktemp)"
trap 'rm -f "$EXPECTED" "$ACTUAL" "$LEGACY_EXPECTED" "$LEGACY_ACTUAL" "$EXPECTED_101" "$ACTUAL_101"' EXIT
sed -n '/^[[:space:]]*sin101_[a-z_]*;/s/[[:space:];]//gp' \
  "$ROOT/src/source_provider_exports.map" | sort -u > "$EXPECTED_101"
"$NM" -D --defined-only --format=posix "$PROVIDER" |
  awk '$1 ~ /^sin101_/ { sub(/@@.*/, "", $1); print $1 }' | sort -u > "$ACTUAL_101"
diff -u "$EXPECTED_101" "$ACTUAL_101" || {
  echo "ERROR: Phase101 12-function ABI drift" >&2; exit 1; }
test "$(wc -l < "$ACTUAL_101")" -eq 12 || {
  echo 'ERROR: Phase101 provider must export exactly 12 C methods' >&2; exit 1; }
"$READELF" -d "$PROVIDER_SMOKE" | grep -q 'NEEDED.*libsin_source_provider_abi.so' || {
  echo 'ERROR: Phase101 smoke must load the new shared library' >&2; exit 1; }
if "$READELF" -d "$PROVIDER" | grep -q 'NEEDED.*libsin_legacy_bridge_abi.so'; then
  echo 'ERROR: Phase101 must resolve Phase100 dynamically, not via direct DT_NEEDED' >&2; exit 1
fi
echo 'PASS: Phase101 provider shared library and Android test executable, 12 exports verified.'

# Phase102: verify compiled native provider-epoch lifecycle regression runner.
SIN102_RUNNER="$BUILD/sin102_provider_lifecycle"
test -s "$SIN102_RUNNER" || { echo "ERROR: missing Phase102 Android lifecycle test" >&2; exit 1; }
"$READELF" -h "$SIN102_RUNNER" | grep -q 'Machine:.*Advanced Micro Devices X86-64' || {
  echo "ERROR: Phase102 runner has wrong target architecture" >&2; exit 1; }
"$READELF" -d "$SIN102_RUNNER" | grep -q 'NEEDED.*libsin_source_provider_abi.so' || {
  echo "ERROR: Phase102 runner must link the separate provider ABI library" >&2; exit 1; }
echo 'PASS: Phase102 provider-epoch test executable built for Android.'


# Round19: verify the independent scene actor ABI builds for Android, not SourceVR.
SCENE="$BUILD/libsin_scene_actor_c_abi.so"
SCENE_SMOKE="$BUILD/sin_android_scene_actor_c_abi_smoke"
for f in "$SCENE" "$SCENE_SMOKE"; do
  test -s "$f" || { echo "ERROR: missing Round19 Android output: $f" >&2; exit 1; }
done
"$READELF" -h "$SCENE" | grep -q 'Machine:.*Advanced Micro Devices X86-64' || {
  echo "ERROR: scene actor module built for wrong ABI" >&2; exit 1; }
"$READELF" -d "$SCENE" | grep -q 'SONAME.*libsin_scene_actor_c_abi.so' || {
  echo "ERROR: scene actor module missing proper SONAME" >&2; exit 1; }
"$READELF" -h "$SCENE_SMOKE" | grep -q 'Machine:.*Advanced Micro Devices X86-64' || {
  echo "ERROR: scene actor C11 dlopen smoke target wrong ABI" >&2; exit 1; }
SCENE_EXPECTED="$(mktemp)"
SCENE_ACTUAL="$(mktemp)"
awk '/^[[:space:]]+sin_scene_[A-Za-z0-9_]+;/{gsub(/;/,"",$1); print $1}' \
  "$ROOT/src/scene_actor_exports.map" | sort -u > "$SCENE_EXPECTED"
"$NM" -D --defined-only --format=posix "$SCENE" |
  awk 'NF{sub(/@@.*/,"",$1); print $1}' | sort -u > "$SCENE_ACTUAL"
diff -u "$SCENE_EXPECTED" "$SCENE_ACTUAL" || {
  echo "ERROR: Round19 scene actor public ABI changed" >&2; exit 1; }
test "$(wc -l < "$SCENE_ACTUAL")" -eq 8 || {
  echo "ERROR: scene actor module must export exactly eight functions" >&2; exit 1; }
rm -f "$SCENE_EXPECTED" "$SCENE_ACTUAL"
echo "PASS: Round19 scene actor library and C11 dlopen test built for Android, eight exports verified."

# Engine-neutral assault NPC module and C11 dlopen runner for Bionic x86_64.
ASSAULT_NPC="$BUILD/libsin_assault_npc_c_abi.so"
ASSAULT_NPC_SMOKE="$BUILD/sin_android_assault_npc_c_abi_smoke"
for f in "$ASSAULT_NPC" "$ASSAULT_NPC_SMOKE"; do
  test -s "$f" || { echo "ERROR: missing assault NPC Bionic artifact $f" >&2; exit 1; }
  "$READELF" -h "$f" | grep -q 'Machine:.*Advanced Micro Devices X86-64' || {
    echo "ERROR: assault NPC artifact is not emulator x86_64: $f" >&2; exit 1; }
done
"$READELF" -d "$ASSAULT_NPC" | grep -q 'SONAME.*libsin_assault_npc_c_abi.so' || {
  echo 'ERROR: assault NPC SONAME mismatch' >&2; exit 1; }
"$READELF" -d "$ASSAULT_NPC_SMOKE" | grep -q 'NEEDED.*libdl.so' || {
  echo 'ERROR: assault NPC Bionic smoke missing libdl' >&2; exit 1; }
if "$READELF" -d "$ASSAULT_NPC_SMOKE" | grep -q 'NEEDED.*libsin_assault_npc_c_abi.so'; then
  echo 'ERROR: assault NPC smoke bypassed dlopen' >&2; exit 1
fi
NPC_EXPECTED="$(mktemp)"
NPC_ACTUAL="$(mktemp)"
awk '/^[[:space:]]+sin_assault_[A-Za-z0-9_]+;/{gsub(/;/,"",$1); print $1}' \
  "$ROOT/src/assault_npc_exports.map" | sort -u > "$NPC_EXPECTED"
"$NM" -D --defined-only --format=posix "$ASSAULT_NPC" |
  awk 'NF{sub(/@@.*/,"",$1); print $1}' | sort -u > "$NPC_ACTUAL"
diff -u "$NPC_EXPECTED" "$NPC_ACTUAL" || {
  echo 'ERROR: assault NPC Bionic exports mismatch' >&2; exit 1; }
test "$(wc -l < "$NPC_ACTUAL")" -eq 8 || {
  echo 'ERROR: assault NPC Bionic must export exactly eight C methods' >&2; exit 1; }
rm -f "$NPC_EXPECTED" "$NPC_ACTUAL"
echo 'PASS: assault NPC x86_64 Bionic shared module and fake-host dlopen smoke built; eight exports verified.'

# Multi-NPC goal BeginAssault fixture uses the actual shared C ABI, with
# fake host NPCs only. The CI emulator step will execute this Bionic binary.
GOAL_FANOUT="$BUILD/sin_android_assault_goal_fanout_tests"
test -s "$GOAL_FANOUT" || { echo 'ERROR: missing goal fanout Bionic executable' >&2; exit 1; }
"$READELF" -h "$GOAL_FANOUT" | grep -q 'Machine:.*Advanced Micro Devices X86-64' || {
  echo 'ERROR: goal fanout fixture is not Android x86_64' >&2; exit 1; }
"$READELF" -d "$GOAL_FANOUT" | grep -q 'NEEDED.*libsin_assault_npc_c_abi.so' || {
  echo 'ERROR: goal fanout executable not linked to assault C ABI shared library' >&2; exit 1; }
echo 'PASS: multi-NPC ai_goal_assault fanout Bionic test binary linked to shared assault module.'

# Portable stereo/pose math regression built for Android x86_64 Bionic.
XR_TRACKING="$BUILD/sin_android_xr_source_tracking_tests"
test -s "$XR_TRACKING" || { echo 'ERROR: missing Android XR pose regression executable' >&2; exit 1; }
"$READELF" -h "$XR_TRACKING" | grep -q 'Machine:.*Advanced Micro Devices X86-64' || {
  echo 'ERROR: XR pose regression has wrong emulator architecture' >&2; exit 1; }
echo 'PASS: XR to Source coordinate math executable compiled for Android x86_64 Bionic.'

# Real Android Bionic x86_64 build of the two inert, game-NAMED .so probes.
# The emulator will independently dlopen these through a simulated module
# resolver. SourceVR engine-specific module loading is NOT executed here.
PROBE_ROOT="$BUILD/sourcevr-probe"
PROBE_SMOKE="$BUILD/sin_android_sourcevr_module_probe_tests"
for role in server client; do
  so="$PROBE_ROOT/lib${role}.so"
  test -s "$so" || { echo "ERROR: missing Bionic module $so" >&2; exit 1; }
  "$READELF" -h "$so" | grep -q 'Machine:.*Advanced Micro Devices X86-64' || {
    echo "ERROR: wrong emulator module architecture: $so" >&2; exit 1; }
  "$READELF" -d "$so" | grep -q "SONAME.*lib${role}.so" || {
    echo "ERROR: wrong emulator SONAME: $so" >&2; exit 1; }
  actual="$(mktemp)"
  "$NM" -D --defined-only --format=posix "$so" |
    awk 'NF && $2 != "A" {sub(/@@.*/,"",$1); print $1}' | sort -u > "$actual"
  test "$(wc -l < "$actual")" -eq 1 && grep -Fxq CreateInterface "$actual" || {
    cat "$actual" >&2
    echo "ERROR: Bionic probe must export ONLY CreateInterface" >&2
    rm -f "$actual"
    exit 1
  }
  rm -f "$actual"
done
test -s "$PROBE_SMOKE" || { echo 'ERROR: missing emulator probe test' >&2; exit 1; }
"$READELF" -d "$PROBE_SMOKE" | grep -q 'NEEDED.*libdl.so' || {
  echo 'ERROR: probe smoke must link Bionic libdl' >&2; exit 1; }
for role in server client; do
  if "$READELF" -d "$PROBE_SMOKE" | grep -q "NEEDED.*lib${role}.so"; then
    echo 'ERROR: Bionic probe smoke must dlopen modules, not link them' >&2; exit 1
  fi
done
echo 'PASS: Bionic x86_64 game-named libserver/libclient module probes built and exports audited.'

# Phase134: compile the same portable grunt task host-contract for Android Bionic.
GRUNT_AI="$BUILD/libsin_original_grunt_ai.a"
GRUNT_HELP_UP_TEST="$BUILD/sin_android_original_grunt_help_up_task_tests"
test -s "$GRUNT_AI" || { echo "ERROR: missing emulator grunt AI library: $GRUNT_AI" >&2; exit 1; }
test -s "$GRUNT_HELP_UP_TEST" || { echo "ERROR: missing emulator grunt help-up test: $GRUNT_HELP_UP_TEST" >&2; exit 1; }
"$READELF" -h "$GRUNT_HELP_UP_TEST" | grep -q 'Machine:.*Advanced Micro Devices X86-64' || {
  echo 'ERROR: grunt help-up regression executable is not Android x86_64' >&2; exit 1; }
echo 'PASS: original grunt TASK_GRUNT_HELP_UP library and Bionic x86_64 regression executable built.'


# Phase 135: verify the portable relationship regression is Android x86_64.
GRUNT_RELATIONSHIP_TEST="$BUILD/sin_android_original_grunt_relationship_tests"
test -s "$GRUNT_RELATIONSHIP_TEST" || {
  echo "ERROR: missing emulator grunt relationship regression: $GRUNT_RELATIONSHIP_TEST" >&2
  exit 1
}
"$READELF" -h "$GRUNT_RELATIONSHIP_TEST" | grep -q 'Machine:.*Advanced Micro Devices X86-64' || {
  echo 'ERROR: grunt relationship regression is not Android x86_64' >&2; exit 1;
}
echo 'PASS: original grunt hostility-mode relationship model built for Android Bionic x86_64.'
