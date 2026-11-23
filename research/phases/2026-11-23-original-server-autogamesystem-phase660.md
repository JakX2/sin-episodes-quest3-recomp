# Phase 660: Original SiN server AutoGameSystem registration and startup initialization

## Context

The recovered original 2006 `server.dll` startup routine from Phase658
calls `0x221F5700` at native callsite **`0x221E7927`**, just before
two other startup boolean gates. This is not the same as generic physics,
materials, models, props or level rendering already working when the
owner ported SiN maps into Half-Life 2.

This phase reconstructs the original **auto-game-system registration
and startup activation lifecycle**, particularly the server's pending
system linked list, dynamic pointer array, deferred registration,
and first failing virtual initializer. It does not claim a complete
replacement game DLL or actual Windows engine process boot.

The original unchanged owner-provided files are:

- `server.dll` SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- `client.dll` SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

## Original complete native executable methods: 347 bytes

| Entry VA | Original role | Complete bytes | Entire original SHA256 |
|---|---|---:|---|
| `0x221F5700` | Admit pending systems, then initialize each vtable slot zero | **277** | `1e2a6e068b24b061fbd5cba16741f8ca8f470421b7ccbca3fa0239f579a9c530` |
| `0x221F5820` | Original AutoGameSystem object registration constructor | **70** | `b3b95c759a1f9654324a0749f2a28593155a75e32270562c8ea8a557e316fe16` |

Both original methods end in a native `RET` and the next original
byte is `INT3`. The owner-retail read-only audit verifies whole-method
hashes, executable section, the native `server.dll` and `client.dll`
full SHA256s and four exact original startup/helper relative `CALL` edges,
plus 48 additional original constructor callers:

1. Server `Init` site `0x221E7927` calls `0x221F5700`.
2. Init routine `0x221F575C` calls original array grower `0x221F51A0`.
3. Native `0x221F5794` calls original overlapping-pointer move `0x223A5D10` (a present but normally unneeded middle-insert block).
4. Late registration `0x221F5848` calls original array insert `0x221F54F0`.

No original proprietary machine-code blobs or retail DLLs are committed.

## Genuine original game-system registration callers

Independent full-code `objdump` traversal of the owner's original
server PE32 `.text` identifies **48 distinct, instruction-aligned
direct `CALL rel32` sites** to `0x221F5820`, spanning original
game code between **`0x2205F073` and `0x223C7BD5`**.
This refutes a hypothesis that the original server has no
registered AutoGameSystems. The exact site VAs are pinned in
`AUTO_CONSTRUCTOR_CALL_SITES` in the Phase660 private auditor.

For example, the original direct callers include
`0x2205F073`, `0x2209A2F1`, `0x22121C40`,
`0x2219B443`, `0x221F2FA3`, `0x2221EF64`,
`0x22287F93`, `0x2234A3E3`, `0x223B72A5`,
and `0x223C7BD5`. The independent audit now verifies these
**48 full direct E8 call edges**, plus the four startup and helper
edges identified above, for **52 exact original direct calls total**.

A static callsite count is *not* a runtime object count:
some registrations can execute conditionally, may be part of
static initialization and may be deduplicated by the original
initializer. The owner-run Windows snapshot is the next source
for actual post-DllMain registration count.

### 27 actual AutoGameSystem subobject vtables, six substantive Init targets

The original constructor census contains **48 direct native calls** to
the base AutoGameSystem constructor, but several enclosing C++ classes
use **multiple inheritance**. Correct attribution requires tracking
the exact `ECX` pointer *passed into* `0x221F5820`, not merely
choosing the first derived vtable store following the call.

This is now pinned with exact original instruction bytes for:

- `0x2209A2F1`: `ECX = enclosing_object + 0x630`;
  registered subobject vtable `0x223EC6E4`, virtual Init `0x2209CE50`.
  The nearby primary vtable `0x223EC71C` is **not** the registered one.
- `0x22121C40`: `ECX = enclosing_object + 4`;
  registered subobject vtable `0x22401C0C`, virtual Init `0x22122540`,
  **not** primary vtable `0x22401C00`.
