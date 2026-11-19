# Owner-verified original 2006 HUD texture converter ABI

Read-only original private input: client.dll SHA-256
6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.
Audited directly from the owner's `bin.zip`, without copying private
original executable bodies into this repository.

Run `python research/tools/audit_sin_client_hud_converter_native.py
/path/to/original/client.dll` to authenticate original method bodies,
static object/vtable, one-time registration callsite, original name
and duplicate-registration warning.

The original startup `VGui_Startup` checks a one-byte flag at
`0x24378B28`. On first run, it pushes a **static converter object**
at `0x2431DDCC` and exact type name `CHudTextureHandle`, then
sets the one-time flag BEFORE calling converter registration helper
`0x24009340`. The helper interns the name, checks its existing
registry and **warns/ignores duplicate registrations**. If missing,
it calls the original 0x24009F40 insertion path using an internal
symbol collection. No claim is made about the native symbol-map ABI.

The static object's vptr points at `0x242AC530` with exactly three
authenticated virtual methods (complete code digests, no original bytes):

| Virtual | Original VA | Body bytes | Original SHA-256 | x86 ret |
|---|---|---:|---|---|
| GetData | 0x241EE190 | 63 | 5333d07db76f00b568573f0d28dda9b3f5eefff04ba7d2f2423bc66616f142e2 | 12 |
| SetData | 0x241EE1D0 | 72 | 068038676bdacc50bddfcf3fc5f969aa80fabe3eb6a8474f9fb56a2e789e1ace | 12 |
| InitFromDefault | 0x241EE220 | 58 | 0b7072358a0c49198703a8013b99056fc40d8ccd3030ce13df04739e3a689602 | 8 |

All three virtual methods retrieve their property destination through
entry callback offset `+0x14`. GetData converts a nonnull icon to
its short name (offset +4 of original texture object), SetData
resolves a KeyValues string through the original gHUD icon lookup,
and InitFromDefault uses the entry's default string at +0x0C.
Missing or empty names clear the icon handle.

This improves on earlier SDK-only method ordering by authenticating
**the actual SiN 2006 x86 object and all three method bodies**.

The registry algorithm is to be reconstructed independently, with
host-managed callbacks and persistent converter ownership. This
audit alone does not prove a live VGUI registry has been modified
inside the original game; our previous owner milestone remains
0x53494E61. Nor does it establish successful client Init or rendering.
