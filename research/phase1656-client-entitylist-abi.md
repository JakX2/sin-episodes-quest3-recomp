# Phase 1656: original `VClientEntityList003` secondary-pointer ABI

## Provenance and scope

This phase **joins** original PE32 evidence already authenticated by the
project's Phases 226, 228, 229, 230 and 231. It does **not** claim nine new
original-function decompilations. The Phase 1655 baseline at
`beb513d07699e6abba3c294f46695217febe5422` passed 256/256 native
Win32 tests and all seven applicable CI workflow families.

The original 2006 PE32 client image base is `0x24000000`. Owner-original
entire-DLL SHA-256 must equal
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
The original server SHA must equal
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
These are read-only provenance checks, not permission to load the originals in
CI. No original DLL, compiled native method body, or user content is committed.

## Two distinct virtual interfaces

| Original object | Original VA | RVA | Meaning |
|---|---:|---:|---|
| Containing client entity list | `0x2433FDC8` | `0x33FDC8` | Parent object |
| Factory-returned entity-list interface | `0x2434FDDC` | `0x34FDDC` | Secondary subobject: parent + `0x10014` |
| Primary parent vtable | `0x242973B8` | `0x2973B8` | Separate 3-virtual table |
| Embedded secondary vtable | `0x24297390` | `0x297390` | Nine original virtuals |
| Original secondary factory thunk | `0x241056B0` | `0x1056B0` | Returns secondary, not parent |

The containing constructor writes the primary final vptr at `0x241081A2`
and secondary final vptr at `0x241081A8`. Previous owner-authenticated
audits cover the six-byte factory, both construction stores, per-cell PE32
HIGHLOW relocations, selected complete method hashes and native RET
instructions.

**New, directly authenticated Phase 1656 RTTI result:** the four-byte
inter-table word at `0x242973B4` is the primary vftable's `[-1]`
Microsoft C++ Complete Object Locator pointer, not a tenth secondary
virtual. The original RTTI is independently consistent with the separate
secondary subobject:

| RTTI evidence | Original virtual address | Verified fields |
|---|---:|---|
| Secondary vtable[-1] pointer | `0x2429738C` | `0x242E59D4` |
| Secondary Complete Object Locator | `0x242E59D4` | signature 0, offset `0x10014`, cdOffset 0 |
| Primary vtable[-1] pointer | `0x242973B4` | `0x242E5A50` |
| Primary Complete Object Locator | `0x242E5A50` | signature 0, offset 0, cdOffset 0 |
| Shared TypeDescriptor | `0x242FA77C` | MSVC name `.?AVCClientEntityList@@` |
| Shared ClassHierarchyDescriptor | `0x242E5A40` | Relocated from both locator records |

Both vtable[-1] and both COL descriptor pointer fields carry PE32
HIGHLOW relocations. These findings were checked against the exact
SHA-matched owner originals from the Drive `SE1/bin.zip` archive,
without executing any original code.

**Additional original MSVC class hierarchy evidence:** the shared
ClassHierarchyDescriptor at `0x242E5A40` is the complete four-DWORD record
`(signature=0, attributes=1, numBaseClasses=3, pBaseClassArray=0x242E5A30)`.
The `attributes=1` bit identifies **multiple inheritance** in MSVC RTTI.
The array at `0x242E5A30` contains three relocated descriptor pointers:

| Original BaseClassDescriptor | RTTI type name | PMD member displacement |
|---|---|---:|
| `0x242E5A18` | `CClientEntityList` | `0` |
| `0x242E5A00` | `CBaseEntityList` | `0` |
| `0x242E59E8` | `IClientEntityList` | `0x10014` |

All three 24-byte BaseClassDescriptors have `pdisp=-1`, `vdisp=0`,
and `attributes=0`. Their TypeDescriptors are respectively
`0x242FA77C`, `0x242FA75C`, `0x242FA73C`.
The containing class reports two contained bases; each base reports zero.
Each class-array element and base-descriptor type pointer is a genuine
PE32 HIGHLOW relocation. The `IClientEntityList` PMD independently
matches the factory's secondary address and the constructor's offset.
**This is direct original native RTTI verification, not the SDK's inferred
class layout.**

