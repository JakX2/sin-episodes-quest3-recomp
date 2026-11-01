# Phase 79: HL2VR 0.989 client/server module selection and VR interface contract

**Date:** 2026-11-01. **Status:** verified static instruction-level ABI audit against the original owner-supplied Quest engine binaries. **No real SiN module was loaded. R1 remains blocked.**

## Inputs and exact engine binary provenance

| Engine from owner's packages | SHA-256 | Native warning ADRP/ADD | Warning/success continuation |
|---|---|---|---|
| HL2VR 0.989 Vulkan `libengine.so` | `d8ecc43d92ac5cd30f1111b0e568eab839858e5626e79c74ebba629d2e13e94a` | `0x61e868` / `0x61e86c` | `0x61e8b4` |
| HL2VR 0.989 Turbo `libengine.so` | `43ee3b08e5979ea2e923acbaefab04ed252603561e6323450c15079a27d0159a` | `0x61e970` / `0x61e974` | `0x61e9bc` |
| PortalVR (APK version 1.12) `libengine.so` | `82891887a5911363fb4aacb62fbb8ad9678e8a1ced3d35ed476609552555255d` | `0x60ee80` / `0x60ee84` | `0x60eecc` |

The HL2VR extracted APK has the exact official published 0.989 APK digest, as documented in Phase 78. Libraries are original owner-provided source binaries, **never distributed in this repository**. Only clean-room inspection metadata and our own verification tool are committed.

## Proven module selection logic

The **HL2VR Android** engine reads `HL2QUEST_GAME` from the process environment; if absent or empty it examines the `-game` command-line argument with the default `hl2`. It then compares the selected game name to `portal`, `portal2`, `episodic`, and `ep2`.

| Game value | Client library base | Server library name |
|---|---|---|
| `hl2` (default / other unmatched) | `client` | `server.so` |
| `episodic`, `ep2` | `client_episodic` | `server_episodic.so` |
| `portal` | `client_portal` | `server_portal.so` |
| `portal2` | `client_portal2` | `server_portal2.so` |

**Precise instruction sites (Vulkan build):**

- Client module selector begins at `0x61e670`: `getenv` at `0x61e688`, command-line fallback at `0x61e698`, per-game comparisons and assignments `0x61e6ec` through `0x61e768`. The diagnostic `Android engine client selection: %s` is called near `0x61e768`.
- Client module load call is an **indirect filesystem vtable call at offset `0xC8`**, `0x61e7c0` through `0x61e7e4`; it passes a client module base name plus `GAMEBIN` as the filesystem path category. **Neither this call nor the `gameinfo.txt` path proves that user-supplied custom game binaries are loadable from external storage.**
- Server selection is at `0x5ec758` onward: `HL2QUEST_GAME` / `-game` selection, module-name assignments `0x5ec7c4` to `0x5ec840`, followed by `Android engine server selection: %s`.
- Server module load uses an indirect filesystem vtable call at offset `0xC8` around `0x5ec894` through `0x5ec8b0`, with the `GAMEBIN` search-path category.
- The server code prepares `ServerGameDLL%03d`, starts with **`ServerGameDLL010`**, then attempts decreasing interface revisions down through **004** if the factory returns null. Later it requests `ServerGameEnts001`, `ServerGameClients004` with 003 fallback, `HLTVDirector001` and other host services. This does **not** establish that our rebuilt SiN objects have matching ABI/vtables.

The client and server names and interface versions above come from actual `ADRP+ADD` string references and nearby AArch64 control flow, not just detached ASCII strings.

## What the famous VR compatibility warning actually means

On both HL2VR 0.989 builds **and PortalVR**, the client initialization sequence:

1. Loads a native client module, obtains its factory, and requests `VClient017` (mandatory in the observed loader).
2. When the host's VR interface state is present, requests `ClientVirtualReality001` from the client factory.
3. If that VR factory request returns null, **prints** `client.dll is not VR-compatible.` as a nonfatal diagnostic within this function.
4. Both the non-null and warning paths join at the same continuation address, and a subsequent `ClientRenderTargets001` request follows.

The control flow is **verified via actual ARM64 branch instructions**, not guessed from strings. For the Vulkan engine, the relevant sequence is the conditional branch at `0x61e864`, warning string at `0x61e868`, logging at `0x61e870`, and unconditional branch at `0x61e874` to `0x61e8b4`. In Turbo, these are respectively `0x61e96c`, `0x61e970`, `0x61e978`, `0x61e97c`, and `0x61e9bc`. In PortalVR, the corresponding positions are `0x60ee7c`, `0x60ee80`, `0x60ee88`, `0x60ee8c` and `0x60eecc`.

**The warning itself is not a hard load rejection.** A client with a null VR interface may still fail other initialization, stereo rendering, motion controls or gameplay checks, and no substitution was attempted. The next client interface factory call is `ClientRenderTargets001`; its null-handling and importance to VR also need verification with real runtime.

## Reproducible fail-closed tool

The new independently authored `scripts/verify_sourcevr_loader.py` accepts only an **exact pinned** original engine ELF. It uses the Python standard library to check AArch64 ELF sections, whole-file SHA-256, `ADRP` + `ADD` references to the relevant interface/module-name literals, and the instruction-level convergence of the VR warning and successful factory lookup. It emits only metadata; it does not dump code or copyrighted game assets.

```sh
python3 scripts/verify_sourcevr_loader.py --profile hl2vr-vulkan-0989 --engine /private/hl2vr/vulkan/libengine.so
python3 scripts/verify_sourcevr_loader.py --profile hl2vr-turbo-0989 --engine /private/hl2vr/turbo/libengine.so
python3 scripts/verify_sourcevr_loader.py --profile portalvr-112 --engine /private/portalvr/libengine.so
python3 -m unittest discover -s research/tests -p 'test_*.py' -v
```

All **three actual owner-supplied engine binaries passed** the tool's pinned source reference checks locally. **Eight original-free synthetic AArch64 ELF tests passed**: instruction decoding, branch convergence, a valid miniature ELF with fabricated string references, a modified-branch negative fixture, wrong SHA-256, non-AArch64 ELF, corrupt section table and absent interface.

The verifier does not automatically recover a full load call graph, disassemble a VR interaction API, test network prediction or assert the exact caller of a filesystem load routine. Those additional code-path conclusions above come from separate manual examination of the pinned engine disassembly. All automated pass results remain deliberately marked `sourcevr_module_loading_verified=false` and `sin_quest_runtime_verified=false`.

## Next real-engine milestone

**R1 #12 cannot yet be closed.** To move from module *selection* to actual SiN *module integration*:

1. Secure a legal, accessible corresponding engine/module development environment or authorization from the maintainer for custom game modules. Do not assume the APK's internal engine source is public.
2. Trace Android linker namespace and filesystem implementation behind the engine's indirect `GAMEBIN` module loader, including filename translation, app-private data and package-signing requirements. **Do not modify the owner's shipped APK as a substitute for a supported SDK.**
3. Establish actual binary-compatible `VClient017`, `ServerGameDLL010`, `ServerGameEnts001`, `ServerGameClients004/003`, and `ClientVirtualReality001` header/vtable contracts from buildable licensed source.
4. Build an independently authored tiny native test module and test *real* factory calls on an Android runtime with documented loader support.
5. Only after that, link Phases 74/77 entity runtime and Phase 76 original ScriptData weapon settings. A running SiN level/VR headset validation is still required.

**No Quest APK produced, no proprietary asset committed, no external module loader proven.** See issues #12 and #20.
