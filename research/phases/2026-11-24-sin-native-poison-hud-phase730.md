# Phase 730: native client poison HUD integration

Fresh original client/server SHA pins confirmed. Original CHudPoisonTimer
RTTI primary table 242c91c4, secondary +14 panel table 242c8f94,
constructor 24236610 explicitly installs both. Prior Phase153/154
portable helpers used method-relative offsets. Crucial layout correction:
primary +120 is secondary +10c (same float), primary +124/128/12c
are secondary +110/114/118 (same resource pointers). Primary color
+b7..ba is secondary +a3..a6; alpha byte +ba is the fade gate.
Primary arrays +bc/dd/fe are 33 bytes each. Original scheme descriptors
24236450/242364c0/24236530/242365a0 resolve:
PoisonTimerColor (Color, default 255 255 255 0),
MutagenBarForeground (Mutagen_bar_foreground),
MutagenBarBackground (Mutagen_bar_background),
MutagenBarInactive (Mutagen_bar_inactive).
Constructor zeros actual color bytes before scheme application.
No fabricated scheme initialization: host applies existing variables.

Reset 242360b0 writes only +120. Bind 242360c0 resolves each string
and stores resource immediately, including null results. Tick
24236320 compares player +11ec to old HUD +120, returns on equality,
otherwise FadeIn if incoming<1 and alpha==0, FadeOut if incoming>=1
and alpha==255, including infinity/NaN behavior per Phase154.
Re-read actual player +11ec AFTER sound call (previous portable model
used a single sampled value); double 1.0 minus current stores f32.
Alpha not written by this method. Actual scheme animation/sound
delivery remains host responsibility.

Paint 24236120 calls base panel paint on +14 first. Selects +128 if
fraction>0 else +12c (including NaN). Gate +124 and selected nonnull.
Original 24154ab0 scales two ZERO coordinate floats using global
240ffad0 and design height; 24154d50 gets x/y truncated to 0,
design width=1600 (2429b69c), height=1200 (2429b6a0), selected
background and required foreground pointers, fraction, color pointer
twice and mode=3. Native emits semantic draw request with these exact
values. Existing rendering helper remains host work, not implemented.
Native minimum span 0x130, not full HUD allocation or base construction.

Loaded-pair regression exercises real server poison timer -> client
RecvProp -> native HUD update and paint decision. Verifies resource
order, complete-object alias, missing resources, active/inactive
selection, color, equality skip, alpha gates, callback mutation
re-read and unordered values, failure reconciliation. No new
physics, renderer, or commercial binary assets.