- `0x221BBC0F`: `ECX = enclosing_object + 4`;
  registered subobject vtable `0x224324E4`, virtual Init
  `0x2205F0F0`, **not** primary vtable `0x2243251C`.
- `0x223BD8F5`: the static instance similarly uses `ECX=0x22507AB4`,
  with its own `0x224324E4` vtable at that address; the enclosing
  primary vtable is stored separately at `0x22507AB0`.

These four exact constructor instances/paths and native vtable writes
are included as independent instruction-aligned witnesses in the
owner-retail auditor. The complete corrected mapping is in
`research/data/original_server_autogamesystem_vtables_phase660.json`.

The corrected **27 unique registered subobject vtables** divide as:

- **20 vtables** point to common virtual slot zero
  `0x2205F0F0`, an authenticated 3-byte
  `MOV AL,1; RET`, original whole-method SHA256
  `e18aacce29affcdddaa5f641ac1ec12552e0fe9d066e6408941b1792818b8619`.
- **One more vtable** points to `0x221AB1E0`, a *different
  address containing the same original 3-byte* `MOV AL,1; RET`.
- The remaining **six substantive virtual Init targets** are
  `0x2209CE50`, `0x220FBED0`, `0x22122540`,
  `0x221BF980`, `0x221FF7D0`, and `0x22223490`.

The private audit checks the complete original `.rdata` mapping,
all 48 rel32 constructor call edges, the exact 20/7 entry address
split, both immediate-success method bodies, the 21/6
behavioral classification, and the critical multiple-inheritance
subobject pointer/value instructions.

A vtable census does **not** guarantee which systems exist in the
runtime registry or which initializers execute in the actual original
game process. The owner-side Windows diagnostic remains necessary
to establish that independently. A passing immediate-success
initializer is not a full game-DLL startup.

## Original global memory layout

| Original VA | Type | Role |
|---|---|---|
| `0x22572CC0` | DWORD pointer | Pending singly linked list head |
| `0x22572CB8` | BYTE | Initialization-started flag |
| `0x22510A58` | DWORD | Live pointer-array base |
| `0x22510A5C` | DWORD | Pointer-array capacity |
| `0x22510A64` | DWORD | Live system count |
| `0x22510A68` | DWORD | Pointer-array cached base |
| `0x22443E74` | vtable pointer | AutoGameSystem object vtable |
| `object+4` | DWORD pointer | Next pending object (before initialization) |

The initialized flag and pending head are in unbacked original
`.data` virtual memory. The original pointer array has **DWORD
entries**, not the 12-byte nested symbol-tree record layout.

## Original constructor `0x221F5820`

Native `__thiscall ECX=object`, no arguments, returns same object
pointer in EAX. After checking the initialized byte and installing
vtable `0x22443E74` at `object+0`:

- If flag **zero**, prepend this object to pending head:
  `object+4 = old_head`, `head = object`.
- If flag **nonzero**, call the original pointer-array indexed insertion
  routine **`0x221F54F0`** with the current count, the array-owner
  at `0x22510A58`, and a reference to the new object. It does not
  relink the pending list.

The indexed insertion helper is deliberately **a typed mandatory host
operation** here. This phase does not claim to decompile its body.

## Original initializer `0x221F5700`

The initializer first walks the pending objects from current head.
For each one it scans existing live array elements for duplicate
pointer identity. Duplicate objects are **not appended**; instead the
original indirect engine diagnostic prints:

`AutoGameSystem already added to game system list!!!`

For a new object, it compares `count+1` to capacity using native
32-bit signed semantics, calls original grower `0x221F51A0` when
needed, refreshes the real array pointer, increments count, updates the
array cache, and writes the object pointer at the former count
position. The original includes an overlapping move dependency
`0x223A5D10` in its generic indexed insertion block, though normal
append index equals the prior count, so no move is necessary in
single-threaded non-reentrant execution.

After processing pending objects, native `InitAll` clears
`DWORD[0x22572CC0]`, **sets initialized byte to 1**, then calls
**virtual slot zero** of every system in the live pointer array
from index 0 upward. It checks returned AL each time. At the first
false AL it returns false **without clearing the initialized flag,
restoring the pending list or reverting successful earlier callbacks**.
If no failure occurs, it returns true.

