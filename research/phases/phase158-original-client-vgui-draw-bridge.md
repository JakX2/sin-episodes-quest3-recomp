# Phase 158: original client VGUI seven-to-nine draw bridge and two native paths

**10 November 2026.** Static, original-byte-grounded reconstruction of the actual owner-supplied SiN Episodes: Emergence (2006) Windows x86 client.dll, stacked on Phase 157. We recover a complete small draw adapter and the **pure numerical branches** of its 757-byte native drawing worker. This is not a complete VGUI renderer or working game DLL.

## Binary identity

Original client.dll SHA256: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d

Original server.dll SHA256: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650

| Original client function | Full body length | Original function-region SHA256 | Actual coverage |
|---|---:|---|---|
| 0x241417C0 | 69 bytes | 1907a1db9b89a39b6e4d9583ecfddd9c3c6da4cd47a3096571723747870dc637 | All original seven-to-nine argument expansion, signed widths and native call/return semantics |
| 0x24141DD0 | 757 bytes | ee860c6fc19ae1f9954dd8a40920be425f6a5191dbb332198087868508de1766 | Verified entry routing, clipping interpolation and UV arithmetic; original VGUI calls remain unimplemented |

Original hashes, 33 deliberately partial opcode witnesses and 17 native CALL sites were checked directly against the original executable bytes. No game DLL bytes, assets or executable disassembly are committed.

## Recovered complete wrapper

At 0x241417C0, native THIS receives an original engine resource object, and the routine reads **seven 32-bit caller stack words**. It computes the two rectangle dimensions (native signed 32-bit subtraction), then calls 0x24141DD0 with nine stack words:

| Callee word position | Native source |
|---:|---|
| 1 | Caller word 1 |
| 2 | Caller word 2 |
| 3 | Original int32(this+0xAC) - int32(this+0xA8) |
| 4 | Original int32(this+0xB4) - int32(this+0xB0) |
| 5 | Caller word 3 |
| 6 | Caller word 4 |
| 7 | Caller word 5 |
| 8 | Caller word 6 |
| 9 | Caller word 7 |

The original adapter calls the worker at 0x241417FD and returns with RET 0x1C; its worker returns with RET 0x24 at 0x241420C2. These are machine-proven original x86 stack contracts, not guessed C++ parameter types. The new portable expansion preserves all raw 32-bit word payloads, even if they represent original pointer or float bits.

## Worker routing: original instance fields

At 0x24141DD0, byte THIS+0x84 determines the first branch. When nonzero, the code uses the **surface clipping** path. When zero, the texture-ID integer at THIS+0x94 is checked: ID == -1 returns immediately; all other IDs use the **textured-subrectangle** path, even if negative.

Other original object fields read include a surface handle +0x88, UV floats at +0x98, +0x9C, +0xA0, +0xA4, rectangle bounds +0xA8, +0xAC, +0xB0, +0xB4, and an optional chained resource pointer at +0x00. The source-level class name has NOT been proven.

### Surface-clip route (+0x84 != 0)

The native routine retrieves an integer surface limit through virtual offset +0xFC, then uses x87 FILD and FIDIV to produce and store:

    ratio = float32(int32(surface_limit - worker_arg8) / surface_limit)

The numerator uses original 32-bit wrapping subtraction. The worker calls a surface clip callback through virtual +0x18C. On rejected clipping, it exits without final surface draw (+0x190). On success, it can adjust four float vertex slots (original temporary stack +0x40, +0x48, +0x50, +0x58):

- Worker argument 6 nonzero: update the first pair toward the second by ratio.
- Otherwise, when argument 8 differs from surface limit: update the second pair toward the first by 1 minus ratio.
- Otherwise: leave both vertex pairs unchanged.

Original ratio is rounded to float32 before use; subsequent vertex math is x87 extended precision until per-value float32 stores. The portable plan explicitly models clipping success/failure, branch order, signed subtraction, NaNs and division by zero without attempting native VGUI callbacks.

