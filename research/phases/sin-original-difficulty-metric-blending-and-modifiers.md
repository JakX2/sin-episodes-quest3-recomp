# Phase 78: Original SiN 14 named difficulty metrics and modifier algorithms

## Primary binary evidence

Inspected original Windows x86 PE32 `server.dll` (SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`) and paired `client.dll` (SHA-256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`). The auditor invokes Phase 77's exact SHA-pinned original archive verifier. No proprietary DLL executable bytes are committed.

## Fourteen original named metric records

The original name-pointer table at `0x2245F04C` contains **14 pointers to case-sensitive C strings**. The 14-name search at `0x222D3370` scans until `CMP EDI,0xE` at `0x222D33C0`. Static records begin at `0x22523478`; each is **seven float32 fields (28 bytes)**. Field 0 is current, fields 1/2 are blend endpoints. Do not assign invented meanings to fields 3–6.

| # | Original metric name | Initial current | Endpoint 0 | Endpoint 1 |
|--:|---|---:|---:|---:|
| 0 | `DAMAGE_TAKEN_SCALE` | 1.075 | 0.75 | 1.4 |
| 1 | `DAMAGE_DONE_SCALE` | 1 | 1 | 1 |
| 2 | `AMMO_QUANTITY_SCALE` | 1 | 1 | 1 |
| 3 | `NUM_FORCED_HEALTH_KITS` | 0 | 0 | 0 |
| 4 | `PERCENT_HEALTH_KITS_DROP` | 0.4 | 0.5 | 0.3 |
| 5 | `PERCENT_NPC_2ND_FIRE` | 0.25 | 0 | 0.5 |
| 6 | `PERCENT_NPC_MAKE_DIFFERENT` | 0.15 | -0.1 | 0.4 |
| 7 | `PERCENT_NPC_CHANGE_SPAWN_NUM` | 0.15 | -0.1 | 0.4 |
| 8 | `PERCENT_NPC_WEARS_HELMET` | **0.35000002** | 0.1 | 0.6 |
| 9 | `PERCENT_NPC_HELMET_SHOT_OFF` | 0.6 | 0.8 | 0.4 |
| 10 | `PERCENT_NPC_TAKES_COVER` | 0.2 | 0.2 | 0.2 |
| 11 | `RANDOM_NPC_WAIT_BEFORE_SHOOT` | 0 | 0.1 | -0.1 |
| 12 | `PERCENT_NPC_ACCURATE` | 0.75 | 1 | 0.5 |
| 13 | `PERCENT_NPC_MOBILE` | 0.5 | 0.3 | 0.7 |

The C++17 header preserves **all 7 raw float values** per entry. Particularly, record #8's current float is original IEEE-754 **`0x3EB33334`**, not the `0x3EB33333` produced by a literal `0.35f`. Verified compiled C++ data against all 98 original float32 values, **98/98 bits identical**.

## Recovered original math: metric interpolation

Original `0x222D3490` is called from `0x222B9194` and `0x222BA3BA`. Its x87 assembly loads original `1.0f` at `0x223D22F8`, subtracts its parameter `t`, multiplies by first endpoint at record `+4`, multiplies `t` by second endpoint at `+8`, adds, and stores the result at record `+0`. The unrolled loop processes **all 14 metrics** as two halves of seven.

Finite-input C++ equivalent:

```cpp
current = (1.0f - t) * field1 + t * field2;
```

**No clamp of `t` occurs inside this function.** Callers may constrain it separately. Original x87 intermediate rounding is not guaranteed to equal ARM64 `float` bit for bit on every input, so the C++ code is an algorithmic reference rather than a byte-perfect emulator.

## Recovered original math: modifier selection and clamps

- Ten original static 16-byte modifier records at `0x22521DE0`; all four float32 initializers in each record are `1.0f`. Original getter `0x222C07A0` selects float at `+0` for **selector < 2** and float at `+8` for **selector >= 2**. Existing message/archive callers use selectors 1 and 3.
- Original additive update `0x222C07C0`, called at `0x222BAEF8`, adds `delta` to selected current value and **clamps the result to [0.05, 2.0]**. The compiled 32-bit float bound constants are `0x3D4CCCCD` and `0x40000000`.
- Original absolute setter `0x222C0810` uses the same bounds and is reached from archive restoration at `0x222B926C` and `0x222B9279`.

Only finite-input paths are reproduced here. Original NaN behavior, effects of negative game ticks and other x87 exception modes require future investigation.

## Verification and source

- `research/tools/audit_sin_metric_math.py` validates **31 individual opcode sites, seven direct CALL targets, nine region SHA-256 fingerprints** covering actual original instruction bodies, name pointer table, 392-byte metric table and modifier initializers. It also validates all 14 C strings and selected independent values. The auditor chains Phase 77's exact dual-DLL SHA-256 and archive checks. JSON emits *metadata and numeric constants only*.
- `research/tests/test_sin_metric_math.py` uses original-free synthetic fixtures to mutate every checked opcode, call target, name pointer and selected scalar, and tests SHA guard behavior.
- `include/sin_compat/difficulty_metric_math.hpp` exposes the exact initializer table, case-sensitive name lookup, interpolation, selector index and finite modifier clamp/update reference.
- `tests/test_difficulty_metric_math.cpp` verifies all 14 endpoint/midpoint results, unchecked extrapolation, index lookup, the single bit-exact precision trap, both modifier bounds and selector branches.
- `CMakeLists.txt` registers the C++17 regression with CTest.

Reproduce against private DLLs:

```powershell
py research/tools/audit_sin_metric_math.py --client "C:\SiN\bin\client.dll" --server "C:\SiN\bin\server.dll" --output "C:\private\sin-phase78-math.json"
```

## Scope and next work

This is **actual original named table data and two recovered mathematical helper families**, not a complete dynamic-difficulty scoring function. Unidentified metric fields 3..6, update scheduling and signals, event-driven metric changes, advisor scoring, class ABI/SourceVR and real Quest 3 gameplay remain to be recovered. No playable ARM64 game or complete original DLL recompilation is claimed.
