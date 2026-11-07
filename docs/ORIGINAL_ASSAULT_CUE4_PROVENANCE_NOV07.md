# Original SiN Episodes server.dll: assault cue-4 provenance and CStatAdvisor RTTI

**7 November 2026 | Owner-provided original Windows PE32 `server.dll`**

Pinned SHA-256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.

## New original binary finding

The disassembly instruction at **`0x222C4D2B`** is:

```asm
mov dword ptr [ecx+0x20], 4
```

A bare offset match previously made this a tempting candidate for the
unknown `CAI_AssaultBehavior.m_ReceivedAssaultCue == 4` producer. Direct
RTTI/vtable provenance shows **it is part of a `CStatAdvisor` virtual
method**, not a proven assault-cue writer:

| Evidence | Original x86 VA | Actual witness |
| --- | --- | --- |
| RTTI type descriptor | `0x2252191C` | `.?AVCStatAdvisor@@` |
| CompleteObjectLocator pointer immediately before vtable | `0x2245CBD0` | `0x224CA970` |
| CompleteObjectLocator type-descriptor pointer | `0x224CA970 + 0x0C` | `0x2252191C` |
| Verified CStatAdvisor virtual entry | `0x2245CBF8` | `0x222C4D00` |
| Adjacent virtual entries | `0x2245CBFC`, `0x2245CC00` | `0x222C4D40`, `0x222C4D80` |
| Value-4 field write inside first method | `0x222C4D2B` | `C7 41 20 04 00 00 00` |

The same short `CStatAdvisor` vtable-method neighborhood also
writes `+0x20 = 3` at `0x222C4D1A`, `0` at `0x222C4D6A`, and
`2` at `0x222C4D80`. This is evidence of a **different class's
state field**, not of cue values changing in `CAI_AssaultBehavior`.

## AssaultBehavior writes verified separately

Original `CAI_AssaultBehavior` RTTI type descriptor is at
`0x224E4C34`, with `.?AVCAI_AssaultBehavior@@` at `0x224E4C3C`.

- `0x220CFFA0`: original `ai_goal_assault::InputBeginAssault`
  writes received cue `1` to behavior `+0x20`.
- `0x220CEAC1`: original cue-mode-`3` handler writes received cue `3`.
- `0x220CE539`: generic helper at `0x220CE520` writes its caller's
  integer argument to behavior `+0x20`, then tail-jumps at
  `0x220CE547` to original `SetCondition` at `0x22039760`.
- `0x220CEA93`: original cue predicate tests received cue `4`
  as a success override. The predicate's existence does not
  establish an actual producer for that value.

An exhaustive **bytewise** scan of the original `.text` section found no
E8/E9 relative CALL/JMP encodings targeting `0x220CE520`. A separate
literal-pointer search of `.rdata` and `.data` found no 32-bit address
equal to `0x220CE520`. These are narrow observations, **not proof the
helper is unused**: indirect vtable dispatch, computed addresses,
engine callbacks and other execution paths remain unverified.

## Reproduce without changing the binary

```bash
python3 research/tools/audit_sin_assault_cue4_provenance.py \
  --server /path/to/original/server.dll
python3 -m unittest discover -s research/tests \
  -p 'test_audit_sin_assault_cue4_provenance.py' -v
```

The verifier checks the **real DLL hash before PE inspection** and
fails closed on changed RTTI, vtable pointers, selected x86 writes,
the generic setter tail target, or newly found direct/static
setter references. It prints only JSON research metadata.

The synthetic tests deliberately mutate all important witnesses;
they do not embed or require the commercial original DLL. The
original DLL and game assets remain outside this repository.

## Boundaries and next trace

**Proven:** the `0x222C4D2B` literal write belongs to a
`CStatAdvisor` virtual function; two assault-specific constant
writes and one parameterized write are separately identifiable.

**Not proven:** that `CAI_AssaultBehavior` ever receives a `4` in
the released campaign, what calls the generic cue setter, or what
runtime event produces the override. No reconstructed engine
sensors, NPC factories, map execution, original source code, or
Quest 3 playability is implied.

Useful next step: identify **typed callers of the parameterized cue
setter via object provenance**, including indirect dispatch, rather
than matching unrelated `[register+0x20]` writes.