### Textured-subrectangle route (+0x84 == 0, texture ID != -1)

The original routine calculates four UV texture coordinates from two rectangle dimensions W/H, four float UV bounds, two crop offsets (worker args 5 and 6) and two spans (worker args 7 and 8):

    du = float32(float_at_A0 - float_at_98)
    dv = float32(float_at_A4 - float_at_9C)

    u0 = float_at_98 + offset5 / W * du
    v0 = float_at_9C + offset6 / H * dv
    u1 = float_at_98 + (offset5+span7) / W * du
    v1 = float_at_9C + (offset6+span8) / H * dv

The native width reciprocal and both UV spans are stored to float32; height reciprocal remains extended in the x87 stack. Original coordinate integer sums wrap modulo 2^32; there is no native zero-extent guard. The independently authored geometry helper explicitly represents these operations and can report a planned optional chained draw when THIS+0x00 is nonnull.

Native drawing then uses the engine surface singleton, virtual texture-ID setter +0x70, virtual color setter +0x18, textured-subrectangle draw +0x178, and optional nested draw adapter at 0x241420B6. **The portable model never claims those engine callback side effects took place.**

## Cross-referenced original calls

Seven independently verified original direct CALL sites reach 0x24141DD0:

0x241417FD, 0x2414220B, 0x24154EA0, 0x24154F62, 0x24155072, 0x24155129, 0x24155239.

Five are inside the already verified original 1271-byte Phase157 shared HUD renderer. The worker calls the original surface getter 0x24001040 at nine verified addresses and invokes wrapper 0x241417C0 at 0x241420B6, totaling 17 direct CALL edges.

## New code, tests, atlas

- include/sin_compat/original_client_vgui_draw_bridge.hpp: C++17 reference for complete argument adapter, branch routing, x87 clipping interpolation and texture UV preflight, using existing Phase157 rectangle helpers.
- tests/test_original_client_vgui_draw_bridge.cpp: real original word ordering, signed wrap, texture-ID branch, accepted/rejected surface clip, forward/reverse interpolation, NaN/zero divisor, UV bounds, chain flag and composition with Phase157 HUD preflight.
- research/tools/audit_sin_original_client_vgui_draw_bridge.py: exact original two-DLL SHA chain, two complete native method SHA pins, 33 sparse opcode anchors and 17 direct call sites.
- research/tests/test_sin_original_client_vgui_draw_bridge.py: original-free synthetic per-witness corruption checks, symbolic x86 stack-word mapping and 168 independent integer clip cases.
- Root CMake/CTest test registration.
- Phase152 reviewed method ledger expands from **40 to 42 entries** (25 server / 17 client): 69-byte adapter **behavior_reconstructed**, 757-byte engine draw worker **machine_code_verified**, with only partial geometry in the portable model. These manually reviewed ledger entries are NOT the total number of decompiled native methods in all older phases.

Owner-only original-audit command:

    python3 research/tools/audit_sin_original_client_vgui_draw_bridge.py --client /private/client.dll --server /private/server.dll --output /private/phase158-client-vgui-bridge.json

Original-free tests:

    python3 -m unittest discover -s research/tests -p test_sin_original_client_vgui_draw_bridge.py -v
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
    cmake --build build --target sin_original_client_vgui_draw_bridge_tests
    ctest --test-dir build -R sin_original_client_vgui_draw_bridge_tests --output-on-failure

## Next steps / important limitations

Recover original VGUI surface lifetime/interface behavior at virtual +0xFC, +0x18C, +0x190, +0x70, +0x18, +0x178; continue the separate x87 native helper at 0x241420D0; trace timer HUD +0xA6 setters through actual class ownership. Exact extended x87 last-bit parity on non-x86 targets and original VGUI ABI remain unproven.

No complete original C++ source, live original client.dll rebuild, playable Windows game, Android gameplay or Quest 3 port is claimed.
