# Phase 487: original 2006 soundscape dependency DAG and bounded owned expansion

## Private source evidence

The user's private original `SE1/scripts.zip` was opened read-only
from connected Drive. The original `scripts/soundscapes_manifest.txt`
matches its prior SHA256
`d2c02a5f0dddb3cdcb92c655d222c6a549d77f5f940211d6429a1ef996391863`.
**Each of the seven referenced original soundscape files matches the
individual hashes pinned in
`audit_sin_original_soundscape_catalog.py`.**

This is a real original-resource graph analysis, **not** a synthetic-only
guess. The seven files' original scripts are never committed, uploaded to
GitHub or copied into the owner diagnostic artifact.

| Original manifest file | Root definitions |
| --- | ---: |
| scripts/soundscapes.txt | 14 |
| scripts/soundscapes_intro.txt | 26 |
| scripts/soundscapes_docks.txt | 95 |
| scripts/soundscapes_druglab.txt | 81 |
| scripts/soundscapes_pit.txt | 40 |
| scripts/soundscapes_highrise.txt | 38 |
| scripts/soundscapes_finale.txt | 7 |
| **Total** | **301** |

The exact original reference graph, treating names
**ASCII case-insensitively** while preserving every occurrence:

| Property | Original read-only result |
| --- | ---: |
| Named definitions | 301 |
| playsoundscape reference occurrences | 242 |
| Resolved edge occurrences | 239 |
| Unresolved occurrences | 3 |
| Distinct unresolved names | 2 |
| Distinct resolved destination names | 83 |
| Definitions with at least one resolved outgoing reference | 120 |
| Definitions with no resolved outgoing reference | 181 |
| Cross-file resolved occurrences | 4 |
| Maximum outgoing resolved occurrences on a definition | 5 |
| Directed cycles | **0** |
| Maximum nested dependency depth (root is depth 1) | **5** |
| Maximum single-root expansion steps (resolved definitions) | **10** |
| Sum of resolved expansion steps over all 301 possible roots | **602** |

The original unresolved targets are
`d2_depot_cellblock_medium` (twice, same source) and
`druglab.dudeattacked` (once). These are intentional retained graph
diagnostics, **not** proof that game playback will fail.

The source graph has 301 directed strongly connected components
because all 301 vertices are acyclic singleton components. The
maximum source expansion is only ten enter-events, despite repeated
references that must not be deduplicated.

## New independently authored runtime components

`client_soundscape_graph_owned.h` derives a portable *owned*
soundscape reference graph from the Phase 483 hierarchical
`CatalogState` populated by the Phase 484 native resource bridge:

1. Validate catalog counts, unique root names, references' owner
   identities and their original source-file attribution.
2. Copy only the owned root names/source paths/reference edges,
   never retain pointers to engine KeyValues objects or ephemeral
   `std::string_view` into the resource reader.
3. Preserve **all repeated references and their source order**,
   including case-insensitive resolution.
4. Compute resolved/missing counters, distinct destinations,
   cross-file edges and topological depth using **iterative Kahn
   ordering**, not unbounded recursive stack traversal.
5. Refuse directed reference cycles, incomplete metadata,
   ambiguous duplicate source names, or allocation failure
   **without destroying the previously published good graph**.
6. Expand an arbitrary root into an independently owned, ordered
   depth-first structural event plan with explicit
   `DefinitionEntered` and `MissingReference` events.
   Missing references are retained with owner/target information
   and are not treated as globally fatal to the real source catalog.
7. Publish an expansion **only after the whole traversal is
   successful**. A failed lookup or traversal never exposes
   a truncated event plan.

Independent bounded policies: 4,096 maximum definitions, 16,384
reference edges, 16,384 expansion events and 256 walk levels.
These are port-owned defensive limits, **not** authenticated original
Source engine limits.

## Tests and protected source comparison

`test_client_soundscape_graph_owned.cpp` verifies native Windows
source-order depth-first entry, duplicate links, same-file/cross-file
statistics, missing targets, rollback on synthetic A-to-B-to-A
cycles and forged catalog metadata, a synthetic 301/242 scale case,
exponential expansion cap and deep-chain refusal. All fixture names,
waves and resources are invented, with no original scripts.

`audit_sin_original_soundscape_graph.py` adds:

- `--scripts <private-original-scripts.zip>`: read-only
  SHA-authenticated seven-file analysis and **exact table+graph
  invariants** above; outputs derived count-only metadata.
- `--self-test`: public original-free synthetic graph, duplicated
  edge, missing source, duplicate-name and cycle validation. This is
  registered separately as a CTest alongside the native Win32 test.

## Important limitations

The graph planner models only **playsoundscape dependency topology**.
It does **not** currently apply original `volume`, `pitch`,
`dsp`, `dsp_volume`, `position`, `time`, `soundlevel`,
`playlooping`, `playrandom` or `rndwave` audio semantics.

It does not pick tracks, precache assets, schedule randomly selected
wave clips, contact Source's sound emitters, select map-triggered
soundscapes, or perform real soundscape shutdown. Therefore its
structure is useful for implementing audio later, but should not
be described as a complete original C_SoundscapeSystem or as
original audio-mixing parity.

The normal independent production `VClient011::Init` still returns
zero. The last actual owner-run original-engine checkpoint remains
`0x53494E74`. The Phase 486 opt-in retail soundscape file gate
`0x53494E76` is **not yet observed**. Phase 487 has no new live
retail game exit, and cannot claim Windows gameplay, Android
gameplay or Quest 3 use.
