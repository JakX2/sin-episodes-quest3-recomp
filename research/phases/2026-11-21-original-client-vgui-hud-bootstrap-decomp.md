# Phase 540: original SiN `client.dll` VGUI bootstrap and HUD handle registration decompilation

## Scope and provenance

This continues the **original 2006** `VClient011::Init` decompilation, specifically its post-interface fail-closed call at `0x240FE3C7` to `0x241EDFB0`. It does **not** add any new VGUI, physics, HUD renderer, input-device, Android or Quest implementation. We inspect only source-level control flow, native function boundaries, original interface strings and return semantics.

The owner's whole original DLL SHA-256 identities are fixed to `client.dll` = `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d` and `server.dll` = `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. In addition, original `VClient011::Init` is SHA-authenticated in its entire 1,220-byte range `0x240FE0E0..0x240FE5A3` by the earlier Phase538 auditor.

Four **additional original complete functions** are now individually authenticated, totaling **239 bytes**, with their exact following INT3 padding verified:

| Native VA | Complete method bytes | SHA-256 | Observed role |
|---|---:|---|---|
| `0x241EDFB0` | 123 | `64cbfa079f093b1e577a59721396d73742eb4543b836e7fb0cc55d724783417d` | Two VGUI interface acquisitions and one-time HUD handle setup |
| `0x24009340` | 101 | `a0851e6f97bc087734723394de37dfb925db41c1764087f4c9e94ede1a1b384e` | Named client handle registration helper invoked with `CHudTextureHandle` |
| `0x2422ECD0` | 1 | `ae3f4619b0413d70d3004b9131c3752153074e45725be13b9a148978895e359e` | Literal no-op return (`ret`) |
| `0x2422ECF0` | 14 | `d1f9d41fb9e6c8628fa934c2ac6a848a9aae39e604b44f7b4f1ddab5ceed29b3` | Forwards literal numeric argument `2` to virtual +4 on original engine VGUI interface |

These methods are distinct from the **525 bytes** recovered in Phase539 and the **1,220-byte** already authenticated Init. All written pseudocode is newly authored explanatory source, not the original Ritual C++ text.

## Original client Init branch to HUD bootstrap

At `0x240FE3C6`, the parent Init pushes its preserved engine factory register. The call at `0x240FE3C7` invokes `0x241EDFB0`; `0x240FE3CC` clears that argument from the stack. `0x240FE3CF` tests the returned byte, and the exact native JE at `0x240FE3D1` branches to **the same global Init failure return at `0x240FE59E`** when it is zero. This is a *required* client setup stage, not a best-effort optional HUD feature.

### Two hard-required VGUI names

The helper starts by passing literal `CLIENT` (original pointer `0x242AC524`) to a native setup routine at `0x240010D0`, along with a stack-local output location and literal `1`. Its zero result yields immediate `al=0` return from the VGUI helper. The exact semantics of this loader-like routine are not reconstructed; the literal alone does not prove a specific filename or DLL-load policy.

After that prerequisite succeeds, it invokes the original factory argument for two interfaces, stores the return pointers, and rejects null:

| Original query site | Native null-terminated name | Result stored at | Failure destination |
|---|---|---|---|
| `0x241EDFCB` | `VGUI_InputInternal001` (`0x242AC50C`) | `0x24378AB8` | `0x241EDFC8` (returns false) |
| `0x241EDFE2` | `VGUI_System010` (`0x242D32C0`) | `0x24378ABC` | `0x241EDFC8` (returns false) |

The exact `push null; push interface_name; call passed factory; add esp,8; test eax; mov global,eax; je fail` shapes and both short failure branches are verified, including pointer destinations. These are independent of Phase538's initial 21 engine interface requests.

### The one-time `CHudTextureHandle` path

Once both VGUI interfaces are non-null, the helper checks byte at original global **`0x24378B28`**. On the first pass (zero), it writes **1**, pushes original pointer `0x2431DDCC` along with the literal name **`CHudTextureHandle`** (`0x242AC4F8`), and calls **`0x24009340`**. On later passes when the flag is nonzero it skips that call, rather than re-registering.

