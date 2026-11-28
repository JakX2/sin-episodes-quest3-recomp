# Phase 1678: original 93-byte PostNetwork cdecl diagnostic formatter

## Exact original retail discovery

The owner-provided Google Drive `sin files/SE1/bin.zip` was inspected **read-only**, containing original retail `client.dll` with SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`. The Drive's original retail `engine.dll` SHA256 is `f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81`. These match exact hashes required by existing repository provenance checks. **Neither binary is committed or redistributed**.

The previously opaque downstream formatter called from Phase1675's 30-byte binary-count and 33-byte signed-short diagnostic helpers has been bounded and source-authenticated:

| Original retail property | Verified value |
|---|---|
| Client native entry VA | `0x24197FE0` |
| Entire executable method | **93 bytes**, through original single-byte `RET` at `0x2419803C` |
| Whole original 93-byte SHA256 | `880a186d02da86b94b953158d2fdaee8a93d59456fdaf4f482031da3bab09cac` |
| Compiler padding | 3 `INT3` bytes before next method at `0x24198040` |
| x86 calling convention | plain `__cdecl`, **RET0** (callers clean three original DWORD arguments via `ADD ESP,0x0C`) |
| Local stack buffer | 4,096 bytes with compiler stack-probe helper `0x242493E0` |
| First import | `vstdlib.dll!Q_vsnprintf` via IAT `0x2427625C`, receiving buffer, 4096 capacity, format and variadic value address |
| Second import | `tier0.dll!Msg` via IAT `0x242761EC` |
| Exact original Msg template | `%i %s %s : %s\n`, stored at `0x242A3F90` |
| Original owner record pointer | `this+0x18` |
| Original selected record's name pointer | `[record+0x04]` |
| Original diagnostic label pointer | `this+0x38` |
| Original global context pointer | DWORD at `0x242FA458`, reads field `+0x18` |
| Native function HIGHLOW relocation sites | `0x24198006`, `0x24198021`, `0x2419802C`, `0x24198032` |
| Original direct scalar-format caller sites | `0x24198373`, `0x24198396` |

The literal format and the call order are read directly from the unchanged, SHA-checked original client.dll. Although original `Msg` field order is established, the **concrete retail owning classes, lifetime of its strings, global context meaning and logging side effects remain unknown**. Source code here does not assign invented engine identities.

## Original-free reconstruction and tests

The Phase1672 MSVC x86 synthetic dispatcher fixture now reserves the additional original DWORD **`this+0x38`** before any host callback fields. Its original `this+0x18`, `+0x2C`, `+0x34` offsets remain identical. This simultaneously protects Phase1677's native byte comparator, Phase1675's native scalar previews, and the newly recovered formatter context.

New `pc/independent_slot0/native_original_postnetwork_cdecl_msg_formatter.h` provides a safe original-free `static __declspec(noinline) void __cdecl FormatScalar(Fixture*,const char*,int32_t)` function. It uses bounded host `std::snprintf` to model the original 4096-byte `Q_vsnprintf` buffer and assembles the authenticated `Msg` template. Since only the original diagnostic calls with a single integer argument are proven here, it accepts exactly the source-pinned `binary (%i bytes)` and `short (%i)` formats, and refuses unknown strings. It authorizes the record pointer and record-name token at `+0x04`, the `this+0x38` label pointer, and the host-owned global context number. It emits only through an injected host-owned `Msg` callback. This is **not** a recreation of the full retail vstdlib formatter's general varargs semantics.

New `test_original_postnetwork_cdecl_msg_formatter.cpp` uses actual native MSVC x86 methods to exercise the Phase1675 `__thiscall RET16` helpers nested through the new `__cdecl RET0` method and the caller's full ESP restoration. It checks signed-short `-32768`/`32767`, negative binary byte counts, native record-selection suppression, exact original source message template and its four data inputs, and refusal of forged record names, wrong labels, unsupported formatter pointers or missing backend. Public code performs no arbitrary original-pointer dereference.

New `research/tools/audit_sin_original_postnetwork_cdecl_msg_formatter.py` SHA-gates both complete original retail client+engine, both 2006 original scalar caller functions and their E8 CALLs, the entire new 93-byte method SHA, structural x86 opcode witnesses, the four real HIGHLOW relocation sites, exact output literal, named imports, compiler padding and native RET. Its public `--self-test` constructs only **invented non-executable sparse opcode islands** and rejects covered mutations, altered bounds and forged retail SHAs. No proprietary x86 body, original DLL or game data is shipped in this repository.

## Verification and limitations

Phase1677 is the verified stacked parent: **300/300** native Win32 MSVC x86 tests, all five applicable GitHub Actions workflows green at exact head `55ad59ae200d61c317858435e56416a1e4f58518`.

Phase1678 adds **two** original-free CTests (whole original SHA audit synthetic and actual native cdecl formatter chain) for **302 target**, pending exact-head CI evidence. These tests do **not** constitute retail Source-engine game boot, production prediction interface initialization, Android or Quest3 boot. No physics, surfaces, props, models, materials or rendering work is added under the user's priority override.
