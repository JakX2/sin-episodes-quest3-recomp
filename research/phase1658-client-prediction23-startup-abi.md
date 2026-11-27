# Phase 1658: VClientPrediction001 23-slot PE32 startup ABI

## Verified original provenance

This phase is a **read-only original DLL audit**, synthetic MSVC x86 ABI
fixture, and owner-only factory readback extension. Stacked on Phase1657
PR #1666, SHA `3698ebbdfe2236ed7e1f04b0219f19585d76aa63`. It does
**not** register an incomplete prediction interface, claim engine boot,
or recover all 23 original native method implementations.

The original retail files were read statically, with full SHA-256:

| Original retail PE32 file | Entire file SHA-256 |
|---|---|
| `client.dll` | `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d` |
| `engine.dll` | `f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81` |

No original game DLL, native code bytes, proprietary resources, or engine
memory contents are committed.

## The earlier original engine client startup gate

The original `engine.dll` at VA `0x20023CF2` requests
`VClientPrediction001` through the saved original client
`CreateInterface` factory (global pointer `0x203E3F04`).
It stores the result at `0x203E4014`, and at `0x20023D21` calls
the returned prediction object's **virtual slot 1**, through
`[vptr+4]`, with `ECX=this` and zero stack arguments.

The *very next* factory request begins at `0x20023D24` and asks for
`VClientEntityList003`. The following requests are for
`VCENTERPRINT002` and `ClientLeafSystem001`.
Therefore Phase1657's entity-list progress alone cannot cross the first
missing client factory boundary. The original engine may still crash
if a missing prediction singleton is dereferenced after error logging.

This is source evidence from the original **50-byte** engine callsite
window, SHA-256
`51531be82672348be6b725dedab1145cc085db88498634fc9e19add2a069ac22`.
It is NOT a successful owner Windows startup test.

## Original prediction registration and object

| Original client symbol or object | VA | Backing |
|---|---:|---|
| `VClientPrediction001` string | `0x242C6824` | `.rdata` |
| Static InterfaceReg constructor wrapper | `0x24271450` | `.text` |
| Original InterfaceReg storage | `0x2437FA34` | virtual `.data` |
| Six-byte factory thunk | `0x2422EA30` | `.text` |
| Original prediction singleton | `0x2437FA00` | virtual/unbacked `.data` |
| 50-byte prediction constructor | `0x24195A10` | `.text` |
| Complete prediction vtable | `0x242A363C` | `.rdata` |
| vtable[-1] RTTI COL pointer | `0x242A3638` | `.rdata` |

The on-disk singleton **has no file-backed initial vptr**. Its native
constructor stores `0x242A363C` into its primary vfptr, then initializes
member fields. Reading an arbitrary on-disk DWORD at `0x2437FA00`
would manufacture runtime evidence.

The complete original vtable has 23 contiguous PE32 function pointers,
exactly 92 bytes, SHA-256:
`17c4d590cbf557162939e041589450821200671285adf245d3d3f456350806de`.

| Slot | Original target | Interpretation authenticated in Phase1658 |
|---:|---:|---|
| 0 | `0x24195A50` | MSVC scalar deleting destructor, RET 4 |
| 1 | `0x24195F80` | Parameterless Init, RET |
| 2 | `0x24195710` | **No-argument empty RET**, engine shutdown virtual slot 2 |
| 3 | `0x24197210` | Target pointer authenticated only |
| 4 | `0x24195F90` | Target pointer authenticated only |
| 5 | `0x24196040` | Target pointer authenticated only |
| 6 | `0x241960E0` | Target pointer authenticated only |
| 7 | `0x241957A0` | **No-argument 16-byte reset**, fields +0x24/+0x28/+0x2C |
| 8 | `0x24195DD0` | Target pointer authenticated only |
| 9 | `0x24195DF0` | Target pointer authenticated only |
| 10 | `0x24195E30` | Target pointer authenticated only |
| 11 | `0x24195E80` | Target pointer authenticated only |
| 12 | `0x24195EC0` | Target pointer authenticated only |
| 13 | `0x24195F10` | Target pointer authenticated only |
| 14 | `0x24195F50` | Target pointer authenticated only |
| 15 | `0x24195970` | Target pointer authenticated only |
| 16 | `0x24195980` | Target pointer authenticated only |
| 17 | `0x24195990` | Target pointer authenticated only |
| 18 | `0x241968A0` | Target pointer authenticated only |
| 19 | `0x241964F0` | Target pointer authenticated only |
| 20 | `0x24196790` | Target pointer authenticated only |
| 21 | `0x24196AE0` | Target pointer authenticated only |
| 22 | `0x241977B0` | Target pointer authenticated only |

