# Phase 723: dispenser idle screen and restore

SHA-pinned server.dll fresh disassembly. Idle think 222f6aa0 checks
+4b4 handle against entity serial and nonnull pointer, skips setup
if live. Otherwise creates vgui_screen and casts, stores handle or
invalid. Failure to allocate still goes to pose and scheduling.
Original position from absolute origin +2bc, original angle vector
helper 22269bc0 on +36c, forward normalized by 2251b1d0; position
origin -4*forward +16.5*right +3.6*up (constants 2246ad60/5c/58).
Angles from +354/+358/+35c with yaw and roll +90 (223f6e40).
Host samples compatible transform and angle basis services. Original
x87 mixed stored products and intermediate rounding approximated
with double and float stores; not claimed exact at extreme values.
Resolve screen again for each effect: origin helper 221438a0,
angles 221439c0, size 222914b0 (7.1,2.0), keyvalue vslot32
panelname=vgui_dispenser_panel (225285bc -> 2246a8dc), raw byte
+3b7 OR 0x80, raw DWORD +3b4 AND ~2, spawn vslot25.
These flag offsets are distinct from the Phase716 m_fScreenFlags
metadata. Borrowed host owns original size/dirty/collision service.
Then percentFull updater 222f64d0 and schedule now+0.1.
Port uses serial-checked resolve per original effect; refuses
null/stale after creation instead of original unsafe dereferences.

Restore 222f7e50: base 220f0ac0, register secondary listener for
button_press, pose/animation, if active think exactly 222f7100
and nextthink >= now (ordered) start spray, otherwise stop spray.
Reads nextthink again, if strictly less than now, install idle
think 222f6aa0 and schedule now+0.1. Reuses Phase719 start/stop
helpers. Host supplies actual event subscription, think slot identity
and scheduler. No copied fixed addresses or source engine replacement.
Original client did not yield a matching literal panel name; no
dedicated client widget implementation claimed.

Loaded native test exercises allocation failure, existing/live and
stale screen, placement with known basis, flags/order, restore
equality and past deadline, restart/stop, failure reconciliation.
Also renames new Phase722 test max helper to avoid Win32 macro risk.
