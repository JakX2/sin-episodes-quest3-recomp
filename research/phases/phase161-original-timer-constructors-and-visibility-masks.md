# Phase 161 (RTTI-corrected): original Breath, Poison, and Health HUD constructors

**10 November 2026. Corrective revision following an original MSVC RTTI audit.** This report supersedes an earlier draft of Phase161 which incorrectly identified the CHudHealth constructor at 0x24235E80 as CHudPoisonTimer. The old assertion that poison uses visibility mask 0x18 was WRONG and has been retracted.

Owner-provided 2006 Windows PE32 originals:
- client.dll SHA256: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
- server.dll SHA256: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650

## Three genuine original constructor identities

| RTTI-proven class | Constructor address | Original code length | Full code-region SHA256 |
|---|---|---:|---|
| CHudBreathTimer | 0x24230F30 | 230 | 38c93563155918495201f0bbb99a75c339e8a16533033cb91917ef58424cb3b1 |
| CHudHealth | 0x24235E80 | 201 | 7dc7ea0b731d13de89fcd70f5ff34c5b64aa07ed62a38a1a1d097542817bcafb |
| CHudPoisonTimer | 0x24236610 | 233 | d7eef583516dc96c6fb1eb82bf419b5546c1ac4333509c970225d860b7071ff1 |

Method-region hashes prove code identity, not the class name. This revision adds proof of the native class name by checking each original 2006 MSVC TypeDescriptor, both CompleteObjectLocator pointers preceding its two vtables, and both COL-to-TypeDescriptor pointers. Twelve distinct native pointer checks now reject the previous type confusion.

| Class | Native primary vtable | Native secondary Panel vtable | Native TypeDescriptor |
|---|---|---|---|
| Breath | 0x242C78CC | 0x242C769C | 0x24320D34 |
| Health | 0x242C8DEC | 0x242C8B24 | 0x24320E8C |
| Poison | 0x242C91C4 | 0x242C8F94 | 0x24320EA8 |

Each constructor contains actual machine-code writes of those two vtable addresses to complete-object offsets +0x00 and +0x14.

## Correct three-class visibility masks

All three constructors call original CHudElement mask setter 0x24141850, but their pushed mask values differ:

- Breath: 0x10 at constructor 0x24230FFC, setter call at 0x24231000.
- Health: 0x18 at constructor 0x24235F27, setter call at 0x24235F3D.
- Poison: 0x10 at constructor 0x242366E1, setter call at 0x242366E5.

BREATH AND POISON SHARE MASK 0x10. HEALTH ALONE HAS THE ADDITIONAL BIT 0x08. When engine/player checks pass and effective player flags have bit 0x08, the original Phase160 CHudElement gate permits the two timers but suppresses health. All three remain subject to the bit 0x10 signed player virtual-state check. The previous opposite claim was an RTTI attribution error, not a subtle gameplay exception.

## Actual color member identity and constructor field stores

Both Breath and the REAL Poison constructor explicitly write zero to complete-object byte offsets +0xB7, +0xB8, +0xB9, +0xBA. The native Panel subobject begins at complete-object +0x14, so the same four bytes are Panel-relative +0xA3..+0xA6.

Original property metadata independently confirms this four-byte area is a Color property, not merely an unnamed status DWORD:

- Breath registrar at 0x24230DE0: owner name CHudBreathTimer, property name BreathTimerColor, member m_breathTimerColor, type Color, default 0 0 0 0, pointer helper 0x242309F0.
- Poison registrar at 0x24236450: owner name CHudPoisonTimer, property name PoisonTimerColor, member m_poisonTimerColor, type Color, default 255 255 255 0, pointer helper 0x24236030.

The fourth byte at Panel-relative +0xA6 is therefore part of the named Color field and is consistent with the alpha channel. Identifying the actual engine parser/setter and later writers, and whether alpha evolves during runtime, remains a separate research task.

The CHudHealth constructor at 0x24235E80 does NOT contain those same four explicit stores. Instead, it zeroes three DWORDs at complete-object offsets +0x2FC, +0x300 and +0x304. Those stores were wrongly attributed to poison in the earlier Phase161 draft.

## Eight small original timer member-address helpers

These remain valid and are correctly owned by the Breath/Poison classes:

| Original function | Owner | Bytes | Exact original SHA256 | Behavior |
|---|---|---:|---|---|
| 0x242309F0 | Breath | 16 | 7b425799d9562e26157a8051624ce73a5a995f1e1c2a0fa98f43f3eb7cf7762b | Nullable panel pointer +0xA3 |
| 0x24230A00 | Breath | 6 | bd36350ae3bfe08df6fb572ebee9f406ed4ea953003deb777e245d2f5a156e11 | Complete offset 0xB7 |
| 0x24236030 | Poison | 16 | 7b425799d9562e26157a8051624ce73a5a995f1e1c2a0fa98f43f3eb7cf7762b | Nullable panel pointer +0xA3 |
| 0x24236040 | Poison | 6 | bd36350ae3bfe08df6fb572ebee9f406ed4ea953003deb777e245d2f5a156e11 | Complete offset 0xB7 |
| 0x24230A10 | Breath | 16 | 5eb92598979c5339d1371426334129d2144e713d68e18894b79a2474b90cdae9 | Nullable panel pointer +0xA8 |
| 0x24230A20 | Breath | 6 | bcc59c532702d05a8d70becbe0c2599d8af785c9b1cba14bc44b166f2c7b5501 | Complete offset 0xBC |
| 0x24236050 | Poison | 16 | 5eb92598979c5339d1371426334129d2144e713d68e18894b79a2474b90cdae9 | Nullable panel pointer +0xA8 |
| 0x24236060 | Poison | 6 | bcc59c532702d05a8d70becbe0c2599d8af785c9b1cba14bc44b166f2c7b5501 | Complete offset 0xBC |

The original nullable helper returns 0 on zero input; nonnull addresses use native modulo-32-bit addition. The complete-object offsets differ by exactly the native Panel subobject displacement +0x14.

## Code, validation and honest coverage

- The C++17 constructor contract now distinguishes three actual RTTI classes: Breath and Poison install mask 0x10 and zero four Color bytes; Health installs mask 0x18 and zeros three tail DWORD fields. It models direct observed stores only, not original Panel constructor/registry calls.
- A corrected CTest exercises the three masks against the previously SHA-verified CHudElement global suppression gate and explicitly catches the old false poison 0x18 assertion.
- The owner-only auditor verifies 11 original full code-region SHA256s (3 constructors and 8 helpers), 12 additional MSVC RTTI/COL pointer relationships, exact constructor CALLs and sparse opcode witnesses, while enforcing original full-file client/server checksums.
- The original-free Python synthetic suite now mutates every class-identity RTTI pointer and catches misattribution.
- Reviewed recovery ledger contains 62 entries: 25 server / 37 client. The two timers and Health are machine-code-verified constructors with partial direct-field reconstructions; the eight tiny helpers are behavior-reconstructed. This ledger is not a total game decompilation percentage.

Private verifier command:

    python3 research/tools/audit_sin_original_client_timer_constructors.py --client /private/client.dll --server /private/server.dll --output /private/phase161-corrected.json

## Further work

Original Color registration and setter 0x2400A300, original VGUI Panel setup, callback lifecycle, and real rendering remain incomplete. Genuine Windows x86 game DLL integration is still a separate milestone before Android/Quest runtime work. No proprietary original binary data or executable code is committed.
