# Phase 662: Original server AutoGameSystem six-object initialization

## Why this method matters

Phase660 identified six substantive server AutoGameSystem virtual
initialization methods. Phase661 reconstructed four short methods
and linked their behavior into the compiled original Phase660
AutoGameSystems registration-and-init reference model.

This phase recovers the *fifth* method, `0x22223490`, whose
entire original 363-byte body consists of six repeated 40-byte
allocations, different derived auxiliary-object constructors and
conditional storage into six different owner fields. The sixth
remaining native method `0x221BF980` is substantially different and
still needs reverse engineering.

This targets original SiN game-specific startup behavior, not
Half-Life 2 physics, surfaces, prop/model rendering or loading.

## Direct original retail audit

The owner's unchanged original 2006 PE32 binary SHA256 fingerprints:

- `server.dll`: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- `client.dll`: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

Original `server.dll` function `0x22223490` occupies **363
complete executable bytes**, whole-method SHA256
`1caa170fc1a0e04a3f9b5cc6a83200a015dd1449a7b58fcce39b09771c243233`,
ends in `POP EDI; POP ESI; RET` at `0x222235FA`, and is followed
by `INT3` at `0x222235FB`. The Phase660 identified AutoGameSystem
subobject vtable at `0x2244AECC` has its virtual slot zero pointer
equal to `0x22223490`.

The original code has **12 exact direct `E8 rel32` call edges**:
six repeated calls to allocator `0x2226E790` and one each to
six different auxiliary-object constructors. The exact original
original-body hash, call edges, vtable and each per-block literal
and field write are verified by
`research/tools/audit_original_server_object_init_phase662.py`.

| Order | `malloc(40)` at | Original initializer constructor | New object's vtable | Enclosing owner's field |
|---|---|---|---|---|
| 1 | `0x22223496` | `0x22225BA0` | `0x2244B1E4` | `this+0x0C` |
| 2 | `0x222234D1` | `0x22225F10` | `0x2244B1D0` | `this+0x08` |
| 3 | `0x2222350C` | `0x22226280` | `0x2244B1BC` | `this+0x10` |
| 4 | `0x22223547` | `0x222265F0` | `0x2244B1A8` | `this+0x14` |
| 5 | `0x22223582` | `0x22226960` | `0x2244B194` | `this+0x18` |
| 6 | `0x222235BD` | `0x22226CD0` | `0x2244B180` | `this+0x1C` |

## Recovered original execution semantics

For each of the **six items in order**, the native code:

1. Calls original allocator `0x2226E790` with **40 bytes**
   regardless of whether the destination owner field already
   contains a nonnull pointer.
2. If allocation returned a nonnull pointer, writes the item's
   original vtable pointer at `[new_object+0]`, invokes its
   original constructor with `ECX=new_object+4`, and pushes the
   original constructor stack DWORDs
   `(0x40,0,0,comparator_function,hash_function)`.
3. If allocation returned null, skips its constructor and leaves
   constructed object pointer zero.
4. Inspects the owner's item-specific field. If the field is
   currently **zero**, writes the object pointer to it, even
   when that pointer is null. If nonzero, it **leaves the
   previously stored pointer untouched**, even though the newly
   allocated object constructor has already run.
5. Continues to the next item. The final method return is
   **unconditional AL=1**, independent of one or all allocator
   failures, provided that the original helper calls return
   normally and no native fault occurs.

This is an important distinction from typical code that first checks
a field and only allocates if absent. The original method performs
allocation and construction even when there is already an owner
pointer and does not replace the existing field's value.

## New source model, evidence and tests

Added:
- `include/sin_compat/original_server_object_init_phase662.hpp`;
- `src/original_server_object_init_phase662.cpp`;
- `tests/test_original_server_object_init_phase662.cpp`;
- `research/tools/audit_original_server_object_init_phase662.py`;
- `.github/workflows/sin-phase662-object-init.yml`.

