# Phase 1724: original full executable-body fingerprints for all 191 ClientClass creators

Unchanged original SiN Episodes (2006) PE32 client.dll whole SHA256:
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

## All original creator executable bodies are now source-authenticated

Every one of the original **191** registered ClientClass creator
functions has an original function boundary and full executable-body
SHA256 fingerprint. The source split is:

| Original source creator family | Functions | Original source evidence |
| --- | ---: | --- |
| Static temp-event getters | 42 | Complete 6-byte methods, Phase1717 |
| Canonical entity methods, 60-byte | 68 | Whole code/template SHA, Phase1722 |
| Canonical entity methods, 86-byte | 50 | Whole code/template SHA, Phase1722 |
| Noncanonical common-allocator entity methods | 24 | Individual whole-method SHA, this phase |
| Exceptional entity methods | 7 | Individual whole-method SHA, Phase1721 |
| **Total** | **191** | **14,868 original executable bytes** |

All 191 complete original source-function bodies concatenated in
original authenticated ClientClass linked-list order have SHA256:
`c62a23ee2494ee996a00fb77745f734a821993f86546774b6d43669095767d5b`.

This result is cryptographic **source-integrity coverage**, not
complete portable implementation of 191 original functions.

## The remaining 24 complicated bodies

An original-source scan identified 24 common allocation methods
whose function body did not match the two simple 60/86-byte
constructor templates. Their whole function lengths range from
92 to 1,000 bytes, and source native trailing INT3 runs from
4 to 15 bytes. Each remains source-proven to begin with the
original 11-byte common allocation request into `0x240C4FF0`.

The SHA of the original 24 complete bodies concatenated in
ascending original class ordinal is:
`9633b2d099c8882f69e517be7700b5d1d7a323655f1954ea8cdfc4790238f6c9`.

The source-only CSV retains all 24 original class names, creator
addresses, allocation sizes, complete executable method lengths,
original function padding and full-function SHA256:
`research/manifests/original_client_24_complex_creator_bodies_phase1724.csv`.

LF CSV SHA256:
`7d11b2d9eb4ad1c22d399cbad38fe8d9ce8c65e44a55c0d670f0ff732914a2d2`.

These functions include the original keypads, physics props,
doors, environment controls, corpse and dynamic-light creators.
The particularly long CInfoLightingRelative function has a
1,000-byte original source body. These bodies need individual
constructor/virtual dispatch reconstruction; their method
fingerprints do not make those side effects safely callable.

## Source-only compiler inventory and original-private audit

`include/sin_compat/original_client_all_creator_body_fingerprints.hpp`
provides an immutable C++17 classification of all 191 original
source creator method bodies by known family and original code size.
The 24 noncanonical records are explicitly distinguished from
the 118 canonical entity creators, 7 special entities and
42 static temporary-event getters. Wrong class/creator identities
are refused. No original x86 PE32 address is published as a
host executable function pointer.

Public source-only audit:
`python research/tools/audit_sin_original_client_all_creator_body_fingerprints.py --self-test`

Owner-private unchanged DLL audit:
`python research/tools/audit_sin_original_client_all_creator_body_fingerprints.py --client /private/client.dll`

Private original-mode tests the whole original DLL SHA, complete
118 canonical method opcode/template SHA via Phase1722, all
24 full noncanonical per-method SHA/RET/INT3 boundaries, all
seven complete exceptional method SHA, all 42 exact native
static getter bytes, and the 14,868-byte concatenated whole
191-function SHA in actual original linked-list order.
Original executable code is read-only and not exported.

Native test:
`tests/test_original_client_all_creator_body_fingerprints.cpp`.
Validates 42+68+50+24+7=191, 14,868 total original function
bytes and correct original source creator identity. Crucially,
all host-owned 191 native32 ClientClass objects keep their
original entity creator (+0) and temp-event creator (+4)
callback fields **NULL**. No real engine callbacks are invoked.

Linux strict C++17 and ASan/UBSan, Windows Win32 x86 and
Android CMake builds include the original-free reference code.

## Actual porting and networking status

The project now has integrity fingerprints for every 2006 PE32
registered creator entrypoint, and selected entrypoint semantics.
But source fingerprint coverage is NOT disassembly to portable
C++ source, native 2006 object vtable reimplementation, actual
allocator ownership, constructors/destructors, RecvProxy
network engine ABI or serializable gameplay behavior.

`VClient011.slot5` remains deliberately guarded and cannot
return a host-executable native ClientClass registry. The original
game files were not overwritten, and this phase does not prove
Windows retail map loading, Android/Quest 3 gameplay, renderer
or physics implementation.
