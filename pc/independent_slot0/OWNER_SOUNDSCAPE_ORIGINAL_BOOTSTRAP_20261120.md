# Phase 495: original SiN 2006 C_SoundscapeSystem::Init resource lifecycle

## Source proof

The privately authenticated original retail 2006 client.dll
SHA256 is
6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.

The source Init body at 0x240F4160, 343 bytes,
SHA256 0fbb82377e54b7119c2c0e1a0b107c8958b354010993e03a481829cb2a2d661a,
calls the original per-file helper at 0x240F4070,
233 bytes,
SHA256 69d75f737e839fd49af874f8cda11ad9b8b95feb51b2e6d084c34a2eac5c9bfa.

Actual original field and branch control:

1. Reset object field this+0xCC to zero.
2. Obtain an optional current-map string from helper 0x241560A0
   and, if nonempty, format
   scripts/soundscapes_%s.txt using literal at 0x24293D2C.
3. Construct KeyValues and LoadFromFile for
   scripts/soundscapes_manifest.txt with pathID "GAME"
   and file read mode "rb".
4. If the manifest load fails, print a diagnostic, free the
   temporary object and RETURN TRUE via AL=1 at 0x240F42B2.
   The optional map path is not probed after this branch.
5. If it succeeds, iterate first/next manifest entries. Match
   key "file" case-insensitively. For each matching value,
   call 0x240F4070 to load the resource with pathID nullptr.
   The caller does not test a return from this helper.
   Other key names produce an original warning and are skipped.
6. Every matching file value is also compared case-insensitively
   with the optional map-specific candidate. When it matches,
   the candidate is cleared and **must not be appended twice**.
7. If the optional candidate remains at the end, call the original
   filesystem virtual slot 10 (byte offset 0x28) with pathID
   nullptr to ask if it exists; only if it exists, call
   the same original per-file helper.
8. Delete the temporary manifest KeyValues object and RETURN TRUE
   via AL=1 at 0x240F4290. This includes missing individual files.

The per-file helper at 0x240F4070 allocates its own KeyValues,
loads the specific resource with a *null* pathID, then walks
the loaded root names and appends named source records into
C_SoundscapeSystem's original vector around this+0x1C,
count this+0x28. When the file load fails it simply deletes
the temporary KeyValues object; it does NOT abort the original
Init. It also merges valid loaded children through the original
KeyValues ownership functions.

This means **original Init returned true** is NOT synonymous with
**all soundscape resources loaded or playable**. The strict
user-retail test from Phases 486/493/494 intentionally continues
to require the exact SHA-verified seven-file catalog and
0x53494E76 proof; that diagnostic is separate.

## Independently authored source-compatible model

client_soundscape_original_bootstrap_owned.h implements
OwnedInit::Initialize with two injected, separately testable
callback contracts:

- Read(path, pathID, output) -> bool: only "GAME" for manifest,
  nullptr for individual scripts.
- Exists(path, pathID) -> bool: only nullptr for optional map file.

The owned initializer resets its modeled field +0xCC, preserves
manifest key and file iteration order, skips unknown manifest
keys, continues after failed individual script loads, and
optionally checks/loads a map-specific soundscape with the
original source file naming convention.

For successful independently parsed files, it records source
names and ordered catalog definitions via existing Phase 483
KeyValues-like parser. It resolves nested names across the
finally accepted set, keeping unresolved references separate.
Like the original per-file registration, it does NOT impose
the stricter owner's duplicate-top-level-name rejection
at this bootstrap layer.

The valid zero-file manifest case still checks an optional
map file; the strict manifest parser's NoFileEntries refusal
must not incorrectly block this original source branch.

C++ safety policies not authenticated as original behavior:
- Safe map suffixes limited to 64 ASCII characters
  (letters, digits, underscore and hyphen);
- each individual source capped to 256 KiB, consistent with
  the independent VFileSystem012 reader;
- owned parser depth, name and node caps;
- malformed individual document skipped conservatively;
- callback exception or OOM refused atomically without
  publishing corrupted state;
- clean owned state when reinitializing after a missing or
  invalid manifest. The original object may retain more state
  if called twice without Shutdown; this policy is not parity.

The report separates original_bool_init_true from the
independently observed completeness of the loaded catalog.
In the modeled safe paths, missing manifests, malformed
manifests and partial source sets can all have original Init
return TRUE even when zero resources were accepted.

## Native regression suite

test_client_soundscape_original_bootstrap_owned.cpp:
- Original GAME-vs-null resource provider scopes.
- Two manifest scripts, unknown key, per-map fallback and
  source-ordered cross-file reference resolution.
- No duplicate optional map loading when named in manifest.
- Missing manifest returns true but no source catalog.
- Valid empty manifest still probes map-specific file.
- Missing or malformed per-file document returns true and
  keeps later files available.
- Invalid manifest and malicious map suffix refusal policy.
- Callback failure rollback with revision and state checks.
- Original per-file duplicate names preserved without false
  declaration of a unique catalog.
- Independent per-file size limit.
- Reinitialize and Clear lifecycle.

NO real IFileSystem virtual method is invoked by this class.
NO original source methods or copyrighted KeyValues are
distributed with the tests. Synthetic data only in public CI.

## Startup/Quest implications

This recovers important original game-system initialization
policy, but it is not yet a complete C_SoundscapeSystem runtime:
no soundscape selection, sound emitter scheduling, wave playback,
DSP, engine KeyValues ownership, map trigger binding or actual
VClient011::Init success. The original game-system registration
must remain incomplete until those external interfaces and the
remaining AutoGameSystem dependencies are reconstructed.

The last actual owner-retail checkpoint remains 0x53494E74.
Phase 494 fixed the *real filesystem argument* used by the
opt-in original 0x53494E76/77 diagnostic; Phase 495 is a
separate independent bootstrap model, not a claim of
passing that retail test. Android/Quest 3 still requires
real gameplay support after Windows boot.
