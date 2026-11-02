#!/usr/bin/env bash
# Bionic execution of event-bridge dlopen with fabricated host, not SourceVR.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${1:-$ROOT/build-android-emulator-x86_64}"
HOST_LIB="$BUILD/libsin_compat_host_abi.so"
BRIDGE="$BUILD/libsin_compat_event_bridge.so"
EXE="$BUILD/sin_android_event_bridge_smoke"
REMOTE='/data/local/tmp/sin_compat_api26_smoke'
ADB="${ADB:-adb}"
command -v "$ADB" >/dev/null || { echo 'ERROR: missing adb' >&2; exit 2; }
for file in "$HOST_LIB" "$BRIDGE" "$EXE"; do
  test -s "$file" || { echo "ERROR: missing $file" >&2; exit 2; }
done
EXPECTED_ABI="${SIN_SMOKE_EXPECTED_ABI:-x86_64}"
case "$EXPECTED_ABI" in arm64-v8a|x86_64) ;; *)
  echo "ERROR: unsupported expected ABI $EXPECTED_ABI" >&2; exit 2 ;;
esac
ABI="$("$ADB" shell getprop ro.product.cpu.abi | tr -d '\r')"
[[ "$ABI" == "$EXPECTED_ABI" ]] || { echo "ERROR: device ABI $ABI != $EXPECTED_ABI" >&2; exit 2; }
API="$("$ADB" shell getprop ro.build.version.sdk | tr -d '\r')"
[[ "$API" =~ ^[0-9]+$ && "$API" -ge 26 ]] || { echo "ERROR: Android API $API < 26" >&2; exit 2; }
QEMU="$("$ADB" shell getprop ro.kernel.qemu | tr -d '\r')"
REPORT="${SIN_BRIDGE_SMOKE_REPORT_PATH:-}"
[[ -z "$REPORT" || ! -e "$REPORT" ]] || {
  echo "ERROR: report exists, refusing overwrite: $REPORT" >&2; exit 2;
}
LOCAL_HOST="$(sha256sum "$HOST_LIB" | awk '{print $1}')"
LOCAL_BRIDGE="$(sha256sum "$BRIDGE" | awk '{print $1}')"
LOCAL_EXE="$(sha256sum "$EXE" | awk '{print $1}')"
"$ADB" shell "mkdir -p '$REMOTE'"
"$ADB" push "$HOST_LIB" "$REMOTE/libsin_compat_host_abi.so" >/dev/null
"$ADB" push "$BRIDGE" "$REMOTE/libsin_compat_event_bridge.so" >/dev/null
"$ADB" push "$EXE" "$REMOTE/sin_android_event_bridge_smoke" >/dev/null
HASHES="$("$ADB" shell "sha256sum '$REMOTE/libsin_compat_host_abi.so' '$REMOTE/libsin_compat_event_bridge.so' '$REMOTE/sin_android_event_bridge_smoke'" | tr -d '\r')"
mapfile -t LINES <<< "$HASHES"
[[ ${#LINES[@]} -eq 3 ]] || { echo 'ERROR: expected three device hashes' >&2; exit 1; }
[[ "$(awk '{print $1}' <<< "${LINES[0]}")" == "$LOCAL_HOST" &&
   "$(awk '{print $1}' <<< "${LINES[1]}")" == "$LOCAL_BRIDGE" &&
   "$(awk '{print $1}' <<< "${LINES[2]}")" == "$LOCAL_EXE" ]] || {
  echo 'ERROR: Android transfer SHA-256 mismatch' >&2; exit 1;
}
OUTPUT="$("$ADB" shell "chmod 700 '$REMOTE/sin_android_event_bridge_smoke' && cd '$REMOTE' && ./sin_android_event_bridge_smoke ./libsin_compat_event_bridge.so" | tr -d '\r')"
printf '%s\n' "$OUTPUT"
grep -Fxq 'Native event bridge shared-library dlopen lifecycle passed' <<< "$OUTPUT" || {
  echo 'ERROR: event bridge pass marker missing' >&2; exit 1;
}
if [[ -n "$REPORT" ]]; then
  mkdir -p "$(dirname "$REPORT")"
  {
    printf 'schema=sin_compat_event_bridge_android_smoke_v1\nresult=PASS\n'
    printf 'abi=%s\nandroid_api=%s\nqemu_property=%s\n' "$ABI" "$API" "$QEMU"
    printf 'host_abi_sha256=%s\nevent_bridge_sha256=%s\nloader_sha256=%s\n' \
      "$LOCAL_HOST" "$LOCAL_BRIDGE" "$LOCAL_EXE"
    printf 'remote_sha256_verified=true\nloader_marker_verified=true\n'
    printf 'scope=standalone_native_event_bridge_not_sourcevr\n'
  } > "$REPORT"
fi
echo 'PASS: Android process loaded native event bridge (not inside SourceVR).'
