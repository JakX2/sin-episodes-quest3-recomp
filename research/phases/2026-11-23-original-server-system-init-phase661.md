# Phase 661: Four original SiN server AutoGameSystem virtual-Init bodies

## Why these methods

Phase660 recovered original server auto-game-system registration and
initialization, and authenticated **27 derived AutoGameSystem subobject
vtables**: 21 have a three-byte unconditional true initializer and six
execute substantive code. This phase reconstructs four of those six
remaining executable methods as a single typed C++ source module.

This work specifically targets SiN-specific startup and missing
gameplay initialization, not HL2 physics/surfaces, prop/model loading
or rendering. Retail executable files stay private to the owner.

## Exact owner-retail native audit

Whole unchanged 2006 original DLL SHA256 fingerprints:

- `server.dll`:
  `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- `client.dll`:
  `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

The owner-original PE32 scan verified these **four complete native x86
methods, totaling 160 bytes**, including the true return sequence
and authentic current vtable slot-zero pointers:

| Original VA | Original 2006 length | Whole-function SHA256 | Original AutoGameSystem vtable |
|---|---:|---|---|
| `0x2209CE50` | 30 | `842f384f1327ad0aeaf17a656c7567d2a76d01e93613dd8f4bb14d02db4e6e09` | `0x223EC6E4` |
| `0x220FBED0` | 75 | `d09c76d88825a2eda533193684cd7bd144a3ff54dfc445dcb0f8ead5f13db969` | `0x223FF514` |
| `0x22122540` | 16 | `89c9fa58f846248977eb590a39b829b37023154d9958a8b9113b6a0c69a8c33a` | `0x22401C0C` |
| `0x221FF7D0` | 39 | `f640285aa3362c82b6fb889ed84b5d7299291900cb6839b1b9946057316912a7` | `0x22446A28` |

**Important function boundary:** the 16-byte script initializer
`0x22122540` ends in `RET`, but the following original byte at
`0x22122550` is an executable `TEST` instruction beginning a
*different function*, **not** an `INT3`. The other three functions
are followed by `INT3`. The auditor checks each specific boundary.

The independent `research/tools/audit_original_server_system_init_phase661.py`
authenticates all four whole-method SHA256s, their original
`.text` executable location and four original `.rdata` vtable
entries, **seven real original direct rel32 calls**, the six original
NUL-terminated string literals, and the exact branch/virtual slot
witnesses. The owner-only audit uses:

```sh
python research/tools/audit_original_server_system_init_phase661.py \
  --server ORIGINAL_2006_SERVER_DLL --client ORIGINAL_2006_CLIENT_DLL
```

The public original-free test uses **invented instruction bytes only**:

```sh
python research/tools/audit_original_server_system_init_phase661.py --self-test
```

## Original behavior of the four virtual Init implementations

### 1. `0x2209CE50`: nested owning-class callback

The native `this` is an AutoGameSystem subobject **at
enclosing class + `0x630`** (this is the important Phase660
multiple-inheritance correction). It:

1. Reads the enclosing object's primary vtable at `[this-0x630]`.
2. Calls **primary vtable byte offset `0x18`** with
   `ECX=this-0x630`, obtaining a full DWORD EAX return value.
3. Passes that EAX to original helper `0x2209CDD0`, again with
   `ECX=this-0x630`.
4. Unconditionally returns `AL=1`, regardless of the helper's
   return value (assuming the called functions return normally).

The underlying primary virtual method and `0x2209CDD0` are
**separate typed host dependencies**, not guessed fake native code.
No assumption is made that the helper is a no-op.

### 2. `0x220FBED0`: five named engine registrations

This method calls original `0x220FB9B0` five times, each with
`ECX=this`, a first DWORD stack argument `0`, then a static
NUL-terminated string pointer in this exact order:

```text
phonemes
phonemes_weak
phonemes_strong
random
randomAlert
```

The strings reside at
`0x2240016C`, `0x2240015C`, `0x2240014C`,
`0x223D8EF8`, `0x22400140`, respectively.
The callee returns are ignored, then the method returns `AL=1`.
The original helper `0x220FB9B0` is not yet reconstructed:
its callback contract is explicit and every invocation is tested.

### 3. `0x22122540`: decal script initialization

The original method pushes the literal pointer
`0x22401D70` to `scripts/decals_subrect.txt`,
subtracts **four bytes** from `ECX` to get the enclosing class
object, calls original `0x22122280`, and returns `AL=1`.
The helper itself performs substantial downstream file-related
work, which remains a separate original dependency. The derived
source model does **not** invent whether that file load succeeded.

### 4. `0x221FF7D0`: spatial-partition engine callback

The original function uses global
`DWORD[0x225721B8]`, the `SpatialPartition001` engine
interface acquired earlier in Phase658. It dispatches through the
interface's **virtual byte offset `+0x28`**, with one pointer
argument:

- If `ECX!=0`, pass `ECX+8`.
- If `ECX==0`, pass a null pointer.

It always ends `MOV AL,1; RET` on both native branches,
irrespective of the engine callback's EAX. The host supplies
original engine-interface memory reads and the typed slot-10
callback, and cannot silently convert one branch into the other.

## Source implementation and integration tests

Added:
- `include/sin_compat/original_server_system_init_phase661.hpp`
- `src/original_server_system_init_phase661.cpp`
- `tests/test_original_server_system_init_phase661.cpp`
- `research/tools/audit_original_server_system_init_phase661.py`
- `.github/workflows/sin-phase661-system-init.yml`

`Initializers::run` dispatches exact four original VAs,
preserving the inferred object-pointer transforms, the five
literal names, ordered helper callbacks and branch differences.
It returns a typed result with `original_return_al=true`
only after callbacks return to the model. Null/invalid pointer
rejection and host callback exceptions are **portable safeguards**,
not new claims about the native x86 instructions.

The integration test compiles and links the *real previously
reconstructed* Phase660 `AutoGameSystems` reference model:
it registers four synthetic virtual system objects, applies
their original derived vtable slot-zero values after base
registration, invokes `AutoGameSystems::initialize_all`, and
dispatches each callback into actual Phase661 modeled methods
instead of a synthetic always-success callback.

This test verifies reverse-registration invocation order:
spatial-partition → decal-script → five named entries →
nested-primary callback and helper, and the original Phase660
initialized flag, cleared pending list, array count and correct
game-system pointer storage. Standalone tests additionally cover
both spatial partition `ECX` paths; primary callback EAX
propagation; exact string order and zero argument; pointer
adjustments; original AL return semantics; missing guest memory;
callback failure, reentrancy, and exceptions.

The strict C++17 GCC/Linux and native MSVC Win32 x86 CMake
targets link **both Phase660 and Phase661 implementations**.
A dedicated Clang AddressSanitizer/UndefinedBehaviorSanitizer
test compiles the same full chain with no proprietary original
binaries in GitHub Actions.

## Remaining real startup blockers

Two of the six substantive AutoGameSystem virtual Init
implementations still require full decompilation:

- `0x221BF980`: original source-engine resources/allocator/
  filesystem callbacks with conditional branching.
- `0x22223490`: repeated allocation and construction of
  game-specific objects and global registrations.

The helpers `0x2209CDD0`, `0x220FB9B0`, and
`0x22122280` are additional native dependency chains.
They have been named and their direct original callsites
authenticated, not fully replaced.

**No original native function was run in the Windows Source
engine in this phase.** The successful reference-model and
Android compatibility tests are not equivalent to working
replacement game DLLs, entity AI behavior or playable Quest 3.