**Additional native method authentication:** original slot 2 at
`0x24195710` is exactly one byte, `RET`, whose whole-function SHA256 is
`ae3f4619b0413d70d3004b9131c3752153074e45725be13b9a148978895e359e`.
Original `engine.dll` invokes it in its **client shutdown path**, at
VA `0x20023DF7` via virtual byte offset `+8`; the original 11-byte
read/call window at `0x20023DEF` has SHA256
`4495aebe5d5ab5d1808a077cd1993f35112fc2a0be1388c1bff2ef5491b7438d`.

Original slot 7 at `0x241957A0` is a complete **16-byte** no-argument
reset method, SHA256
`a6da88d507f80f25c7ebde380cf57df4b325e3acaba22aef172d84d6082733b2`.
It zeroes prediction DWORDs `this+0x28` and `this+0x2C`, then stores
`0xFFFFFFFF` at `this+0x24`, followed by `RET`.
These two exact original method extents and their semantics are now
checked by static authentication and original-free mutation tests.
This does not make the other 19 unexamined method signatures safe or
constitute evidence that the game has started.

The subsequent `.rdata` bytes begin with non-pointer debug text,
not a 24th prediction method. Every pointer carries a HIGHLOW
relocation and targets the original SHA-pinned client's `.text`.

## Exact slot 1 startup dependency

Original full 16-byte slot1 method SHA-256:
`7829179302e1698746d72b1e4bdfdc51704ef8c90bd8d25ea03e6b54331eab11`.

The former anonymous dependency is now **identified directly from
the original retail client as the `cl_predict` ConVar**, not an external
engine-provided service.

Original `cl_predict` constructor wrapper `0x2426C550` registers
a ConVar at `0x24374528`, passing string `cl_predict` at
`0x242A3A04`, **default value `1`** at `0x242D6CCC`,
flags `0x200` and help string `Perform client side prediction.`.
It calls the original delegating ConVar constructor `0x241216C0`,
which invokes the base constructor `0x241212E0`.

The base ConVar constructor stores `this` at **ConVar+0x18**
(its `m_pParent` pointer) and converts/stores the numeric integer
at **ConVar+0x2C** (its `m_nValue`). Because the ConVar lives at
`0x24374528`, the absolute ConVar parent-pointer field address is
**`0x24374540`**, exactly the value loaded by prediction slot 1.

Therefore prediction slot 1 performs:
(1) load `cl_predict.m_pParent` from `0x24374540`,
(2) compare `m_pParent->m_nValue` at offset `+0x2C` to zero,
(3) write the 0/1 byte to prediction `this+0x10`, and
(4) `RET`, no stack arguments.

Original `cl_predict` wrapper full 42-byte SHA-256:
`17281254628e8b8b69d1def3b311502cc351d08583b5c92b977fc74357ad6858`.
Original 64-byte delegating ConVar constructor full SHA-256:
`e1ddea23dbd98b86e8a02f0b56494b364e1a9102e4824f466b51f1a0bcbd5196`.
Original 244-byte base ConVar constructor full SHA-256:
`510247221f012afd3817fb65b99122d8907e752d7d472ad46cf6d0a93d3ab761`.

The original prediction method itself **does not check** the ConVar
parent pointer before dereferencing it. The standalone host fixture
therefore refuses missing ConVar metadata. A live DLL bootstrap still
requires native ConVar registration and lifetime; this static evidence
does not prove an original-game runtime startup.

