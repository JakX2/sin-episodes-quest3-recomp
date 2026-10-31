#!/usr/bin/env bash
# User-owned ARM64 Android / Quest device only; not secure identity attestation.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
if [[ $# -gt 2 ]]; then echo "Usage: $0 [build-dir] [new-evidence-report]" >&2; exit 2; fi
if [[ -n "${SIN_SMOKE_EXPECTED_ABI:-}" && "$SIN_SMOKE_EXPECTED_ABI" != arm64-v8a ]]; then
  echo 'ERROR: strict ARM64 hardware wrapper refuses x86_64.' >&2; exit 2
fi
export SIN_SMOKE_EXPECTED_ABI=arm64-v8a SIN_SMOKE_REQUIRE_PHYSICAL=1
export SIN_SMOKE_REPORT_PATH="${2:-$PWD/sin-compat-arm64-hardware-evidence.txt}"
exec bash "$ROOT/scripts/run_android_host_abi_smoke.sh" "${1:-$ROOT/build-android-arm64}"
