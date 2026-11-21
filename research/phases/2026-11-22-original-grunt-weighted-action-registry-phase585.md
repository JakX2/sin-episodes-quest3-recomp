# Gameplay Phase585: the original weighted action-registration selector

## Original 2006 SiN source provenance

This phase follows Phase584's native action mapping `0x220EEBD0` into its downstream registration selector `0x220EB1C0`, working only against the owner's untouched commercial SiN Episodes: Emergence PE32 `server.dll` / `client.dll`. This is original gamecode research, **not** generic HL2 physics, surface-property, prop/model loading/rendering, platform boot or Android/Quest porting.

- Original `server.dll` whole SHA256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- Original `client.dll` whole SHA256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- Original native image base `0x22000000`.
- **One new complete function** `0x220EB1C0` through `0x220EB2B0` inclusive, **241 bytes**, following byte `0x220EB2B1` original `INT3` padding.
- SHA256 for entire original native extent: **`c96a31109689d9493ca04cbd1e4d9dcceedcca2bd9bc65660b3a3b0e1ebbbec1`**.
- No raw disassembly dump, commercial binary or proprietary game asset included in research output.

## Original registration dataflow

Prior Phase584 lookup `0x220EEBD0` invokes this cdecl-like, stack-cleaned-by-caller method with **three parameters**: a pointer to an opaque original registry/record list, the desired translated numeric code, and the owner's current action/state field **`+0x3EC`**. The native callsites are `0x220EEBFD` (zero-mode pointer) and `0x220EEC1D` (registry-mode pointer). Both are authenticated incoming edges, not new native bytes.

### Null versus empty collection

- A **null registry pointer returns exactly `0`** at original `0x220EB1CA–0x220EB1CE`. This is distinct from unresolved lookup.
- For a non-null registry, the method starts with selected index **`-1`**. It queries current registry revision `0x2211C110`, compares to global at `0x224E8854`, and may invoke registration refresh `0x220EAFF0`.
- Iterates original record count from `0x2211B7D0(registry)` using `0x2211BD00(registry,index)`; when no entries or no matching key, returns **`-1`**.

### Native 24-byte-plus registration fields

The selected original record is returned as an opaque pointer from `0x2211BD00`. Its concrete fields used here are:

| Original record offset | Native operation | Source-bounded interpretation |
| --- | --- | --- |
| `+0x0D` | `test byte,0x10` | If this bit is not set, call original `0x220EAF70(registry,index)` before reading record fields |
| `+0x10` | Compare signed 32-bit dword with desired translated code | Numeric registered key |
| `+0x14` | Read signed 32-bit dword | Signed weight for indexed selection |

The record type and original Ritual C++ variable names remain unproven. This is not inferred from HL2 SDK definitions.

### Selection rules with signed weights

1. For a matching key, read signed weight `w` at `+0x14`.
2. If the caller's third parameter is zero and **`w < 0`**, the original records the current **index** as the result and returns immediately, before random selection.
3. Otherwise compute the absolute magnitude `|w|` (using native x86 `neg` if negative), accumulate into the running signed total, then invoke an original inclusive integer-random interface using bounds `0..(running_total-1)`. The original chooses between engine global branches `0x2255876C != 0` and the function pointer at `0x223D21AC`; the host has only one abstract RNG callback, not an invented engine-global mode.
4. If the running total is zero, the routine selects the current matching index. Otherwise, **if random draw is smaller than this entry's magnitude**, replace the selected index with the current record index; if the draw is at least that magnitude, preserve the prior selected index.
5. Continue scanning all registered entries. The resulting integer is a **record index**, not the record's numeric key, weight or final action name. A record later in the list can probabilistically replace an earlier match.

The native method performs an RNG call **even when the running weight sum is zero**, with upper bound `-1`. The original RNG's behavior under this abnormal range has not been recovered. Original negative signed minimum and accumulator overflow behavior is also not assumed safe. The standalone typed host reference explicitly returns `supported=false` for signed-overflow scenarios instead of fabricating x86/CRT behavior.

## Original direct-call provenance and regression

Seven authentic `E8 rel32` callsites in the new complete method:

| Site | Target | Native role bound |
|---|---|---|
| `0x220EB1D1` | `0x2211C110` | Registry revision getter |
| `0x220EB1DF` | `0x220EAFF0` | Optional registry refresh |
| `0x220EB1F7` | `0x2211B7D0` | Record count |
| `0x220EB20A` | `0x2211BD00` | Record at current index |
| `0x220EB219` | `0x220EAF70` | Optional per-record preparation |
| `0x220EB268` | `0x2228BC00` | Alternate random route |
| `0x220EB29A` | `0x2211B7D0` | Re-read count after each iteration |

Also authenticated: two Phase584 callers; original global-pointer random call at `0x220EB278` and 34 further native field/opcode witnesses.

`research/tools/audit_sin_original_weighted_lookup_phase585.py` authenticates both private DLL SHA identities, whole method SHA, 241 distinct native bytes, original `INT3` boundary, 7 direct E8 edges, 2 original Phase584 inbound E8 edges and 34 opcode witnesses. Synthetic mutations of every byte of every checked `E8` and wrong-DLL refusal: **PASS**.

The independent C++17 source model in `include/sin_compat/original_grunt_weighted_lookup_phase585.hpp` injects registry revision/refresh, per-entry preparation and random callbacks. Tests cover null collection 0 versus empty -1, key filtering, flag-bit preparations, positive-weight random replacement and retention, negative-weight immediate path with zero third argument, nonzero-state negative-weight accumulation, revision refresh, total-zero upper range and signed overflow guards. Strict GCC/Clang `-Werror`, GCC ASan/UBSan, standalone CMake/CTest **2/2 PASS**.

**Remaining dependencies:** original registry allocation and record setters (`0x2211BD00`, `0x220EAF70`), original random range edge behavior, dynamic action registration names / numbers, and complete source equivalents for the ultimate action-state transitions. No playable replacement DLL, Android or Quest 3 game is claimed.