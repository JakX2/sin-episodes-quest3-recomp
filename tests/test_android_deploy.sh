#!/usr/bin/env bash
# Offline argument/guard test only. No Android executable is run here.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/build"
printf 'placeholder\n' > "$TMP/build/libsin_compat_host_abi.so"
printf 'placeholder\n' > "$TMP/build/sin_android_host_abi_smoke"
export MOCK_LIBRARY_HASH="$(sha256sum "$TMP/build/libsin_compat_host_abi.so" | awk '{print $1}')"
export MOCK_LOADER_HASH="$(sha256sum "$TMP/build/sin_android_host_abi_smoke" | awk '{print $1}')"
cat > "$TMP/adb" <<'MOCK'
#!/usr/bin/env bash
printf '%s\n' "$*" >> "$MOCK_ADB_LOG"
if [[ "${1:-}" == shell && "${2:-}" == getprop ]]; then
  case "$3" in
    ro.product.cpu.abi) printf '%s\n' "${MOCK_DEVICE_ABI:-arm64-v8a}";;
    ro.build.version.sdk) printf '%s\n' "${MOCK_DEVICE_API:-32}";;
    ro.kernel.qemu) printf '%s\n' "${MOCK_QEMU:-0}";;
    *) exit 9;;
  esac
fi
elif [[ "${1:-}" == shell && "${2:-}" == 'sha256sum '* ]]; then
  printf '%s  %s\n' "${MOCK_REMOTE_LIB_SHA:-$MOCK_LIBRARY_HASH}" /data/local/tmp/sin_compat_api26_smoke/libsin_compat_host_abi.so
  printf '%s  %s\n' "${MOCK_REMOTE_EXE_SHA:-$MOCK_LOADER_HASH}" /data/local/tmp/sin_compat_api26_smoke/sin_android_host_abi_smoke
elif [[ "${1:-}" == shell && "${2:-}" == 'chmod 700 '* ]]; then
  [[ "${MOCK_SMOKE_EXIT:-0}" == 0 ]] || exit "$MOCK_SMOKE_EXIT"
  if [[ "${MOCK_SMOKE_OUTPUT:-pass}" == pass ]]; then
    echo 'C ABI dlopen v1 smoke test passed'
  else
    echo 'Unrelated output'
  fi
fi
MOCK
chmod +x "$TMP/adb"
export MOCK_ADB_LOG="$TMP/adb.log"
export ADB="$TMP/adb"
OUTPUT="$(bash "$ROOT/scripts/run_android_host_abi_smoke.sh" "$TMP/build")"
grep -q 'PASS: on-device' <<< "$OUTPUT"
grep -q 'push .*libsin_compat_host_abi.so /data/local/tmp/sin_compat_api26_smoke/libsin_compat_host_abi.so' "$TMP/adb.log"
grep -q 'push .*sin_android_host_abi_smoke /data/local/tmp/sin_compat_api26_smoke/sin_android_host_abi_smoke' "$TMP/adb.log"
grep -q 'cd .*sin_compat_api26_smoke.*&& ./sin_android_host_abi_smoke ./libsin_compat_host_abi.so' "$TMP/adb.log"
: > "$TMP/adb.log"
if MOCK_DEVICE_ABI=x86_64 bash "$ROOT/scripts/run_android_host_abi_smoke.sh" "$TMP/build" >"$TMP/out" 2>&1; then
  echo 'ERROR: wrong-architecture device accepted' >&2; exit 1
fi
if grep -q '^push ' "$TMP/adb.log"; then
  echo 'ERROR: pushed files to wrong architecture device' >&2; exit 1
fi
: > "$TMP/adb.log"
if MOCK_DEVICE_API=25 bash "$ROOT/scripts/run_android_host_abi_smoke.sh" "$TMP/build" >"$TMP/out" 2>&1; then
  echo 'ERROR: API below min accepted' >&2; exit 1
fi
if grep -q '^push ' "$TMP/adb.log"; then
  echo 'ERROR: pushed files to unsupported API device' >&2; exit 1
fi
if bash "$ROOT/scripts/run_android_host_abi_smoke.sh" "$TMP/missing" >"$TMP/out" 2>&1; then
  echo 'ERROR: missing binaries accepted' >&2; exit 1
