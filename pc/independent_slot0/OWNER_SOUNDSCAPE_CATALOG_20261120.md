# Phase 483: independent soundscape KeyValues trees and cross-file catalog

## Private original resources authenticated

The original SiN Episodes 2006 scripts.zip contains seven soundscape
files listed by the SHA-pinned original soundscaples_manifest root
(Phase 482). All seven texts have now been individually SHA256-pinned
in audit_sin_original_soundscape_catalog.py. The private auditor
parses each file, reports only **derived metadata**, and does not
copy content or original DLLs into the repository.

Derived original asset inventory:

| Original resource property | Read-only SHA-verified result |
| --- | ---: |
| Soundscape script files | 7 |
| Named top-level soundscapes | 301 |
| Unique names (case-insensitive) | 301 |
| Nested KeyValues property nodes | 4,874 |
| playlooping blocks | 281 |
| playrandom blocks | 150 |
| rndwave blocks | 150 |
| wave string properties | 1,224 |
| playsoundscape blocks | 242 |
| Nested named playsoundscape references | 242 |
| References unresolved within these seven files | 3 |

The three unresolved references refer to only two distinct target
names: d2_depot_cellblock_medium (twice) and druglab.dudeattacked
(once). These observations are **not proof** that the original game's
sound runtime fails or that additional engine-side names cannot
exist. The port retains the references and reports diagnostics
instead of silently deleting them or rejecting the entire original
catalog.

## Independently authored parser

client_soundscape_catalog_owned.h uses the Phase 482 bounded
KeyValues lexer to create **ordered nested trees** for each
soundscape. Repeated child names are preserved, which is necessary
for multiple playlooping, playrandom, rndwave and wave entries.
The independent catalog:

- collects named top-level definitions with source file attribution;
- retains nested key/value structure without flattening repeated keys;
- indexes names with an ASCII case-insensitive lookup;
- records playsoundscape graph edges and unresolved targets;
- rejects malformed/empty documents, duplicate top-level names,
  excessive nesting, node counts or definition counts;
- publishes the entire catalog atomically after parsing every
  referenced file, preserving the prior snapshot if any file fails.

Caps: maximum 20 nested block levels, 24,000 property nodes and
4,096 top-level definitions. These are independent safety choices,
not claims about the original engine's maximums.

test_client_soundscape_catalog_owned.cpp exercises cross-file name
resolution, repeated looping blocks, random-wave children, one
missing reference, casefold lookup, malformed input rollback,
duplicate-name refusal and resource bounds. All data in public
tests are invented synthetic scripts, not retail game assets.

The original resource auditor has a public original-free --self-test
and a private --scripts option. Neither the original scripts nor
raw proprietary DLL method bodies are committed.

## Explicitly incomplete for startup

This is a **definition catalog**. The actual 2006 Source soundscape
system still needs its native/port-owned playback selection,
recursion/dependency rules, DSP and soundlevel interpretation,
wave/sound emitter interface, map listener registration and cleanup.
Original C_SoundscapeSystem::Init at 0x240F4160 is known to return
true on both observed final exits, even with some resource failures;
the owned parser intentionally demands coherent snapshots instead.

The production VClient011::Init remains fail-closed (returns 0).
The latest original owner-run real Windows startup checkpoint stays
at 0x53494E74. Phase 483's catalog and validation are **not**
evidence of playable Windows, Android or Quest 3 builds.
