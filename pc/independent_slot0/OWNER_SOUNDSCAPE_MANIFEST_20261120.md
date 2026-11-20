# Phase 482: original soundscape manifest dependency bootstrap

## Sources (strictly private, read-only)

- Original 2006 client.dll SHA256:
  6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.
- Original soundscape Init at VA 0x240F4160, 343 bytes, complete method
  SHA256 0fbb82377e54b7119c2c0e1a0b107c8958b354010993e03a481829cb2a2d661a.
- Original scripts/soundscapes_manifest.txt SHA256:
  d2c02a5f0dddb3cdcb92c655d222c6a549d77f5f940211d6429a1ef996391863.
- The private 2006 client.dll and scripts.zip were obtained from the user's
  connected original SiN game files. No raw DLL/asset bytes were committed.

The original method's top-level flow resets field this+0xCC, forms an
optional map-specific fallback filename, constructs an original
KeyValues instance, calls LoadFromFile for
scripts/soundscapes_manifest.txt with "GAME", then walks first/next
top-level keys. The key-name comparison selects entries named "file".
These callsites, branch shapes and two final true-return exits are
authenticated against the original method in
audit_sin_original_soundscape_manifest.py.

On missing/failed manifest load, and on regular cleanup, the original
method can still return true. That fact must not be confused with an
actually populated soundscape catalog.

## Surprising authentic resource detail

The 2006 retail manifest's exact root is **soundscaples_manifest**
(not a corrected "soundscapes_manifest"). The original KeyValues load
uses the actual passed manifest filename, and the parser proceeds with
the resource's root as supplied. The independent parser deliberately
does NOT demand a guessed corrected root.

Seven top-level "file" keys, in order, reference these retail scripts:

1. scripts/soundscapes.txt
2. scripts/soundscapes_intro.txt
3. scripts/soundscapes_docks.txt
4. scripts/soundscapes_druglab.txt
5. scripts/soundscapes_pit.txt
6. scripts/soundscapes_highrise.txt
7. scripts/soundscapes_finale.txt

The SHA-pinned original scripts.zip contains all seven. This phase
does not publish file contents or dependent sound assets.

## Independent implementation

client_soundscape_manifest_owned.h implements a bounded, KeyValues-like
lexer for quoted/unquoted tokens, // comments, braces, nested unknown
entries and the original repeated file-key pattern. It preserves root
spelling, entry order and duplicate file references instead of
silently deduplicating. Unknown top-level keys count as warnings.

A callback-owned file-provider adapter reads the seven selected text
resources into an **atomic owned snapshot**. Failures do not change
the previous valid snapshot. A conservative independent input policy
refuses absolute/traversal/unsafe paths and arbitrary extensions,
manifest bytes >256 KiB, individual resource texts >512 KiB, more than
64 manifest file entries, or a combined resource set >4 MiB.

These caps and failure semantics are explicitly safety choices of
this implementation, NOT claims about Source 2006 KeyValues behavior.

Native synthetic fixture test_client_soundscape_manifest_owned.cpp
covers the misspelled root, comments and case-insensitive file keys,
unknown nested entries, exact file ordering, duplicate keys, BOM/CRLF,
safe-path validation, malformed/truncated syntax, missing dependency
rollback, oversized resources, total-size cap and independent cleanup.

The separate SHA-pinned original auditor has a synthetic --self-test
for public CI; its real-client/scripts mode is strictly private and
read-only. Original assets are not uploaded or used in CI.

## Remaining technical gates

The staged script texts are NOT yet parsed into soundscape names,
playlooping/playrandom/playsoundscape blocks or runtime audio settings.
Original C_SoundscapeSystem resource loader 0x240F4070 and later
KeyValues::LoadFromFile and soundscape selection/emit calls remain
partially unknown. This manifest loader does not call VFileSystem012,
register Source engine sound callbacks, or attach to an actual
C_SoundscapeSystem game object.

C_SoundscapeSystem remains one of four incompletely reconstructed
AutoGameSystem methods. Production VClient011::Init still returns 0,
and VClient011 slot33 remains fail-closed. The real last owner-retail
execution checkpoint remains **0x53494E74**, not game boot.