Original whole 36-byte secondary vtable SHA-256:
`85b1a89b4c59052c027286beed20e00242cbdf428fa1559dbd666a51fe18be75`.
Original whole 12-byte primary vtable SHA-256:
`f4ccb17ffb03a35df448ebfe6b7fcb29fec20cff98f7403121ac0bc8216bd8d3`.

### Secondary virtual order

| Slot | Original target VA | Callee stack cleanup | Evidence source |
|---:|---:|---:|---|
| 0 | `0x241056F0` | 4 | Phase 229 |
| 1 | `0x24105BB0` | 4 | Phase 230 |
| 2 | `0x241074D0` | 4 | Phase 230 |
| 3 | `0x24105AF0` | 4 | Phase 230 |
| 4 | `0x24105BE0` | 4 | Phase 230 |
| 5 | `0x24105700` | 4 | Phase 229 |
| 6 | `0x24105740` | 0 | Phase 229 |
| 7 | `0x24105720` | 4 | Phase 229 |
| 8 | `0x24105730` | 0 | Phase 229 |

The primary table's three separate targets are `0x241089D0`,
`0x24108A70`, and `0x241082A0` (Phase 231). The last is a primary
deleting destructor, **not** secondary slot 0.

## Added regression layers

1. **Owner-only static PE32 audit**
   (`research/tools/audit_sin_original_client_entitylist_secondary_abi.py`):
   refuses files with either wrong entire SHA; checks all nine secondary and
   all three parent pointers against previous independently authenticated
   phases, relocated factory and constructor pointer operands, both
   original 20-byte RTTI Complete Object Locators, three exact
   BaseClassDescriptors and hierarchy flags, shared descriptor names,
   two whole vtable SHA-256s, and separate runtime virtual `.data` objects. Reuses all earlier whole-method hash audits. Does not execute
   original game code.
2. **Portable original-free mutation self-test**:
   invented pointer windows, one-byte corruption of all 12 table cells,
   swap/truncation/extra-slot errors, forged factory return, constructor
   RTTI COL, hierarchy and base-descriptor mutations, and fake original-SHA refusal.
3. **Native MSVC x86 ABI fixture**:
   compiler-generated, independently owned primary and secondary objects;
   verifies +`0x10014` identity, all nine actual vtable positions,
   `ECX`-receiver identity and `ESP` callee cleanup for seven one-DWORD
   and two no-DWORD entries. No retail DLL loading.
4. **Read-only owner diagnostic**:
   a bounded injected-memory adapter checks the loaded original client
   parent's and secondary object's vptrs and all 12 function pointer
   targets, with no vcall. A separate native test falsifies each of the 14
   reads one at a time (2 vptrs plus 9+3 vtable slots). The owner diagnostic
   continues to require exact original SHA and explicit disposable-copy
   confirmation; original `DllMain` / constructor side effects can occur
   only in explicit owner live mode.

The three new Windows CTests target a **259/259** suite; they and the
portable decomp self-test must pass final-head GitHub Actions before any green
claim. The ZIP package itself is rebuilt by Actions and includes no game DLLs.

## Safety and the next boot gate

**Do not register `VClientEntityList003` in the independent
`client.dll` yet.** The original factory-returned subobject is no longer
an unresolved *shape*, but real entity ownership, handle-generation
lifetimes and all engine callbacks have not been independently proven.
Returning a structurally correct dummy pointer would still be unsafe.

This phase is a compiler-ABI and read-only diagnosis milestone, **not**
proof that original SiN Episodes Windows game modules load or that Android
or Quest 3 gameplay works. The next decisive result is an owner-run
SHA-matched factory readback and subsequent verified replacement-module
initialization against the real Windows engine.
