# Phase 735: SDK HUD animation controller and hologram FGD discrepancy

Fresh owner SDK archive and original DLL hashes match earlier pins.
SE1/scripts/HudAnimations.txt contains PoisonTimeFadeIn and
PoisonTimeFadeOut as events animating HudPoisonTimer's
PoisonTimerColor to RGBA 255 255 255 255 and 255 255 255 0,
respectively, Linear, delay 0.0, duration 0.5. These are animation
sequences, not audio scripts. No SDK asset bytes copied.

Original client 24236320..242363ea: fade branch pushes event name,
calls global 2437f8a0 vslot9 (+24), uses returned controller as this
for 240104b0. That routine resolves sequence symbol and can return
AL=0 (missing sequence); HUD caller ignores it and rereads player
+11ec before storing complement at secondary +10c. Original .25,
0, and 1 comparisons in fade-out branch simplify to ordered >=1
and alpha255; unordered does not dispatch. Port Phase730 legacy
callback retains name sound for source/ABI compatibility, but
explicitly documents animation semantics. New separate animation API
shares same tick decision, resolves controller only when needed,
dispatches sequence, ignores original started boolean but guards
null controller/transport failure. Host supplies actual VGUI
animation controller and existing SDK script loader, no renderer.
Tests cover no dispatch, alpha intermediate, correct names, ignored
false sequence result, reread and guarded callback failures.

FGD npc_hologram declares HologramOnDone and HOlogramOffDone.
Case-insensitive raw scans of full authenticated server.dll found
zero occurrences of either name. Inspected actual original
22361980..223619d7 (on completion) updates state=2 and emits
Hologram.Loop; 223619e0..22361a31 (off completion) updates state=0.
Neither invokes a map output. Phase714 therefore remains unchanged:
do not fabricate completion outputs based solely on editor text.
This is evidence for the inspected binary, not proof about every
historical SDK/game build. No physics, rendering or assets added.
