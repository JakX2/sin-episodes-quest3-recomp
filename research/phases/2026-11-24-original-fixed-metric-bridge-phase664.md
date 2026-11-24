# Phase 664: Original live metric slots and nonadaptive skill adapter

## Objective

Carry the user's Phase663 opt-in **Easy / Normal / Hard fixed-difficulty
replacement** closer to the real SiN Episodes 2006 server runtime by
reconstructing the game's original indexed **writable gameplay-metric
storage contract**. This phase explicitly does not decompile HL2-equivalent
physics/surfaces/models/rendering and does not invent a working Source
`ServerGameDLL003::DLLInit` implementation.

All original executable images remain in the owner's private local
environment. No proprietary DLL or complete original functions are
committed publicly.

## Independently authenticated original 2006 game.dll evidence

The exact original 2006 retail `server.dll`
SHA256 is
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
and paired original `client.dll`
SHA256 is
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

**Four complete original x86 methods, 79 executable bytes**, have
now been pinned and checked against the original retail file:

| Original VA | Complete bytes | Whole-method SHA256 | Observed function |
|---|---:|---|---|
| `0x222D3340` | 21 | `4a72161a3ebd7f8c6a332b2debc57fc659ca099ad4e49ddbd9f5e00ca89706e9` | Compute `&record[index]` |
| `0x222D3360` | 12 | `7a5bf78b22450cc27c7c160ceb0908227130c4eacf6726b6870fa8b2e78b396b` | Return indexed original C-string name |
| `0x222D33E0` | 21 | `38aa064ed75ca9b3f73f1a410f40f1c01182184eec3414ffdf2cb67e71ed3635` | Read indexed current float |
| `0x222D3440` | 25 | `0b9a04060f10abc14883ac86728db7b7c4d5246e4d4897bb70b553296bcc512f` | Write indexed current float |

All four original code regions end with RET followed by INT3.
These functions are additional exact original evidence, distinct from
Phase78's 392-byte static metric data/pointer-table audit.
Phase664's owner-retail verifier chains Phase78 and independently
checks these four complete SHA256s, exact x86 instruction
displacements and all 14 original current-value and indexed
name table entries.

The real original source algorithm is:

```text
Original preferred PE32 module base: 0x22000000
Image size:                     0x00628000
Original 14-record table VA:     0x22523478
Record-table RVA:               0x00523478
Original name-pointer table VA:  0x2245F04C
Name-table RVA:                 0x0045F04C
Record stride:                  28 bytes (seven IEEE-754 f32 fields)
Current metric field:           record+0 (first float)
Original indexed pointer:       module_base + 0x523478 + index*28
Original indexed name:          *(module_base+0x45F04C+index*4)
```

The native x86 indexed accessor and setter do **not include bounds
checks**. The new adapter deliberately enforces `0<=index<14` and
the loaded module's exact image size. A different Source game DLL,
independently rebuilt game DLL, ARM64 game layout, or stale binary hash
must **not** be treated as if it shared these preferred-image RVAs.

The PE32 relocation issue is critical: the observed VAs above are
from the original image preferred base, but the real Windows loader
can map it elsewhere. The runtime bridge takes the **actual loaded
module base** as an argument, then uses stable RVAs; it never assumes
the process loaded the DLL at `0x22000000`.

## Real host integration contract

`OriginalMetricBridge` in
`include/sin_compat/fixed_metric_bridge_phase664.hpp` and
`src/fixed_metric_bridge_phase664.cpp` accepts two existing
components:

1. Phase663 `FixedDifficultyRuntime` with the user-selected fixed
   skill (1=Easy, 2=Normal, 3=Hard).
2. A mandatory `IOriginalGameHost` which the eventual engine or
   replay runtime must implement with **real PE32 memory reads/writes
   and the actual Source `skill` and `dd_enabled` ConVars**.

All original metric pointer computation uses the pinned strided
layout. Before changing anything, the new code:

- Verifies a page-aligned, non-overflowing PE32 module range.
- Checks every relocated name-pointer-table entry is inside that
  image and that every whole NUL-terminated exact-case name matches
  the independently authenticated original 14-name table.
- Snapshots all 14 current float fields, bit-for-bit, so an original
  savegame/advisor may have modified them before the game host
  selects a difficulty.
- Reads the actual `dd_enabled` ConVar, sets it to zero, verifies
  its readback, then selects and verifies `skill` 1/2/3 in the
  host engine. Phase663's authenticated x86 gate at
  `0x222BC47F` demonstrates that disabled `dd_enabled` skips at
  least the native update call `0x222BC0E0`. More indirect paths
  may exist; this is not a claim all adaptivity disappears.
