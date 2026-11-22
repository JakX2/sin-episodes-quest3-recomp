# Gameplay Phase607: all eight original Grunt AI behavior eligibility vfuncs

## Original 2006 binary source, not modern SDK inference

The owner-original 2006 SiN Episodes: Emergence server.dll and client.dll are available from their connected Drive SE1/bin.zip. Exact original full SHA256 server 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650; client 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.

Phase606 directly authenticated the original MobGrunt behavior selector 0x2234E590, which traverses the Phase535 behavior registration vector and invokes **each candidate behavior's vtable byte offset +0x18** before accepting/activating it. This phase reads the authentic eight original RTTI-backed behavior vtables and recovers their **eight distinct concrete +0x18 method bodies and original control-flow gates**, without borrowing 2013 Source SDK implementations. No original executable or raw disassembly is distributed.

## Eight complete original native methods, 659 bytes

| Actual original registered behavior | Native eligible virtual +0x18 | TRUE whole-method size | Exact original full SHA256 |
| --- | --- | ---: | --- |
| CAI_RappelBehavior | 0x220DD770 | **39** | da145e5611e0ae6b7652baf1c8feb2c8860deb74d4ce6a2ec8d99de99da5b7f7 |
| CAI_FuncTankBehavior | 0x22392830 | **81** | 383cef6c407791c89da83a3b218c1a763865fcf5a5c2acc097f4a989ecf9df13 |
| CAI_TakecoverBehavior | 0x22398250 | **103** | 74868593fcb6cb8efc228c9b555a8596e6128a80d8e8470b980e8568cd19b6d9 |
| CAI_BeserkBehavior | 0x22391D50 | **25** | 99990331374c0ef4afd6108e86e138ae77cf66c8304f64bd51a10fd73d64731b |
| CAI_ActBusyBehavior | 0x2238D8C0 | **73** | 7d436aaae82633a72e6c6d87c81a221a7ef675954e128b792b66dd46d48c5ad1 |
| CAI_AssaultBehavior | 0x220CEAF0 | **255** | 0c5b688341149ae7ed28a49f9f930c87116810c67f1107a444e48742ba570466 |
| CGruntStandoffBehavior | 0x220DF3F0 | **52** | 57f2f114d160db1b46c8c3a36809487f138e508df8b72f4ea9f2ef7fb49b8ff6 |
| CAI_FollowBehavior | 0x220D15D0 | **31** | c60968717e032c273c8f7a7c5d32edb08f3f8d8d44aa3781d3f68d11579fd542 |

Original RTTI vtables at +0x18, complete method SHA256s, true end padding and source opcode witnesses are independently checked. Entry values are all genuine original 2006 code addresses in executable .text.

### Two particularly dangerous end-boundary corrections

A naive scan for four consecutive INT3 bytes after a function would overcount two critical methods:
- **Assault** eligibility actually terminates at exclusive **0x220CEBEF** (255 bytes), with only **one** original INT3 at 0x220CEBEF. A DIFFERENT native routine begins at **0x220CEBF0**. A longer naive scan might call it a 361-byte predicate and attribute another method's code to Assault.
- **Follow** eligibility terminates at exclusive **0x220D15EF** (31 bytes), with only **one** original INT3 at 0x220D15EF. Another separate routine begins at **0x220D15F0**. Naive scan could report 187 bytes, which is wrong for the original +0x18 vtable method.

These real original source extents are verified by whole-method SHA, native one-byte padding and the exact next function's opening opcode bytes. The other six methods have authentic 7–15 byte INT3 pads.

## Original gameplay eligibility conditions now observed

These are source-gated behavior rules, **not recovered original developer symbolic variable names**:

**Rappel (+0x18 at 0x220DD770):** invoke bound owning NPC virtual+0x450. If it returns false, ineligible. Otherwise return true when behavior byte+0x11 is nonzero, or when behavior byte+0x12 equals zero.

**FuncTank (0x22392830):** original behavior handle at +0x14 must be non-invalid and resolve through the game's serial-checked entity table at 0x224F5620; owner virtual+0x450 must be true; and owner condition check through original 0x220397C0 with raw condition ID 0x3B must be false. Only then eligible.

**Takecover (0x22398250):** uses x87 float comparisons involving member+0x68, original global time state at 0x22572190, and original constant 0x223D2590. It also consults owner virtual+0x5C0 mask 0x08000000; when absent, clears behavior byte+0x11. Requires the enabled behavior flag, NPC field+0x850 equal 3, and original 0x222AA640 to return a non-null result on the qualifying branch. No guessed engine-level time-unit names are assigned.

**Beserk (0x22391D50):** owner virtual+0x450 must return true; otherwise ineligible. On success, returns the stored behavior byte+0x11. The game's RTTI spells **Beserk**, not Berserk.

**ActBusy (0x2238D8C0):** immediately eligible when any of its behavior bytes +0x20, +0x11 or +0x22 is nonzero. Otherwise byte+0x10 must be set, original helper 0x2238D340 must succeed, and an x87 comparison involving member float+0x24 and a time-state float must pass. Exact historical semantic flags and floating comparison boundary names remain unresolved.

**Assault (0x220CEAF0):** owner virtual+0x450 must succeed; condition ID 0x3B through native 0x220397C0 must be absent; multiple original float-time, target/pointer and helper conditions (including 0x220CE240, 0x220CE350 and original owner virtual+0x154) then determine eligibility, with behavior flag+0x26 affecting final return. This is a complete original native function but **not yet a fully portable, all-branches semantic reconstruction**.

**GruntStandoff (0x220DF3F0):** if behavior byte+0xCD is clear, it clears behavior byte+0x31. Requires byte+0x31 nonzero, owner field+0x850 equal 3, and original 0x222AA640 non-null. Its full method and true return path are authenticated.

**Follow (0x220D15D0):** owner virtual+0x450 must succeed, and then the original method tail-dispatches to behavior virtual **+0xE4**. The target(s) of that second eligibility subpredicate still need to be recovered. The short +0x18 method alone is NOT enough to decide Follow availability.

## Next work

The original Phase606 portable C++ selector models ordered first-eligible selection with Host callbacks and no invented eligibility values. Phase607 provides actual native method addresses and partial/complete branch predicates to implement those callbacks incrementally, but it does **not** silently pretend all eight complex eligibility methods have fully recompiled gameplay semantics. Prioritize tracing Follow +0xE4, Assault 0x220CE240/0x220CE350 and Takecover's float/owner state; then write precise host-side parity tests.

## Authentic binary audit and synthetic regression

Read-only SHA-pinned native verifier: research/tools/audit_sin_original_grunt_eligibility_phase607.py

    python research/tools/audit_sin_original_grunt_eligibility_phase607.py --self-test
    python research/tools/audit_sin_original_grunt_eligibility_phase607.py --bin-zip /private/SE1/bin.zip

Validates both untouched original DLL hashes, eight RTTI-backed registered behavior vtable+0x18 function pointers, eight whole-function hashes, precise one-INT3 or normal post-method boundaries, two verified next-method signatures, seven original E8 direct helper calls plus Grunt-to-selector edge from Phase606, and 22 individual control/field opcode witnesses. Original-free self-test flips all five bytes of seven actual E8 calls (**35 invalid mutations rejected**) and wrong-source binary refusal.

**Phase607 adds eight fully native-code-authenticated methods / 659 original bytes. Phases601–607 cumulative: 33 distinct source-authenticated original functions / 10,685 original 2006 bytes**. This is not a new playable Windows server.dll or Android/Quest 3 port. Generic HL2-equivalent physics, surfaces, props, models and rendering remain deliberately excluded.
