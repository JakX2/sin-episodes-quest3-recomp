#!/usr/bin/env bash
# Local-only bulk Ghidra pass. Never commit retail DLLs or private decompilation.
set -euo pipefail

if [[ $# -lt 3 || $# -gt 4 ]]; then
  echo "Usage: $0 /path/to/ghidra /private/extracted/bin /private/output [emit-c]" >&2
  exit 2
fi

ghidra_home="$1"
original_bin="$2"
output="$3"
mode="metadata-only"
if [[ $# -eq 4 ]]; then
  mode="$4"
fi
if [[ "$mode" != "metadata-only" && "$mode" != "emit-c" ]]; then
  echo "Mode must be metadata-only or emit-c" >&2
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
  if [[ -n "${SIN_GHIDRA_VTABLE_SEED_DIRECTORY:-}" ]]; then
    if [[ -z "${SIN_GHIDRA_SEED_DIRECTORY:-}" ]]; then
      echo "Original vtable entries require reviewed factory seeds" >&2
      exit 1
    fi
    for dll in client.dll server.dll; do
      test -s "$SIN_GHIDRA_VTABLE_SEED_DIRECTORY/${dll}_vtable_method_seeds.csv"
    done
    post_scripts+=(-postScript SeedAuthenticatedVtablePrefixMethods.java "$SIN_GHIDRA_VTABLE_SEED_DIRECTORY" "$output")
  fi
  if [[ -n "${SIN_GHIDRA_TARGETED_PRIVATE_DIRECTORY:-}" ]]; then
    if [[ -z "${SIN_GHIDRA_SEED_DIRECTORY:-}" || "${SIN_PRIVATE_DECOMP_ACK:-}" != "1" ]]; then
      echo "Targeted private decompilation requires reviewed seeds and explicit acknowledgement" >&2
      exit 1
    fi
    # Ensure private C-like output is never under the checkout or public
    # metadata directory even when the caller supplies a symlink path.
    mkdir -p "$SIN_GHIDRA_TARGETED_PRIVATE_DIRECTORY"
    private_output="$(cd "$SIN_GHIDRA_TARGETED_PRIVATE_DIRECTORY" && pwd -P)"
    repository_root="$(cd "$(dirname "$0")/../.." && pwd -P)"
    case "$private_output/" in
      "$repository_root/"* | "$output/"*)
        echo "Refusing original pseudocode output in repository or metadata artifact directory" >&2
        exit 1
        ;;
    esac
    umask 077
    chmod 700 "$private_output"
    post_scripts+=(-postScript ExportTargetedInterfacePseudocode.java "$SIN_GHIDRA_SEED_DIRECTORY" "$private_output")
    if [[ -n "${SIN_GHIDRA_VTABLE_SEED_DIRECTORY:-}" ]]; then
      # C-like output remains ephemeral beside the existing private factory
      # C output; only the sanitized status is eligible for metadata upload.
      post_scripts+=(-postScript ExportTargetedVtableMethodPseudocode.java "$SIN_GHIDRA_VTABLE_SEED_DIRECTORY" "$private_output")
    fi
  fi
  if [[ "${SIN_GHIDRA_FACTORY_XREFS:-0}" == "1" ]]; then
    if [[ -z "${SIN_GHIDRA_SEED_DIRECTORY:-}" ]]; then
      echo "Reviewed factory data xrefs require seed directory" >&2
      exit 1
    fi
    post_scripts+=(-postScript ExportReviewedFactoryDataReferences.java "$SIN_GHIDRA_SEED_DIRECTORY" "$output")
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
  if [[ "${SIN_GHIDRA_FACTORY_XREFS:-0}" == "1" ]]; then
    for dll in client.dll server.dll; do
      test -s "$output/${dll}_factory_data_ref_status.csv"
      test -s "$output/${dll}_factory_direct_data_refs.csv"
    done
  fi
  if [[ -n "${SIN_GHIDRA_VTABLE_SEED_DIRECTORY:-}" ]]; then
    for dll in client.dll server.dll; do
      test -s "$output/${dll}_vtable_seed_results.csv"
    done
  fi
  if [[ -n "${SIN_GHIDRA_TARGETED_PRIVATE_DIRECTORY:-}" ]]; then
    for dll in client.dll server.dll; do
      test -s "$private_output/${dll}_targeted_decomp_status.csv"
      if [[ -n "${SIN_GHIDRA_VTABLE_SEED_DIRECTORY:-}" ]]; then
        test -s "$private_output/${dll}_vtable_targeted_decomp_status.csv"
      fi
    done
  fi
fi
echo "Ghidra candidate atlas generated under $output"
echo "Metadata mode uses Ghidra 12.2 Java script; emit-c requires a PyGhidra-enabled environment."
echo "Do not push raw Ghidra decompilation to GitHub."
