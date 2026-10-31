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
cmake --build "$BUILD" --target sin_compat_host_abi sin_android_host_abi_smoke --parallel 2
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
