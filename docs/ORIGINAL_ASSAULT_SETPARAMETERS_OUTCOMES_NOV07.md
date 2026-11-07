# SiN Episodes (2006) assault SetParameters: three distinct cue-state exits

**7 November 2026. Direct original x86 server.dll evidence.** SHA-256:
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.

## Finding

The native diagnostic identifies `CAI_AssaultBehavior::SetParameters`
at `0x220CFC00`. Both the goal actor configuration hook
(`0x220CFF27`) and direct MobGrunt `Assault(string)`
(`0x22357E8B`) call this original function.

Its selected **control-flow outcomes are not equivalent**:

| Path | Original instruction evidence | Effect on behavior cue fields |
| --- | --- | --- |
| No matching/eligible rally point | `0x220CFC67` and `0x220CFE22` to `0x220CFE26`, unconditional exit at `0x220CFE3E` | No write to `configured_mode (+0x1C)` or `received_cue (+0x20)` on this exit |
| Candidate chosen, but selected handle missing or unresolved | Handle stored at `0x220CFE5A`, invalid guard `0x220CFE5D`, branch to `0x220CFE81`, zero write `0x220CFEA0` | Configured mode is explicitly **0**; reset helper is not called |
| Rally link resolved | `0x220CFE7F` to `0x220CFED4`, argument load `0x220CFED4`, mode store `0x220CFED8`, direct call `0x220CFEDD` | Store caller's requested mode, then invoke `0x220CE960`; helper zeros received cue at `0x220CE979` |

The success helper also clears state bytes `+0x24`, `+0x25`,
`+0x26`, the active assault point handle `+0x14`, and
one state field `+0x28`. The member names of several of these
fields are **not recovered**. Their original x86 offsets are not
valid ABI layouts for Quest 3.

A selected but invalid rally handle may trigger an additional
base-NPC fallback around `0x220CFECA`; this reference deliberately
models **cue fields only** and does not claim that the entire
failure path is side-effect-free.

## Reconstructed port-facing contract

`include/sin_compat/original_assault_setparameters_outcomes.hpp`
provides a **new host classification**:

- `NoRallyCandidate`: leave the portable cue fields alone;
- `UnresolvedRally`: configured cue mode becomes zero, while
  already received cue remains unchanged;
- `ResolvedRally`: configured cue becomes the requested value,
  received cue resets to zero.

`configure_original_assault_parameters` invokes a host callback that
returns one of these three outcomes. This enum and callback are
**new testable port abstractions**, not the original Ritual source
or a verified return type of `SetParameters`. The earlier
boolean goal-setup adapter stays available for existing tests.

That distinction matters for an eventual `npc_sintek_grunt`
Source-compatible implementation: a failed selected handle must not
automatically be treated the same as no candidate, and successful
reconfiguration must not carry a stale `BeginAssault` cue forward.

## Reproduce

```bash
python3 research/tools/audit_sin_assault_setparameters_outcomes.py \
  --server /path/to/original/server.dll
python3 -m unittest discover -s research/tests \
  -p 'test_audit_sin_assault_setparameters_outcomes.py' -v
cmake -S . -B build && cmake --build build
ctest --test-dir build -R sin_original_assault_setparameters_outcomes_tests \
  --output-on-failure
```

The Python tool verifies **26 original instruction witnesses, five
relative control-flow branches, three direct native call targets, and
an original native diagnostic literal**. Its 12 synthetic regression
tests cover corrupted branches, wrong cue offsets, a wrong initializer
target, changed caller target, changed diagnostic name, and SHA failure.
The C++17 reference adds explicit behavior checks.

**Limits:** no original commercial DLL or BSP is committed, no SourceVR
NPC factory or navigation exists here, and no running Quest 3 game is
claimed. The original binary is used only as local read-only evidence.