The derived C++17 `InitAuxiliaryObjects::run` preserves the
six-item order, original allocation sizes, original constructor
function identities, derived vtables, exact pointer-field offsets,
write-on-zero rules and true-return semantics. The allocator and all
six original constructors remain **mandatory typed host callbacks**,
not invented no-op implementations.

Regression tests cover:
- Six allocations and six constructors with prepopulated owner
  fields, proving the constructor calls still occur and original
  fields are not overwritten.
- Six failed allocations, zero constructors, zero-valued stores,
  and **native original return true**.
- Mixed successful/null allocations and exact four/fewer field
  writes based on destination values.
- Early host allocator and constructor failures, preserved
  partial guest state, exception handling, and reentrant calls
  as protective reference-host behavior. These host errors are
  *not* claims that the original x86 returns false.
- A compiled **Phase660→662 integration**, registering one
  derived AutoGameSystem and dispatching through its authentic
  vtable slot-zero into the Phase662 model. It verifies the
  initialized flag, active array, and all six constructed objects
  in the original order.

The portable GCC/Linux and native Win32 MSVC x86 CMake targets
compile the exact Phase660 and Phase662 source models. Independent
Clang AddressSanitizer/UndefinedBehaviorSanitizer CI executes the
same tests. The original-retail private audit checks whole native
method identity and 12 distinct original callsites. Public
`--self-test` uses invented instruction bytes, not retail code.

Read-only owner-private audit:

```sh
python research/tools/audit_original_server_object_init_phase662.py \
  --server ORIGINAL_2006_SERVER_DLL --client ORIGINAL_2006_CLIENT_DLL
```

Original-free public synthetic test:

```sh
python research/tools/audit_original_server_object_init_phase662.py --self-test
```

## Remaining sixth initializer identified as soundscape-manifest setup

The original remaining `0x221BF980` routine has now been examined
directly in the unchanged retail `server.dll` disassembly:

- **330 complete original x86 bytes**, full-body SHA256
  `8af434e893f80f38ed7ff0a4686e802921297376f4a663e7e0ff5f8a242052fa`,
  `RET` at `0x221BFAC9`, followed by `INT3`.
- Its native string constants explicitly identify
  `scripts/soundscapes_manifest.txt` at `0x22433AB8`,
  `scripts/soundscapes_%s.txt` at `0x22433ADC`,
  `GAME` at `0x223EA728` and expected manifest token
  `file` at `0x22433AB0`.
- It uses the previously recovered game DLL filesystem interface
  pointer global `0x225721A4` and game-global pointer
  `0x22572190`. It traverses entries in the soundscape manifest,
  logs unexpected entry types or missing manifests, and invokes
  engine/filesystem callbacks before storing `0xFFFFFFFF` at
  `[this+0x24]`. It returns `AL=1` on its observed normal
  completion path.

**This identifies it as soundscape-manifest initialization**, not an
unknown SiN-specific NPC/weapon/AI system. The method remains
**unmodeled in derived C++**. Its entire hash establishes the native
body's identity, but does not prove its filesystem helper semantics.
Because the owner's HL2 map port already uses working Source engine
services, it is lower priority than actual missing SiN game logic
unless genuine Windows engine boot evidence shows this particular
startup gate must be implemented.

Accordingly, after Phase662 the next best use of reconstruction work
is to gather owner-side Windows original-game startup evidence and
return to **SiN-specific entity factories, AI/NPC scheduling, weapon
callbacks, damage, and dynamic difficulty**, rather than duplicating
generic Source soundscape behavior without a proven need.

## Proof limits and next phase

This recovers **363 additional complete original executable bytes**,
and reconstructs five of six substantive original server AutoGameSystem
virtual Init entrypoints across Phases 661 and 662.

The sixth remaining original target, `0x221BF980`, performs
conditional filesystem/resource traversal and callbacks and
cannot safely be approximated as another fixed set of six
allocations. Additionally, the allocator `0x2226E790`, six
constructor implementations and their nested callback chains remain
external dependencies to be reconstructed for a real game process.

GitHub CTest and sanitizers are **source-model evidence over
synthetic guest memory**, not original Windows engine execution,
complete SiN gameplay, or Quest 3 VR readiness.