fi
# Explicit x86_64 override is reserved for the real Android emulator job.
: > "$TMP/adb.log"
MOCK_DEVICE_ABI=x86_64 SIN_SMOKE_EXPECTED_ABI=x86_64 \
  bash "$ROOT/scripts/run_android_host_abi_smoke.sh" "$TMP/build" > "$TMP/out"
grep -q 'PASS: on-device' "$TMP/out"
grep -q '^push ' "$TMP/adb.log"
: > "$TMP/adb.log"
if MOCK_DEVICE_ABI=arm64-v8a SIN_SMOKE_EXPECTED_ABI=x86_64 \
  bash "$ROOT/scripts/run_android_host_abi_smoke.sh" "$TMP/build" > "$TMP/out" 2>&1; then
  echo 'ERROR: x86_64 emulator setting accepted an arm64 device' >&2; exit 1
fi
if grep -q '^push ' "$TMP/adb.log"; then
  echo 'ERROR: cross-architecture emulator setting pushed files' >&2; exit 1
fi
: > "$TMP/adb.log"
if SIN_SMOKE_EXPECTED_ABI=armeabi-v7a \
  bash "$ROOT/scripts/run_android_host_abi_smoke.sh" "$TMP/build" > "$TMP/out" 2>&1; then
  echo 'ERROR: unsupported ABI override accepted' >&2; exit 1
fi
if grep -q '^push ' "$TMP/adb.log"; then
  echo 'ERROR: unsupported ABI override pushed files' >&2; exit 1
fi
# Phase 72: genuine loader marker, transferred byte integrity and no false reports.
REPORT="$TMP/phase72-evidence.txt"
SIN_SMOKE_REPORT_PATH="$REPORT" bash "$ROOT/scripts/run_android_host_abi_smoke.sh" "$TMP/build" > "$TMP/out"
grep -qx 'result=PASS' "$REPORT"
grep -qx 'loader_marker_verified=true' "$REPORT"
grep -qx 'remote_sha256_verified=true' "$REPORT"
if SIN_SMOKE_REPORT_PATH="$REPORT" bash "$ROOT/scripts/run_android_host_abi_smoke.sh" "$TMP/build" > "$TMP/out" 2>&1; then
  echo 'ERROR: overwrote existing evidence report' >&2; exit 1
fi
for fault in bad-marker bad-hash failed-loader qemu; do
  R="$TMP/$fault.txt"
  rm -f "$R"
  case "$fault" in
    bad-marker) MOCK_SMOKE_OUTPUT=bad SIN_SMOKE_REPORT_PATH="$R" bash "$ROOT/scripts/run_android_host_abi_smoke.sh" "$TMP/build" > "$TMP/out" 2>&1 && exit 1;;
    bad-hash) MOCK_REMOTE_LIB_SHA="$(printf '%064d' 0)" SIN_SMOKE_REPORT_PATH="$R" bash "$ROOT/scripts/run_android_host_abi_smoke.sh" "$TMP/build" > "$TMP/out" 2>&1 && exit 1;;
    failed-loader) MOCK_SMOKE_EXIT=7 SIN_SMOKE_REPORT_PATH="$R" bash "$ROOT/scripts/run_android_host_abi_smoke.sh" "$TMP/build" > "$TMP/out" 2>&1 && exit 1;;
    qemu) MOCK_QEMU=1 SIN_SMOKE_REQUIRE_PHYSICAL=1 SIN_SMOKE_REPORT_PATH="$R" bash "$ROOT/scripts/run_android_host_abi_smoke.sh" "$TMP/build" > "$TMP/out" 2>&1 && exit 1;;
  esac
  [[ ! -e "$R" ]] || { echo "ERROR: false evidence report on $fault" >&2; exit 1; }
done
# Successful hardware-handoff mock does not establish real physical device identity.
SIN_SMOKE_REQUIRE_PHYSICAL=1 SIN_SMOKE_REPORT_PATH="$TMP/hardware-mock.txt" bash "$ROOT/scripts/run_android_host_abi_smoke.sh" "$TMP/build" > "$TMP/out"
grep -qx 'require_physical=1' "$TMP/hardware-mock.txt"
printf '%s\n' 'Offline adb deployment guard tests pass (no Android device used).'
