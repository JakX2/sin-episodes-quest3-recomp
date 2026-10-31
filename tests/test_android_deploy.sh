#!/usr/bin/env bash
# Offline argument/guard test only. No Android executable is run here.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/build"
printf 'placeholder\n' > "$TMP/build/libsin_compat_host_abi.so"
printf 'placeholder\n' > "$TMP/build/sin_android_host_abi_smoke"
cat > "$TMP/adb" <<'MOCK'
#!/usr/bin/env bash
printf '%s\n' "$*" >> "$MOCK_ADB_LOG"
if [[ "${1:-}" == shell && "${2:-}" == getprop ]]; then
  case "$3" in
    ro.product.cpu.abi) printf '%s\n' "${MOCK_DEVICE_ABI:-arm64-v8a}";;
    ro.build.version.sdk) printf '%s\n' "${MOCK_DEVICE_API:-32}";;
    *) exit 9;;
  esac
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
printf '%s\n' 'Offline adb deployment guard tests pass (no Android device used).'
