# Phase 732: SDK poison gas map input handlers

Owner SDK bin/SinEpisodes.fgd explicitly exposes SetRadius, SetDuration,
SetSpawnRate, SoundOn, SetStartColor, SetEndColor. Original datamap
builder 222fee40..222ff583 installs their callbacks at descriptor
function slots 22596c5c/90/c4/f8 and 22596d2c/60. Actual input
variant tag +18 and union payload +8. This is separate from the
factory config path in Phase728 and from FGD editor presets.

SetRadius 222fe8b0: if tag1 load float, otherwise zero; store parent
+3a0 even without visual. If visual +3b4 nonnull, multiply by 0.75f
(223d99a0) and compare against child +3cc before rounded store.
SetDuration 222fe370: tag1 raw float, otherwise zero, parent +3a4
only. SetSpawnRate 222fe930: tag1 or zero, parent +3a8 then child
+3b0 if live, dirty only when different/unordered.
SoundOn 222fe3a0: only if visual pointer nonnull, tag6 copies
payload byte otherwise zero, parent +3b8. No immediate sound call.
SetStartColor 222fe9b0 and SetEndColor 222fea50: only if visual
pointer nonnull; tags 3/15 load 3 floats, otherwise global vector
225885b8..c0. Multiply by f32 1/255 (223d3a20, 0x3b808081);
round each component, no clamp. Child vector offsets +394/+3a0
respectively, helpers 222fe6b0/720 compare all components. Tracked
child changes mark deferred flag +200 if byte +1f0 set; otherwise
OR bit1 into edict at +16c if nonnull. Host dirty callback supplies
that existing engine mechanism; native owns decision and field store.
Host validates live typed child, supplies fallback vector for
non-vector input rather than inventing unverified global contents.
Port rejects missing/short child span and blocks partial failure.
The original turn-on/off paths already exist in Phase728.

Tests exercise SDK-declared inputs, null child, actual offsets,
unchanged suppression, wrong tags, original scaling, live colors,
fallback, failure after parent store and short child bounds.
No renderer, particle simulation or physics replacement. Also
corrects synthetic health pickup test classname to item_megahealth,
verified in both FGD and original factory strings at 2246c93c
(xrefs 223c42d8/42f1).
