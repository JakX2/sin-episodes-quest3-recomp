# Phase 1651: Original SiN 2006 server.dll InterfaceReg startup ABI

**Stack:** Phase 1650 PR #1659, exact head `64340e1906e43347c08df51064e9c393b839f6a4`. Its original-free decomp synthetic audit passed and its actual owner-private 2006 engine.dll allocator/relocation evidence was verified. The remaining Phase 1650 native Windows/Android workflows were still queued/in progress when this branch was created.

## Why this matters

The goal is not only to decompile isolated functions, but to register **game DLL interfaces that the original compatible engine can request**. The previously recovered original `ServerGameEnts001` semantics are incomplete as a replacement retail service. This phase audits the **actual original server.dll static registration graph** instead of inventing an InterfaceReg list or prematurely exporting a partial original interface.

The Drive `sin files/SE1/bin.zip` contains the owner-original `server.dll`, SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`, PE32 i386, image base `0x22000000`. It was inspected locally and independently, without copying proprietary executable code to GitHub.

## Three source-authenticated static registrations

The original 2006 server.dll export `CreateInterface` resolves to original VA `0x221FB5F0`. The 32-byte original `InterfaceReg` constructor at `0x221FB5D0` takes the record in x86 `ECX` and two x86 stack arguments, stores three 4-byte values, and pushes the record onto the head at `0x22573C5C`.

| Interface | Static wrapper VA | Creator VA | 12-byte record VA | Creator-returned singleton VA | Singleton vtable VA |
| --- | --- | --- | --- | --- | --- |
| `ServerGameDLL003` | `0x223BF140` | `0x221E7630` | `0x225722EC` | `0x22510650` | `0x22442EFC` |
| `ServerGameEnts001` | `0x223BF210` | `0x221E82C0` | `0x225723E4` | `0x22510654` | `0x22443030` |
| `ServerGameClients003` | `0x223BF230` | `0x221E8320` | `0x225723F0` | `0x22510658` | `0x22443054` |

Exact source-derived `InterfaceReg` record:

```text
+0x00 creator function pointer    DWORD
+0x04 interface-name pointer     DWORD
+0x08 next registration pointer  DWORD
sizeof(InterfaceReg) = 0x0C
```

The constructor stores the two user-provided arguments, links `next` to the prior global head, then replaces the head with the current record. The original factory compares requested names, follows these records and invokes the matching creator. For a known interface, it writes result code `0`; for an unknown interface it sets result code `1` and returns null. The three creator functions are six-byte x86 `mov eax, imm32; ret` thunks returning those **distinct static singleton object addresses**, not an unverified newly allocated heap object.

These original references were checked against the full original private 2006 `server.dll` bytes: all three registration wrappers, the 32-byte constructor, the complete 114-byte exported factory body, three singleton return thunks and three first-vtable-pointer DWORDs.

## Reproducible owner-private audit

```powershell
py -3 research/tools/audit_sin_original_server_interfacereg_abi.py --server "C:\Games\SiN Episodes\SE1\bin\server.dll"
```

The path is illustrative; use the actual unmodified original DLL or extract it locally from the owner's original `bin.zip`. The audit refuses wrong whole-file SHA-256, non-PE32/wrong image base, modified constructor or factory window hashes, modified registration strings, wrong static record locations, wrong creator thunk or singleton/vtable pointer, wrong DLL export count/name/target. It reports names and address metadata only. It never imports, loads, executes or writes into the DLL and never exports original instruction bytes.

The original server export table contains five named symbols: `CreateInterface` and four `CGrenade*::Detonate` methods. The original `client.dll` exports only `CreateInterface`; that separate fact does **not** make either reconstructed replacement DLL startup-ready.

## Native/regression evidence

`--self-test` uses invented x86 21-byte registration wrappers, 32-byte record constructor patterns and six-byte static singleton thunks. It mutates every constructor byte and each registration wrapper byte, checks exact address/call targets, wrong creators/record pointers, and fake original-SHA rejection. The synthetic fixture does not load proprietary game code. The regression is registered in the Linux original-free decomp audit and Windows MSVC Win32 CTest, expected total **250** if Phase 1650's 249-case baseline is green.

## Limits

This establishes the **original interface registration ABI** and original singleton creator addresses, but not safe independent instantiation, correct complete virtual tables, engine-to-DLL ownership, real `ServerGameEnts001` wiring, original engine callback ordering, or playable Windows/Android/Quest 3 startup. No retail process was launched in the assistant environment. Do **not** enable an incomplete `ServerGameEnts001` factory entry or misrepresent the original singleton data as an active host pin.

Under the user's priority override, no unrelated HL2-equivalent physics, surfaces, props or model loading is added.
