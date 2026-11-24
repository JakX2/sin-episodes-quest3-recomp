# Phase665: original CInfoChallenge map I/O and fixed-difficulty reporting

## Scope and provenance

Continues the owner's choice to replace adaptive difficulty with selectable
Half-Life-style fixed skill. Retain map-facing entity contracts; do not delete
original entities or claim an original Source engine boot.

The exact owner original 2006 PE32 server SHA256 is
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
The paired client SHA256 is
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
Original executable images remain private.

## Original entity identity and layout

- Static factory registration `0x223C3ED0`, original literal
  `info_challenge` at `0x224686C8`, factory object `0x22527CBC`,
  factory vtable `0x2246831C`, exact RTTI
  `.?AV?$CEntityFactory@VCInfoChallenge@@@@`.
- Factory allocation size **0x3E0 bytes**, constructor `0x222F4610`
  installs entity vtable `0x2246843C`.
- Datamap class string `CInfoChallenge` at `0x22468308`.
  Three original initialized input records at `0x22527D2C`,
  `0x22527D60`, `0x22527D94`, each 52 bytes.
- Entity virtual slot byte offset `+0x64` points to `0x222F4340`.
  This routine stores the snapshot from `0x222D3630` at entity
  offset `+0x3DC`. Its precise engine lifecycle invocation is not
  yet established by an observed retail process.

| Map input | Native handler | Output | Original field |
|---|---|---|---|
| GetInitialChallenge | 0x222F4390 | OnInitialChallenge | +0x394 |
| GetCurrentChallenge | 0x222F43B0 | OnCurrentChallenge | +0x3AC |
| GetChallengeDifference | 0x222F43E0 | OnChallengeDifference | +0x3C4 |

All three handlers pass the entity itself as both activator and caller
to output helper `0x2212DC30`. Its underlying engine output/event
implementation remains a typed host dependency.

## Three distinct value sources

The original snapshot method calls `0x222D3630` and stores its float
result. The initial-output handler instead calls singleton accessor
`0x222BC4A0` and reads the returned manager's DWORD at offset +8 as the
output value. It does not simply return the entity snapshot.

The current-output handler calls `0x222D3730`. The difference handler
calls the same routine, subtracts the stored snapshot, and multiplies
by the original float constant **100.0** at `0x223D6314` before
the final float32 store.

The nonempty aggregate path in `0x222D3730` also multiplies its own
aggregate by 100, unlike `0x222D3630`. The apparent difference in
units must not be silently repaired in the original-reference model.
The two aggregate helpers are pinned and authenticated but not
reimplemented here. In particular, their real advisor inputs and
the relationship to manager+8 remain external requirements.

## New source policy

`include/sin_compat/info_challenge_phase665.hpp` supplies a
source reference for the snapshot method and all three input handlers.
`IChallengeValues` exposes the three original distinct value sources;
`IChallengeOutput` preserves exact output field/name/self dispatch.
The model uses a wider intermediate for the native arithmetic without
claiming full x87 exception/rounding-state equivalence.

The separate opt-in `FixedChallengeValues` adapter makes a **new**
normalized reporting choice: Easy=0, Normal=0.5, Hard=1. Both snapshot
and current value use that same normalized scale. The initial value
captures skill at adapter construction (the host must instantiate it
at game start); the current value follows explicit skill selection.
A stable skill gives zero difference; an explicit change from Hard
to Easy gives -100. This is deliberately not a claim of original
retail output scaling. The actual BSP output targets/thresholds and
gameplay meaning must be checked before enabling this policy in a
real campaign.

The adapter is not registered in a live engine factory. Actual
Source entity creation, savegame format and game output consumers
remain separate work. Snapshot restoration is host-owned; it is not
the original datamap serializer.

## Audit and regression

`research/tools/audit_original_info_challenge_phase665.py` pins eight
complete methods totaling **679 original native bytes**:
factory registration, constructor, snapshot, three I/O handlers,
and two aggregate dependencies. Eight direct call edges, three
initialized datamap input records, original RTTI, output offsets,
snapshot field, and scale constant are checked. The full 679 bytes
are authenticated research coverage, not 679 bytes of equivalent
decompiled source.

Private original verification independently confirmed the whole-method
hashes, input descriptors, eight calls, field offsets and scale
instruction witnesses. No original DLL is uploaded to public CI.

Public tests link the actual Phase663 fixed policy, verify original
distinct values (e.g. current=40, snapshot=0.2 yields 3980), output
names, offsets and entity self-caller/activator, unsupported inputs,
snapshot restore, provider failure, nonfinite-value safeguards,
output failure, callback exceptions and reentry. Fixed-mode tests
verify no adaptive drift despite repeated performance/trigger reports.

Run public original-free audit:
```sh
python research/tools/audit_original_info_challenge_phase665.py --self-test
```

Run original authentication privately:
```sh
python research/tools/audit_original_info_challenge_phase665.py --server ORIGINAL_SERVER.dll --client ORIGINAL_CLIENT.dll
```

No real retail gameplay, replacement game DLL, map output threshold
validation or Quest 3 execution is claimed.
