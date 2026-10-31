#!/usr/bin/env bash
# Run the NDK-built standalone loader under the Android shell on a user-owned
# ARM64 Android device. This is not a SourceVR/SiN in-engine test or APK.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${1:-$ROOT/build-android-arm64}"
SO="$BUILD/libsin_compat_host_abi.so"
EXE="$BUILD/sin_android_host_abi_smoke"
REMOTE='/data/local/tmp/sin_compat_api26_smoke'
for f in "$SO" "$EXE"; do
  if [[ ! -s "$f" ]]; then
    echo "ERROR: missing build artifact: $f" >&2
    exit 2
  fi
done
ADB="${ADB:-adb}"
command -v "$ADB" >/dev/null || { echo 'ERROR: install adb/platform-tools first.' >&2; exit 2; }
# ARM64 is the real-device default. An explicit x86_64 override is supported
# only for the Phase 71 CI emulator runtime test, not for Quest hardware.
EXPECTED_ABI="${SIN_SMOKE_EXPECTED_ABI:-arm64-v8a}"
case "$EXPECTED_ABI" in
  arm64-v8a|x86_64) ;;
  *) echo "ERROR: unsupported expected Android ABI '$EXPECTED_ABI'" >&2; exit 2;;
esac
ABI="$("$ADB" shell getprop ro.product.cpu.abi | tr -d '\r')"
if [[ "$ABI" != "$EXPECTED_ABI" ]]; then
  echo "ERROR: target ABI is '$ABI', expected $EXPECTED_ABI" >&2
  exit 2
fi
API="$("$ADB" shell getprop ro.build.version.sdk | tr -d '\r')"
if [[ ! "$API" =~ ^[0-9]+$ || "$API" -lt 26 ]]; then
  echo "ERROR: target Android API is '$API', expected >=26" >&2
  exit 2
fi
# A non-emulator mode is an operational guard, not proof of Quest hardware identity.
REQUIRE_PHYSICAL="${SIN_SMOKE_REQUIRE_PHYSICAL:-0}"
case "$REQUIRE_PHYSICAL" in
  0|1) ;;
  *) echo 'ERROR: SIN_SMOKE_REQUIRE_PHYSICAL must be 0 or 1' >&2; exit 2;;
esac
QEMU="$("$ADB" shell getprop ro.kernel.qemu | tr -d '\r')"
if [[ "$REQUIRE_PHYSICAL" == 1 && "$QEMU" == 1 ]]; then
  echo 'ERROR: hardware handoff rejected Android emulator (ro.kernel.qemu=1)' >&2
  exit 2
fi
if [[ -n "${SIN_SMOKE_REPORT_PATH:-}" && -e "$SIN_SMOKE_REPORT_PATH" ]]; then
  echo 'ERROR: evidence report exists; refusing to overwrite it' >&2; exit 2
fi
HOST_LIB_SHA="$(sha256sum "$SO" | awk '{print $1}')"
HOST_EXE_SHA="$(sha256sum "$EXE" | awk '{print $1}')"
"$ADB" shell "mkdir -p '$REMOTE'"
"$ADB" push "$SO" "$REMOTE/libsin_compat_host_abi.so" >/dev/null
"$ADB" push "$EXE" "$REMOTE/sin_android_host_abi_smoke" >/dev/null
# Confirm Android has the *same bytes* before execution.
REMOTE_HASHES="$("$ADB" shell "sha256sum '$REMOTE/libsin_compat_host_abi.so' '$REMOTE/sin_android_host_abi_smoke'" | tr -d '\r')"
mapfile -t HASH_LINES <<< "$REMOTE_HASHES"
if [[ ${#HASH_LINES[@]} -ne 2 ]]; then
  echo 'ERROR: Android did not return two SHA-256 hashes' >&2; exit 1
fi
REMOTE_LIB_SHA="$(awk '{print $1}' <<< "${HASH_LINES[0]}")"
REMOTE_EXE_SHA="$(awk '{print $1}' <<< "${HASH_LINES[1]}")"
if [[ "$REMOTE_LIB_SHA" != "$HOST_LIB_SHA" || "$REMOTE_EXE_SHA" != "$HOST_EXE_SHA" ]]; then
  echo 'ERROR: transferred Android binary SHA-256 mismatch' >&2; exit 1
fi
SMOKE_OUTPUT="$("$ADB" shell "chmod 700 '$REMOTE/sin_android_host_abi_smoke' && cd '$REMOTE' && ./sin_android_host_abi_smoke ./libsin_compat_host_abi.so" | tr -d '\r')"
printf '%s\n' "$SMOKE_OUTPUT"
if ! grep -Fxq 'C ABI dlopen v1 smoke test passed' <<< "$SMOKE_OUTPUT"; then
  echo 'ERROR: missing exact native loader pass marker' >&2; exit 1
fi
if [[ -n "${SIN_SMOKE_REPORT_PATH:-}" ]]; then
  mkdir -p "$(dirname "$SIN_SMOKE_REPORT_PATH")"
  {
    printf 'schema=sin_compat_hardware_smoke_v1\nresult=PASS\nruntime=android_shell_dlopen\n'
    printf 'abi=%s\nandroid_api=%s\nqemu_property=%s\n' "$ABI" "$API" "$QEMU"
    printf 'require_physical=%s\n' "$REQUIRE_PHYSICAL"
    printf 'library_sha256=%s\nloader_sha256=%s\n' "$HOST_LIB_SHA" "$HOST_EXE_SHA"
    printf 'remote_sha256_verified=true\nloader_marker_verified=true\n'
    printf 'scope=standalone_native_abi_not_sourcevr\n'
  } > "$SIN_SMOKE_REPORT_PATH"
  echo "Evidence report written: $SIN_SMOKE_REPORT_PATH"
fi
echo 'PASS: on-device Android shell dlopen of portable SiN compatibility module (not in SourceVR).'
