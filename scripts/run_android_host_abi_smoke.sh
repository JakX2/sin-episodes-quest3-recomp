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
ABI="$("$ADB" shell getprop ro.product.cpu.abi | tr -d '\r')"
if [[ "$ABI" != 'arm64-v8a' ]]; then
  echo "ERROR: target ABI is '$ABI', expected arm64-v8a" >&2
  exit 2
fi
API="$("$ADB" shell getprop ro.build.version.sdk | tr -d '\r')"
if [[ ! "$API" =~ ^[0-9]+$ || "$API" -lt 26 ]]; then
  echo "ERROR: target Android API is '$API', expected >=26" >&2
  exit 2
fi
"$ADB" shell "mkdir -p '$REMOTE'"
"$ADB" push "$SO" "$REMOTE/libsin_compat_host_abi.so" >/dev/null
"$ADB" push "$EXE" "$REMOTE/sin_android_host_abi_smoke" >/dev/null
"$ADB" shell "chmod 700 '$REMOTE/sin_android_host_abi_smoke' && cd '$REMOTE' && ./sin_android_host_abi_smoke ./libsin_compat_host_abi.so"
echo 'PASS: on-device Android shell dlopen of portable SiN compatibility module (not in SourceVR).'
