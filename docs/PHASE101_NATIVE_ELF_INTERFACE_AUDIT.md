# Phase 101: Native ELF interface contract audit (offline)

**Goal:** Replace speculation about SourceVR's native module interfaces with reproducible metadata from a specific, privately owned ARM64 \`.so\`. This is preparatory research for milestone **R1 (engine host)**, **not** a real adapter.

## Context

Our Phase 100 file listing confirms that the SourceVR APK bundles many content-addressed variants of \`libengine.so\`, \`libclient.so\`, \`libserver.so\`, \`liblauncher.so\`, and \`libvrxr.so\` alongside the Android bootstrap and bridge libraries. Names alone do not tell us which variant belongs to Half-Life 2 versus Portal or to a specific engine build.

The public SourceVR README (https://github.com/tinsarfal/SourceVR) currently states that content-only mods are supported but user-supplied \`client.dll\`/\`server.dll\` game-code mods are unsupported. The public repository does not provide a ready-to-build, documented third-party Android game-module SDK. Thus, **discovering \`CreateInterface\` alone will not make SiN playable**.

## Audit only one file locally

With a privately extracted ELF library (e.g. one \`libclient.so\` variant), run:

\`\`\`sh
python3 scripts/sourcevr_elf_contract_audit.py "/path/to/libclient.so" -o sourcevr-client-abi.json
python3 scripts/sourcevr_elf_contract_audit.py "/path/to/libserver.so" -o sourcevr-server-abi.json
python3 -m unittest discover -s research/tests -p 'test_sourcevr_elf_contract_audit.py'
\`\`\`

The inspector uses **only Python's standard library**, does not execute any code, and reads bounded ELF64 little-endian headers, \`SHT_DYNSYM\`, string tables and \`SHT_DYNAMIC\` entries. It records:

- ELF architecture (especially AArch64), SONAME and declared \`DT_NEEDED\` libraries.
- Whether exported native entry points such as \`CreateInterface\`, \`GiveFnptrsToDll\`, \`GetEntityAPI2\` or \`GetNewDLLFunctions\` are present in the dynamic symbol table.
- Compact example interface-related symbol names and dynamic linker imports.
- Explicit unknown results when the dynamic symbol table or section table is absent, without guessing the loader's behavior.

If a candidate module is ELF32 or lacks usable section headers, the inspector refuses to infer its ABI. The program refuses files exceeding 512 MiB, 8 MiB dynamic string tables, 200,000 symbols or 8192 sections. **There is no symbol-based license or binary-compatibility guarantee.**

Do not commit input libraries, APK contents, manifests or local JSON analysis if it contains private metadata. This audit works only on locally held, individually extracted ELF binaries. The Phase 100 package tool accepts a TXT listing or original APK without unpacking; this separate Phase 101 tool reads a single extracted module and never opens the user's huge APK.

## Gate to next code integration

A runtime adapter becomes viable only after:

1. Identify which exact native \`libengine.so\`, \`libclient.so\`, \`libserver.so\` and VR module are selected by the target game's private manifest/load file.
2. Verify **the actual C/C++ engine SDK**, license, build and supported game-module loading path; do not infer SourceVR custom DLL support from exported \`CreateInterface\`.
3. Build a matching ARM64-native compatibility module against **verified engine headers**, add lifecycle callbacks and demonstrate one engine-owned entity callback.
4. Only then integrate our existing \`sin_compat_host_abi\` behavior into a real game loop and proceed to BSP/weapon/VR integration.

The original SiN Windows DLLs are x86. They are research inputs, not loadable Android game modules. No original or SourceVR binaries belong in this public repository.