This is a required gate for success of the original server's later
`GameDLLInit`. The exact virtual function implementations of each
game-specific system remain separate engine/gameplay dependencies.

## Source model, audit and test coverage

Added:
- `include/sin_compat/original_server_autogamesystem_phase660.hpp`
- `src/original_server_autogamesystem_phase660.cpp`
- `tests/test_original_server_autogamesystem_phase660.cpp`
- `research/tools/audit_original_server_autogamesystem_phase660.py`
- `.github/workflows/sin-phase660-server-autogame.yml`

The source model uses typed guest-memory read/write callbacks and
requires the host to provide original array grower `0x221F51A0`,
late indexed insertion `0x221F54F0`, duplicate warning and individual
system virtual slot zero. These are **not** silently implemented as
non-original code. Portable refusal of invalid guest memory, callback
failure, reentrant calls and cyclic linked lists is protective host
behavior, not claimed original x86 semantics.

Tests exercise three pending registrations and duplicate pointer
suppression; system-array relocation on growth; the precise pointer
order and cache/count write sequence; starting with no registrations;
late registration and second initialization; a virtual init failure
after earlier successes (including preserved state, no rollback);
callback exceptions; partial writes; and malformed cyclic pending
list.

A public invented-byte mutation audit checks code signatures
synthetically without loading or distributing retail DLLs:

```sh
python research/tools/audit_original_server_autogamesystem_phase660.py --self-test
```

Owner-private original authentication:

```sh
python research/tools/audit_original_server_autogamesystem_phase660.py \
  --server ORIGINAL_2006_SERVER_DLL --client ORIGINAL_2006_CLIENT_DLL
```

Strict C++17 GCC/Linux and MSVC Win32 targets plus a dedicated Clang
AddressSanitizer/UndefinedBehaviorSanitizer workflow are added. CI
results are recorded against the exact tested commit in the PR body
when complete.

## Trusted owner-side Windows runtime snapshot

The existing Phase657/659 Windows factory diagnostic is extended in
Phase660 to print a **read-only** `phase660_autogame_snapshot`
JSONL observation of the genuine `server.dll` after its static
initializers / DllMain have run.

Once both exact 2006 retail DLL SHA256 checks pass and the trusted
modules load, it safely reads the following offsets relative to the
original server PE32 module base:

- pending head RVA `0x572CC0`;
- initialized byte RVA `0x572CB8`;
- live count RVA `0x510A64`;
- current capacity RVA `0x510A5C`.

The diagnostic follows at most **64** `object+4` pending links
using bounded `ReadProcessMemory` calls with cycle and address
overflow protection. It emits only count/capacity/flag and presence,
**never prints addresses or calls any virtual method or pending
system callback**.

Download the latest compiled
`sin-pc-win32-original-factory-probe-phase657` artifact from a
passing Phase660 Win32 Actions run, then run locally:

```powershell
.\sin_pc_original_factory_abi_probe_phase657.exe "C:\SiN\bin\client.dll" "C:\SiN\bin\server.dll"
```

It will emit both prior `phase659_registry_snapshot` (client) and
new `phase660_autogame_snapshot` (server) records. The exact
owner-side runtime traces have **not** been collected in CI. A
post-DllMain observation is not equivalent to the later state during
the original engine's `GameDLLInit` or to an in-level SiN game boot.

## Proof boundaries and next work

The **entire two original x86 method bodies are authenticated**, but
the C++ implementation only models the recovered registration and
initialization behavior using synthetic guest memory and typed
callbacks. It does not execute original native x86 code, initialize
each actual game system or prove that the original retail Source engine
can load a replacement SiN server DLL.

Next startup blockers are identifying the exact engine/game-specific
system objects registered to this list and their virtual slot-zero
initializers; authenticating late `0x221F54F0` and
`0x221F51A0` pointer-array allocator helpers; and tracing subsequent
original `server.dll` gates `0x221AD8A0` / `0x22149B60` without
inventing successful initialization or reimplementing working HL2
systems.
