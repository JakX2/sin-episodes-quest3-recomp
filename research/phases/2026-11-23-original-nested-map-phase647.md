# Phase 647: original SiN nested map initialization and slot-array release

## Owner priority and original DLL evidence

Recover missing original *SiN Episodes: Emergence* 2006 gameplay/entity symbol registration logic rather than reimplementing HL2-equivalent physics, materials/surface properties, props, model handling or rendering. This phase uses the owner's unchanged original retail PE32 DLLs:

- `server.dll` SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- `client.dll` SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

The owner-private audit authenticates **three complete original methods, 212 executable x86 bytes**, each with original post-method INT3 padding:

| Function | Native length | Whole original SHA256 |
|---|---:|---|
| `0x2228EAB0` nested slot-array constructor | 90 bytes | `0ab5d6c1640e57969dace5266e647d13832946ea0db14d8ab500ec3d981d8e58` |
| `0x2228F360` nested map constructor | 81 bytes | `4bd47b0db93a0629b8732063cb35968b49c37dadb68e65b46dbe590972984ee8` |
| `0x2228EB10` nested slot-array release | 41 bytes | `d157aa7fc3dc00d0e282b9507a754499e5870ab4382005547b909d6f08f9db99` |

The original-only audit additionally checks **five direct rel32 calls** (including Phase646 constructor and factory callsites), **six exact native branch targets**, **two comparator-function installations**, and **73 address-pinned original x86 instruction witnesses**. Source C++/tests/scripts are derived work; no retail DLLs or executable code blobs are committed.

## Exact 90-byte original `0x2228EAB0` semantics

The method is `__thiscall ECX=array, 3 DWORD arguments` (growth-step, initial capacity, function-pointer):

1. `DWORD [array+0]` = incoming callback pointer (third argument).
2. `DWORD [array+4]` = 0 (slot-array pointer).
3. `DWORD [array+8]` = incoming initial capacity (second argument).
4. `DWORD [array+0xC]` = incoming growth-step policy (first argument).
5. If and only if the initial capacity is nonzero, compute **12 × capacity modulo 2^32** using original `LEA` and two `ADD` instructions, then call original `0x2226E390` allocator with that 32-bit byte count. Overwrite `DWORD [array+4]` with EAX even when zero. There is no native check of successful allocation.
6. Set `WORD [array+0x10]` and `WORD [array+0x14]` to **FFFF**.
7. Re-read `DWORD [array+4]`, write it to `DWORD [array+0x18]` as an array-pointer cache. Zero `WORD [array+0x12]` and `WORD [array+0x16]`.

The array record stride is **12 bytes**, unlike Phase646's outer symbol-tree records which are **16 bytes**. Original `RET 0x0C` cleans three DWORD parameters. The portable model preserves partial committed writes on later host failure and uses a required typed host allocator callback. It does not claim to have implemented Valve's underlying memory manager.

## Exact 81-byte original `0x2228F360` semantics

Native `__thiscall ECX=map`, three DWORD args (growth-step, initial capacity, selector). The routine calls original **`0x2228EAB0`**, passing its first two arguments unchanged and forcing **callback pointer = 0** as the array constructor's third argument. On return it initializes five DWORD map fields:

- `map+0x1C = 0`
- `map+0x20 = 0`
- `map+0x24 = 8`
- `map+0x28 = 0`
- `map+0x2C = 0`

It then tests only **the selector's low byte**, not the whole DWORD, and reads `[map+0]`. If the map comparator is still zero, it installs:

- selector low byte zero: **0x2228E910**, a direct bytewise comparison method (internals not reconstructed in this phase);
- selector low byte nonzero: **0x2228E9B0**, the original string-comparison method (internals not reconstructed in this phase).

If the already stored comparator pointer is nonzero, the original leaves it intact.

The Phase646 constructor passes **(growth-step=0, capacity=0, selector=1)**, so the real nested map begins with **0x2228E9B0**. No allocation is made in that default zero-capacity constructor. The actual original `0x2228E9B0` and `0x2228E910` comparator **implementations** remain separate research targets, even though their selection is proven. This discovery does not mean all nested string symbols are resolved.

## Exact 41-byte original `0x2228EB10` semantics

On `ECX=array`, it tests `DWORD[array+0xC]` against FFFFFFFF and skips all work when equal. Otherwise, if `DWORD[array+4]` is nonzero, it calls original free wrapper **`0x2226E3E0`**, then zeros `[array+4]`. Whether the array pointer was zero or nonzero, it clears **`DWORD[array+8]` capacity**.

It **does not** reset the cached array pointer at `[array+0x18]`, root/free-head indices, comparator, growth-step, or map fields. This asymmetry is preserved. The free wrapper's original ABI is modeled by Phase645; its memory-manager implementation remains a required host callback.

## Portable code and verification

Added three C++17 source-model classes `ArrayConstructor`, `MapConstructor`, `ArrayRelease`, which use a typed guest-memory/allocator host, preserve exact original field write order, widths, selector-byte semantics and 32-bit allocation-size wrap. Portable overflow/address checks, callback failure and reentrancy refusal are defensive non-native safeguards.

- A dedicated compiled regression suite tests zero/nonzero initial capacities, exact 12-byte slot sizing, wrapped byte calculations, null allocator returns, both original comparator selectors including DWORD `0x100` (low byte zero), precise five extra map fields, disabled release, unchanged cached pointer after release, host allocator/free failures, partial writes, exceptions and reentrant callbacks.
- The extended compiled Phase638→639→643→640→642→641→644→645→646→647 synthetic integration test calls the *real modeled* Phase647 nested map constructor **from the Phase646 table's original callback at `table+0x1C`**, rather than a no-op. It verifies comparator `0x2228E9B0` selection and field initialization, then inserts 39 symbols through the original modeled Phase646 outer comparator and all prior tree machinery. It still tests one original wrapper allocate, five relocations/reallocations, red-black invariants and mapped records after every insert; finally the original outer array and nested-array releases are exercised. A separate integration case constructs a nonzero-capacity map through the Phase645 recovered allocator ABI and releases it through the same original free wrapper.
- Local standalone C++17 strict-warning tests **passed**, and an independent **AddressSanitizer + UndefinedBehaviorSanitizer** run of the standalone Phase647 suite **passed**.
- Owner-private auditor was run against the actual original `server.dll` and `client.dll`, not approximations. Original-free Python mutation self-test passes on invented bytes and refuses a fake DLL. Original source bytes remain private.

### Owner retail authentication command

`python research/tools/audit_original_nested_map_phase647.py --server /path/to/retail/server.dll --client /path/to/retail/client.dll`

Public CI compatible synthetic test:

`python research/tools/audit_original_nested_map_phase647.py --self-test`

## Remaining original SiN-specific dependencies

- **`0x2228E9B0` and `0x2228E910`** comparator implementations are now precisely identified and selected, but are not reconstructed in Phase647. The former calls another original string function at `0x223A5574`.
- **`0x2228F7A0`** nested string-tree lookup used in Phase638 forwarding remains a typed synthetic host callback.
- Original engine memory-manager virtual implementations, Windows retail game invocation, actual NPC behavior and Android/Quest 3 in-level execution are not demonstrated. Green CI verifies reconstructed reference behavior only.

Stay focused on missing SiN-specific gameplay and AI behavior, not generic Half-Life 2 subsystems.
