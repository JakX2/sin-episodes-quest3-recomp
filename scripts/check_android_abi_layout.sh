#!/usr/bin/env bash
# Freestanding source-ABI contract compile, NOT an Android NDK/Quest build.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CLANG="${CLANG:-clang}"
READELF="${READELF:-readelf}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
RESOURCE_DIR="$($CLANG -print-resource-dir)"
"$CLANG" --target=aarch64-linux-android26 \
  -ffreestanding -nostdinc -isystem "$RESOURCE_DIR/include" \
  -std=c11 -Wall -Wextra -Werror -pedantic \
  -I"$ROOT/include" -c "$ROOT/tests/abi_layout_contract.c" \
  -o "$TMP/sin_host_abi_layout_android_arm64.o"
header="$($READELF -h "$TMP/sin_host_abi_layout_android_arm64.o")"
if ! grep -q 'Machine:.*AArch64' <<< "$header"; then
    echo 'ERROR: ABI contract object is not AArch64' >&2
    exit 1
fi
if ! grep -q 'Class:.*ELF64' <<< "$header"; then
    echo 'ERROR: ABI contract object is not ELF64' >&2
    exit 1
fi
printf '%s\n' 'AArch64 Android API26 freestanding C header layout compile passed.' \
  'This does NOT link or run the SiN C++ module, nor use the Android NDK.'
