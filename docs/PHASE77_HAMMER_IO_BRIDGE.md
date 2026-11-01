# Phase 77: Turn original Hammer I/O formats into portable event connections

**Status:** Independently authored C++17 map-output value parser, synthetic CTest and private SDK-derived aggregate scripting audit. **Not a VMF-to-Source engine loader, game module, live SiN BSP execution or Quest gameplay.**

## Why this phase

Phase 74's `EntityIoRuntime::connect` already supports explicit per-output target, input, parameter, delay, and max-fire count. Phase 76's owner-supplied SiN Episodes SDK contains original Hammer map sources inside VPK v2 archives. Phase 77 adds the missing *data-to-runtime* bridge: `parse_hammer_output_value`, which reads a five-field Hammer output value and creates a validated `EntityOutputConnection` ready for the portable scheduler.

The parser recognizes the editor's comma delimiter and the 0x1B separator found in extracted Source BSP entity I/O data. It trims outer spaces, preserves comma characters inside ESC-delimited parameters, validates an exact five-field shape, rejects invalid/negative/nonfinite delays, validates max-fire counts (`-1` unlimited, `0` disabled, positive finite) and rejects oversized values. An output's **key name** (`OnTrigger`, `OutColor`, etc.) is supplied separately to `EntityIoRuntime::connect`.

This deliberately **does not** interpret wildcards, `!activator`, `!caller` or target lookup. The host's `IEntityOutputHost::resolve_targets` remains responsible; source entity keyvalue loading, correct special-name handling and real-engine event dispatch are still unimplemented.

## SDK source maps: real aggregate I/O measurements

The independent `research/tools/audit_sin_sdk_io_patterns.py` privately reads the original three VMFs through Phase 76's VPK CRC-validated reader, inventories each source entity's `targetname` and output values, then publishes **counts only**, never source entities, parameters, targetnames, geometry or map bytes.

| Aggregate property | se1_docks04 | se1_highrise01 | arena_office |
|---|---:|---:|---:|
| Output connections | 436 | 713 | 60 |
| Immediate connections | 387 | 599 | 55 |
| Delayed connections | 49 | 114 | 5 |
| Unlimited-fire links | 418 | 651 | 58 |
| One-shot links | 18 | 62 | 2 |
| Wildcard/dynamic targets | 38 | 42 | 0 |
| Special targets (`!`-prefixed) | 0 | 0 | 20 |
| Targets with a unique literal VMF name | 376 | 655 | 27 |
| Targets with duplicate literal VMF names | 18 | 11 | 1 |
| Literal targets not found by this static scan | 4 | 5 | 12 |
| Non-`On`-prefixed output keys | 6 | 9 | 0 |
| Invalid five-field shapes in this scan | 0 | 0 | 0 |

The original samples include output keys not starting with `On`. Do **not** filter to `On*` only when importing Hammer connections. The Phase 76 private summary's "needs review" count for 6/9 unusual entries must not be mistaken for six/nine malformed five-field payloads; this separate audit identifies zero malformed payload shapes and exactly 15 non-`On` output keys across the two campaign VMFs.

**Static name resolution is strictly provisional:** unresolved names may be created/spawned later; duplicates can be intentional; wildcard expansion depends on engine-defined semantics. The audit does not establish that these inputs execute successfully in the released game.

## Representative gameplay pattern

The docks' original authored scripting includes a `logic_relay` which schedules a **staggered sequence** of Break/EnableMotion inputs separated by fractional-second delays. This is not a simple toggle. The new synthetic C++ test independently authors a tiny fake relay with 0.10, 0.15, 0.23, 0.25 and 0.30-second connections. It verifies correct delivery ordering, two-target wildcard fanout, `!activator` context preservation and one-shot versus repeating links, together with invalid connection payload handling.

No original map entity names, positions, graphics, scripts or proprietary source snippets appear in the test.

## Reproduction

On the Phase 77 branch:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
python3 -m unittest discover -s research/tests -p 'test_*.py'
python3 research/tools/audit_sin_sdk_io_patterns.py /private/path/SinEpisodesSDK.zip --output /private/path/sdk-io-patterns.json
```

The last command requires the owner's original private SDK. Its generated report should stay outside git. Four additional Python synthetic fixture tests and one native CTest exercise the new functionality. Local standalone compilation used GCC C++17 with `-Wall -Wextra -Wpedantic -Werror`. GitHub CI should independently run CTest, Python, ARM64 compilation, and the existing Android Bionic emulator smoke (which still exercises only the separate weapon ABI, **not** this parser).

## Remaining P0 issues

- No accessible, legally usable Quest-capable SourceVR **game module loader/headers**, issue #12.
- No map `.vmf`/compiled `.bsp` engine entity instantiation, engine time service, target-name resolver or real `AcceptInput` callbacks, issue #20.
- No original level transition/save-restore, NPC combat logic, actual dynamic difficulty or headset gameplay demonstrated.
- Parsed connections are not loaded into a real Source map and this **does not** imply original Source event-queue parity.

**Next milestone:** extend map-driven portable synthetic tests around NPC makers/difficulty, then test with real engine hooks only after the R1 dependency is available.
