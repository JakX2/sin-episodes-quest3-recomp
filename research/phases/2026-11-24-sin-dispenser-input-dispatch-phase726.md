# Phase 726: original dispenser Use and button event routing

Fresh SHA-matching original server. Primary slot87 222f7ff0 consumes
variant tag at argument3. Tag 1 calls 222b61d0 with descriptor=2,
owner=activator, subject=dispenser, subobject=0, THEN 222f7f00
native use body (Phase719). Tag 0 emits descriptor=3 and invalidates
m_hUser +4ac, with no immediate steam stop or timer reset. Other
tags no-op. Original use event helper 222b61d0 creates "use" event
(2245c804) and writes descriptor (2245c84c), owner (2245c858),
subject (223ec29c), subobject (2245c860); handle conversion and
actual event emission are host services. Phase719 use API retains
its ABI and old behavior; new variant API owns one busy scope across
event and native use body. Release leads to next native think stop.

Secondary listener +494 vtable 2246aa5c slot1 222f7790 compares
button_press (2245665c) with case-insensitive CRT 223a5574. Checks
m_hVguiScreen +4b4 serial and nonnull, resolves again, obtains index
via engine slot18 using screen+16c. Event GetInt("entindex",0)
(2240fc84) must match. Only then reads GetString("command","")
(22456720,223d47a8) and compares "eject" (2246ae94)
case-insensitively. On match installs think 222f7330 and schedules
current global time. It DOES NOT immediately eject or check loaded;
eject method itself owns the gate. No unrelated-event base call.
Port maps ASCII original literals, rejects malformed strings or
stale second resolution and blocks on uncertain partial callbacks.
Scheduler must call the existing Phase720 eject method later.

Native test covers use event before healing, release then actual
native use-think steam stop, unloaded press, absent/stale screen,
unmatched index and command, mixed-case strings, deferred schedule
even when unloaded, and partial event failure. No new physics,
UI rendering, or proprietary bytes. Actual engine event registration
and scheduler remain host dependencies.
