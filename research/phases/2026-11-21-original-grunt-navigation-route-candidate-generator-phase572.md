# Phase 572: actual original Grunt navigator virtual +0x54 candidate route generator

**Source:** the owner's authenticated original 2006 `server.dll` / `client.dll`
from original `SE1/bin.zip`. This follows Phase569's indirect call through
navigator vtable `+0x54` and Phase571's newly recovered explicit waypoint
constructor. No generic HL2 physics, maps, props, surface materials, models,
renderer, Android or VR features are implemented.

## Cryptographic provenance and complete native boundaries

- Original `server.dll` SHA-256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- Original `client.dll` SHA-256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- Original `.text` PE preferred image base: `0x22000000`.

| Original native method | Exact full bytes | Whole-body SHA256 | Verified bounded role |
|---|---:|---|---|
| **`0x220774E0`** | **894** (`0x37E`) | `799f3f619354b5398526851b212f4aa662322f52f86886b2d4cb1b02f6f21e01` | Original navigator `+0x54` candidate route builder |
| `0x220748B0` | **5** | `f4c6d7ae520f88aecb3ea65952e885437fa4a6ce4b5c3439a161d1c5d8e42863` | Adjacent navigator virtual `+0x50`, constant true with `ret 4` |

**Totals:** two newly SHA-authenticated complete functions, **899 distinct
original server code bytes**. All method extents terminate in `ret 4`, with
original `INT3` padding following their terminal instructions. The 894-byte
function ends at address `0x2207785D`, with the next original routine at
`0x22077860`. Its full binary extent is included in the hash, but this does
**not** mean the opaque engine calls inside it have been reimplemented.

The original 2006 constructor at `0x220747A0` stores the navigator's primary
vtable **`0x223E9354`** at object offset 0. From genuine original `.rdata`:

- vtable byte `+0x50` => `0x220748B0` (`mov al,1; ret 4`).
- vtable byte `+0x54` => **`0x220774E0`** (`894` bytes).
- Original caller `0x220754BC` from Phase569 uses `call [vtable+0x54]`, with
  component `+0x28` candidate holder passed as stack argument.

This resolves the actual **function pointer identity** that remained unknown
in Phase569. These are real compiled methods, not names inferred from a
Half-Life 2 SDK class or a different Source engine version.

## Native candidate generation: original state transition

First operation of `0x220774E0`: call original `0x220B2CF0(candidate_holder)`
to release any previous waypoint chain and clear its holder. It occurs
**before** checking the active route or selecting a candidate. Then read
navigator `+0x24` active-route object and its first waypoint at route `+0x00`.
When it is null, return Boolean false. A navigator object having been
allocated (Phase567) does not imply its route has a usable first waypoint.

When the active first waypoint exists:

1. Read the first waypoint's numeric dword `+0x24`. Cases **1 and 3** set a
   special internal flag; other values take a distinct gate.
2. Obtain an x87 float from the separate navigator object at `+0x14`, calling
   that object's virtual byte `+0x58` with argument zero. Its concrete
   class and original source name are **not** yet proven.
3. For first kinds 1 and 3, call the native 2D/3D segment-distance helper
   **`0x2206D9F0`**, with original owner position at `navigator+0x04`'s
   `+0x360` and current waypoint position. For kind 3 with a successor of
   kind 3, an additional segment distance and two times a return from
   `0x22066270(owner+0x610)` contributes to the gate.
4. The native x87 comparison **selects the larger** of the owner virtual
   measurement and the computed distance path (ordered finite cases).
   It compares that result with the original double **`0.01`** at
   `0x223DE1D0`, requiring ordered **strictly greater than** to proceed.
5. For first kinds other than 1 or 3, compare the virtual measurement to
   original double **`0.1`** at `0x223D8FE8`. The ordered strict greater-
   than gate must pass before the shared `>0.01` gate.
6. After the gate, iterate original active linked waypoints using forward
   pointer `+0x28`. Repeated native `0x2206D9F0` calls and x87 comparisons
   test distances to saved vectors with the **original double `0.01`**.
   Different branches either copy an existing 48-byte waypoint into a new
   pool allocation or perform additional vector work and test an opaque
   engine callback before creating a new point.
7. For a direct-copy path, the original obtains **`0x30` bytes** from pool
   `0x2254AFF4` via `0x2226EE80`, copies **12 dwords** from an existing node,
   sets cached distance `+0x14=-1.0f`, clears forward/back links, resets
   selected bitfields and ID, and adds the resulting node to the candidate
   chain. The native source has a null-allocator path that can still
   dereference `eax` at `0x2207768E`: it is **not** a safe allocator contract.
