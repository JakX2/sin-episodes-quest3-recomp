# Phase 683: map-owned SiN weapon ADS and activity behavior

Original server.dll SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650.
Rechecked original 0x222ccd40..0x222ccde5 (166 bytes,
SHA256 49ac8baece08aa0f19970266fa0dd82f52d4caa5ce8aecbdc770b879e9a02e05):
player resolve and +0x120 gate; +0x4ac reload and +0x3c8 block;
weapon +0x578 zoom, +0x579 automatic restore; player newly pressed
+0x93c bit 0x80000. Original helpers 0x222cbb60/0x222cbbc0
request player FOV first (original recheck, 0.25s/0.15s), then
activity +0x314 (0xc0 on enter, 0x9d only if current +0x4bc==0xc0
on exit), then write +0x578. These are the original Phase45 and
Phase47 behavior models, now compiled into the native server.

Original owner scripts authenticated:
weapon_magnum.txt SHA256 2f77a013eb591e787ba139647a6d7ca340ac3456c13c7fed1a5ad255d42aef3d
(20 clip, FOV 50), weapon_assault_rifle.txt
491cdb481d9bd43941e36c6733f32f2ce3a1e10e2cc4ab992ead7b9b2d593f5f
(45 clip, FOV 40), weapon_scattergun.txt
09f000e235a5c9e14cbd38fe1f4a2ba8a913a96e29823b474dc51cd5a6aebc6c
(6 clip, FOV 60). All clip2=-1, default_clip2=0. These are
default metadata, not an independently reconstructed live ammo store.
The host supplies the live resolved zoom FOV and original branch
observations; no assumption that script defaults always apply.

The loaded native MapEntityApi and BSP adapter recognize these
three exact weapon classnames. Actual engine shell creation/base
keyvalues/spawn are delegated to the host. The DLL owns zoom and
restore bits by epoch/index/generation; exposes profile, state,
reconcile, frame and activity selection. Original activity
selection includes assault rifle recoil counter and all three
zoomed primary/secondary variants. Frame forwards FOV then activity
through transient host callbacks. Port-only VR mode suppresses
tracked-camera FOV. Callback failure/exception is treated as
uncertain, blocks further behavior pending explicit observed-state
reconciliation; original failure behavior is not claimed. No engine
memory writes or fabricated firing.

Loaded-module tests drive a synthetic BSP with all three weapons,
original order, reload interruption, restoration without button
press, owner gate, activity variants, VR suppression, callback
failure/resync and stale identity teardown. Actual engine-owned
weapon classes, usercmd binding, network fields and ammo/attack
execution remain unimplemented. No original assets redistributed.
