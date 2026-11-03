# Phase 84: original SiN client.dll/server.dll CreateInterface and virtual tables

**Evidence:** owner-held original Windows x86 SiN Episodes (2006), exact SHA-256-matched `client.dll` `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`, `server.dll` `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. No proprietary DLL bytes committed.

## True factory interfaces

| Game DLL | Original PE export `CreateInterface` | Registration function | Registry head | Registered names |
|---|---|---|---|---:|
| client.dll | `0x24164180` | `0x24164160` | `0x24369378` | **55** |
| server.dll | `0x221FB5F0` | `0x221FB5D0` | `0x22573C5C` | **10** |

The exact original x86 registration uses **12-byte linked nodes:** function callback pointer `+0`, case-sensitive interface name `+4`, next pointer `+8`. The constructor prepends the newly registered node to a BSS head. `CreateInterface` walks nodes, compares names exactly, calls the matching callback, and sets optional status **0 on found, 1 on absent**. Runtime list order depends on static initialization order, not alphabetical names.

### Eight client game-facing registrations

| Interface name | Callback VA | Returned singleton VA |
|---|---|---|
| `GameClientExports001` | `0x240FD810` | `0x242FA4B0` |
| `VClient011` | `0x240FD840` | `0x242FA4B8` |
| `VClientEntityList003` | `0x241056B0` | `0x2434FDDC` |
| `ClientLeafSystem001` | `0x24108B20` | `0x24357F30` |
| `IEffects001` | `0x24127780` | `0x242FB554` |
| `VCENTERPRINT002` | `0x241EC540` | `0x2431DBF0` |
| `GameMovement001` | `0x2420FA40` | `0x24379920` |
| `VClientPrediction001` | `0x2422EA30` | `0x2437FA00` |

Another **47 original client factory registrations** are named `*_IMaterialProxy003`. All 55 individual initializer/callback/node entries appear in the auditor's metadata JSON.

### Ten server registrations

| Name | Callback VA | Returned singleton VA |
|---|---|---|
| `IEffects001` | `0x22129680` | `0x224ECE00` |
| `ServerGameDLL003` | `0x221E7630` | `0x22510650` |
| `ServerGameEnts001` | `0x221E82C0` | `0x22510654` |
| `ServerGameClients003` | `0x221E8320` | `0x22510658` |
| `PluginHelpersCheck001` | `0x221FC4F0` | `0x22511A14` |
| `PlayerInfoManager001` | `0x22257780` | `0x2251A048` |
| `PlayerInfoManager002` | `0x222577A0` | `0x2251A048` |
| `BotManager001` | `0x222577B0` | `0x2251A04C` |
| `HLTVDirector001` | `0x2225F8C0` | `0x2257AEA0` |
| `GameMovement001` | `0x222CFFE0` | `0x225934B0` |

There are also **four extra MSVC-decorated `CGrenade*::Detonate` PE exports** in `server.dll`. These are exports, **not** additional registered game interfaces.

## Exactly verified original vtable memory

Each listed singleton's first 32-bit word points at the original table. The audit checks all consecutive pointer words against their **full-array SHA-256**, requires each target in the PE `.text` section, and checks that the following word does *not* continue the method-pointer run:

| Interface | Original vtable VA | Verified consecutive executable pointer slots |
|---|---|---:|
| `VClient011` | `0x24296C6C` | **48** |
| `GameClientExports001` | `0x24296B18` | **4** |
| `ServerGameDLL003` | `0x22442EFC` | **31** |
| `ServerGameEnts001` | `0x22443030` | **7** |
| `ServerGameClients003` | `0x22443054` | **15** |

Crucially, the three server interface singleton pointers are **four bytes apart** at `0x22510650/54/58`, referring to **three adjacent x86 virtual-interface subobjects**. This is strong direct memory-layout evidence; it is *not* yet a complete method-signature or ARM64 class-layout map.

## Actual Android runtime mismatch

In the user-supplied **HL2VR 0.989** APK, both native AArch64 `libclient.so` and `libserver.so` export `CreateInterface`. Their embedded interface-version strings include **`VClient017`** and **`ServerGameDLL010`**, versus SiN's proven registered **`VClient011`** and **`ServerGameDLL003`**. Other strings overlap, such as `GameMovement001`, but a string alone does not prove a native factory's current registration. The separately supplied SourceVR 0.5.5 archive has different native module hashes and requires its own actual ELF interface audit.

**These differences mean Windows binaries cannot be renamed to `.so` or old interface identifiers simply aliased to modern versions without method-level validation.** The native ARM64 SourceVR ABI, arguments, lifecycle, entity registration, callback order and threading remain to be established.

## Implementation and tests

- `research/tools/audit_sin_original_factories.py`: original dual-DLL SHA-pinned PE32 export/registration scan, exact factory and constructor instruction checks, 55+10 entries, 5 full-array vtable hashes; outputs metadata only.
- `research/tests/test_sin_original_factories.py`: **11 synthetic, original-binary-free mutation regression cases**.
- `include/sin_compat/original_factory_contract.hpp`: independent host-side C++17 linked registry model. **Not** an actual Source game-module factory.
- `research/tests/sin_original_factory_reference.cpp`: case-sensitive version matching, found/missing status, duplicate registration precedence, unrelated client/server registries and null return-code tests.
- `CMakeLists.txt`: CTest registration.

Private original DLL audit passed, with 11 local Python test cases and strict C++17 reference test passing. GitHub Actions status tracked separately.

## Phase 85

1. Trace original `VClient011` and `ServerGameDLL003` **method signatures and engine initialization** from their verified vtables (not just pointer totals).
2. Inspect **actual native SourceVR 0.5.5 ELF modules** for registered `CreateInterface` names and method ABI; do not assume HL2VR 0.989 identity.
3. Implement a real adapter only once the initialization/lifecycle ABI is grounded; then loader smoke against the selected Android runtime and a SiN map.
4. Continue reconstructing entity factories, prediction, HUD/VR and server gameplay logic in their proper modules.

**Status:** concrete original DLL factory/ABI reverse engineering, *not* full Ritual client/server decompilation or a running Quest 3 port.
