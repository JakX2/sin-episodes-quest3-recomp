#!/usr/bin/env bash
# Build and inspect the independently authored C++ ABI library with the real
# Google Android NDK. This does not produce an APK or SourceVR plugin.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
NDK="${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-}}"
if [[ -z "$NDK" || ! -f "$NDK/build/cmake/android.toolchain.cmake" ]]; then
  echo 'ERROR: ANDROID_NDK_HOME must point to an installed Android NDK with its CMake toolchain.' >&2
  exit 2
fi
if [[ $# -gt 1 ]]; then
  echo "Usage: $0 [absolute-or-relative-build-directory]" >&2
  exit 2
fi
BUILD="${1:-$ROOT/build-android-arm64}"
cmake -S "$ROOT" -B "$BUILD" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-26 \
  -DANDROID_STL=c++_static \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DSIN_BUILD_ANDROID_SMOKE=ON
cmake --build "$BUILD" --target sin_compat_host_abi sin_android_host_abi_smoke sin_legacy_bridge_abi sin_android_legacy_bridge_smoke sin_source_provider_abi sin_android_source_provider_smoke sin102_provider_lifecycle sin_scene_actor_c_abi sin_android_scene_actor_c_abi_smoke sin_assault_npc_c_abi sin_android_assault_npc_c_abi_smoke --parallel 2
LIB="$BUILD/libsin_compat_host_abi.so"
SMOKE="$BUILD/sin_android_host_abi_smoke"
if [[ ! -s "$LIB" ]]; then
  echo "ERROR: Android shared-library output missing: $LIB" >&2
  exit 1
fi

# The C11 loader is also a genuine Android Bionic-linked executable, not a
# desktop binary or a freestanding ABI contract. It is *not executed* in CI.
if [[ ! -s "$SMOKE" ]]; then
  echo "ERROR: Android dlopen smoke executable missing: $SMOKE" >&2
  exit 1
fi
# Use binutils from the host for readelf (understands ELF64 AArch64), and the
# NDK's architecture-agnostic llvm-nm for dynamic symbol inspection.
HOST_TAG=linux-x86_64
if [[ "$(uname -s)" == Darwin ]]; then HOST_TAG=darwin-x86_64; fi
NM="$NDK/toolchains/llvm/prebuilt/$HOST_TAG/bin/llvm-nm"
CC="$NDK/toolchains/llvm/prebuilt/$HOST_TAG/bin/aarch64-linux-android26-clang"
if [[ ! -x "$NM" || ! -x "$CC" ]]; then
  echo 'ERROR: NDK LLVM tools missing for this host/API level.' >&2
  exit 1
fi
READELF="${READELF:-readelf}"
ELF="$($READELF -h "$LIB")"
grep -q 'Class:.*ELF64' <<< "$ELF" || { echo 'ERROR: not ELF64' >&2; exit 1; }
grep -q 'Machine:.*AArch64' <<< "$ELF" || { echo 'ERROR: not AArch64' >&2; exit 1; }
grep -q 'Type:.*DYN' <<< "$ELF" || { echo 'ERROR: not a shared ELF object' >&2; exit 1; }
SMOKE_ELF="$($READELF -h "$SMOKE")"
grep -q 'Class:.*ELF64' <<< "$SMOKE_ELF" || { echo 'ERROR: smoke binary not ELF64' >&2; exit 1; }
grep -q 'Machine:.*AArch64' <<< "$SMOKE_ELF" || { echo 'ERROR: smoke binary not AArch64' >&2; exit 1; }
grep -q 'Type:.*DYN' <<< "$SMOKE_ELF" || { echo 'ERROR: smoke binary not PIE executable' >&2; exit 1; }
SMOKE_DYNAMIC="$($READELF -d "$SMOKE")"
grep -q 'NEEDED.*libc.so' <<< "$SMOKE_DYNAMIC" || { echo 'ERROR: smoke binary not linked to Android libc' >&2; exit 1; }
grep -q 'NEEDED.*libdl.so' <<< "$SMOKE_DYNAMIC" || { echo 'ERROR: smoke binary missing Android libdl' >&2; exit 1; }
if grep -q 'NEEDED.*libsin_compat_host_abi.so' <<< "$SMOKE_DYNAMIC"; then
  echo 'ERROR: smoke loader linked the library, it must use dlopen at runtime' >&2
  exit 1
fi
SONAME="$($READELF -d "$LIB")"
grep -q 'SONAME.*libsin_compat_host_abi.so' <<< "$SONAME" || {
  echo 'ERROR: unexpected shared-library SONAME' >&2
  exit 1
}
if grep -q 'NEEDED.*libc++_shared.so' <<< "$SONAME"; then
  echo 'ERROR: unexpected libc++_shared.so dependency; expected c++_static.' >&2
  exit 1
fi
# Check exports against the fixed public C ABI v1 surface. Internal C++
# implementation symbols must not become part of the Android library ABI.
EXPECTED="$(mktemp)"
ACTUAL="$(mktemp)"
CONTRACT="$(mktemp --suffix=.o)"
trap 'rm -f "$EXPECTED" "$ACTUAL" "$CONTRACT"' EXIT
cat > "$EXPECTED" <<'NAMES'
sin_abi_version_v1
sin_session_cleanup_v1
sin_session_create_v1
sin_session_destroy_v1
sin_session_ensure_viewmodel_v1
sin_session_get_ammo_v1
sin_session_hide_weapon_v1
sin_session_holster_v1
sin_session_reload_begin_v1
sin_session_reload_end_v1
sin_session_reload_event_v1
sin_session_set_ammo_v1
sin_session_usercmd_v1
NAMES
"$NM" -D --defined-only --format=posix "$LIB" | awk 'NF { print $1 }' | sort -u > "$ACTUAL"
if ! diff -u "$EXPECTED" "$ACTUAL"; then
  echo 'ERROR: unexpected/missing Android public symbol(s)' >&2
  exit 1
fi
"$CC" -std=c11 -Wall -Wextra -Werror -pedantic -I"$ROOT/include" \
  -c "$ROOT/tests/abi_layout_contract.c" -o "$CONTRACT"
$READELF -h "$CONTRACT" | grep -q 'Machine:.*AArch64' || {
  echo 'ERROR: actual NDK contract translation unit is not AArch64' >&2
  exit 1
}
printf 'Android NDK arm64-v8a API 26 C++ shared library verified: %s\n' "$LIB"
printf '%s\n' 'ELF64 AArch64, shared SONAME, 13 C ABI exports, and native NDK C ABI layout verified.' \
  'This is NOT an in-SourceVR load test or Quest APK.'

# Record exact provenance of *both* files so the GitHub artifact can be
# checked/deployed independently; do not include any private game assets.
{
  printf 'build_target=arm64-v8a\nandroid_api=26\n'
  grep '^Pkg.Revision' "$NDK/source.properties" | sed 's/ *= */=/' | tr -d ' '
  sha256sum "$LIB" "$SMOKE" | sed "s#$BUILD/##"
} > "$BUILD/android-smoke-manifest.txt"
printf 'Android dlopen executable verified (compiled only, not executed): %s\n' "$SMOKE"

# Phase100: separately versioned keyboard/sound prebridge. Do NOT conflate
# its 11 symbols with the original 13-export weapon-session ABI.
LEGACY="$BUILD/libsin_legacy_bridge_abi.so"
LEGACY_SMOKE="$BUILD/sin_android_legacy_bridge_smoke"
for f in "$LEGACY" "$LEGACY_SMOKE"; do
  test -s "$f" || { echo "ERROR: missing Phase100 ARM64 artifact $f" >&2; exit 1; }
  "$READELF" -h "$f" | grep -q 'Machine:.*AArch64' || {
    echo "ERROR: Phase100 output is not AArch64: $f" >&2; exit 1;
  }
done
"$READELF" -d "$LEGACY" | grep -q 'SONAME.*libsin_legacy_bridge_abi.so' || {
  echo 'ERROR: Phase100 legacy bridge missing expected SONAME' >&2; exit 1; }
"$READELF" -d "$LEGACY_SMOKE" | grep -q 'NEEDED.*libdl.so' || {
  echo 'ERROR: Phase100 Android loader missing libdl' >&2; exit 1; }
if "$READELF" -d "$LEGACY_SMOKE" | grep -q 'NEEDED.*libsin_legacy_bridge_abi.so'; then
  echo 'ERROR: Phase100 loader is linked to module instead of using dlopen' >&2; exit 1
fi
LEGACY_EXPECTED="$(mktemp)"
LEGACY_ACTUAL="$(mktemp)"
trap 'rm -f "$EXPECTED" "$ACTUAL" "$CONTRACT" "$LEGACY_EXPECTED" "$LEGACY_ACTUAL"' EXIT
sed -n '/^[[:space:]]*sin100_[a-z_]*;/s/[[:space:];]//gp' \
  "$ROOT/src/legacy_bridge_exports.map" | sort -u > "$LEGACY_EXPECTED"
"$NM" -D --defined-only --format=posix "$LEGACY" |
  awk 'NF{ sub(/@@.*/, "", $1); print $1 }' | sort -u > "$LEGACY_ACTUAL"
diff -u "$LEGACY_EXPECTED" "$LEGACY_ACTUAL" || {
  echo 'ERROR: Phase100 ARM64 module exports changed' >&2; exit 1; }
test "$(wc -l < "$LEGACY_ACTUAL")" -eq 11 || {
  echo 'ERROR: Phase100 C ABI must export 11 symbols' >&2; exit 1; }
{
  printf 'phase100_scope=standalone_prebridge_not_sourcevr\n'
  sha256sum "$LEGACY" "$LEGACY_SMOKE" | sed "s#$BUILD/##"
} >> "$BUILD/android-smoke-manifest.txt"
echo 'PASS: Phase100 ARM64 shared module and Bionic dlopen executable built, eleven exports verified.'

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
"$READELF" -h "$SIN102_RUNNER" | grep -q 'Machine:.*AArch64' || {
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
"$READELF" -h "$SCENE" | grep -q 'Machine:.*AArch64' || {
  echo "ERROR: scene actor module built for wrong ABI" >&2; exit 1; }
"$READELF" -d "$SCENE" | grep -q 'SONAME.*libsin_scene_actor_c_abi.so' || {
  echo "ERROR: scene actor module missing proper SONAME" >&2; exit 1; }
"$READELF" -h "$SCENE_SMOKE" | grep -q 'Machine:.*AArch64' || {
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

# Host-backed assault NPC C ABI: independent module, NOT a SourceVR NPC.
ASSAULT_NPC="$BUILD/libsin_assault_npc_c_abi.so"
ASSAULT_NPC_SMOKE="$BUILD/sin_android_assault_npc_c_abi_smoke"
for f in "$ASSAULT_NPC" "$ASSAULT_NPC_SMOKE"; do
  test -s "$f" || { echo "ERROR: assault NPC ARM64 artifact missing: $f" >&2; exit 1; }
  "$READELF" -h "$f" | grep -q 'Machine:.*AArch64' || {
    echo "ERROR: assault NPC artifact has wrong architecture: $f" >&2; exit 1; }
done
"$READELF" -d "$ASSAULT_NPC" | grep -q 'SONAME.*libsin_assault_npc_c_abi.so' || {
  echo 'ERROR: assault NPC module has wrong SONAME' >&2; exit 1; }
"$READELF" -d "$ASSAULT_NPC_SMOKE" | grep -q 'NEEDED.*libdl.so' || {
  echo 'ERROR: assault NPC ARM64 smoke does not link Android libdl' >&2; exit 1; }
if "$READELF" -d "$ASSAULT_NPC_SMOKE" | grep -q 'NEEDED.*libsin_assault_npc_c_abi.so'; then
  echo 'ERROR: assault NPC smoke must dlopen module, not link to it' >&2; exit 1
fi
NPC_EXPECTED="$(mktemp)"
NPC_ACTUAL="$(mktemp)"
awk '/^[[:space:]]+sin_assault_[A-Za-z0-9_]+;/{gsub(/;/,"",$1); print $1}' \
  "$ROOT/src/assault_npc_exports.map" | sort -u > "$NPC_EXPECTED"
"$NM" -D --defined-only --format=posix "$ASSAULT_NPC" |
  awk 'NF{sub(/@@.*/,"",$1); print $1}' | sort -u > "$NPC_ACTUAL"
diff -u "$NPC_EXPECTED" "$NPC_ACTUAL" || {
  echo 'ERROR: assault NPC ABI exports drifted' >&2; exit 1; }
test "$(wc -l < "$NPC_ACTUAL")" -eq 8 || {
  echo 'ERROR: assault NPC ABI must export eight functions' >&2; exit 1; }
rm -f "$NPC_EXPECTED" "$NPC_ACTUAL"
{
  printf 'assault_npc_scope=mock_engine_callbacks_not_sourcevr\n'
  sha256sum "$ASSAULT_NPC" "$ASSAULT_NPC_SMOKE" | sed "s#$BUILD/##"
} >> "$BUILD/android-smoke-manifest.txt"
echo 'PASS: assault NPC standalone ARM64 callback module, C11 dlopen smoke and eight exports built; no engine integration.'
