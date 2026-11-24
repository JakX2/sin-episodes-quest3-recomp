# Phase 717: native CPropKeypad interaction

Fresh SHA256 verification of both owner originals matched the existing
pins. Server allocation 0x41c (223021b1), client 0x538 (242204c1).
Datamap 2252a570: Enable, Disable, SetInput, Clear, OnInput.
Server event listener 22302640 on +0x394 secondary interface compares
prop_keypad_input (22470f28), resolves own entity index through engine
slot18 using +0x16c, compares event GetInt("entindex",0). On match
reads GetString("Command",""), calls 22302560. Nonmatching index ignored;
unrelated event delegates 222915d0. No disabled-state gate here.

22302560 marks +0x3c9 dirty unconditionally, calls imported
Q_strncat(dst,command,32,1) (IAT 223d21b0, import table confirmed).
Fires OnInput at +0x3ec with accumulated string (null for empty),
self activator/caller, zero delay via 22301de0. Emits sound formed
from telephone. (22471040, length 10) plus full command via 221b77b0.
Ten telephone.0..9 precache strings observed at 22301b10.
22301ee0 marks dirty, Q_strncpy(dst,source,31) (IAT 223d21c4), fires
OnInput without sound. SetInput 22301f50 resolves input variant;
Clear 22301f90 substitutes empty literal. Enable/Disable 22302150/
22302110 tracked byte +0x3c8. The port models bounded string operations
for NUL-terminated inputs; malformed storage and commands >=256 bytes
are port guards, not original restrictions. Host owns variant decoding.

Client panel tick 242205d0 resolves serial-checked handle at +d8,
casts to CPropKeypad, skips display if null or byte +510 set; else
widget at +dc vslot139 receives +511 string. Base tick 240053a0 last.
Panel command 242206f0 checks same resolved typed entity and disabled
byte, creates prop_keypad_input (force=1), if present gets index via
networkable +8 slot9, sets entindex (event slot10) and Command (slot12),
fires nonlocal (0); base command 240fad20 last. Host resolves/casts
screen and owns widget, event, audio and output services.

Paired regression supplies real Phase716 property callbacks, exercises
client event creation, server index filter, cumulative input, display
update, disabled distinction, missing event, unrelated event, full
buffer, original replacement capacity, empty output and failure guard.
No actual retail event transport or widget creation claimed. No new
physics/model/renderer work, no original code or asset bytes committed.
