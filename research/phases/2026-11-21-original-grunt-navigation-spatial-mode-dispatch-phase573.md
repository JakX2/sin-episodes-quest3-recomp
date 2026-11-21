# Phase 573: original Grunt route-candidate spatial mode dispatcher

**Scope:** trace the actual 2006 SiN Episodes `server.dll` predicate called
from Phase572's Grunt waypoint-generation branch. The work authenticates
**gameplay-specific waypoint-kind dispatch**, not generic engine physics,
materials, surface properties, models, props, rendering, Android or VR.
The meanings of delegated spatial/tracing routines are deliberately not
assigned invented Source SDK labels.

## Original DLL identity and exact native method boundary

Owner-private original `SE1/bin.zip` independently authenticated:

- `server.dll` SHA256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- `client.dll` SHA256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- PE image base `0x22000000`; native `.text` source method **`0x22071C50`**
  through **`0x22071EB3`** inclusive, exactly **612 bytes**.
- Exact complete method SHA256 **`ce5242e9a2834f0ddf6287dd8d32549c32dc22e74a38eb3b6faaca4091fb7aaa`**.

The method's native `ret 0x20` appears at `0x22071E9E`, followed by a
three-byte alignment NOP at `0x22071EA1` and the authentic four-entry
**data** jump table at `0x22071EA4–0x22071EB3`. The following byte
`0x22071EB4` is original `INT3` padding. The table is included in the
612-byte method hash and does **not** count as four additional functions.

The prior Phase572 candidate-generator callsite `0x220777BC` directly calls
this exact native routine with eight stack-argument words (native
`ret 0x20`). Its first stack parameter comes from the waypoint's numeric
`+0x24` field, so the game chooses a method based on **waypoint kind**.

## Four original modes, exact genuine targets

Original dispatcher compares the unsigned kind to `3`: values greater than
3 (including negative signed values interpreted unsigned) go to a fallback
branch. Original jump table entries:

| Input kind | Jump table original RVA/VA | Branch destination | Gameplay-dependent native callee |
|---|---|---|---|
| `0` | `0x22071EA4` | `0x22071CE7` | `0x22071A20` after additional original flags/engine gates |
| `1` | `0x22071EA8` | `0x22071E49` | `0x2206E9E0` |
| `2` | `0x22071EAC` | `0x22071E2D` | `0x22070560` |
| `3` | `0x22071EB0` | `0x22071E65` | `0x2206EB40` |
| unsigned `>3` | no jump-table entry | `0x22071E79` | `0x2206D9F0` for fallback distance and raw status `-4` |

The exact table and all eight native `E8 rel32` instructions are checked
against the authentic code. The four method names above are not recovered
Ritual C++ names; they are independently observed addresses.

## Original report status and return contract

Before dispatch, original code optionally uses a caller output pointer,
falling back to a native **56-byte local temporary** if it is null. It
initializes result fields and calls `0x2206D9F0` to establish distance at
output `+0x20`. It clears output fields `+0x1C`, `+0x24`, copies a native
three-component default vector to output `+0x10` and records the caller
vector fields. This initial distance call occurs **before** the mode switch.

After the mode-specific native operation, the routine evaluates original
output **signed dword `+0x00`**. `0x22071E8D..0x22071E98` implements
`xor eax,eax; cmp [esi],eax; setge al` and returns the resulting Boolean.
In other words, the method returns true when the original report status
is **nonnegative** and false for negative statuses. It does *not* simply
return the mode-specific callback's raw Boolean without interpreting the
output status.

The out-of-range path assigns exactly **`0xFFFFFFFC` (`-4`)** to output
status `+0x00`, invokes `0x2206D9F0`, stores the distance at `+0x24`, and
returns false by the common signed-status gate. One separate mode-0 branch
assigns **`-2`** at `0x22071DCB`; others delegate state mutation to
`0x22071A20`, `0x2206E9E0`, `0x22070560`, `0x2206EB40`.

Mode0 also derives extra native flag bits from an input flag word and an
owner flag byte at `+0xA9`, calls owner virtual `+0x1C4` under another input
flag and, depending on x87 result, invokes `0x2206E6B0`, `0x22071A20` or
`0x2206E0D0`. The source-level interpretation of these engine virtuals
remains unproven. The original mode0 conditionally writes owner byte
`+0x0C=1`, then clears it on the main return path. This is a genuine
side-effect distinction from the simple mode1/2/3 call dispatch.

## Newly authenticated native CALL edges

All eight original direct E8 instructions checked, including exact targets:

| Original callsite | Original callee |
|---|---|
| `0x22071C90` | `0x2206D9F0` |
| `0x22071D52` | `0x2206E6B0` |
| `0x22071D8D` | `0x22071A20` |
| `0x22071DA9` | `0x2206E0D0` |
| `0x22071E42` | `0x2206E9E0` |
| `0x22071E5E` | `0x22070560` |
| `0x22071E72` | `0x2206EB40` |
| `0x22071E82` | `0x2206D9F0` |

One incoming Phase572 callsite `0x220777BC -> 0x22071C50` is separately
authenticated. There are 36 further independent original opcode/field
witnesses, including unsigned dispatch, raw status, owner flag and native
jump-table layout.

## Safe standalone C++17 host reference and explicit limits

`include/sin_compat/original_grunt_spatial_dispatch_phase573.hpp` models
only grounded **dispatch order and result status**. A synthetic `Host`
interface returns mode-specific raw output status; a caller can provide
its own finite metrics. It initializes output, calls the abstract initial
distance provider, routes unsigned modes 0..3 to distinct callbacks,
sets out-of-range mode status to `-4` and computes its fallback distance,
then returns true on nonnegative native status. This is **not** a generic
visibility/collision engine, and cannot yet replace real original trace
filter callbacks.

`tests/test_original_grunt_spatial_dispatch_phase573.cpp` checks each of
four mode routes, output initialization and call order, mode0 reported `-2`,
an additional negative report, 4/9/-1 (unsigned) out-of-range rejection,
no unintended mode callbacks, and raw status preservation.

Auditor: `research/tools/audit_sin_original_grunt_spatial_dispatch_phase573.py`

```bash
python3 research/tools/audit_sin_original_grunt_spatial_dispatch_phase573.py --self-test
python3 research/tools/audit_sin_original_grunt_spatial_dispatch_phase573.py --bin-zip /private/SE1/bin.zip

c++ -std=c++17 -UNDEBUG -Wall -Wextra -Wpedantic -Werror -Iinclude \
 tests/test_original_grunt_spatial_dispatch_phase573.cpp -o /tmp/phase573
/tmp/phase573
```

Owner-private original pair authentication **PASS**, full 612-byte method
SHA, **8 original direct calls**, **1 real incoming Grunt nav call**,
**4 native jump table words**, **36 independent witnesses**: PASS. The
original-free Python mutation self-test rejects each native E8 opcode or
rel32 displacement change, and refuses fake original binaries.

**What is not known:** the full source semantics/virtual contracts of
`0x22071A20`, `0x2206E9E0`, `0x22070560`, `0x2206EB40`, `0x2206E6B0`,
`0x2206E0D0`, native owner subclass semantics, and whether specific map
paths actually choose the candidate trace branch in gameplay. These
must be independently recovered before claiming a complete source-equivalent
spatial predicate. No original DLL, assembly dump or game asset is uploaded.