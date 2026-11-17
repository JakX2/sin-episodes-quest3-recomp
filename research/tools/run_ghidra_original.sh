#!/usr/bin/env bash
# Local-only bulk Ghidra pass. Never commit retail DLLs or private decompilation.
set -euo pipefail

if [[ $# -lt 3 || $# -gt 4 ]]; then
  echo "Usage: $0 /path/to/ghidra /private/extracted/bin /private/output [metadata-only|emit-c|targeted-c]" >&2
  exit 2
fi

ghidra_home="$1"
original_bin="$2"
output="$3"
mode="metadata-only"
if [[ $# -eq 4 ]]; then
  mode="$4"
fi
if [[ "$mode" != "metadata-only" && "$mode" != "emit-c" && "$mode" != "targeted-c" ]]; then
  echo "Mode must be metadata-only, emit-c or targeted-c" >&2
  exit 2
fi

headless="$ghidra_home/support/analyzeHeadless"
if [[ ! -x "$headless" ]]; then
  echo "Missing Ghidra analyzeHeadless executable at: $headless" >&2
  exit 1
fi

for dll in client.dll server.dll; do
  if [[ ! -f "$original_bin/$dll" ]]; then
    echo "Missing owner-original $original_bin/$dll" >&2
    exit 1
  fi
done

scripts="$(cd "$(dirname "$0")/../ghidra" && pwd)"
mkdir -p "$output"
output="$(cd "$output" && pwd)"
private_project="$(mktemp -d /tmp/sin-ghidra.XXXXXXXX)"
trap 'rm -rf "$private_project"' EXIT

if [[ "$mode" == "targeted-c" ]]; then
  # Deliberately opt-in: the sole output is private, never uploaded by CI.
  # The owner must supply a local trusted Ghidra and owner-original DLLs.
  repository_root="$(cd "$(dirname "$0")/../.." && pwd -P)"
  case "$output/" in
    "$repository_root/"*)
      echo "Refusing to generate game pseudocode inside the public checkout" >&2
      exit 1
      ;;
  esac
  umask 077
  chmod 700 "$output"
  seed_dir="$(mktemp -d /tmp/sin-reviewed-factories.XXXXXXXX)"
  trap 'rm -rf -- "$private_project" "$seed_dir"' EXIT
  python3 "$scripts/../tools/verify_original_batch.py" \
    --original-dir "$original_bin" --out "$output/original_pe32_identity.json"
  python3 "$scripts/../tools/prepare_reviewed_factory_seeds.py" \
    --manifest "$scripts/../catalog/original_dll_interface_registrations.json" \
    --out-dir "$seed_dir"
  SIN_PRIVATE_DECOMP_ACK=1 "$headless" "$private_project" SiNPrivateTargeted     -import "$original_bin/client.dll" "$original_bin/server.dll"     -scriptPath "$scripts"     -postScript SeedReviewedInterfaceFactories.java "$seed_dir" "$output"     -postScript ExportTargetedInterfacePseudocode.java "$seed_dir" "$output"     -deleteProject
  for dll in client.dll server.dll; do
    test -s "$output/${dll}_targeted_decomp_status.csv"
  done
  echo "Private targeted pseudocode and status CSVs saved only under: $output"
  echo "Ghidra output is approximate and no original methods are newly SHA verified."
  exit 0
fi

if [[ "$mode" == "emit-c" ]]; then
  "$headless" "$private_project" SiNOriginalPrivate \
    -import "$original_bin/client.dll" "$original_bin/server.dll" \
    -scriptPath "$scripts" \
    -postScript ExportOriginalFunctionAtlas.py "$output" emit-c \
    -deleteProject
else
  # Optional reviewed static registration addresses enrich discovery ONLY after
  # the upstream job has authenticated the complete owner-original PE32 files.
  post_scripts=()
  if [[ -n "${SIN_GHIDRA_SEED_DIRECTORY:-}" ]]; then
    for dll in client.dll server.dll; do
      test -s "$SIN_GHIDRA_SEED_DIRECTORY/${dll}_factory_seeds.csv"
    done
    post_scripts+=(-postScript SeedReviewedInterfaceFactories.java "$SIN_GHIDRA_SEED_DIRECTORY" "$output")
  fi
  "$headless" "$private_project" SiNOriginalPrivate \
    -import "$original_bin/client.dll" "$original_bin/server.dll" \
    -scriptPath "$scripts" \
    "${post_scripts[@]}" \
    -postScript ExportOriginalFunctionAtlas.java "$output" \
    -deleteProject
  if [[ -n "${SIN_GHIDRA_SEED_DIRECTORY:-}" ]]; then
    for dll in client.dll server.dll; do
      test -s "$output/${dll}_factory_seed_results.csv"
    done
  fi
fi
echo "Ghidra candidate atlas generated under $output"
echo "Metadata mode uses Ghidra 12.2 Java; targeted-c generates private C-like output only."\necho "Do not push raw Ghidra decompilation to GitHub."