The native constructor (50 bytes) has SHA-256
`9b0b5c77c72ae369a286600971788b3820d79ffff740fc6c7721b3fc5277f8a7`.
Slot0 deleting destructor (31 bytes) has SHA-256
`93ddcd6e073f64faf09cd08ed68214da156d0b38db3ef3989471aab733e15111`
and ends in `RET 4` after conditional memory release.
The factory was already identified in Phase226; this phase does not
count it again or claim new whole-function decompilation credit.

## Original MSVC RTTI, distinct from entity-list layout

The original prediction vtable[-1] cell points to the MSVC
Complete Object Locator at `0x242E8E94`. This 20-byte structure
has signature 0, owner offset 0, cdOffset 0, TypeDescriptor
`0x242FDAC4` (class `CPrediction`) and shared hierarchy
`0x242E8E84`. The hierarchy attributes are 0 and it names
two base classes, `CPrediction` and `IPrediction`, both with
PMD offset 0. This is **not** the entity-list secondary
multiple-inheritance offset `+0x10014`.

## Tests and owner readback

The read-only `research/tools/audit_sin_original_client_prediction23_abi.py`
requires BOTH complete original client and engine SHA matches.
It verifies the registration, singleton factory, constructor, vtable
and every cell's PE32 relocation, slots 0/1/2/7 complete method
hashes and the engine's slot2 shutdown vcall, original `cl_predict` registration and **both original
ConVar constructors**, including self-parent and +`0x2C` integer field
stores, RTTI COL/hierarchy/base descriptors, and original engine
callsite ordering. Public `--self-test` constructs invented windows,
mutates each byte and rejects fabricated original hashes.

`native_client_prediction23_init_probe.h` is an intentionally separate
host-owned MSVC Win32 x86 synthetic 23-slot object with a deleting
destructor in slot0, parameterless Init in slot1, empty no-argument
shutdown in slot2, and reset in slot7. It checks
ECX/ESP invariants and transforms an invented **cl_predict
ConVar** `m_pParent->m_nValue` at +`0x2C` into a byte at
`this+0x10`. Synthetic `ConVarView` separately authenticates the
+0x18 parent pointer and +0x2C integer offset and tests a distinct
parent alias. Slots 3-6 and 8-22 are **inert sentinels**, not guesses
of original method signatures. Missing ConVar parent pointers are
refused in this fixture, unlike the original unguarded dereference.

A second native test exercises 24 exact fake-memory DWORD reads:
one prediction singleton vptr plus 23 method targets. The same
read-only adapter is called in the existing **owner-only** factory
diagnostic after the original DLL's complete SHA check and explicit
disposable-copy confirmation. The owner probe never invokes original
prediction slot1 or any other retail virtual method.

A third **native** observer test checks `cl_predict`'s ConVar
`m_pParent` at object +`0x18` and, **only when that parent equals the
ConVar itself**, the integer `m_nValue` at object +`0x2C`. The
snapshot is limited to **two DWORD reads**, uses PE32 overflow guards
and refuses null/unreadable/foreign parent aliases without dereferencing
them. The owner diagnostic event `original_cl_predict_convar_startup`
prints status, self-parent boolean and integer value only, never raw
addresses. This is post-`DllMain` original readback, not a game's
full console-variable registration, network update or engine boot.
The third native test exercises fake self-parent, default 1, zero,
negative integer, missing and external parent, and every unreadable
DWORD using purely invented memory.

There are now **FOUR** Phase1658 CTests (one Python synthetic original
PE32 audit, three MSVC native tests), raising Phase1657's expected
suite from 261 to **265** tests. Wait for the final commit's actual
GitHub Actions result before claiming a pass.

## Remaining startup work

- `VClientPrediction001` stays **unregistered** in the independent
  replacement DLL; all original prediction method signatures and
  lifecycle remain incomplete.
- The `0x24374540` dependency is now identified as
  `cl_predict.m_pParent`. Independently establish native ConVar
  registration/ownership and preserve its effective numeric value
  before implementing safe prediction initialization in the game host.
- No original DLL code or byte resources were committed.
- No retail Windows gameplay boot, Android native startup, Quest 3
  execution, physics, surface, prop or model implementation is claimed.
