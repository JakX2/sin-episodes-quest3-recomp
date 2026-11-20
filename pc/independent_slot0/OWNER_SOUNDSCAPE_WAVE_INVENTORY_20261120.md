# Phase 488: owned SiN 2006 soundscape audio-source declaration inventory

## Original SHA-authenticated evidence

Read-only private analysis of the user's genuine seven original
soundscape script files from `SE1/scripts.zip` (connected Google
Drive) verifies **all seven individual SHA-256 hashes** against the
existing original-file constants and validates the original
manifest hash. The public repository includes only derived
metadata, original-free synthetic fixtures, and independently
authored parser/owner code.

Exact original source counts:

| Original resource observation | Count |
| --- | ---: |
| Soundscape definitions | 301 |
| Definitions with at least one directly declared wave | 284 |
| playlooping groups | 281 |
| playrandom groups | 150 |
| rndwave blocks | 150 |
| Direct looping wave declarations | 281 |
| Random candidate wave declarations | 943 |
| **Total raw wave occurrences** | **1,224** |
| Unique case-insensitive raw wave tokens | 510 |
| .wav occurrences | 782 |
| .mp3 occurrences | 442 |
| Raw values starting with `*` | 449 |
| Raw values starting with `)` | 91 |
| Raw values starting with compound `*)` | 20 |
| Raw values without those leading markers | 684 |
| Smallest rndwave candidate list | 2 |
| Largest rndwave candidate list | 20 |

Original source structural checks show every one of the 281
playlooping blocks contains a direct `wave` property and every
playrandom block contains a corresponding `rndwave` block.
All 1,224 values are relative forward-slash audio asset paths
after accounting for the leading `*` and `)` characters. There
were no original path traversal segments or backslashes.

Original distribution of random candidate group sizes (number of
rndwave wave entries per random block):

| Candidates | Groups | Candidates | Groups |
| ---: | ---: | ---: | ---: |
| 2 | 1 | 9 | 1 |
| 3 | 7 | 10 | 5 |
| 4 | 30 | 11 | 1 |
| 5 | 41 | 12 | 5 |
| 6 | 18 | 15 | 3 |
| 7 | 29 | 16 | 2 |
| 8 | 5 | 17 | 1 |
| 20 | 1 | | |

Notice the original strings may begin with one or two **Source
sound-control characters**. For example, `*ambient/example.wav`
or `*)ambient/example.wav` is only an *invented illustrative
token*: `original_raw` retains the prefix exactly, while a
**separate** `asset_relative_path` for future packaging tools
exposes the file path without those markers. This separation is
critical: stripping a prefix before eventual sound-system dispatch
would change or discard Source engine semantics.

## New independent C++ data ownership

`client_soundscape_wave_inventory_owned.h` extracts ordered
playlooping and playrandom/rndwave declarations from the nested
KeyValues-like `CatalogState` built in Phase 483. It stores:

- All raw wave strings, source/definition ownership, marker prefixes
  and a conservatively validated relative asset path.
- The exact group structure, including ordered candidate wave
  references and repeated declarations.
- Every option scalar as a raw string: `volume`, `pitch`,
  `soundlevel`, `position`, `time`, `attenuation`, and any
  additional group scalar. The code does **not** parse floats or
  reinterpret the original scheduling or DSP policy.
- A complete aggregate summary and per-definition group indexes.
- A group-level `ExpandScene` that uses Phase 487's owned acyclic
  dependency walk to collect all structurally reachable sound
  source groups, including repeated dependencies. This only
  describes sources which *could be relevant*, not which sounds
  actually play.

The owning object is transactional. Malformed wave placement,
unsafe relative paths, empty groups, excessive size, or allocation
failure leave the previous good published registry intact.
`ExpandScene` similarly publishes no partial scene plan after
graph mismatch or expansion refusal.

Independent bounds: maximum 8,192 groups, 16,384 wave declarations,
128 candidate entries per group, 16,384 scene group occurrences and
65,536 scene wave occurrences. These are newly authored portable
safety policies, **not original engine limits**.

## Test and private audit split

`test_client_soundscape_wave_inventory_owned.cpp` tests all
path marker forms, source WAV/MP3 extensions, original-property
retention, ordered looping/random declarations, repeated nested
soundscape scene expansion, explicit unknown/missing names, failure
rollback and invented synthetic **301 roots / 431 groups / 1,224
wave occurrences**. It invokes **no real mixer** and uses no
copyright game audio or original script text.

`audit_sin_original_soundscape_waves.py --scripts
<private-scripts.zip>` validates and reports *only derived metrics*
on owner files after their exact SHA pins. Its public `--self-test`
constructs invented soundscape strings, checks marker validation,
rejects malformed placements and never opens original files.

## Still missing for PC, Android and Quest 3

These data owners do **not** decode or play .wav/.mp3, randomize
candidate selection, instantiate source sound emitters, register
map soundscape entities, propagate nested DSP/volume/pitch settings,
implement crossfades, or handle original soundscape shutdown.

The audio-ready data boundaries are progressing, but actual
`C_SoundscapeSystem` behavior and its engine callbacks are still
pending. Normal standalone `VClient011::Init` remains fail-closed,
and `client.dll` has **not** reached a playable retail Windows
startup. The last **observed** owner-retail checkpoint remains
`0x53494E74`; the prepared Phase 486 `0x53494E76` soundscape
read diagnostic has not been reported as a real retail result.
Android NDK and emulator smoke tests are not Quest 3 gameplay.
