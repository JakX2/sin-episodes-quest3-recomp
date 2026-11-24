# Phase 718: native CPropPhone phonebook polling

Fresh inspection of SHA-pinned original server/client. CPropPhone size
0x4b4 server (22302e21) / 0x788 client (24220891), direct parent
DT_BaseAnimating (server pointer 224e88b4 -> 2254d11c; client
242f7420 -> 243362a8). DT_CPropPhone one baseclass property; constructor
calls 223029d5 / 24220849. No new client scalar fields. Separate native
one-record graph pins the existing aggregate and preserves prior ABIs.

Original constructor 223029e0 after base 220f15e0 zeros byte +494 and
DWORDs +498/49c/4a0/4a4/4ac/4b0; invalidates handle +4a8.
Datamap 2252a7b4, 52-byte descriptors: Disabled +494, FailSound
+498, KeypadName +4ac, PhoneBook +49c, m_fLastUse +4a0,
m_nDigitsMatched +4a4, m_hKeypad +4a8, DialNumber 22302d80,
Enable 22302740, Disable 22302730. Runtime pointer +4b0 holds the
loaded KeyValues phonebook. KeypadName is used as panelname in Think,
default prop_keypad if null. Precache 223027c0 loads PhoneBook +49c
with root PhoneBook; no full KeyValues loading, variant decoding or
datamap registration claimed.

Original polling 22302ac0:
- Disabled or null loaded book: remove existing keypad if handle not
  invalid (22302d2b), invalidate handle; no reschedule.
- If missing handle: create and cast prop_keypad; save handle. If
  invalid, return without schedule. Default panel name prop_keypad,
  attachment keypad, fallback transform when attachment unavailable,
  panelname, 16x16, flag writes and spawn. Host performs configuration
  with existing entity and transform services.
- Resolve keypad serial, read its input, compare length >=4 and greater
  than last matched count. Find KeyValues child via 22264e00. Found
  string beginning '*' sets matched length then emits suffix, preserves
  input. Other found string emits sound then clears input and resets
  matched count. Not found uses optional FailSound then clears/resets.
- Reschedule at clock+0.25 (constant 223d99d0).
- DialNumber delegates to keypad SetInput if handle valid. Enable
  clears disabled byte and schedules unconditionally, Disable sets byte
  only. Original spawn/restore also install think pointer and schedule.

Port owns decisions and direct original-layout scalar writes. The
compatible host creates/configures/removes screens, resolves serial
handles, looks up phonebook entries, supplies sound and scheduling.
Calls Phase717 native SetInput/Clear on the actual resolved control.
Rejects stale handle rather than dereferencing null, guards malformed
strings and partial callbacks with explicit reconciliation. No
physics/model work; no claim of full engine spawning or actual audio.
Paired regression covers baseclass identity, dependency pins, missing
book/allocation, exact four-digit threshold, prefix dedup, terminal
clear, failure fallback, disable/removal, re-enable, stale handle and
nested native keypad output ordering.
