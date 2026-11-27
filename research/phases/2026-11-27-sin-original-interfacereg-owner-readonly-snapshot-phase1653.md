# Phase 1653: Inspect original 2006 PE32 client/server InterfaceReg lists after DLL load

**Stack:** Phase 1652 PR #1661, exact head `7e7448035d726826d181b283df7d38d98500547b`, original 48-slot `VClient011` audit and native replacement owner checks. Its final-head CI was pending when this branch was created.

## Why this test is closer to actual startup

Original DLL byte audits prove static registration *instructions* and expected linked-list entries, but they do **not** show whether the original Windows loader actually ran every static constructor and linked the registrations into the module-local registry at runtime.

The older Phase 657 owner-side Windows probe already verifies **exact original SHA-256** of the owner's `client.dll` and `server.dll` before using `LoadLibraryExA(LOAD_WITH_ALTERED_SEARCH_PATH)` to load those trusted original PE32 DLLs. It then invokes only their exported `CreateInterface` on three known version strings per DLL (plus a known-missing name), and reads other globals. **Original DllMain/static constructors and those original factory thunks are executed in that owner-side tool.** This is separate from the already-tested zero-write *external process* edict observer. Public CI does not have or execute either retail DLL.

Phase 1653 extends that exact SHA-gated Windows probe with a **read-only, no-vcall, no-creator-invocation post-load snapshot** of the now-populated `InterfaceReg` list. The code only calls `ReadProcessMemory(GetCurrentProcess())` after the trusted original module is loaded; it does not dereference registry pointers directly.

## SHA-derived original 2006 PE32 section boundaries

The layout is based on actual original on-disk PE32 section headers and the full manifests already authenticated in Phase 177. These are **not** source SDK 2013 constants or guessed pointers.

| Field | Client.dll | Server.dll |
| --- | --- | --- |
| Original image base | `0x24000000` | `0x22000000` |
| Loaded image size | `0x3D9000` | `0x628000` |
| `InterfaceReg` global list head RVA | `0x369378` | `0x573C5C` |
| `.text` RVA extent | `0x1000..0x275FBA` | `0x1000..0x3D111A` |
| `.rdata` RVA extent | `0x276000..0x2F29FD` | `0x3D2000..0x4D5626` |
| `.data` RVA extent | `0x2F3000..0x38F320` | `0x4D6000..0x5A0B24` |
| Static registration manifest count | **55** | **10** |

The module base is taken from the **actual loaded** HMODULE, so legal PE32 loader relocation is accounted for. The vtable/InterfaceReg schema was recovered previously from the exact MSVC x86 constructors: each record contains a creator pointer at `+0x00`, interface-name pointer at `+0x04`, and next pointer at `+0x08`, **12 bytes** total.

The snapshot checks the expected manifest count and priority identity set. Client priority names include `VClient011`, `VClientEntityList003`, `VClientPrediction001`, `GameClientExports001`, `ClientLeafSystem001`, `IEffects001`, `VCENTERPRINT002` and `GameMovement001`. Server priority names include `ServerGameDLL003`, `ServerGameEnts001`, `ServerGameClients003`, both PlayerInfoManager revisions, and the five other core original registrations from Phase 177.

## Implementation and guardrails

`pc/independent_slot0/original_interfacereg_readonly_snapshot.h` is a pure C++17 callback-driven PE32 memory-view validator. It refuses to read **any original pointer** outside the pinned section extents in its supplied layout. Specifically:

- Validate module address overflow, section ordering, head inside the image, expected counts, trusted priority-name set and callback existence
- Read the head through callbacks; follow at most **64** linked records, with cycle detection and 12-byte record bounds inside mapped `.data`
- Require each creator pointer to reside inside the original module's `.text`; require name pointers and every byte of their NUL-terminated ASCII names inside `.rdata`
- Bound each name to **96 bytes**, forbid empty/unprintable names and duplicate registration names
- Compare the module head before and after the traversal, reject changed heads and inconclusive reads
- Verify exactly 55 client or 10 server records and the specified priority names; emit only status label, count and priority count, never original interface addresses or raw code

A successful verdict is deliberately called `verified_unpinned`. It is **not** a lock on the original engine, not an atomic registry snapshot, and does not exclude an ABA change between individual reads.

The existing `research/tools/sin_win32_factory_probe_phase657.cpp` uses that callback validator for **both** original modules after its existing SHA-checked load, then emits a new JSON-line event `original_interfacereg_snapshot` with expected and observed counts. It never calls an individual registered creator through the traversal itself. No original application process is patched.

## Tests

`pc/independent_slot0/test_original_interfacereg_readonly_snapshot.cpp` constructs only fake PE32 address tokens and bytes in a `std::map`, not an original DLL or Windows game process. It covers:

- Complete synthetic 55-entry client and 10-entry server registration lists, including 8/10 exact priority names
- All cross-section and relocated module bounds; head unreadable or empty; creator outside `.text`; name outside `.rdata`; linked node outside `.data`
- Unreadable records/names, malformed and unterminated C strings, duplicate interface names and duplicate list nodes/cycles
- Registry head changed between the two readbacks, wrong expected record count, missing priority, excessive 65-node linked lists

The Windows MSVC x86 native CMake build also **compiles** the owner probe executable with `bcrypt.lib` but never invokes it against commercial DLLs. One new Windows CTest case is expected, increasing the total from Phase 1652's expected 251 to **252**. No GitHub Actions runner is supplied with proprietary original files or asked to run the original game.

Owner-only optional invocation after building the helper and placing the two original files with their dependencies in a trusted **disposable** installation copy:

```powershell
.\sin_original_interfacereg_owner_probe.exe `
  "C:\SiN-Test-Copy\SE1\bin\client.dll" `
  "C:\SiN-Test-Copy\SE1\bin\server.dll"
```

The example paths are illustrative. This **loads and initializes** the original game DLLs within the owner's isolated test process; do not treat it as an external read-only attachment or run it against a production installation just to test the registry. The original owner files are never modified by the observer, though their normal DllMain may have its own effects. No such owner-local live execution was performed by ChatGPT or public CI.

## Limits

This tests the **original** factory registration list, not the current independent replacement's incomplete factory set. Even a perfect observation would not show the independent `VClient011::Init` completing, nor would it prove the original engine can use an independent `ServerGameEnts001` object safely. It does not construct game entities or implement missing gameplay/HUD methods. Windows gameplay and Android/Quest 3 startup remain unverified.

Unrelated HL2-equivalent physics, surfaces, props and model loading stay out of scope under the user priority override.
