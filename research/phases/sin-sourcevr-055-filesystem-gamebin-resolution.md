# SourceVR 0.5.5 real filesystem game-module loader

**7 November 2026 | Private source archive SHA256-pinned ARM64 investigation**

## Concrete implementation behind virtual module loader

PR #146 previously proved that each native `libengine.so` requests
`server.so` and `client` from `GAMEBIN` using a virtual loader
slot at offset `+0xC8`, then uses Android's `dlsym` to resolve
`CreateInterface`. This phase follows that boundary into the
**same filesystem module shipped in all seven native game profiles**.

The selected `libfilesystem_stdio.so` has full SHA256
`6b2fa7b745cd338ccfde04b6d3f152cbcf8c07cf1073ee424584d7296a5ad30e`
in HL2, Episode One, Episode Two, Lost Coast, Portal, Portal 2,
and Entropy: Zero manifests. Its ELF is a stripped, AArch64 Android
module. The Itanium-style RTTI and vtable data nonetheless retain
specific class references.

| Native filesystem evidence | ELF virtual address | Proof |
| --- | --- | --- |
| RTTI for `CBaseFileSystem` | `0x993A0` | RTTI name pointer `+8` references `15CBaseFileSystem` at `0x9F28` |
| Native virtual table function base | `0x98E38` | preceding pointer at `0x98E30` names above RTTI |
| Virtual `+0xC8` entry | `0x98F00` | function pointer equals `0x2DD90` |
| Native `+0xC8` function | `0x2DD90` | prepares name, game path, calls next loader at `0x2DEC8` |
| Filename/path-selection routine | `0x6EFC0` | invoked by direct `BL` at `0x2DEC8` |
| Dynamic-link wrapper | `0x6EDC0` | called by successful path candidate branches |
| Genuine Android `dlopen` PLT | `0x94C30` | `R_AARCH64_JUMP_SLOT` relocation, actual `BL` at `0x6EE2C` |

This proves that the **filesystem's** `CBaseFileSystem` virtual
`+0xC8` function reaches Android dynamic loading. The same offset
is used at the engine call site, but the actual engine's runtime
interface pointer is not independently identified in a running
process. This is therefore strong *static* linkage evidence, not
a claim of loading a custom module.

## Actual path and naming rules

The filesystem references these exact, live native strings:

- `APP_LIB_PATH` at VA `0x7CF8`, read using `getenv` at `0x6F0C8`.
- `APP_MOD_LIB` at VA `0x8AE1`, read at `0x6F0D8`.
- `.so` at VA `0x92E9`, used in module extension normalization.
- `lib%s` at VA `0x8FD5`. At `0x6F184..0x6F19C`,
  the original code checks for an existing `lib` filename prefix.
- An internal full-path builder is called before `dlopen`.
  The filesystem first attempts the mod library path and has
  additional fallback handling using the general app library path.

For the **HL2** native launcher, SHA256
`84a9b779eabc5eef2e8ad33a217f0669d005990f085402b9a3766df66a64f18b`,
the loader environment bootstrap at `0x37308..0x3738C`:

1. Calls `getenv("APP_LIB_PATH")` and skips its default bootstrap
   if that variable already exists.
2. Otherwise uses `dladdr` on a launcher function and strips the
   module filename to determine the launcher directory.
3. Passes the directory to `setenv("APP_LIB_PATH", path, 1)`
   at `0x37378`, and `setenv("APP_MOD_LIB", path, 1)`
   at `0x3738C`.

This default bootstrap is important: it indicates where the
native module search is **designed** to start. It is not proof
that editing an environment variable inside a packaged app is
supported or authorized, nor that Android permits arbitrary native
modules to be installed there. The internal native profile manifest
still specifies all original shipped libraries and their hashes.

## Reproducible binary-backed audit

`research/tools/audit_sourcevr_filesystem_gamebin_route.py`
checks actual ELF64 RTTI/vtable addends, ARM64 direct calls,
the true `dlopen`, `getenv`, `dladdr` and `setenv` dynamic relocation
targets, plus the complete filesystem and HL2 launcher hashes.
It validates that all seven native game manifests select the same
filesystem blob.

`research/tests/test_audit_sourcevr_filesystem_gamebin_route.py`
is an original-asset-free synthetic negative/positive regression
fixture. The original binary audit runs **only against private
owner-provided archives**, never GitHub Actions.

Run with your own privately joined SourceVR archive:

```bash
python3 research/tools/audit_sourcevr_filesystem_gamebin_route.py \
  --sourcevr-zip /private/sourcevr-joined.zip
python3 -m unittest discover -s research/tests \
  -p 'test_audit_sourcevr_filesystem_gamebin_route.py' -v
```

## Fastest credible next integration step

We now have a specific native load candidate route, but the engine's
`ServerGameDLL010..004` and `VClient017` game interfaces remain
incompatible with SiN 2006 `ServerGameDLL003` and `VClient011`.

The next useful proof is a **minimal independently built Android
ARM64 library load probe** that exports `CreateInterface` and
fails closed before any unverified game-ABI virtual dispatch.
That can validate Android path/namespace behavior without
misrepresenting a dummy object as a working SiN game server.
After confirming loader acceptance, the remaining required work is
the true engine game-module interface and implemented gameplay,
not a renamed Windows DLL.

No proprietary APK, engine .so, client.dll, server.dll,
map, model or asset is committed. There is still no playable
standalone Quest 3 game.