8. A second branch constructs a native temporary vector and uses
   `0x2228B220` and a call to **`0x22071C50`** with additional numeric
   arguments and a float **100.0** literal (`0x42C80000`). The exact
   gameplay contract of that test is **not authenticated** here, so labeling
   it a specific line-of-sight, collision or visibility check would be
   premature. If admitted, another pool allocation invokes the exact
   Phase571 explicit constructor **`0x220B2AB0`** to build a 48-byte node.
9. If the accumulated new candidate first pointer remains null, return
   false. If nonnull, call original `0x220B2A10(candidate_holder,first)` at
   `0x2207784D` and return true, after linking the new candidate chain.

Native pointer, x87 and node-pool side effects remain visible in original
witnesses and whole-body SHA. We do **not** claim that this one phase already
produces all original tactical routes, or that the generic engine callback
`0x22071C50` has been assigned its source-level name correctly.

### Original complete direct CALL inventory (11 E8 edges)

| Callsite | Original destination | Authenticated role in this method |
|---|---|---|
| `0x220774F4` | `0x220B2CF0` | Clear candidate holder/node chain |
| `0x22077545` | `0x2206D9F0` | Early first-waypoint distance |
| `0x22077567` | `0x2206D9F0` | Adjacent kind-3 waypoint distance |
| `0x22077581` | `0x22066270` | Owner `+0x610` numeric contribution, precise type unknown |
| `0x2207762F` | `0x2206D9F0` | Iterated waypoint metric |
| `0x22077659` | `0x2226EE80` | Original pool allocation, 48 bytes |
| `0x22077742` | `0x2228B220` | Original geometry/vector helper, semantics unresolved |
| `0x220777BC` | `0x22071C50` | Native engine predicate, result gates point addition |
| `0x220777CC` | `0x2226EE80` | Second 48-byte point allocation |
| `0x220777E8` | `0x220B2AB0` | Exact explicit waypoint constructor (Phase571) |
| `0x2207784D` | `0x220B2A10` | Store first candidate waypoint in holder |

These edges are authenticated by original `E8 rel32` opcodes and displacement,
**not** guessed from nearby source comments. There is also a genuine indirect
virtual call `0x22077529 -> [vtable+0x58]` of a different navigator member,
and a distinct indirect original math-helper call at `0x22077735`.

## Deliberately bounded original-free C++ host reference

The accompanying `original_grunt_route_generator_phase572.hpp` and test
reconstruct the **proved entry gates and final commit order**, with an
interface for the *still opaque* interior waypoint generation:

- Always clear candidate holder first.
- No active first node => `NoActiveWaypoint` without invoking any engine math.
- For finite first-kind **1/3**, use the greater of abstracted original
  owner measurement and computed special path metric, then require **`>0.01`**.
- For other kinds, require owner measurement **`>0.1`**.
- Only when an injected callback indicates candidate built *and* its first
  pointer is present does the model invoke commit.
- All NaN/nonfinite and malformed pointer paths are rejected for host safety.
  Original x87 status-word behavior and exceptions are **not** modeled.

This host interface is intentional. **It does not regenerate actual original
waypoint sequences** and may not be called a drop-in port implementation until
`0x22071C50`, `0x2228B220`, vector interpolation, branch-specific node
construction, and native error cases are understood. The exact 894-byte
function is authenticated, while the C++ output is currently bounded.

## Reproduction and next research

```bash
python3 research/tools/audit_sin_original_grunt_route_generator_phase572.py --self-test
python3 research/tools/audit_sin_original_grunt_route_generator_phase572.py --bin-zip /private/SE1/bin.zip

c++ -std=c++17 -UNDEBUG -Wall -Wextra -Wpedantic -Werror -Iinclude \
  tests/test_original_grunt_route_generator_phase572.cpp -o /tmp/phase572
/tmp/phase572
```

Auditor confirms two original whole-method SHA256s (**899 new code bytes**),
**11 exact native E8 edges**, both original navigator vtable entries and
method constructor vtable pointer, **54 original opcode/field/branch
witnesses**, both double thresholds, and the 100.0f literal. Its
original-free self-test rejects all five-byte call mutations and wrong DLLs.
Strict GCC/Clang tests, sanitizer and CTest check the independent host
model's gating and callback order.

**Next:** recover the actual `0x22071C50` callback/trace contract and the
`0x2228B220` vector helper; then reproduce the original `0x220774E0` interior
loop with behaviorally equivalent source. Separately, follow the actual
Grunt event-1 caller that requests `GRUNT_FLANK`, now that this candidate
path builder and its waypoint types are grounded.

No original commercial DLLs, raw assembly dumps or game assets are committed.
This phase is evidence-backed partial gameplay code reconstruction, **not**
engine-loadable Windows DLLs, Quest3 runtime, Android APK or a playable port.