- Writes **all 14 original mutable float current fields** in
  original index order, verifying each new float bit pattern by
  reading it back. The field endpoints and names remain source
  authenticated; profile values remain explicitly new balance policy.
- Only after 14 verified writes publishes the user-selected
  Phase663 policy state. This happens even if skill is unchanged:
  loading an original savegame could have restored stale adaptive
  values into all 14 positions.

Failures do **not** claim atomic memory or ConVar transactions. An
error attempts reverse-order rollback of every touched float and
reports how many writes were actually verified. The real host must
**abort startup or gameplay restore** on any failure, even when all
float rollback writes succeed; `dd_enabled` or `skill` might
already have changed in the engine. This is a new portable-safety
policy, not a decompilation of original native error handling.

This adapter does **not** blindly write to fixed retail DLL virtual
addresses, bypass ConVar protection, run unknown virtual methods or
claim access to a real Source engine game thread. The engine host
must guarantee trusted binary provenance, module lifetime, and
game-thread access. Reconstructed future server DLLs should implement
the same indexed 14-value interface with owned storage, not imitate
the original PE32 absolute addresses.

## Genuine original owner-side Windows read-only evidence

The already compiled SHA256-gated Win32 retail factory diagnostic
`research/tools/sin_win32_factory_probe_phase657.cpp` now adds:

- One JSONL `phase664_metric_current` record for each of the
  **14 original 2006 indexed metric float locations**, including
  its expected original name, observed current raw float32 bits and
  original static initializer raw bits.
- One JSONL `phase664_metric_snapshot` summary with name-match
  count, initial-current match count, and the readable effective
  `dd_enabled` integer from the original ConVar parent pointer
  at RVA `0x5923A8` plus field `0x2C` when available.

The original SHA gate prevents reading arbitrary DLL layouts as
retail SiN. It uses `ReadProcessMemory` for every read and rejects
mismatched name pointers without printing raw process addresses.
It **does not write game memory, call game virtual methods or
invoke engine `GameDLLInit`**. Original LoadLibrary/DllMain and
original game-process engine startup are not equivalent.

The same owner-side probe remains uploaded by the Windows CI
artifact `sin-pc-win32-original-factory-probe-phase657`.
After downloading and extracting the artifact, the owner can
check the original DLLs locally with:

```powershell
.\sin_pc_original_factory_abi_probe_phase657.exe "C:\SiN\bin\client.dll" "C:\SiN\bin\server.dll"
```

The command's effect is read-only after trusted retail DLL load.
It may still fail to load game DLLs outside the actual Source
engine because required runtime imports or interfaces are absent;
such a failure is **not** evidence the original game cannot run.

## Regression coverage and evidence

- `tests/test_fixed_metric_bridge_phase664.cpp` compiles actual
  Phase663 fixed profile with Phase664 original metric adapter.
  Its synthetic game host maps all 14 exact-case original names,
  seven-float record strides, and relocated pointer table.
- Validates original name matching, image bounds, stale pointer
  rejection, invalid skill and module VA, bit-exact normal/current
  values, the independent read/write ConVar gate, true republish
  after savegame data overwrite, partial write failures,
  wrong write-readback values, host exceptions, rollback
  accounting, and reentrant host callbacks.
- `research/tools/audit_original_fixed_metric_bridge_phase664.py`
  verifies the unchanged owner retail binaries' original indexed
  method bodies, code/table hashes, raw floats and names. The
  public `--self-test` works only on invented data.
- Native MSVC Win32 x86, Linux strict C++17, Clang ASan/UBSan
  and Android build/smoke regressions are run in GitHub Actions.
  A passing CTest proves only source-model compatibility.

## Remaining real integration priorities

1. Implement an actual engine-owned `IOriginalGameHost` in the
   selected Windows Source runtime. Verify the original DLL SHA256
   before interpreting PE32 RVAs, and ensure the current module is
   the trusted original 2006 server.
2. Run the SHA-gated read-only probe with the owner's private
   original retail DLLs. If the engine-independent LoadLibrary
   cannot satisfy imports, collect the evidence from an actual
   engine-hosted process or a purpose-built runtime test rather
   than bypassing the fingerprint gate.
3. Run real combat/map tests with `skill` 1/2/3 and original
   `dd_enabled=0`; check direct AND indirect difficulty calls,
   damage, accuracy, NPC spawn counts, helmet/weapon variations,
   health drops, savegame restore, and original
   `trigger_dynamic_difficulty`, `info_challenge` and secret.
4. On Android/Quest 3, replace retail PE32 VA binding with the
   independent server game's 14-value indexed storage contract.
   This phase **does not produce playable native game modules**.

This phase strengthens the actual bridge between the recovered
difficulty metrics and the original Windows module without claiming
that an unknown engine host or original retail game run exists.