The separately authenticated 101-byte `0x24009340` method uses an original native token/name handling call at `0x241D4A80`, a lookup call `0x2400A4D0`, and a conditional registration path through `0x24009F40`. This evidence supports a name-index registration role but does **not** yet disclose the original class layout, value ownership, or meanings of the numeric lookup return. A prior entry in the native data structure affects whether the helper emits its diagnostic path. Both branches are left untyped.

The original VGUI helper then invokes **`0x2422ECD0`**, which is exactly **one instruction, `ret`**. Any future source reconstruction must preserve that this call does *nothing* in the 2006 build; it must not invent a side effect. Next it calls **`0x2422ECF0`**. That 14-byte function reads `0x2433F6B0`, accesses its native vtable, pushes literal integer **2**, dispatches virtual byte slot **`+0x04`**, and returns that virtual result unchanged.

At the caller, `0x241EE025` tests `eax`, and `0x241EE027` applies `setne al`. Thus the final VGUI bootstrap return is **true precisely when that last VGUI virtual dispatch returns nonzero**, assuming all earlier setup checks have succeeded. A nonzero `CHudTextureHandle` side effect is *not* itself the final Boolean condition.

## Readable original-decompilation sketch

```cpp
// Explanatory pseudocode from authentic 2006 x86. Not exact original source.
bool OriginalInitClientVGui_241EDFB0(FactoryPtr vgui_factory) {
    // Native 0x240010D0 also takes an output location and literal 1.
    if (!OriginalClientContextSetup_240010D0("CLIENT", /*out*/, 1))
        return false;

    void *input = vgui_factory("VGUI_InputInternal001", nullptr);
    original_global_24378AB8 = input;
    if (!input) return false;

    void *system = vgui_factory("VGUI_System010", nullptr);
    original_global_24378ABC = system;
    if (!system) return false;

    if (original_byte_24378B28 == 0) {
        original_byte_24378B28 = 1;
        OriginalNamedRegistration_24009340(
            "CHudTextureHandle", /*original pointer*/0x2431DDCC);
    }

    OriginalNoop_2422ECD0();                 // original body is only RET
    auto result = OriginalVGuiQuery_2422ECF0();
    return result != 0;
}

// The original 14-byte wrapper at 0x2422ECF0 performs the equivalent of:
int OriginalVGuiQuery_2422ECF0() {
    return original_global_2433F6B0->virtual_byte_04(/*literal*/ 2);
}
```

Do **not** infer that the original GUI returned success simply because both interface objects were available. The final virtual call is a distinct required operation, and `Init` checks this helper's Boolean. Neither `0x240010D0` nor the class layout behind `0x2433F6B0` has been fully semantically recovered.

## Reproducible verifier

Run, without committing the private DLLs:

```sh
python3 research/tools/audit_sin_original_client_vgui_hud_startup.py --self-test
python3 research/tools/audit_sin_original_client_vgui_hud_startup.py --bin-zip /private/original/bin.zip
```

The owner's unchanged original `bin.zip` passed all function SHA checks, executable section and method-boundary checks, parent Init fail-branch, both exact engine query names, both global stores and both short null branches, one-time HUD registration flag and string, 3 HUD helper direct-call edges, literal no-op function and final virtual-2 forwarding contract. The synthetic test independently rejects mutated `CALL rel32`, short JE opcodes/offsets and bad original DLL SHA inputs.

## Next source-level decompilation targets

- `0x240010D0`: determine the actual `CLIENT` registration and factory-selection algorithm. Avoid assuming it is a normal Windows `LoadLibrary` call just because it accepts a name.
- `0x241D4A80`, `0x2400A4D0`, `0x24009F40`: resolve name/token and handle registration behavior of the 101-byte HUD helper and the meaning of its original pointer argument.
- `0x2422ECF0` dispatch's concrete object: recover its class and vtable identity to label the virtual `+0x04` action, rather than guessing from Source SDK names.
- Continue decompiling original client HUD, messages, local player, scripts and original server NPC and weapon code. Physics/surface, prop and model implementation remain explicitly outside the active research priority.

**No native replacement client.dll startup, rendered HUD, playable SiN level, Android APK or Quest 3 execution is established here.**