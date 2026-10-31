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
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build "$BUILD" --target sin_compat_host_abi --parallel 2
LIB="$BUILD/libsin_compat_host_abi.so"
if [[ ! -s "$LIB" ]]; then
  echo "ERROR: Android shared-library output missing: $LIB" >&2
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
