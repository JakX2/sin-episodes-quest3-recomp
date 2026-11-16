# Phase 250: original client EntityParticleTrailInfo RecvTable and CRT lifecycle

16 November 2026. Stacked draft on Phase249 PR #305. Based only on the owner-supplied original **SiN Episodes: Emergence (2006)** retail Windows PE32 gameplay modules. No original proprietary DLLs, whole native method bodies, disassembly dumps or game assets are committed.

## Strict owner-original whole-file provenance

| Original retail DLL | Whole file bytes | Complete SHA256 |
| --- | ---: | --- |
| client.dll | 3,592,192 | 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d |
| server.dll | 6,049,792 | 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650 |

Both complete original DLL hashes must match before any native method credit. The private auditor also checks entire individual original native function SHA256s, x86 return/alignment, native callsites and original loader relocations.

## Four newly authenticated original complete native functions

| Original client VA | Entire x86 bytes | Complete original native code SHA256 | Function |
| --- | ---: | --- | --- |
| **0x241281D0** | 151 | e698bb21d94f55d82c0ba326c696c30b94be1cf7c2f3817f9bf0f2762614173b | Lazy network RecvTable setup and three-property registration |
| **0x24268960** | 22 | 24f60e9f71ec970f8cbe34bcd6922716367b5df0e3b7d42337b0e40d3126d9ba | CRT table-object constructor and termination registration |
| **0x24268980** | 16 | 9066c780197419033104792af48e29300c27fe14781d802f78bbbc74a81556ec | CRT initializer calls table setup and caches its return EAX |
| **0x242736B0** | 10 | 4861e23fe970b18f32c9effba6e5d18711adce11b391326c118c157668179b6c | Cleanup thunk loads table object and tail-JMPs its destructor |

**Four new SHA-reviewed original functions, 199 native x86 code bytes. Reviewed catalog: 387 → 391 (server 147, client 240 → 244).** Reviewed whole-native-method coverage is not a decompilation percentage or a functional game DLL.

Native original evidence: **30 sparse opcode witnesses**, **four terminal returns/jumps**, **25 INT3 alignment bytes**, **eight original E8 rel32 CALL relationships**, one original E9 tail JMP, **22 absolute pointer operands and 22 matching genuine PE32 HIGHLOW relocation sites**, and five original ASCII strings in .rdata. The two static initializer pointer slots are located in the original .data section. The entire original PE32 SHA and each native function SHA were calculated from the owner-supplied retail client/server DLLs, not from fabricated synthetic test fixtures.

## Original DT_EntityParticleTrailInfo receive-property ABI

Original function 0x241281D0 observes global once-flag DWORD **0x24361040**, tests bit0, and ORs bit0 in before first-time property construction. On first invocation it creates four consecutive 60-byte receive property descriptors:

| Descriptor original VA | Original name and name VA | Observed member offset | Membership |
| --- | --- | ---: | --- |
| 0x24360F50 | should_never_see_this (0x242762D8) | 0 | **Internal placeholder, not part of registered table** |
| 0x24360F8C | m_flLifetime (0x24278E80) | +8 | Real networked float |
| 0x24360FC8 | m_flStartSize (0x24278DE4) | +12 | Real networked float |
| 0x24361004 | m_flEndSize (0x24278DD8) | +16 | Real networked float |

The placeholder uses native constructor **0x24126DF0**, and the three real float fields use native **0x24126D80** with 4-byte width and original float proxy VA **0x24126C40**. The real table name at original 0x24298EF8 is **DT_EntityParticleTrailInfo**, and original registration **0x24126A30** receives first real descriptor **0x24360F8C**, count **3**, and table object **0x24361044**.

The registration function is called **on every invocation** of 0x241281D0, even when first-time descriptor factories are skipped because the once-flag bit is already set. It always returns **EAX=1**.

### Original CRT startup and cleanup

Two adjacent original .data function-pointer entries, with genuine HIGHLOW relocations, identify related startup methods:

| Original .data DWORD | Function pointer | Observable behavior |
| --- | --- | --- |
| **0x242F3A38** | **0x24268960** | Constructor of global table object and registration of cleanup thunk |
| **0x242F3A3C** | **0x24268980** | Lazy RecvTable initializer and storing EAX to original global 0x24361058 |

Native 0x24268960 calls **0x24126D30** on global table object **0x24361044**, then passes cleanup pointer **0x242736B0** to native registration helper **0x24248788**. Native 0x24268980 pushes zero, calls 0x241281D0 and stores its EAX=1 in global **0x24361058**. Native cleanup thunk 0x242736B0 loads ECX=0x24361044 and tail-JMPs to **0x24126A20**, the opaque original table destructor. Physical pointer order in .data is verified; wider CRT execution order is not inferred.

### Nine verified native rel32 relationships

| Original site | Original destination | Meaning |
| --- | --- | --- |
| 0x241281F2 E8 | 0x24126DF0 | placeholder descriptor constructor |
| 0x2412820C E8 | 0x24126D80 | m_flLifetime float constructor |
| 0x24128226 E8 | 0x24126D80 | m_flStartSize float constructor |
| 0x24128243 E8 | 0x24126D80 | m_flEndSize float constructor |
| 0x2412825C E8 | 0x24126A30 | real three-field table registration |
| 0x24268965 E8 | 0x24126D30 | global table-object constructor |
| 0x2426896F E8 | 0x24248788 | cleanup function registration |
| 0x24268982 E8 | 0x241281D0 | static initializer calls native table function |
| **0x242736B5 E9** | **0x24126A20** | global table-object destructor tail jump |

## Independent reference code and evidence tests

- research/tools/audit_sin_original_client_particle_trail_recvtable_crt.py: original-only dual DLL SHA gates, four whole-method code hashes, 30 independent opcode witnesses, returns/INT3, all 9 E8/E9 links, 22 original pointers/HIGHLOW loader records, five original property/network name strings.
- research/tests/test_sin_original_client_particle_trail_recvtable_crt.py: synthetic-only byte-mutation checks for method witness/exit/padding, all original call edges and pointers, .data/.rdata/.text section checks, every character in original network names, original PE image base and correct accounting. **Synthetic fixtures earn zero authentic original-code credit**.
- include/sin_compat/original_client_particle_trail_recvtable_crt.hpp: independent C++17 reference of three real float descriptors, distinct dummy, exact native addresses, once-bit flag/initializer, unconditional register and separate original CRT constructor/initializer/cleanup callback sequence.
- tests/test_original_client_particle_trail_recvtable_crt.cpp: tests native original constants, 60-byte descriptor stride, exact three networked floats and excluded placeholder, callback order, repeated table registration, cached CRT result and cleanup, and exceptional synthetic factory behavior.
- Root Linux C++17 and true MSVC Windows 32-bit x86 CTests; reviewed-original method JSON catalog updated to 391.

Private owner-original audit command:

    python3 research/tools/audit_sin_original_client_particle_trail_recvtable_crt.py --archive /private/bin.zip

Synthetic-only regression command:

    python3 -m unittest discover -s research/tests -p test_sin_original_client_particle_trail_recvtable_crt.py -v

## Limits of current recovery

Original native float receive-proxy implementations, RecvProp factory implementation, engine-managed client entity lifetimes, Windows CRT object registration and independently loadable reconstructed gameplay client.dll/server.dll still require additional work. **This phase is not independent game boot**, and a factory-forwarding proxy or Android dlopen smoke cannot establish it.

Platform order remains original **Windows PC gameplay first → Android ARM64 next → Quest 3 VR third**.
